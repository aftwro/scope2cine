/* Recording (see camapp.h): encoded frames go into a queue that a writer thread empties onto the
 * SD card (MJPEG in AVI, or H.265 in crash-safe MPEG-TS from its own encoder); clip numbering and
 * splitting; SD card presence, hotplug mount and verify, and formatting. */
#include "camapp.h"

#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "avi.h"
#include "osd.h"
#include "ts.h"

/* Recording: MJPEG AVI on the SD card, new file every REC_SPLIT_BYTES (AVI 1.0 limit).
 * The encoder thread only copies frames into a small queue; a writer thread does
 * the slow SD writes, so card stalls never block the encoder. A full queue drops
 * the frame and counts it. */
#define HEVC_MAX_PACKS 8                           /* H.265: VPS/SPS/PPS/SEI/slices may come as separate packs */
#define REC_SPLIT_BYTES (1000u * 1024 * 1024)
#define REC_TS_SPLIT_BYTES (1900u * 1024 * 1024)      /* 32-bit file offsets: stay below 2 GiB */

/* Each queued frame gets a buffer of its own size, freed after writing, so the queue is limited by
 * bytes (REC_MEM_MAX) as well as frames: 240 small H.265 frames ride out a 2 s card stall at 105p
 * (24 fixed 512 KB slots only covered 0.23 s and dropped frames, 2026-09-26), while big MJPEG frames
 * stop at the byte budget. One frame may be up to REC_SLOT_MAX: 24p at 40 Mbps gives H.265
 * keyframes over 2 MB (2026-09-25). */
#define REC_SLOT_MAX (6 * 1024 * 1024)
uint32_t rec_queued_bytes;               /* bytes held by queued frames */
static uint32_t rec_max_frame;             /* largest frame written in the current file */
static uint32_t rec_keys;                  /* keyframes written in the current file */
static pthread_mutex_t rec_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rec_cond = PTHREAD_COND_INITIALIZER;
static uint8_t *rec_slot[REC_QUEUE];
static uint32_t rec_slot_len[REC_QUEUE];
static uint8_t rec_slot_key[REC_QUEUE];
int rec_head, rec_count;          /* queue: head = next to write */
/* The open clip: MJPEG in AVI or H.265 in MPEG-TS (crash-safe). */
static int rec_hevc;                     /* clip goes through the video encoder to .ts (H.265 or H.264) */
static int rec_h264;                     /* ... and that encoder is H.264 */
avi_t *rec;
static ts_t *rec_ts;
static uint32_t rec_ts_frame;            /* frame counter for the PTS */
static pthread_t rec_writer;
volatile int rec_on, rec_writer_run;
int rec_file_no, rec_fps = 60, rec_skip, rec_decimate;
static uint64_t tl_next_ms;              /* timelapse: monotonic ms of the next frame to keep, 0 = next one */
static long tl_queued;                   /* timelapse: frames queued in this clip */
volatile int rec_stop_pending;           /* timelapse limit reached: the main loop stops the clip */
long rec_frames_total, rec_dropped, rec_errors;
volatile uint64_t rec_bytes_total;      /* bytes written to the card, all clips */
long rec_dropped_at_start;               /* rec_dropped when the current clip started */
long rec_frames_at_start;                /* rec_frames_total when the current clip started */
time_t rec_started;

#define SD_BLOCK "/sys/block/mmcblk0"
#define SD_PART_SYS SD_BLOCK "/mmcblk0p1"
#define SD_PART_DEV "/dev/mmcblk0p1"

/* The card is physically there (the kernel removes the block device when it is pulled). */
static int sd_present(void) { return access(SD_PART_SYS, F_OK) == 0; }

/* /app/bin/sd is a plain directory on the RAM rootfs when the card is not mounted;
 * writing there fills RAM and hangs the system. Only record onto a mounted vfat card.
 * A pulled card leaves a stale mount behind, so the card must also be present. */
int sd_mounted(void)
{
    if (!sd_present())
        return 0;
    FILE *f = fopen("/proc/mounts", "r");
    char line[256];
    int ok = 0;
    while (f && fgets(line, sizeof(line), f))
        if (strstr(line, " " REC_DIR " vfat "))
            ok = 1;
    if (f)
        fclose(f);
    return ok;
}

