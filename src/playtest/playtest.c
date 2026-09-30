/* Usage: playtest FILE [frames] [debug]. Standalone decode test: open a clip with the device's FFmpeg 4.1.3 and decode it with
 * SigmaStar's hardware decoder (sshevc / ssh264 / ssmjpeg); report frame format and speed.
 * Usage: playtest FILE [frames] */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char **argv)
{
    AVFormatContext *fmt = NULL;
    int max = argc > 2 ? atoi(argv[2]) : 120;
    if (argc < 2 || avformat_open_input(&fmt, argv[1], NULL, NULL) != 0) {
        printf("playtest: cannot open\n");
        return 1;
    }
    avformat_find_stream_info(fmt, NULL);
    int vs = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vs < 0) {
        printf("playtest: no video\n");
        return 1;
    }
    AVCodecParameters *par = fmt->streams[vs]->codecpar;
    const char *name = par->codec_id == AV_CODEC_ID_HEVC ? "sshevc" : par->codec_id == AV_CODEC_ID_H264 ? "ssh264" : "ssmjpeg";
    AVCodec *dec = avcodec_find_decoder_by_name(name);
    AVCodecContext *ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(ctx, par);
    int r = dec ? avcodec_open2(ctx, dec, NULL) : -1;
    if (argc > 3) {                                 /* as cardv open_video: debug = 1, flags = 0 after open */
        ctx->debug = atoi(argv[3]);
        ctx->flags = 0;
    }
    printf("playtest: %s %dx%d, decoder %s open %d, stream fps %d/%d\n", avcodec_get_name(par->codec_id), par->width,
           par->height, name, r, fmt->streams[vs]->avg_frame_rate.num, fmt->streams[vs]->avg_frame_rate.den);
    if (r != 0)
        return 1;
    AVPacket pkt;
    AVFrame *fr = av_frame_alloc();
    int frames = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (frames < max && av_read_frame(fmt, &pkt) == 0) {
        if (pkt.stream_index == vs && avcodec_send_packet(ctx, &pkt) == 0)
            while (avcodec_receive_frame(ctx, fr) == 0) {
                if (frames == 30 || frames == 60 || frames == 66 || frames == 90 || frames == 108) {
                    /* lower third as 16x16-region means: Y (80x15), then U and V from NV12 (80x15 each) */
                    FILE *o = fopen("/tmp/regions.bin", frames == 30 ? "wb" : "ab");
                    for (int plane = 0; o && plane < 3; plane++)
                        for (int by = 0; by < 15; by++)
                            for (int bx = 0; bx < 80; bx++) {
                                long sum = 0, cnt = 0;
                                for (int j = 0; j < 16; j++)
                                    for (int i = 0; i < 16; i++) {
                                        int yy = 480 + by * 16 + j, xx = bx * 16 + i;
                                        if (plane == 0)
                                            sum += fr->data[0][yy * fr->linesize[0] + xx];
                                        else
                                            sum += fr->data[1][(yy / 2) * fr->linesize[1] + (xx & ~1) + plane - 1];
                                        cnt++;
                                    }
                                fputc(sum / cnt, o);
                            }
                    if (o)
                        fclose(o);
                }
                if (frames % 6 == 0 && frames <= 132) {  /* blockiness of the lower third: 8-px borders vs elsewhere */
                    double e = 0, o = 0;
                    long ne = 0, no = 0;
                    for (int j = fr->height * 2 / 3; j < fr->height; j++) {
                        const uint8_t *r = fr->data[0] + j * fr->linesize[0];
                        for (int i = 1; i < fr->width; i++) {
                            int d = abs(r[i] - r[i - 1]);
                            if (i % 8 == 0) { e += d; ne++; } else { o += d; no++; }
                        }
                    }
                    printf("blk %3d %s %.2f\n", frames, fr->key_frame ? "K" : " ", (e / ne) / (o / no));
                }
                if (frames == 30) {                     /* bottom 64 rows of frame 30 as NV12 (Y then UV) */
                    FILE *o = fopen("/tmp/bottom.nv12", "wb");
                    for (int j = fr->height - 64; o && j < fr->height; j++)
                        fwrite(fr->data[0] + j * fr->linesize[0], 1, fr->width, o);
                    for (int j = fr->height / 2 - 32; o && j < fr->height / 2; j++)
                        fwrite(fr->data[1] + j * fr->linesize[1], 1, fr->width, o);
                    if (o)
                        fclose(o);
                }
                if (frames < 3)
                    printf("playtest: frame %d fmt %d %dx%d linesize %d/%d data %p/%p pts %lld\n", frames, fr->format,
                           fr->width, fr->height, fr->linesize[0], fr->linesize[1], fr->data[0], fr->data[1],
                           (long long)fr->pts);
                frames++;
                av_frame_unref(fr);
            }
        av_packet_unref(&pkt);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double s = t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("playtest: %d frames in %.2f s = %.1f fps\n", frames, s, frames / s);
    av_frame_free(&fr);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
    return 0;
}
