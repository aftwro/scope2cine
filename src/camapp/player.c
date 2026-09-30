/* Clip playback (see player.h). Recipe from cardv's player (logs/dec_open_video.c,
 * logs/dec_player_display.c) and our playtest: FFmpeg demux + SigmaStar hardware decoder gives
 * NV12 frames (1280x720 for 1080p H.265, the decoder's own scaler); each frame is copied into a
 * display input buffer from MI_SYS_ChnInputPortGetBuf. Decode speed ~34 fps next to the live
 * pipeline, so 60p clips play slower than real time; 24/25/30p play at speed. */
#include "player.h"
#include "mi.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


static pthread_t tid;
static volatile int run, active, paused, at_end, seek_by;
static volatile double pos_s, dur_s;
static char clip[64], path_buf[256];

static double now_s(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* Display port 0 source size (word 2; word 1 is the on-screen size, 1920x1080). */
static void set_port_src(int w, int h)
{
    uint32_t attr[3] = { 0, 0x04380780, (uint32_t)w | (uint32_t)h << 16 };
    int r1 = MI_DISP_DisableInputPort(0, 0), r2 = MI_DISP_SetInputPortAttr(0, 0, attr), r3 = MI_DISP_EnableInputPort(0, 0);
    printf("player: display source %dx%d (rc %d/%d/%d)\n", w, h, r1, r2, r3);
}

/* Copy one NV12 frame into a display input buffer. Conf and info layouts from cardv
 * FUN_0003eaa0: conf +0 type 1 (frame), +0x18 u16 w, +0x1a u16 h, +0x20 pixel format 0xb NV12;
 * info +0x3c luma, +0x40 chroma, +0x60/+0x64 strides (same BufInfo as the output side). */
/* Debug: blockiness of the lower third (mean |dx| at 8-px borders / elsewhere, ~1.05 = clean). */
static double blockiness(const uint8_t *y, int stride, int w, int h)
{
    double e = 0, o = 0;
    long ne = 0, no = 0;
    for (int j = h * 2 / 3; j < h; j += 2)
        for (int i = 1; i < w; i++) {
            int d = y[j * stride + i] - y[j * stride + i - 1];
            d = d < 0 ? -d : d;
            if (i % 8 == 0) { e += d; ne++; } else { o += d; no++; }
        }
    return ne && no && o ? (e / ne) / (o / no) : 0;
}

static void push_frame(const AVFrame *f)
{
    static long n;
    ChnPort port = { MOD_DISP, 0, 0, 0 };
    uint8_t conf[0x74] = { 0 }, info[0x108] = { 0 };
    uint32_t handle = 0;
    *(uint32_t *)conf = 1;
    *(uint16_t *)(conf + 0x18) = f->width;
    *(uint16_t *)(conf + 0x1a) = f->height;
    *(uint32_t *)(conf + 0x20) = 0xb;
    if (MI_SYS_ChnInputPortGetBuf(&port, conf, info, &handle, 100) != 0)
        return;
    MI_SYS_GetCurPts(0, (uint64_t *)info);
    uint8_t *y = *(uint8_t **)(info + 0x3c), *uv = *(uint8_t **)(info + 0x40);
    uint32_t sy = *(uint32_t *)(info + 0x60), suv = *(uint32_t *)(info + 0x64);
    for (int j = 0; j < f->height; j++)
        memcpy(y + j * sy, f->data[0] + j * f->linesize[0], f->width);
    for (int j = 0; j < f->height / 2; j++)
        memcpy(uv + j * suv, f->data[1] + j * f->linesize[1], f->width);
    if (n % 6 == 0 && n <= 132)
        printf("player blk %3ld src %.2f disp %.2f\n", n, blockiness(f->data[0], f->linesize[0], f->width, f->height),
               blockiness(y, sy, f->width, f->height));
    n++;
    MI_SYS_ChnInputPortPutBuf(handle, info, 0);
}

static void *player_thread(void *arg)
{
    AVFormatContext *fmt = NULL;
    AVCodecContext *ctx = NULL;
    AVFrame *fr = av_frame_alloc();
    int vs = -1, src_w = 0, src_h = 0;
    if (avformat_open_input(&fmt, path_buf, NULL, NULL) != 0 || avformat_find_stream_info(fmt, NULL) < 0 ||
        (vs = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        printf("player: cannot open %s\n", path_buf);
        goto done;
    }
    AVCodecParameters *par = fmt->streams[vs]->codecpar;
    const char *name = par->codec_id == AV_CODEC_ID_HEVC ? "sshevc" : par->codec_id == AV_CODEC_ID_H264 ? "ssh264" : "ssmjpeg";
    AVCodec *dec = avcodec_find_decoder_by_name(name);
    ctx = dec ? avcodec_alloc_context3(dec) : NULL;
    if (!ctx || avcodec_parameters_to_context(ctx, par) < 0 || avcodec_open2(ctx, dec, NULL) != 0) {
        printf("player: no decoder for %s\n", avcodec_get_name(par->codec_id));
        goto done;
    }
    AVRational tb = fmt->streams[vs]->time_base;
    dur_s = fmt->duration > 0 ? fmt->duration / (double)AV_TIME_BASE : 0;
    printf("player: %s, %s %dx%d, %.1f s\n", clip, name, par->width, par->height, dur_s);
    double t_ref = -1, pts_ref = 0;                 /* wall clock of the reference frame and its pts */
    AVPacket pkt;
    while (run) {
        if (seek_by) {
            double target = pos_s + seek_by;
            seek_by = 0;
            target = target < 0 ? 0 : dur_s > 0 && target > dur_s ? dur_s : target;
            av_seek_frame(fmt, vs, (int64_t)(target / av_q2d(tb)), AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(ctx);
            at_end = 0;
            t_ref = -1;
        }
        if (paused || at_end) {
            usleep(20000);
            t_ref = -1;                                 /* restart the clock on resume */
            if (at_end && !paused) {                    /* play pressed at the end: from the start */
                av_seek_frame(fmt, vs, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(ctx);
                at_end = 0;
            }
            continue;
        }
        if (av_read_frame(fmt, &pkt) != 0) {
            at_end = paused = 1;                        /* hold the last frame */
            continue;
        }
        if (pkt.stream_index == vs && avcodec_send_packet(ctx, &pkt) == 0) {
            while (run && avcodec_receive_frame(ctx, fr) == 0) {
                double pts = fr->pts != AV_NOPTS_VALUE ? fr->pts * av_q2d(tb) : pos_s;
                if (fr->width != src_w || fr->height != src_h) {
                    src_w = fr->width;
                    src_h = fr->height;
                    set_port_src(src_w, src_h);
                }
                if (t_ref < 0) {
                    t_ref = now_s();
                    pts_ref = pts;
                }
                double wait = t_ref + (pts - pts_ref) - now_s();
                while (run && wait > 0.002 && !seek_by) {   /* pace to the clip's own timing */
                    usleep(wait > 0.02 ? 20000 : (useconds_t)(wait * 1e6));
                    wait = t_ref + (pts - pts_ref) - now_s();
                }
                push_frame(fr);
                pos_s = pts - (fmt->start_time != AV_NOPTS_VALUE ? fmt->start_time / (double)AV_TIME_BASE : 0);
                av_frame_unref(fr);
            }
        }
        av_packet_unref(&pkt);
    }
done:
    av_frame_free(&fr);
    if (ctx)
        avcodec_free_context(&ctx);
    if (fmt)
        avformat_close_input(&fmt);
    if (src_w)
        set_port_src(1920, 1080);
    active = 0;
    return NULL;
}

int player_open(const char *path)
{
    if (active)
        return -1;
    snprintf(path_buf, sizeof(path_buf), "%s", path);
    const char *slash = strrchr(path, '/');
    snprintf(clip, sizeof(clip), "%s", slash ? slash + 1 : path);
    pos_s = dur_s = 0;
    paused = at_end = seek_by = 0;
    run = active = 1;
    if (pthread_create(&tid, NULL, player_thread, NULL) != 0) {
        run = active = 0;
        return -1;
    }
    return 0;
}

void player_stop(void)
{
    if (!run && !active)
        return;
    run = 0;
    pthread_join(tid, NULL);
    active = 0;
}

int player_active(void) { return active; }
void player_toggle_pause(void) { paused = !paused; }
void player_seek(int seconds) { seek_by += seconds; }

void player_status(char *buf, int size)
{
    int p = (int)pos_s, d = (int)dur_s;
    snprintf(buf, size, "PLAY %s  %02d:%02d / %02d:%02d%s", clip, p / 60, p % 60, d / 60, d % 60,
             at_end ? "  END" : paused ? "  PAUSE" : "");
}