/* Clip names: <mode prefix>_nnnn.<ext>; one counter across all modes (and the older REC_ clips). */
static const char *const clip_prefix[N_MODES] = { "NORM", "HDR", "SLOW", "TLAP" };

/* Clip number of a file name with any known prefix, -1 if it is not a clip. */
int clip_number(const char *name)
{
    static const char *const old = "REC";
    for (int m = -1; m < N_MODES; m++) {
        const char *p = m < 0 ? old : clip_prefix[m];
        size_t l = strlen(p);
        int n;
        if (!strncmp(name, p, l) && name[l] == '_' && sscanf(name + l + 1, "%d.", &n) == 1)
            return n;
    }
    return -1;
}

/* Next free clip number: one above the highest clip number of any prefix/extension (0 = no card). */
int clip_next_no(void)
{
    DIR *d = opendir(REC_DIR);
    struct dirent *e;
    int best = 0;
    while (d && (e = readdir(d))) {
        int n = clip_number(e->d_name);
        if (n > best)
            best = n;
    }
    if (d)
        closedir(d);
    return d ? best + 1 : 0;
}

const char *tc_text(uint64_t ms, unsigned fps)
{
    static char b[32];
    fps = fps ? fps : 25;
    uint64_t f = ms * fps / 1000, s = f / fps;
    snprintf(b, sizeof(b), "%02u:%02u:%02u:%02u", (unsigned)(s / 3600 % 100), (unsigned)(s / 60 % 60),
             (unsigned)(s % 60), (unsigned)(f % fps));
    return b;
}

/* Frame rate clips of the current mode are stamped with (what rec_set will use). */
unsigned file_fps(void)
{
    if (settings.mode == MODE_SLOWMO || settings.mode == MODE_TIMELAPSE)
        return play_fps();
    return sensor_fps_valid() == 60 && settings.rec_fps == 30 ? 30 : sensor_fps_valid();
}

static uint64_t side_tc_ms;                    /* record-run timecode at the start of the open file */

/* Record-run timecode now, in ms: the running total, plus the open file while recording. */
uint64_t rec_tc_ms(void)
{
    if (!rec_on || (!rec && !rec_ts) || rec_fps <= 0)
        return settings.tc_run_ms;
    uint32_t n = rec_hevc ? ts_frames(rec_ts) : avi_frames(rec);
    return side_tc_ms + (uint64_t)n * 1000 / rec_fps;
}

/* Sidecar metadata: <clip>.txt next to each file, key=value lines. Written when the file opens (so a
 * power cut still leaves it) and again when it closes, with frames, duration and end timecode. */
static char side_path[80], side_clip[64];
static uint32_t side_shutter_us;
static double side_gain;

static void sidecar_write(uint32_t frames, int closed)
{
    FILE *f = fopen(side_path, "w");
    if (!f)
        return;
    unsigned sfps = sensor_fps_valid(), ffps = rec_fps > 0 ? rec_fps : 25;
    uint64_t len_ms = (uint64_t)frames * 1000 / ffps;
    fprintf(f, "file=%s\ncamera=camapp " __DATE__ " " __TIME__ ", Sony IMX307\nresolution=1920x1080\n", side_clip);
    if (OPEN_GATE)
        fprintf(f, "open_gate=%ux%u (full sensor readout, scaled to 1920x1080)\n", crop_w, crop_h);
    else if (crop_w)
        fprintf(f, "sensor_crop=%ux%u centred (scaled to 1920x1080)\n", crop_w, crop_h);
    fprintf(f, "mode=%s\ncodec=%s\n", mode_names[settings.mode % N_MODES],
            rec_hevc ? (rec_h264 ? "H.264 .ts" : "H.265 .ts") : "MJPEG .avi");
    if (rec_hevc)
        fprintf(f, "bitrate_mbps=%u\n", settings.rec_mbps);
    fprintf(f, "file_fps=%u\ncapture_fps=%u\n", ffps, sfps);
    if (settings.mode == MODE_SLOWMO)
        fprintf(f, "speed=%.2fx slower\n", sfps / (double)ffps);
    if (settings.mode == MODE_TIMELAPSE)
        fprintf(f, "interval=%s\nspeed=%.0fx faster\n", tl_interval_text(), settings.tl_interval_ms * ffps / 1000.0);
    fprintf(f, "tc_start=%s\n", tc_text(side_tc_ms, ffps));
    if (closed)
        fprintf(f, "tc_end=%s\nframes=%u\nduration_s=%.3f\ndropped=%ld\n", tc_text(side_tc_ms + len_ms, ffps), frames,
                len_ms / 1000.0, rec_dropped - rec_dropped_at_start);
    else
        fprintf(f, "status=recording\n");
    fprintf(f, "exposure=%s\n", settings.manual ? "manual" : settings.ae_priority == 1 ? "shutter priority (gain auto, values at the start)"
                               : settings.ae_priority == 2 ? "gain priority (shutter auto, values at the start)" : "auto (values at the start)");
    fprintf(f, "shutter=%s (%.1f deg at %u fps)\n", shutter_text(side_shutter_us), side_shutter_us * sfps * 360 / 1e6, sfps);
    fprintf(f, "gain=+%.1f dB (%.2fx)\n", side_gain > 1 ? 20 * log10(side_gain) : 0.0, side_gain);
    if (!settings.manual)
        fprintf(f, "ev_comp=%+.1f\nmetering=%s\n", settings.ev_thirds / 3.0, metering_names[settings.metering % 3]);
    if (settings.wb_mode == 1)
        fprintf(f, "white_balance=%u K\n", settings.wb_kelvin);
    else if (settings.wb_mode == 2)
        fprintf(f, "white_balance=manual R %.3f G %.3f B %.3f\n", settings.wb_r / 1024.0, settings.wb_g / 1024.0, settings.wb_b / 1024.0);
    else
        fprintf(f, "white_balance=auto\n");
    if (settings.isp_gamma == GAMMA_LOG)
        fprintf(f, "gamma=log (experimental)\n");
    else if (settings.isp_gamma > 0)
        fprintf(f, "gamma=%.1f\n", settings.isp_gamma / 10.0);
    else
        fprintf(f, "gamma=vendor\n");
    fprintf(f, "look_slot=%u\ntemporal_nr=%u\nwdr=%ld\nmirror=%u\nflip=%u\n", settings.look_slot, settings.nr3d_gain,
            (long)settings.wdr, settings.mirror, settings.flip);
    fclose(f);
}

int rec_open_next(void)
{
    char path[64];
    if (!sd_mounted()) {
        printf("rec: refused, SD card not mounted at " REC_DIR "\n");
        return -1;
    }
    const char *ext = rec_hevc ? "ts" : "avi";
    int next = clip_next_no();                      /* unique across .ts and .avi; a split continues upward */
    rec_file_no = next > rec_file_no ? next : rec_file_no + 1;
    snprintf(path, sizeof(path), REC_DIR "/%s_%04d.%s", clip_prefix[settings.mode % N_MODES], rec_file_no, ext);
    int ok;
    if (rec_hevc)
        ok = (rec_ts = ts_open(path, !rec_h264)) != NULL;
    else
        ok = (rec = avi_open(path, 1920, 1080, rec_fps)) != NULL;
    printf("rec: %s %s at %d fps (sensor %u)\n", ok ? "writing" : "cannot open", path, rec_fps, sensor_fps_valid());
    if (ok) {
        snprintf(side_clip, sizeof(side_clip), "%s", path + sizeof(REC_DIR));
        snprintf(side_path, sizeof(side_path), "%.*s.txt", (int)(strrchr(path, '.') - path), path);
        side_tc_ms = settings.tc_run_ms;
        side_shutter_us = settings.manual ? manual_shutter_us() : live_shutter_us;
        if (settings.manual && side_shutter_us > max_manual_us())
            side_shutter_us = max_manual_us();
        side_gain = settings.manual ? settings.sensor_gain / 1024.0 : live_gain;
        sidecar_write(0, 0);
    }
    return ok ? 0 : -1;
}

static uint64_t rec_file_bytes(void) { return rec_hevc ? ts_bytes(rec_ts) : avi_bytes(rec); }

static void rec_close_file(void)
{
    uint32_t n = rec_hevc ? ts_frames(rec_ts) : avi_frames(rec);
    uint64_t bytes = rec_file_bytes();
    int rc = rec_hevc ? ts_close(rec_ts) : avi_close(rec);
    rec = NULL;
    rec_ts = NULL;
    sync();
    printf("rec: closed, %u frames (%u key), %llu bytes, largest frame %u bytes, rc %d\n", n, rec_hevc ? rec_keys : n,
           (unsigned long long)bytes, rec_max_frame, rc);
    rec_keys = 0;
    sidecar_write(n, 1);
    settings.tc_run_ms = side_tc_ms + (uint64_t)n * 1000 / (rec_fps > 0 ? rec_fps : 25);   /* record run: next clip continues */
    settings_changed();
    rec_max_frame = 0;
}

static void *rec_writer_thread(void *arg)
{
    prctl(PR_SET_NAME, "recwrite");                 /* thread name for top / cpustat */
    pthread_mutex_lock(&rec_lock);
    while (rec_writer_run || rec_count) {
        if (!rec_count) {
            pthread_cond_wait(&rec_cond, &rec_lock);
            continue;
        }
        int i = rec_head;
        pthread_mutex_unlock(&rec_lock);              /* write without holding the lock */
        int bad = 0;
        if (rec_hevc) {                               /* split only where a keyframe starts the file */
            if (rec_slot_key[i] && ts_bytes(rec_ts) >= REC_TS_SPLIT_BYTES) {
                rec_close_file();
                bad = rec_open_next() != 0;
            }
            if (!bad)
                bad = ts_write_frame(rec_ts, rec_slot[i], rec_slot_len[i], NULL, 0,
                                     (uint64_t)rec_ts_frame++ * 90000 / rec_fps, rec_slot_key[i]) != 0;
        } else {
            bad = avi_write_frame(rec, rec_slot[i], rec_slot_len[i], NULL, 0) != 0;
            if (!bad && avi_bytes(rec) >= REC_SPLIT_BYTES) {
                rec_close_file();
                bad = rec_open_next() != 0;
            }
        }
        pthread_mutex_lock(&rec_lock);
        if (rec_slot_len[i] > rec_max_frame)
            rec_max_frame = rec_slot_len[i];
        rec_keys += rec_slot_key[i] != 0;
        if (!bad)
            rec_bytes_total += rec_slot_len[i];
        free(rec_slot[i]);
        rec_slot[i] = NULL;
        rec_queued_bytes -= rec_slot_len[i];
        rec_head = (rec_head + 1) % REC_QUEUE;
        rec_count--;
        if (bad) {
            rec_errors++;
            printf("rec: write error, stopping\n");
            rec_on = 0;
            rec_writer_run = 0;
            for (; rec_count > 0; rec_count--, rec_head = (rec_head + 1) % REC_QUEUE) {   /* drop the rest */
                free(rec_slot[rec_head]);
                rec_slot[rec_head] = NULL;
            }
            rec_queued_bytes = 0;
        } else {
            rec_frames_total++;
        }
    }
    pthread_mutex_unlock(&rec_lock);
    if (rec || rec_ts)
        rec_close_file();
    return NULL;
}

static int hevc_start(unsigned src_fps, unsigned fps);
static void hevc_stop(void);


void rec_set(int on, int fps)
{
    if (on && !rec_on) {
        if (rec_writer_run)                         /* previous writer still draining */
            pthread_join(rec_writer, NULL);
        /* At 60 fps the Record fps setting may halve the rate; any other sensor rate is recorded as is.
         * Slow-mo keeps every frame and stamps the clip with the playback rate (rec_fps = file rate). */
        unsigned sfps = sensor_fps_valid();
        int slow = settings.mode == MODE_SLOWMO, tl = settings.mode == MODE_TIMELAPSE;
        rec_decimate = !slow && !tl && sfps == 60 && fps == 30;
        rec_fps = slow || tl ? (int)play_fps() : rec_decimate ? 30 : (int)sfps;
        tl_next_ms = 0;                             /* timelapse: first frame at once */
        tl_queued = 0;
        rec_stop_pending = 0;
        rec_skip = 0;
        rec_head = rec_count = 0;
        rec_queued_bytes = 0;
        rec_hevc = settings.rec_codec != 0 && !tl;   /* timelapse: MJPEG only */
        rec_h264 = settings.rec_codec == 2;
        rec_ts_frame = 0;
        if (rec_open_next() != 0)
            return;
        rec_writer_run = 1;
        rec_on = 1;
        rec_started = time(NULL);
        rec_dropped_at_start = rec_dropped;
        rec_frames_at_start = rec_frames_total;
        pthread_create(&rec_writer, NULL, rec_writer_thread, NULL);
        if (rec_hevc && hevc_start(sfps, slow ? sfps : (unsigned)rec_fps) != 0) {   /* no encoder: undo it and close the clip */
            rec_set(0, 0);
            return;
        }
    } else if (!on && rec_on) {
        if (rec_hevc)
            hevc_stop();                            /* no new frames, then drain the queue */
        pthread_mutex_lock(&rec_lock);
        rec_on = 0;
        rec_writer_run = 0;                         /* writer drains the queue, then closes */
        pthread_cond_signal(&rec_cond);
        pthread_mutex_unlock(&rec_lock);
        pthread_join(rec_writer, NULL);
        printf("rec: stopped; since start %ld frames written, %ld dropped\n", rec_frames_total, rec_dropped);
    }
    tally_update();
}

/* Queue one encoded frame given as n parts (encoder packs). A slot grows for an oversized
 * H.265 keyframe; a full queue drops the frame and counts it. */
static void rec_queue_frame(const void *const *part, const uint32_t *len, int n, int key)
{
    uint32_t total = 0;
    for (int k = 0; k < n; k++)
        total += len[k];
    pthread_mutex_lock(&rec_lock);
    int i = (rec_head + rec_count) % REC_QUEUE;
    int room = rec_count < REC_QUEUE && rec_queued_bytes + total <= REC_MEM_MAX;
    if (rec_on && room && total <= REC_SLOT_MAX)
        rec_slot[i] = malloc(total);
    if (rec_on && room && total <= REC_SLOT_MAX && rec_slot[i]) {
        uint32_t off = 0;
        for (int k = 0; k < n; k++) {
            memcpy(rec_slot[i] + off, part[k], len[k]);
            off += len[k];
        }
        rec_slot_len[i] = total;
        rec_slot_key[i] = key;
        rec_queued_bytes += total;
        rec_count++;
        pthread_cond_signal(&rec_cond);
    } else if (rec_on) {
        rec_dropped++;
        printf("rec: dropped %s frame of %u bytes (%s)\n", key ? "key" : "a", total,
               !room ? "queue full" : total > REC_SLOT_MAX ? "too big" : "out of memory");
    }
    pthread_mutex_unlock(&rec_lock);
}

/* Called from the JPEG encoder thread for every frame (60 fps); only used for MJPEG clips. */
void rec_frame(const void *p1, uint32_t n1, const void *p2, uint32_t n2)
{
    if (!rec_on || rec_hevc)
        return;
    if (rec_decimate && (rec_skip++ & 1))           /* keep every 2nd frame */
        return;
    if (settings.mode == MODE_TIMELAPSE) {          /* one frame per interval, up to the limit */
        if (rec_stop_pending)
            return;
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        uint64_t now = (uint64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
        if (tl_next_ms && now < tl_next_ms)
            return;
        /* next slot on the interval grid; after a stall start a fresh grid instead of bursting */
        tl_next_ms = tl_next_ms && now - tl_next_ms < settings.tl_interval_ms ? tl_next_ms + settings.tl_interval_ms
                                                                             : now + settings.tl_interval_ms;
        if (settings.tl_limit_frames && ++tl_queued >= (long)settings.tl_limit_frames)
            rec_stop_pending = 1;                   /* this frame is the last one */
    }
    const void *part[2] = { p1, p2 };
    uint32_t len[2] = { n1, n2 };
    rec_queue_frame(part, len, n2 ? 2 : 1, 1);
}

/* H.265 recording encoder: SCL output port 2 -> VENC dev 0 chn 0, its own stream thread.
 * Channel attr as cardv (logs/dec_initVideoEncoder.c): type 3, max/pic 1920x1080, buffer W*H/2,
 * by-frame, rc 8 = H.265 CBR, GOP = 1 s, fps, bitrate. Verified with h265test: 59.8 fps next to
 * the JPEG encoder, decodes on the PC. The bind does the 60 -> 30 fps conversion before encoding. */
#define VENC_DEV_HEVC 0
static ChnPort scl_out2 = { MOD_SCL, 0, 0, 2 };
static ChnPort hevc_in = { MOD_VENC, VENC_DEV_HEVC, 0, 0 };
static pthread_t hevc_tid;
static volatile int hevc_run;
static volatile long hevc_frames;

static void *hevc_thread(void *arg)
{
    prctl(PR_SET_NAME, "hevc");                 /* thread name for top / cpustat */
    int fd = MI_VENC_GetFd(VENC_DEV_HEVC, 0);
    while (hevc_run && fd >= 0) {
        fd_set fds;
        struct timeval tv = { 0, 200000 };
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
            continue;
        uint32_t stat[0x28 / 4] = { 0 };
        if (MI_VENC_Query(VENC_DEV_HEVC, 0, stat) != 0 || stat[4] == 0)
            continue;
        uint8_t packs[PACK_SIZE * HEVC_MAX_PACKS] = { 0 };
        uint32_t stream[0x40 / 4] = { 0 };
        stream[0] = (uint32_t)packs;
        stream[1] = stat[4] < HEVC_MAX_PACKS ? stat[4] : HEVC_MAX_PACKS;
        if (MI_VENC_GetStream(VENC_DEV_HEVC, 0, stream, -1) != 0)
            continue;
        const void *part[HEVC_MAX_PACKS];
        uint32_t len[HEVC_MAX_PACKS];
        int key = 0;
        for (uint32_t k = 0; k < stream[1]; k++) {
            part[k] = *(void **)(packs + k * PACK_SIZE + 8);
            len[k] = *(uint32_t *)(packs + k * PACK_SIZE + 0xc);
            const uint8_t *d = part[k];
            if (rec_h264) {
                int type = len[k] > 5 ? d[4] & 0x1f : -1;
                key |= type == 7 || type == 5;                    /* SPS / IDR */
            } else {
                int type = len[k] > 5 ? (d[4] >> 1) & 0x3f : -1;
                key |= type == 32 || type == 19 || type == 20;    /* VPS / IDR */
            }
        }
        rec_queue_frame(part, len, stream[1], key);
        hevc_frames++;
        MI_VENC_ReleaseStream(VENC_DEV_HEVC, 0, stream);
    }
    if (fd >= 0)
        MI_VENC_CloseFd(VENC_DEV_HEVC, 0);
    return NULL;
}

static int hevc_start(unsigned src_fps, unsigned fps)
{
    uint32_t port[6] = { 0, 0, 0x04380780, 0x101, 0xb, 0 };   /* no crop, 1920x1080 NV12 */
    uint32_t dev_attr[8] = { 0x1200, 0xa20 };
    uint32_t mbps = settings.rec_mbps < 2 ? 2 : settings.rec_mbps > 100 ? 100 : settings.rec_mbps;
    /* Rate control from cardv's initVideoEncoder: 8 = H.265 CBR {gop, stat time, fps num, den, bitrate,
     * fluctuate}, 9 = VBR {gop, stat time, fps num, den, max bitrate, max QP, min QP}. */
    uint32_t gop = settings.h265_gop ? settings.h265_gop : src_fps;
    uint32_t qmax = settings.h265_qp_max > 51 ? 51 : settings.h265_qp_max, qmin = settings.h265_qp_min > qmax ? qmax : settings.h265_qp_min;
    int vbr = settings.h265_rc == 1;
    /* H.264: encoder type 2, CBR 1 / VBR 2, same field layout. */
    uint32_t attr[19] = { rec_h264 ? 2 : 3, 1920, 1080, 1920 * 1080 / 2, 0, 1, 1920, 1080, 0, 0,
                          rec_h264 ? (vbr ? 2 : 1) : (vbr ? 9 : 8), gop, 0, fps, 1, mbps * 1000000,
                          vbr ? qmax : 0, vbr ? qmin : 0, 0 };
    int r = MI_SCL_SetOutputPortParam(0, 0, 2, port);
    r = r ? r : MI_SCL_EnableOutputPort(0, 0, 2);
    r = r ? r : MI_VENC_CreateDev(VENC_DEV_HEVC, dev_attr);
    r = r ? r : MI_VENC_CreateChn(VENC_DEV_HEVC, 0, attr);
    r = r ? r : MI_SYS_BindChnPort2(0, &scl_out2, &hevc_in, src_fps, fps, 1, 0);
    r = r ? r : MI_VENC_StartRecvPic(VENC_DEV_HEVC, 0);
    if (r == 0 && settings.mode == MODE_SLOWMO) {  /* stream header frame rate = playback rate */
        /* MI_VENC_ParamH265Vui_t / H264Vui_t (same layout): +8 timing present, +12 num_units_in_tick,
         * +16 time_scale (H.264 counts fields: 2 x fps x units, read back as 150150/1001 at 75p), +20 video
         * signal. Before StartRecvPic Get returns zeros and a Set is overwritten from the channel fps at
         * start, so it is set after the start; if Get is empty the struct is written with the defaults (read back while recording:
         * timing 1, 1001 / fps x 1001, video signal 00 05 01 00 01 01 01) and our time scale. */
        uint8_t vui[256] = { 0 };
        int rv = rec_h264 ? MI_VENC_GetH264Vui(VENC_DEV_HEVC, 0, vui) : MI_VENC_GetH265Vui(VENC_DEV_HEVC, 0, vui);
        uint32_t units = vui[12] | vui[13] << 8 | vui[14] << 16 | (uint32_t)vui[15] << 24;
        if (!units) {
            static const uint8_t sig[] = { 0x00, 0x05, 0x01, 0x00, 0x01, 0x01, 0x01 };
            memcpy(vui + 20, sig, sizeof(sig));
            units = 1001;
            vui[12] = units & 0xff, vui[13] = units >> 8;
        }
        vui[8] = 1;
        uint32_t ts = play_fps() * units * (rec_h264 ? 2 : 1);
        vui[16] = ts, vui[17] = ts >> 8, vui[18] = ts >> 16, vui[19] = ts >> 24;
        rv = rv ? rv : rec_h264 ? MI_VENC_SetH264Vui(VENC_DEV_HEVC, 0, vui) : MI_VENC_SetH265Vui(VENC_DEV_HEVC, 0, vui);
        printf("rec: slow-mo VUI %u/%u (= %u fps): rc %d\n", ts, units, play_fps(), rv);
    }

    printf("rec: %s encoder %u fps (sensor %u), %s %u Mbps, GOP %u", rec_h264 ? "H.264" : "H.265", fps, src_fps,
           vbr ? "VBR max" : "CBR", mbps, gop);
    if (vbr)
        printf(", QP %u-%u", qmin, qmax);
    printf(": rc %d\n", r);
    if (r == 0) {
        hevc_frames = 0;
        hevc_run = 1;
        pthread_create(&hevc_tid, NULL, hevc_thread, NULL);
    }
    return r;
}

/* Undo everything hevc_start did (safe after a partial start). */
static void hevc_stop(void)
{
    if (hevc_run) {
        hevc_run = 0;
        pthread_join(hevc_tid, NULL);
    }
    MI_VENC_StopRecvPic(VENC_DEV_HEVC, 0);
    MI_SYS_UnBindChnPort(0, &scl_out2, &hevc_in);
    MI_VENC_DestroyChn(VENC_DEV_HEVC, 0);
    MI_VENC_DestroyDev(VENC_DEV_HEVC);
    MI_SCL_DisableOutputPort(0, 0, 2);
    printf("rec: %s encoder stopped after %ld frames\n", rec_h264 ? "H.264" : "H.265", hevc_frames);
}


/* ---- SD card format (System menu). Runs in its own thread; the UI polls sd_fmt_state. ---- */
volatile int sd_fmt_state = FMT_IDLE;
char sd_fmt_msg[40];

/* Write a test file, drop caches, read it back: catches cards that silently discard writes. */
static int sd_verify(void)
{
    static uint8_t a[256 * 1024], b[sizeof(a)];
    for (unsigned i = 0; i < sizeof(a); i++)
        a[i] = (uint8_t)(i * 2654435761u >> 13);
    FILE *f = fopen(REC_DIR "/.verify", "wb");
    if (!f || fwrite(a, 1, sizeof(a), f) != sizeof(a) || fflush(f) || fsync(fileno(f))) {
        if (f)
            fclose(f);
        return -1;
    }
    fclose(f);
    sync();
    FILE *d = fopen("/proc/sys/vm/drop_caches", "w");
    if (d) { fputs("3", d); fclose(d); }
    f = fopen(REC_DIR "/.verify", "rb");
    int ok = f && fread(b, 1, sizeof(b), f) == sizeof(b) && !memcmp(a, b, sizeof(a));
    if (f)
        fclose(f);
    unlink(REC_DIR "/.verify");
    return ok ? 0 : -1;
}

static void *sd_format_thread(void *arg)
{
    /* Whole card: wipe MBR/GPT at both ends, one FAT32 LBA partition, filesystem, mount. */
    const char *script =
        "umount " REC_DIR " 2>/dev/null; "
        "[ -b /dev/mmcblk0 ] || exit 2; "
        "SZ=$(cat /sys/block/mmcblk0/size); "
        "dd if=/dev/zero of=/dev/mmcblk0 bs=1M count=1 conv=fsync 2>/dev/null; "
        "dd if=/dev/zero of=/dev/mmcblk0 bs=512 seek=$((SZ-2048)) count=2048 conv=fsync 2>/dev/null; "
        "printf 'o\\nn\\np\\n1\\n\\n\\nt\\nc\\nw\\n' | fdisk /dev/mmcblk0 >/dev/null 2>&1; "
        "blockdev --rereadpt /dev/mmcblk0; sleep 1; "
        "[ -b /dev/mmcblk0p1 ] || exit 3; "
        "mkfs.vfat -F 32 -n CAMERA /dev/mmcblk0p1 >/dev/null 2>&1 || exit 4; "
        "mount -t vfat /dev/mmcblk0p1 " REC_DIR " || exit 5";
    printf("sd: format started\n");
    int rc = system(script);
    int code = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
    if (code == 0 && sd_verify() == 0) {
        snprintf(sd_fmt_msg, sizeof(sd_fmt_msg), "Verified");
        sd_fmt_state = FMT_OK;
    } else {
        snprintf(sd_fmt_msg, sizeof(sd_fmt_msg), code ? "FAILED (step %d)" : "FAILED verify", code);
        sd_fmt_state = FMT_FAILED;
    }
    printf("sd: format %s\n", sd_fmt_msg);
    return NULL;
}

void sd_format_start(void)
{
    pthread_t th;
    if (sd_fmt_state == FMT_RUNNING || rec_on)
        return;
    sd_fmt_state = FMT_RUNNING;
    snprintf(sd_fmt_msg, sizeof(sd_fmt_msg), "Formatting...");
    pthread_create(&th, NULL, sd_format_thread, NULL);
    pthread_detach(th);
}


/* SD hotplug, run once a second: detach a stale mount when the card is pulled; for a new card
 * show "initializing", then mount and verify it (write/read back) on the next tick. */
#define SD_INIT_NOTICE_SECS 30          /* stays up until replaced by the result */
void sd_hotplug_check(void)
{
    static int initializing, failed;                   /* failed: don't retry until the card is pulled */
    if (sd_fmt_state == FMT_RUNNING)
        return;
    int present = sd_present(), in_mounts = 0;
    FILE *f = fopen("/proc/mounts", "r");
    char line[256];
    while (f && fgets(line, sizeof(line), f))
        if (strstr(line, " " REC_DIR " "))
            in_mounts = 1;
    if (f)
        fclose(f);

    if (!present) {
        if (in_mounts) {
            int r = umount2(REC_DIR, MNT_DETACH);       /* lazy: never blocks on the missing card */
            printf("sd: card removed, detach mount = %d\n", r);
            notice_for("SD card removed", OSD_YELLOW, NOTICE_SECS);
        } else if (initializing) {
            notice_for("SD card removed", OSD_YELLOW, NOTICE_SECS);
        }
        initializing = 0;
        failed = 0;
        return;
    }
    if (in_mounts || failed)
        return;                                         /* mounted, or a failed card still inserted */
    if (!initializing) {                                /* just inserted: tell the user, mount next tick */
        initializing = 1;
        notice_for("Please wait... Initializing SD card", OSD_WHITE, SD_INIT_NOTICE_SECS);
        printf("sd: card inserted\n");
        return;
    }
    initializing = 0;
    int r = mount(SD_PART_DEV, REC_DIR, "vfat", MS_NOATIME, "");
    printf("sd: mount = %d\n", r);
    if (r != 0) {
        failed = 1;
        notice_for("SD card error: mount failed", OSD_RED, 5);
        return;
    }
    if (sd_verify() != 0) {
        printf("sd: verify failed\n");
        notice_for("SD card error: not writable", OSD_RED, 5);
        return;
    }
    struct statvfs st;
    char msg[48] = "SD ready";
    if (statvfs(REC_DIR, &st) == 0)
        snprintf(msg, sizeof(msg), "SD ready: %.1f GB free", (double)st.f_bavail * st.f_frsize / (1u << 30));
    printf("sd: %s\n", msg);
    notice_for(msg, OSD_WHITE, 3);
}
