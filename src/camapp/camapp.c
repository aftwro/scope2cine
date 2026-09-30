/* camapp step 3c: sensor -> VIF -> ISP -> SCL -> DISP -> HDMI without cardv, hold, tear down.
 *
 * Call order and struct contents are taken from the mitrace capture of cardv
 * (logs/mi_trace_boot3.log) and Ghidra decompiles of CarDV_*ModuleInit.
 * Pass conditions: VIF IsrCnt in /proc/mi_modules/mi_vif/mi_vif0 increases, and
 * a live picture shows on the HDMI monitor.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include "uvc.h"
#include "avi.h"
#include "ts.h"
#include "player.h"
#include "input.h"
#include "settings.h"
#include "osd.h"
#include "sensordbg.h"
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#include "mi.h"
#include "camapp.h"

int soc_temp_c(void);

/* Each successful bring-up step pushes its undo onto this stack. */
#define MAX_UNDO 32
static struct { const char *name; int (*fn)(void); } undo[MAX_UNDO];
static int n_undo;

static double driver_time_scale(void);
static int step(const char *name, int ret)
{
    printf("%-34s = %d (0x%x)\n", name, ret, ret);
    return ret;
}
#define DO(call) do { if (step(#call, call) != 0) goto fail; } while (0)
#define UNDO(f) (undo[n_undo].name = #f, undo[n_undo++].fn = f)

static ChnPort vif_out = { MOD_VIF, 0, 0, 0 };
static ChnPort isp_in = { MOD_ISP, 0, 0, 0 };
static ChnPort isp_out = { MOD_ISP, 0, 0, 0 };
static ChnPort scl_in = { MOD_SCL, 0, 0, 0 };
static ChnPort scl_out = { MOD_SCL, 0, 0, 0 };   /* dev 0, chn 0, port 0 -> display */
static ChnPort disp_in = { MOD_DISP, 0, 0, 0 };
static ChnPort scl_out1 = { MOD_SCL, 0, 0, 1 };  /* dev 0, chn 0, port 1 -> encoder */
static ChnPort venc_in = { MOD_VENC, VENC_DEV_JPEG, VENC_CHN, 0 };

static int u_sys(void) { return MI_SYS_Exit(0); }
static int u_snr_init(void) { return MI_SNR_DeInitDev(); }
static int u_snr_en(void) { return MI_SNR_Disable(0); }
static int u_vif_grp(void) { return MI_VIF_DestroyDevGroup(0); }
static int u_vif_dev(void) { return MI_VIF_DisableDev(0); }
static int u_vif_port(void) { return MI_VIF_DisableOutputPort(0, 0); }
static int u_isp_dev(void) { return MI_ISP_DestoryDevice(0); }
static int u_isp_chn(void) { return MI_ISP_DestroyChannel(0, 0); }
static int u_isp_start(void) { return MI_ISP_StopChannel(0, 0); }
static int u_isp_port(void) { return MI_ISP_DisableOutputPort(0, 0, 0); }
static int u_bind(void) { return MI_SYS_UnBindChnPort(0, &vif_out, &isp_in); }
static int u_scl_dev(void) { return MI_SCL_DestroyDevice(0); }
static int u_scl_chn(void) { return MI_SCL_DestroyChannel(0, 0); }
static int u_scl_start(void) { return MI_SCL_StopChannel(0, 0); }
static int u_scl_port(void) { return MI_SCL_DisableOutputPort(0, 0, 0); }
static int u_bind_isp_scl(void) { return MI_SYS_UnBindChnPort(0, &isp_out, &scl_in); }
static int u_disp_init(void) { return MI_DISP_DeInitDev(); }
static int u_disp_en(void) { return MI_DISP_Disable(0); }
static int u_hdmi_init(void) { return MI_HDMI_DeInit(); }
static int u_hdmi_open(void) { return MI_HDMI_Close(0); }
static int u_hdmi_start(void) { return MI_HDMI_Stop(0); }
static int u_layer_bind(void) { return MI_DISP_UnBindVideoLayer(0, 0); }
static int u_layer_en(void) { return MI_DISP_DisableVideoLayer(0); }
static int u_bind_scl_disp(void) { return MI_SYS_UnBindChnPort(0, &scl_out, &disp_in); }
static int u_disp_port(void) { return MI_DISP_DisableInputPort(0, 0); }
static int u_scl_port1(void) { return MI_SCL_DisableOutputPort(0, 0, 1); }
static int u_venc_dev(void) { return MI_VENC_DestroyDev(VENC_DEV_JPEG); }
static int u_venc_chn(void) { return MI_VENC_DestroyChn(VENC_DEV_JPEG, VENC_CHN); }
static int u_venc_start(void) { return MI_VENC_StopRecvPic(VENC_DEV_JPEG, VENC_CHN); }
static int u_bind_scl_venc(void) { return MI_SYS_UnBindChnPort(0, &scl_out1, &venc_in); }

static int hdmi_event(uint32_t dev, uint32_t event, void *param, void *user)
{
    printf("HDMI event: dev %u event %u\n", dev, event);
    return 0;
}

static long vif_isr_count(void)
{
    FILE *f = fopen("/proc/mi_modules/mi_vif/mi_vif0", "r");
    char line[512];
    long cnt = -1;
    int next = 0;
    if (!f)
        return -1;
    while (fgets(line, sizeof(line), f)) {
        if (next) { sscanf(line, "%*d %ld", &cnt); break; }
        next = strstr(line, "IsrCnt") != NULL;
    }
    fclose(f);
    return cnt;
}

/* Same as cardv's IQ loader thread: wait until the ISP IQ init is done, then load the API bin. */
static int load_isp_api_bin(void)
{
    uint32_t done = 0;
    for (int i = 0; i < 100 && !done; i++) {
        MI_ISP_IQ_GetParaInitStatus(0, 0, &done);
        if (!done)
            usleep(30000);
    }
    if (!done) {
        printf("IQ para init not done after 3 s\n");
        return -1;
    }
    return MI_ISP_API_CmdLoadBinFile(0, 0, "/bootconfig/iqfile/isp_api.bin", 0x4d2);
}

/* ExposureInfo (0x23c bytes): +0 stable, +4 boundary, +8 {FNx10, sensor gain, ISP gain, shutter us}. */
volatile int snap_request, venc_run;
volatile long venc_frames;
volatile unsigned long long venc_bytes;    /* running total, for noise measurements */
static volatile uint32_t venc_last_size;

volatile int playing;             /* a clip is on the display instead of the live picture */

static long vif_isr_count(void);

static void print_expo(int t)
{
    static long last = -1;
    long now = vif_isr_count();
    long fps = last >= 0 ? now - last : -1;
    last = now;
    uint32_t mode = 0xffffffff, info[0x23c / 4] = { 0 };
    MI_ISP_AE_GetExpoMode(0, 0, &mode);
    MI_ISP_AE_QueryExposureInfo(0, 0, info);
    static long last_enc;
    long enc = venc_frames - last_enc;
    last_enc = venc_frames;
    static long last_uvc;
    long usb = uvc_frames_sent - last_uvc;
    last_uvc = uvc_frames_sent;
    static long last_rec;
    long recd = rec_frames_total - last_rec;
    last_rec = rec_frames_total;
    printf("t=%2ds rec=%3ld usb=%3ld enc=%3ld (%6u B) fps=%3ld AE mode=%u stable=%u shutter=%6u us sensor_gain=%6.2fx isp_gain=%5.2fx\n",
           t, recd, usb, enc, venc_last_size, fps, mode, info[0], (unsigned)(info[5] / driver_time_scale()), info[3] / 1024.0,
           info[4] / 1024.0);                         /* the ISP reports driver time units */
}

/* ISP channel param: {HDR type, 3DNR level, mirror, flip, rotation}. Global so mirror/flip can change it live. */
enum { ISP_PARAM_MIRROR = 2, ISP_PARAM_FLIP = 3 };
static uint32_t isp_param[5] = { 0, 1, 0, 0, 0 };

/* Status LED on GPIO38 (1 = lit): lit when idle, blinking while recording. */
static void led_set(int on)
{
    static int ready;
    if (!ready) {
        FILE *f = fopen("/sys/class/gpio/export", "w");
        if (f) { fputs("38", f); fclose(f); }
        f = fopen("/sys/class/gpio/gpio38/direction", "w");
        if (f) { fputs("out", f); fclose(f); }
        ready = 1;
    }
    FILE *f = fopen("/sys/class/gpio/gpio38/value", "w");
    if (f) { fputs(on ? "1" : "0", f); fclose(f); }
}

/* Photo feedback: border flash + LED blink in a short thread (never stalls the encoder),
 * plus a notice line cleared by the status loop after NOTICE_SECS. */
static volatile time_t notice_until;

/* Recording tally: red frame around the picture while a clip is being written. Called on record
 * start/stop and once a second (catches a stop by the writer thread after a card error). */
#define TALLY_RED 0xffe02020
static volatile int tally_shown;
void tally_update(void)
{
    if (rec_on != tally_shown) {
        tally_shown = rec_on;
        osd_border(rec_on ? TALLY_RED : 0);
    }
}

static void *photo_flash_thread(void *arg)
{
    prctl(PR_SET_NAME, "flash");                 /* thread name for top / cpustat */
    osd_border(0xffffffff);
    led_set(0);
    usleep(150000);
    osd_border(tally_shown ? TALLY_RED : 0);        /* back to the tally while recording */
    led_set(1);
    return NULL;
}

void notice_for(const char *text, uint32_t colour, int secs)
{
    osd_notice(text, colour);
    notice_until = time(NULL) + secs;
}

static void photo_feedback(const char *text, uint32_t colour, int flash)
{
    notice_for(text, colour, NOTICE_SECS);
    if (flash) {
        pthread_t th;
        if (pthread_create(&th, NULL, photo_flash_thread, NULL) == 0)
            pthread_detach(th);
    }
}


static void *venc_thread(void *arg)
{
    prctl(PR_SET_NAME, "jpeg");                 /* thread name for top / cpustat */
    int fd = MI_VENC_GetFd(VENC_DEV_JPEG, VENC_CHN);
    printf("venc fd %d\n", fd);
    while (venc_run) {
        fd_set fds;
        struct timeval tv = { 0, 500000 };
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
            continue;
        uint32_t stat[0x28 / 4] = { 0 };
        if (MI_VENC_Query(VENC_DEV_JPEG, VENC_CHN, stat) != 0 || stat[4] == 0)
            continue;
        uint8_t packs[PACK_SIZE * MAX_PACKS] = { 0 };
        uint32_t stream[0x40 / 4] = { 0 };
        stream[0] = (uint32_t)packs;
        stream[1] = stat[4] < MAX_PACKS ? stat[4] : MAX_PACKS;
        if (MI_VENC_GetStream(VENC_DEV_JPEG, VENC_CHN, stream, -1) != 0)
            continue;
        uint32_t size = 0;
        for (uint32_t i = 0; i < stream[1]; i++)
            size += *(uint32_t *)(packs + i * PACK_SIZE + 0xc);
        if (snap_request) {
            static int img_no;
            char path[64] = "/tmp/snap.jpg";
            if (sd_mounted())
                do
                    snprintf(path, sizeof(path), REC_DIR "/IMG_%04d.jpg", ++img_no);
                while (access(path, F_OK) == 0);
            int on_sd = sd_mounted();
            FILE *f = fopen(path, "wb");
            int ok = f != NULL;
            for (uint32_t i = 0; f && i < stream[1]; i++) {
                uint32_t len = *(uint32_t *)(packs + i * PACK_SIZE + 0xc);
                ok &= fwrite(*(void **)(packs + i * PACK_SIZE + 8), 1, len, f) == len;
            }
            if (f)
                ok &= fclose(f) == 0;
            printf("snap: %u bytes -> %s (%s)\n", size, path, ok ? "ok" : "write error");
            if (!on_sd) {
                photo_feedback("PHOTO FAILED: no SD card", OSD_RED, 0);
            } else if (!ok) {
                photo_feedback("PHOTO FAILED: write error", OSD_RED, 0);
            } else {
                char msg[64], raw[72];
                snprintf(raw, sizeof(raw), "%.*s.DNG", (int)(strrchr(path, '.') - path), path);
                int dng = settings.photo_raw && dng_capture(raw, 0, 0, 0, 0) == 0;
                snprintf(msg, sizeof(msg), "%s saved%s", strrchr(path, '/') + 1, dng ? " + DNG" : settings.photo_raw ? " (DNG failed)" : "");
                photo_feedback(msg, OSD_WHITE, 1);
            }
            snap_request = 0;
        }
        uint32_t n1 = *(uint32_t *)(packs + 0xc), n2 = stream[1] > 1 ? *(uint32_t *)(packs + PACK_SIZE + 0xc) : 0;
        uvc_put_frame(*(void **)(packs + 8), n1, n2 ? *(void **)(packs + PACK_SIZE + 8) : NULL, n2);
        rec_frame(*(void **)(packs + 8), n1, n2 ? *(void **)(packs + PACK_SIZE + 8) : NULL, n2);
        venc_last_size = size;
        venc_bytes += size;
        venc_frames++;
        MI_VENC_ReleaseStream(VENC_DEV_JPEG, VENC_CHN, stream);
    }
    MI_VENC_CloseFd(VENC_DEV_JPEG, VENC_CHN);
    return NULL;
}

/* Manual exposure value: {FNx10, sensor gain (1024 = 1x), ISP gain (1024 = 1x), shutter us}. */
enum { EXPO_FN, EXPO_SENSOR_GAIN, EXPO_ISP_GAIN, EXPO_US };
enum { AE_MODE_AUTO = 0, AE_MODE_MANUAL = 4 };


/* Exposure offset from what auto-exposure would aim for, in stops. */
double ev_offset(const uint32_t *info)
{
    if (!info[AE_AVG_Y] || !info[AE_TARGET])
        return 0;
    return log2((double)info[AE_AVG_Y] / info[AE_TARGET]);
}

#define SETTINGS_PATH "/customer/camapp/settings"
#define SETTINGS_SAVE_DELAY 2        /* seconds after the last change */
static volatile long settings_dirty_at;   /* 0 = clean, else time of last change */

/* Per-mode keys live in MODE_DIR/<mode name>; the main settings file holds the running mode's copy. */
#define MODE_DIR "/customer/camapp/modes"
static void mode_path(char *buf, int size, unsigned mode) { snprintf(buf, size, MODE_DIR "/%s", mode_names[mode]); }

int mode_switch(unsigned mode)
{
    char path[64];
    if (mode >= N_MODES || mode == settings.mode || rec_on)
        return -1;
    mkdir(MODE_DIR, 0755);
    mode_path(path, sizeof(path), settings.mode);
    int r = settings_save_mode(path);
    printf("mode: %s saved to %s, rc %d\n", mode_names[settings.mode], path, r);
    settings.mode = mode;
    mode_path(path, sizeof(path), mode);
    settings_load(path);                            /* first visit: no file, current values carry over */
    printf("mode: switching to %s\n", mode_names[mode]);
    settings_changed();                             /* saved at exit, before the restart */
    restart_requested = stop_requested = 1;
    return 0;
}

void settings_changed(void) { settings_dirty_at = time(NULL); }

/* Sensor window crop (settings crop_w/crop_h, Normal and Slow-mo, fixed for a run). The sensor
 * sends a centred W+28 wide window (full height: the driver keeps VMAX 1125), VIF captures W x H
 * centred, ISP and SCL work at W x H and SCL scales every output to 1920x1080. A narrower line
 * carries less MIPI data, so HMAX (and the frame time) can shrink. */
uint32_t crop_w, crop_h;

static void crop_load(void)
{
    uint32_t w = settings.crop_w, h = settings.crop_h;
    if (settings.mode != MODE_NORMAL && settings.mode != MODE_SLOWMO)
        return;
    if (w == OPEN_GATE_W) {                          /* open gate (experimental): no sensor window */
        crop_w = OPEN_GATE_W, crop_h = OPEN_GATE_H;
        printf("crop: open gate %ux%u (full readout), max %u fps\n", crop_w, crop_h, crop_max_fps(crop_w));
        return;
    }
    w = !w || w > 1920 ? 1920 : w < 320 ? 320 : w & ~7u;
    h = !h || h > 1080 ? 1080 : h < 180 ? 180 : h & ~1u;
    if (w == 1920 && h == 1080)
        return;
    crop_w = w, crop_h = h;
    printf("crop: %ux%u, max %u fps\n", crop_w, crop_h, crop_max_fps(crop_w));
}

/* Fastest clean rate for a line W pixels wide: the line (HMAX / 148.5 MHz) must be ~1 us longer than
 * its MIPI data ((W + 28) x 10 bit over 4 x 445.5 Mbps = (W + 28) x 5/6 clocks). Measured 2026-09-26:
 * 1280 wide clean at HMAX 1250 (105.6p), broken at 1200. Full width stays at the verified 75. Capped
 * at 120, the sensor's rated maximum. */
unsigned crop_max_fps(uint32_t w)
{
    if (!w || w >= 1920)
        return 75;
    unsigned hmin = ((w + 28) * 5 + 5) / 6 + 160, fps = 132000 / hmin;   /* 148.5e6 / 1125 lines */
    return fps > 120 ? 120 : fps < 75 ? 75 : fps;
}

/* IMX307 window registers, written in standby. 0x3007 also holds the orientation: bit 0 VREVERSE
 * (flip), bit 1 HREVERSE (mirror), bits 4-6 WINMODE (4 = window cropping). */
static int crop_sensor(void)
{
    uint32_t pos = (1920 - crop_w) / 2, width = crop_w + 28;
    uint8_t winmode = 0x40 | (settings.flip ? 1 : 0) | (settings.mirror ? 2 : 0);
    int r = sensor_write_reg(0x3000, 1);
    r |= sensor_write_reg(0x3007, winmode);
    r |= sensor_write_reg(0x3040, pos & 0xff) | sensor_write_reg(0x3041, pos >> 8);        /* WINPH */
    r |= sensor_write_reg(0x3042, width & 0xff) | sensor_write_reg(0x3043, width >> 8);    /* WINWH */
    r |= sensor_write_reg(0x3472, width & 0xff) | sensor_write_reg(0x3473, width >> 8);    /* X_OUT */
    r |= sensor_write_reg(0x3000, 0);
    printf("crop: sensor window x %u width %u, 0x3007 = %02x: rc %d\n", pos, width, winmode, r);
    return r;
}

/* The driver's SetOrien only marks the orientation dirty; its frame callback later writes 0x3007
 * with the orientation bits alone, which would clear WINMODE. In crop mode camapp writes 0x3007
 * itself and never calls SetOrien (the driver consumed its start-up orientation before this runs). */
int apply_orientation(void)
{
    return crop_w && !OPEN_GATE ? crop_sensor() : MI_SNR_SetOrien(0, settings.mirror, settings.flip);
}
const char *const metering_names[] = { "AVG", "CTR", "SPOT" };

/* Exposure compensation (auto exposure only): EVComp = {s32 EV, u32 grad}; EV counts in 1/grad stops. */

int apply_ev_comp(void)
{
    int32_t ev[2] = { 0, 0 };
    int r = MI_ISP_AE_GetEVComp(0, 0, ev);
    uint32_t grad = ev[1] ? (uint32_t)ev[1] : 10;
    /* Log gamma exposes 2 stops under (highlight headroom); the log curve lifts mid grey back up. */
    int32_t thirds = settings.ev_thirds + (settings.isp_gamma == GAMMA_LOG ? LOG_EV_THIRDS : 0);
    ev[0] = (thirds * (int32_t)grad + (thirds >= 0 ? 1 : -1)) / 3;   /* rounded */
    int w = MI_ISP_AE_SetEVComp(0, 0, ev);
    printf("evcomp: %+d/3 EV%s -> EV %d grad %u (get %d set %d)\n", settings.ev_thirds,
           settings.isp_gamma == GAMMA_LOG ? " (log -2)" : "", ev[0], grad, r, w);
    return r | w;
}


int spot_w(void)
{
    uint32_t s = settings.spot_size ? settings.spot_size : 2;
    s = s > 8 ? 8 : s < 2 ? 2 : s & ~1u;             /* even widths only: odd ones cannot sit on the centre */
    return (int)s;
}

int weight_box(const struct win_weights *w, int t, struct wgt_box *b)
{
    *b = (struct wgt_box){ WGT_GRID, WGT_GRID, -1, -1 };
    for (int y = 0; y < WGT_GRID; y++)
        for (int x = 0; x < WGT_GRID; x++)
            if (w->tbl[t][y][x]) {
                if (x < b->x0) b->x0 = x;
                if (y < b->y0) b->y0 = y;
                if (x > b->x1) b->x1 = x;
                if (y > b->y1) b->y1 = y;
            }
    return b->x1 >= 0;
}

/* Draw the spot area on the OSD while spot metering is active. */
void show_spot_area(void)
{
    static struct win_weights w;
    struct wgt_box b;
    if (playing || settings.metering != 2 || MI_ISP_AE_GetWinWgt(0, 0, &w) != 0 || !weight_box(&w, 2, &b)) {
        osd_box(0, 0, 0, 0, 0);
        return;
    }
    int W = osd_width(), H = osd_height();
    osd_box(b.x0 * W / WGT_GRID, b.y0 * H / WGT_GRID, (b.x1 + 1 - b.x0) * W / WGT_GRID,
            (b.y1 + 1 - b.y0) * H / WGT_GRID, OSD_YELLOW);
}

/* Replace the vendor average and spot tables with a real average and a real centre spot. */
int install_weight_tables(void)
{
    static struct win_weights w;
    int r = MI_ISP_AE_GetWinWgt(0, 0, &w);
    if (r)
        return r;
    int sx = settings.spot_x > WGT_GRID - SPOT_W ? WGT_GRID - SPOT_W : (int)settings.spot_x;
    int sy = settings.spot_y > WGT_GRID - SPOT_H ? WGT_GRID - SPOT_H : (int)settings.spot_y;
    for (int y = 0; y < WGT_GRID; y++)
        for (int x = 0; x < WGT_GRID; x++) {
            w.tbl[0][y][x] = 8;
            w.tbl[2][y][x] = (x >= sx && x < sx + SPOT_W && y >= sy && y < sy + SPOT_H) ? 255 : 0;
        }
    return MI_ISP_AE_SetWinWgt(0, 0, &w);
}
int apply_metering(void)
{
    uint32_t type = settings.metering <= 2 ? settings.metering : 0;
    return MI_ISP_AE_SetWinWgtType(0, 0, &type);
}

/* The imx307 driver converts times with the 30 fps line time (29.6 us) while the sensor runs its
 * 60 fps timing (14.8 us/line, HMAX 2200): requested frame rates come out 2x faster and requested
 * exposures 2x shorter. Verified 2026-09-24 against VMAX/SHS1 registers and VIF interrupt counts.
 * So we send fps / 2 and exposure x 2, and display register-derived values.
 * In DOL-HDR mode the driver's HDR functions use the right time base (verified 2026-09-24: 12000
 * mfps -> 12.0 output frames/s), so no correction there. The sensor then runs 2 exposures per
 * output frame: its register frame rate is twice the output rate. */
int hdr_on;                                  /* sensor DOL-HDR mode, fixed for this run */
/* Above 60 fps (normal mode) the frame stays 1125 lines (the driver's floor, its "30 fps") and the
 * line gets shorter: HMAX in 148.5 MHz clocks. The driver never rewrites HMAX after init. Clean up
 * to 75 fps; faster lines need more than the 4-lane 445.5 Mbps MIPI link carries (verified
 * 2026-09-25: 75 clean, 77.5 and 80 broken frames). */
#define HMAX_60P 2200
static unsigned hmax_for_fps(unsigned fps)
{
    return fps > 60 ? (148500000u + 1125 * fps / 2) / (1125 * fps) : HMAX_60P;
}
/* Driver time units per real us: the driver assumes a 30 fps line (4400 clocks), the sensor runs
 * HMAX clocks per line. 2 at 60p and below, 2.5 at 75p; the HDR functions are right (1). */
static double driver_time_scale(void)
{
    return hdr_on ? 1 : 2.0 * HMAX_60P / hmax_for_fps(sensor_fps_valid());
}
#define DRIVER_TIME_SCALE driver_time_scale()
volatile sig_atomic_t stop_requested, restart_requested;

const unsigned sensor_fps_steps[N_SENSOR_FPS] = { 24, 25, 30, 48, 50, 60, 72, 75, 90, 100, 120 };

unsigned max_fps(void) { return hdr_on ? 30 : crop_max_fps(crop_w); }
/* Slow-mo takes any whole capture rate from SLOWMO_MIN_FPS to max_fps() (below 60 the driver sets
 * the frame length, above it the line length); a lower rate carried over from another mode reads
 * as the fastest. Other modes use the sensor_fps_steps list. */
unsigned min_fps(void) { return settings.mode == MODE_SLOWMO ? SLOWMO_MIN_FPS : 24; }
unsigned sensor_fps_valid(void)
{
    if (settings.mode == MODE_SLOWMO)
        return settings.sensor_fps < SLOWMO_MIN_FPS || settings.sensor_fps > max_fps() ? max_fps() : settings.sensor_fps;
    for (int i = 0; i < N_SENSOR_FPS; i++)
        if (settings.sensor_fps == sensor_fps_steps[i])
            return settings.sensor_fps > max_fps() || settings.sensor_fps < min_fps() ? max_fps() : settings.sensor_fps;
    return max_fps();
}
const char *tl_interval_text(void)
{
    static char b[16];
    unsigned ms = settings.tl_interval_ms, s = ms / 1000;
    if (ms < 10000)
        snprintf(b, sizeof(b), "%u.%us", s, ms % 1000 / 100);
    else if (s < 60)
        snprintf(b, sizeof(b), "%us", s);
    else if (s % 60)
        snprintf(b, sizeof(b), "%um%02us", s / 60, s % 60);
    else
        snprintf(b, sizeof(b), "%um", s / 60);
    return b;
}

/* Slow-mo playback rate: any whole rate from PLAY_MIN_FPS to one below the capture rate; clips
 * play slower by capture / playback. Timelapse: PLAY_MIN_FPS .. 60. */
unsigned play_fps(void)
{
    unsigned hi = settings.mode == MODE_TIMELAPSE ? 60 : sensor_fps_valid() - 1, p = settings.play_fps;
    return p > hi ? hi : p < PLAY_MIN_FPS ? PLAY_MIN_FPS : p;
}

unsigned frame_us(void) { return 1000000 / sensor_fps_valid(); }
/* Longest manual shutter: 97 % of the frame, or in Timelapse up to the interval (the sensor frame
 * then stretches to fit the exposure). Cap: the driver's pCus_SetAEUSecs computes us x 1000 in 32
 * bits, so it wraps above 4294967 driver units = 2.147 s real at <= 60p, 1.72 s at 75p (measured
 * 2026-09-25: 1 s and 2 s exact from VMAX/SHS1, 3.8 s came out 1.65 s). */
uint32_t max_manual_us(void)
{
    if (settings.mode == MODE_TIMELAPSE) {
        uint32_t cap = (uint32_t)(4290000 / DRIVER_TIME_SCALE), iv = settings.tl_interval_ms * 1000u;
        uint32_t m = iv < cap ? iv : cap;
        return m > frame_us() * 97 / 100 ? m : frame_us() * 97 / 100;
    }
    return frame_us() * 97 / 100;
}
/* Shutter as shown: 1/x, or seconds from 0.4 s up. */
const char *shutter_text(uint32_t us)
{
    static char b[16];
    if (us >= 400000)
        snprintf(b, sizeof(b), "%.1fs", us / 1e6);
    else
        snprintf(b, sizeof(b), "1/%u", (1000000 + us / 2) / (us ? us : 1));
    return b;
}
void apply_exposure(void);
/* Manual shutter time in real us: from the angle at the current output frame rate (angle mode),
 * else the stored speed. */
uint32_t manual_shutter_us(void)
{
    if (!settings.shutter_angle_x10)
        return settings.shutter_us;
    double us = settings.shutter_angle_x10 / 3600.0 * 1e6 / sensor_fps_valid();
    return us < 1 ? 1 : (uint32_t)(us + 0.5);
}
/* JPEG encoder quality factor (word 0 of MI_VENC_ParamJpeg_t; live, measured 30 -> 43 KB, 70 -> 69 KB,
 * 95 -> 193 KB per frame). One encoder serves MJPEG clips, photos and the USB webcam. */
int apply_jpeg_quality(void)
{
    uint32_t p[64] = { 0 };
    uint32_t q = settings.jpeg_quality < 1 ? 1 : settings.jpeg_quality > 100 ? 100 : settings.jpeg_quality;
    int r = MI_VENC_GetJpegParam(VENC_DEV_JPEG, VENC_CHN, p);
    if (r == 0) {
        p[0] = q;
        r = MI_VENC_SetJpegParam(VENC_DEV_JPEG, VENC_CHN, p);
    }
    printf("jpeg: quality %u, rc %d\n", q, r);
    return r;
}

/* SetFps takes milli-fps; halved for the driver bug above. Above 60 fps the driver gets its
 * maximum (1125 lines) and the line length does the rest. */
static int apply_fps(void)
{
    unsigned fps = sensor_fps_valid();
    unsigned req = hdr_on ? fps * 1000 : fps > 60 ? 30000 : fps * 500;
    int r = MI_SNR_SetFps(0, req);
    if (!hdr_on) {
        unsigned hmax = hmax_for_fps(fps);
        sensor_write_reg(0x3001, 1);                /* REGHOLD: both bytes in the same frame */
        sensor_write_reg(0x301c, hmax & 0xff);
        sensor_write_reg(0x301d, hmax >> 8);
        sensor_write_reg(0x3001, 0);
        printf("fps: %u fps (driver request %u mfps, HMAX %u), SetFps = %d\n", fps, req, hmax, r);
    } else {
        printf("fps: %u fps (driver request %u mfps), SetFps = %d\n", fps, req, r);
    }
    return r;
}

/* Apply the current exposure settings: auto, or manual shutter + sensor gain.
 * Manual shutter is clamped to the frame time: if it were longer the driver would stretch the
 * frame (VMAX) and never shrink it back, silently changing the frame rate. The frame rate is
 * re-applied afterwards so VMAX always matches the chosen rate. */
/* AE exposure limits (MI_ISP_AE_ExposureLimit, 8 u32; logs/aeinfo_dump.txt): min shutter, max
 * shutter (driver units: x DRIVER_TIME_SCALE), min/max FN x10, min sensor gain, min ISP gain,
 * max sensor gain, max ISP gain. Vendor: max shutter 10000 (= 1/200 real at 60p), max sensor gain
 * 8x, ISP gain 1x. The max sensor gain also caps manual gain (manual 16x became 8x sensor + 2x ISP),
 * so in manual it is raised to at least the manual gain. Verified: limit 32x -> manual 16x gives
 * GAIN register 80 (24 dB); limit 1000 -> real exposure 489 us. */
/* AE lock: MI_ISP_AE_SetState 1 pauses auto exposure on its current values (verified: +1 EV comp
 * while paused changed nothing, resuming jumped +0.66 EV). Not saved: a lock belongs to one scene. */
int ae_locked;
int osd_hidden;

/* Exposure mode label: M manual, P program (auto), S shutter priority, G gain priority. */
const char *expo_mode_short(void)
{
    return settings.manual ? "M" : settings.ae_priority == 1 ? "S" : settings.ae_priority == 2 ? "G" : "AUTO";
}

static int apply_ae_limits(void)
{
    static uint32_t vendor[8];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_AE_GetExposureLimit(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    uint32_t lim[8];
    memcpy(lim, vendor, sizeof(lim));
    /* ExposureLimit: { min shutter, max shutter (driver us), min FNx10, max FNx10, min sensor gain,
     * min ISP gain, max sensor gain, max ISP gain } (1024 = 1x). */
    lim[0] = 14 * DRIVER_TIME_SCALE;                  /* min shutter ~1 line (vendor 40 us) so 1/64000 works */
    {                                                 /* 0 = Auto: the whole frame (user: not the vendor 1/200) */
        uint32_t frame = frame_us() * 97 / 100, us = settings.ae_max_shutter_us ? settings.ae_max_shutter_us : frame;
        lim[1] = (us < frame ? us : frame) * DRIVER_TIME_SCALE;
        if (settings.manual && max_manual_us() > frame)   /* timelapse long shutter: the limit caps manual too */
            lim[1] = max_manual_us() * DRIVER_TIME_SCALE;
    }
    uint32_t cap = settings.ae_max_gain ? settings.ae_max_gain : vendor[6];
    if (settings.manual && settings.sensor_gain > cap)
        cap = settings.sensor_gain;
    lim[6] = cap;
    /* Priority modes = auto exposure with one side pinned by its limits: S pins the shutter (AE moves
     * only gain), G pins sensor gain and holds ISP gain at 1x (AE moves only the shutter). */
    if (!settings.manual && settings.ae_priority == 1) {
        uint32_t frame = frame_us() * 97 / 100, want = manual_shutter_us();
        lim[0] = lim[1] = (want < frame ? want : frame) * DRIVER_TIME_SCALE;
    } else if (!settings.manual && settings.ae_priority == 2) {
        lim[4] = lim[6] = settings.sensor_gain;
        lim[5] = lim[7] = 1024;
    }
    int r = MI_ISP_AE_SetExposureLimit(0, 0, lim);
    printf("ae limits %s: shutter %u-%u (driver), sensor gain %u-%u, isp gain %u-%u (vendor %u %u %u %u %u %u %u %u), set = %d\n",
           expo_mode_short(), lim[0], lim[1], lim[4], lim[6], lim[5], lim[7], vendor[0], vendor[1], vendor[2], vendor[3],
           vendor[4], vendor[5], vendor[6], vendor[7], r);
    return r;
}

/* AE speed (MI_ISP_AE_Converge, 10 u32: vendor 20 30 | 100 470 470 2000 | 300 200 200 300).
 * The last four are the convergence speeds (probably per error range, breakpoints before them).
 * Measured 2026-09-25 with EV-comp steps traced at 20 Hz (aetrace, logs/aetrace_variants.txt):
 * the sensor applies a new exposure 2-3 frames late, so faster speeds overshoot and hunt.
 * 200 % (600 400 400 600): 12 swings, not settled in 1.5 s (the user's "hunting"). Vendor: one
 * overshoot (~+70 %), settled in ~350 ms. 50 % (150 100 100 150): no overshoot, settled in
 * 200-350 ms, i.e. the fastest stable response; "Instant" uses it. The setting scales the four. */
int apply_ae_speed(void)
{
    static uint32_t vendor[10];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_AE_GetConverge(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    uint32_t c[10];
    memcpy(c, vendor, sizeof(c));
    if (settings.ae_speed >= 1000)                    /* instant: fastest without overshoot (see above) */
        for (int i = 6; i < 10; i++)
            c[i] = vendor[i] / 2;
    else if (settings.ae_speed)
        for (int i = 6; i < 10; i++) {
            uint32_t v = vendor[i] * settings.ae_speed / 100;
            c[i] = v < 1 ? 1 : v > 2000 ? 2000 : v;
        }
    int r = MI_ISP_AE_SetConverge(0, 0, c);
    printf("ae speed: %u%% (%u %u %u %u), set = %d\n", settings.ae_speed ? settings.ae_speed : 100, c[6], c[7], c[8], c[9], r);
    return r;
}

/* Anti-flicker (MI_ISP_AE_Flicker, u32; vendor 2). SigmaStar's enum is 0 off, 1 60 Hz, 2 50 Hz
 * (vendor = 50 Hz, matching its 1/100-based limits); with it on, AE keeps shutter times that are
 * multiples of the mains half-period (10 ms at 50 Hz, 8.33 ms at 60 Hz) so lamps don't band. */
const char *const flicker_names[] = { "Vendor 50Hz", "Off", "50 Hz", "60 Hz" };
int apply_ae_flicker(void)
{
    static const uint32_t api[] = { 2, 0, 2, 1 };
    uint32_t f = api[settings.ae_flicker & 3], back = 99;
    int r = MI_ISP_AE_SetFlicker(0, 0, &f);
    MI_ISP_AE_GetFlicker(0, 0, &back);
    printf("ae flicker: %s (api %u, readback %u), set = %d\n", flicker_names[settings.ae_flicker & 3], f, back, r);
    return r;
}

void expo_mode_step(int dir)
{
    int m = settings.manual ? 3 : (int)(settings.ae_priority % 3);
    m = (m + 4 + dir) % 4;
    settings.manual = m == 3;
    settings.ae_priority = m == 3 ? 0 : m;
    apply_exposure();
}

void apply_exposure(void)
{
    uint32_t mode = settings.manual ? AE_MODE_MANUAL : AE_MODE_AUTO;
    apply_ae_limits();
    apply_ae_speed();
    apply_ae_flicker();
    if (settings.manual)
        ae_locked = 0;                                /* lock only exists in auto */
    uint32_t st = ae_locked;
    MI_ISP_AE_SetState(0, 0, &st);
    if (!settings.manual) {
        printf("exposure: auto%s, SetExpoMode = %d\n", ae_locked ? " (locked)" : "", MI_ISP_AE_SetExpoMode(0, 0, &mode));
        usleep(150000);                               /* let AE take over before resetting VMAX */
        apply_fps();
        return;
    }
    uint32_t max_us = max_manual_us();                /* 97 % of the frame; timelapse: up to the interval */
    uint32_t want = manual_shutter_us(), us = want < max_us ? want : max_us;
    uint32_t expo[4] = { 0 };
    int r1 = MI_ISP_AE_GetManualExpo(0, 0, expo);   /* keep FN and ISP gain as they are */
    expo[EXPO_US] = us * DRIVER_TIME_SCALE;           /* see DRIVER_TIME_SCALE */
    expo[EXPO_SENSOR_GAIN] = settings.sensor_gain;
    int r2 = MI_ISP_AE_SetExpoMode(0, 0, &mode);
    int r3 = MI_ISP_AE_SetManualExpo(0, 0, expo);
    printf("exposure: manual %u us%s (driver %u), sensor gain %u: get=%d mode=%d set=%d\n", us,
           us < want ? " (clamped to frame)" : "", expo[EXPO_US], settings.sensor_gain, r1, r2, r3);
    /* The ISP applies the new exposure a frame or two later; if SetFps ran first, the driver would
     * still see the old (possibly longer) exposure and keep the frame stretched. */
    usleep(150000);                                   /* > 3 frames even at 24 fps */
    apply_fps();
}

/* Playback: the live preview is unbound from the display while a clip plays (see `playing`). */

static void playback_stop(void)
{
    if (!playing)
        return;
    player_stop();
    playing = 0;
    show_spot_area();
    osd_overlay(settings.overlay);
    int r = MI_SYS_BindChnPort2(0, &scl_out, &disp_in, 30, 30, 1, 0);
    printf("playback: stopped, live preview rebound rc %d\n", r);
    osd_notice(NULL, 0);
}

/* Newest clip (highest number, any mode prefix) on the card, or 0 if none. */
int newest_clip(char *path, int size)
{
    DIR *d = opendir(REC_DIR);
    struct dirent *e;
    int best = -1;
    char best_name[64] = "";
    while (d && (e = readdir(d))) {
        int n = clip_number(e->d_name);
        const char *ext = strrchr(e->d_name, '.');
        if (n > best && ext && (!strcmp(ext, ".ts") || !strcmp(ext, ".avi"))) {
            best = n;
            snprintf(best_name, sizeof(best_name), "%s", e->d_name);
        }
    }
    if (d)
        closedir(d);
    if (best < 0)
        return 0;
    snprintf(path, size, REC_DIR "/%s", best_name);
    return 1;
}

void playback_start(const char *path)
{
    if (playing || rec_on) {
        notice_for(rec_on ? "Stop recording first" : "Already playing", OSD_YELLOW, 2);
        return;
    }
    MI_SYS_UnBindChnPort(0, &scl_out, &disp_in);
    if (player_open(path) != 0) {
        MI_SYS_BindChnPort2(0, &scl_out, &disp_in, 30, 30, 1, 0);
        notice_for("Cannot play clip", OSD_RED, 2);
        return;
    }
    playing = 1;
    show_spot_area();                               /* live-only markings off the clip */
    osd_overlay(0);
    printf("playback: %s\n", path);
}

/* While a clip plays: MENU = back to live, play/OK = pause, arrows/AE = skip 5 s. */
int playback_input(const char *cmd)
{
    if (!playing)
        return 0;
    if (!strcmp(cmd, "menu") || !strcmp(cmd, "playstop"))
        playback_stop();
    else if (!strcmp(cmd, "reset") || !strcmp(cmd, "ok") || !strcmp(cmd, "playpause"))
        player_toggle_pause();
    else if (!strcmp(cmd, "left") || !strcmp(cmd, "ae") || !strcmp(cmd, "down"))
        player_seek(-5);
    else if (!strcmp(cmd, "right") || !strcmp(cmd, "up"))
        player_seek(5);
    else if (strcmp(cmd, "play"))
        return 0;                                   /* anything else (FIFO debug...) still works */
    return 1;
}

/* Command pipe: echo "shutter 20833" > /tmp/camapp_fifo */
static void *fifo_thread(void *arg)
{
    prctl(PR_SET_NAME, "fifo");                 /* thread name for top / cpustat */
    const char *path = arg;
    char line[128];
    mkfifo(path, 0666);
    for (;;) {
        FILE *f = fopen(path, "r");      /* blocks until a writer opens it */
        if (!f)
            return NULL;
        while (fgets(line, sizeof(line), f)) {
            handle_cmd(line);
            fflush(stdout);
        }
        fclose(f);
    }
}

static void on_input_action(const char *action)
{
    char line[64];
    snprintf(line, sizeof(line), "%s\n", action);
    handle_cmd(line);
    fflush(stdout);
}


#define SOC_HOT_C 80                            /* status line turns yellow and shows the temperature */

/* SoC temperature in degrees C from the msys driver ("Temperature 57"), -1 if unreadable. */
int soc_temp_c(void)
{
    int c = -1;
    FILE *f = fopen("/sys/devices/virtual/mstar/msys/TEMP_R", "r");
    if (f) {
        if (fscanf(f, "%*s %d", &c) != 1)
            c = -1;
        fclose(f);
    }
    return c;
}

/* Card data rate in bytes/s: measured while recording (smoothed), else estimated from the
 * codec settings (H.265: bitrate + ~3 % TS overhead; MJPEG: current JPEG size x record fps). */
static double rec_rate_bps(void)
{
    static uint64_t last_bytes;
    static double last_t, measured;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double now = ts.tv_sec + ts.tv_nsec / 1e9;
    if (!rec_on) {
        last_t = measured = 0;
    } else if (last_t == 0) {
        last_t = now;
        last_bytes = rec_bytes_total;
    } else if (now - last_t >= 1) {
        double r = (rec_bytes_total - last_bytes) / (now - last_t);
        measured = measured > 0 ? measured * 0.95 + r * 0.05 : r;   /* ~20 s smoothing */
        last_t = now;
        last_bytes = rec_bytes_total;
    }
    if (measured > 0)
        return measured;
    if (settings.rec_codec != 0)
        return settings.rec_mbps * 1e6 / 8 * 1.03;
    /* MJPEG: JPEG size follows scene detail frame by frame; smooth over ~20 s (called once a second) */
    static double mjpeg;
    unsigned sfps = sensor_fps_valid();
    double est = (double)venc_last_size * (sfps == 60 && settings.rec_fps == 30 ? 30 : sfps);
    mjpeg = mjpeg > 0 ? mjpeg * 0.95 + est * 0.05 : est;
    return mjpeg;
}

uint32_t live_shutter_us;                   /* real exposure and total gain, refreshed with the status bar */
double live_gain = 1;

/* Top status bar: record state + timer | exposure | fps, mirror/flip, SD free + time left. */
static void update_osd(void)
{
    char left[48], centre[64], right[80], sd[32] = "NO SD";
    uint32_t info[0x23c / 4] = { 0 };
    MI_ISP_AE_QueryExposureInfo(0, 0, info);        /* live values, auto or manual */
    uint32_t isp_gain = info[4] ? info[4] : 1024;   /* ISP digital gain on top of the sensor gain */
    double gain = info[3] / 1024.0 * isp_gain / 1024.0; /* total: what the picture actually gets */
    const char *digital = isp_gain > 1075 ? " d" : "";  /* > 5 %: mark that part of it is digital */
    double real_fps = 0, real_us = 0;               /* from the sensor registers: the truth */
    if (hdr_on) {                                   /* long exposure from the ISP; output = sensor rate / 2 */
        if (sensor_timing(&real_fps, &real_us) == 0)
            real_fps /= 2;
        real_us = info[5] ? info[5] : 1;
    } else if (sensor_timing(&real_fps, &real_us) || real_us < 1) {
        real_us = info[5] ? info[5] / (double)DRIVER_TIME_SCALE : 1;
    }
    uint32_t us = (uint32_t)(real_us + 0.5);
    live_shutter_us = us;
    live_gain = gain;

    static int next_no;                             /* clip counter: rescanned every few seconds in standby */
    static time_t next_at;
    if (rec_on) {
        long s = time(NULL) - rec_started;
        /* slow card: queue more than half full (card not keeping up) or frames already lost */
        char warn[24] = "";
        long dropped = rec_dropped - rec_dropped_at_start;
        int fill = rec_count * 100 / REC_QUEUE, fill_bytes = (int)((uint64_t)rec_queued_bytes * 100 / REC_MEM_MAX);
        fill = fill_bytes > fill ? fill_bytes : fill;
        if (dropped > 0)
            snprintf(warn, sizeof(warn), " DROP %ld", dropped);
        else if (fill > 50)
            snprintf(warn, sizeof(warn), " BUF %d%%", fill);
        if (settings.mode == MODE_TIMELAPSE) {       /* frames shot = clip length at the playback rate */
            long fr = rec_frames_total - rec_frames_at_start;
            snprintf(left, sizeof(left), "* REC %02ld:%02ld #%04d %ldfr=%.1fs%s", s / 60, s % 60, rec_file_no, fr,
                     fr / (double)play_fps(), warn);
        } else {
            snprintf(left, sizeof(left), "* REC %02ld:%02ld #%04d%s", s / 60, s % 60, rec_file_no, warn);
        }
        next_at = 0;
    } else {
        if (time(NULL) >= next_at) {
            next_no = sd_mounted() ? clip_next_no() : 0;
            if (next_no && rec_file_no >= next_no)  /* same rule as rec_open_next */
                next_no = rec_file_no + 1;
            next_at = time(NULL) + 3;
        }
        if (next_no)
            snprintf(left, sizeof(left), "STBY #%04d", next_no);
        else
            snprintf(left, sizeof(left), "STBY");
    }
    if (playing && !player_active())                   /* the clip ended with an error: back to live */
        playback_stop();
    double ev = ev_offset(info);
    int clipped = info[AE_AVG_Y] >= AE_AVG_Y_CLIP;
    char evs[16];
    snprintf(evs, sizeof(evs), clipped ? "EV >%+.1f" : "EV %+.1f", ev);
    char comp[12] = "";
    if (settings.ev_thirds && !settings.manual)
        snprintf(comp, sizeof(comp), " %+.1f", settings.ev_thirds / 3.0);
    double angle = real_fps > 0 ? real_us * real_fps * 360 / 1e6 : 0;   /* shutter angle, cinema style */
    char ang[16] = "";
    if (angle >= 1)
        snprintf(ang, sizeof(ang), " %.0f" OSD_DEGREE, angle);
    snprintf(centre, sizeof(centre), "%s%s %s%s  %.1fx%s  %s %s  LV %.1f", ae_locked && !settings.manual ? "AE-L" : expo_mode_short(), comp,
             shutter_text(us), ang, gain, digital, metering_names[settings.metering % 3], evs, info[AE_LV_X10] / 10.0);
    if (sd_mounted()) {
        struct statvfs st;
        if (statvfs(REC_DIR, &st) == 0) {
            double free_b = (double)st.f_bavail * st.f_frsize, bps = rec_rate_bps();
            long left_s = bps > 0 ? (long)(free_b / bps) : 0, h = left_s / 3600, m = left_s % 3600 / 60;
            if (bps <= 0)
                snprintf(sd, sizeof(sd), "%.1fG", free_b / (1u << 30));   /* GiB, like df */
            else if (h > 0)
                snprintf(sd, sizeof(sd), "%.1fG %ldh%02ldm", free_b / (1u << 30), h, m);
            else
                snprintf(sd, sizeof(sd), "%.1fG %ldm", free_b / (1u << 30), m);
        }
    }
    char rate[16];
    if (real_fps > 0)
        snprintf(rate, sizeof(rate), "%.3gp%s", real_fps, sensor_fps_valid() == 60 && settings.rec_fps == 30 && settings.mode != MODE_SLOWMO ? " R30" : "");
    if (settings.mode == MODE_SLOWMO)               /* capture > playback rate */
        snprintf(rate, sizeof(rate), "%up>%u", sensor_fps_valid(), play_fps());
    else if (settings.mode == MODE_TIMELAPSE)       /* one frame per interval > playback rate */
        snprintf(rate, sizeof(rate), "TL %s>%u", tl_interval_text(), play_fps());
    else
        snprintf(rate, sizeof(rate), "%up", sensor_fps_valid());
    char loupe[16] = "";
    if (loupe_on)
        snprintf(loupe, sizeof(loupe), " LOUPE %d.%dx", loupe_z10 / 10, loupe_z10 % 10);
    snprintf(right, sizeof(right), "%s%s%s%s%s  %s", rate, hdr_on ? " HDR" : "", loupe,
             settings.mirror ? " MIR" : "", settings.flip ? " FLIP" : "", sd);

    /* Second line: the settings otherwise only visible in the menus. */
    char codec[16], wb[16], nr[32], wdr[16], aes[16] = "", temp[16] = "";   /* bottom corner blocks */
    if (settings.rec_codec != 0)
        snprintf(codec, sizeof(codec), "%s %uM", settings.rec_codec == 2 ? "H264" : "H265", settings.rec_mbps);
    else
        snprintf(codec, sizeof(codec), "MJPG");
    if (settings.wb_mode == 1) {
        snprintf(wb, sizeof(wb), "WB %uK", settings.wb_kelvin);
    } else if (settings.wb_mode == 2) {
        snprintf(wb, sizeof(wb), "WB M");
    } else {                                        /* auto: the temperature AWB is using now */
        uint16_t awb[12] = { 0 };
        if (MI_ISP_AWB_QueryInfo(0, 0, awb) == 0 && awb[6])
            snprintf(wb, sizeof(wb), "AWB %uK", awb[6]);
        else
            snprintf(wb, sizeof(wb), "AWB");
    }
    char tnr[12], lnr[12];
    if (settings.nr3d_gain)
        snprintf(tnr, sizeof(tnr), "%u", settings.nr3d_gain);
    else
        snprintf(tnr, sizeof(tnr), "off");
    if (settings.nr_luma < 0)
        snprintf(lnr, sizeof(lnr), "auto");
    else if (settings.nr_luma == 0)
        snprintf(lnr, sizeof(lnr), "off");
    else
        snprintf(lnr, sizeof(lnr), "%d%%", settings.nr_luma);
    snprintf(nr, sizeof(nr), "TNR %s NR %s", tnr, lnr);
    if (hdr_on)
        snprintf(wdr, sizeof(wdr), "WDR -");
    else if (settings.wdr < 0)
        snprintf(wdr, sizeof(wdr), "WDR auto");
    else if (settings.wdr == 0)
        snprintf(wdr, sizeof(wdr), "WDR off");
    else
        snprintf(wdr, sizeof(wdr), "WDR %d", settings.wdr);
    if (!settings.manual) {
        if (settings.ae_speed == 0)
            snprintf(aes, sizeof(aes), "  AE std");
        else if (settings.ae_speed >= 1000)
            snprintf(aes, sizeof(aes), "  AE inst");
        else
            snprintf(aes, sizeof(aes), "  AE %u%%", settings.ae_speed);
    }
    int soc_c = soc_temp_c();
    int hot = soc_c >= SOC_HOT_C;
    if (hot)
        snprintf(temp, sizeof(temp), "\nSoC %d" OSD_DEGREE "C", soc_c);
    char bl[48], br[96];                            /* left: recording, right: processing */
    snprintf(bl, sizeof(bl), "%s\n%s\nTC %s", codec, wb, tc_text(rec_tc_ms(), rec_on ? (unsigned)rec_fps : file_fps()));
    snprintf(br, sizeof(br), "%s\n%s%s%s", nr, wdr, aes, temp);
    show_spot_area();
    tally_update();
    if (notice_until && time(NULL) >= notice_until) {
        notice_until = 0;
        osd_notice(NULL, 0);
    }
    if (menu_open)
        menu_draw();
    if (playing) {                                  /* playback: clip, time and state instead of exposure */
        snprintf(left, sizeof(left), "PLAY");
        player_status(centre, sizeof(centre));
    }
    osd_status(left, rec_on ? OSD_RED : OSD_WHITE, centre, (clipped || fabs(ev) > 1.0) ? OSD_YELLOW : OSD_WHITE, right,
               bl, br, hot ? OSD_YELLOW : OSD_WHITE);
}

static void on_signal(int sig) { stop_requested = 1; }

/* Usage: camapp [secs] [hdr]   secs = 0 (default) runs until SIGTERM/SIGINT.
 * "hdr" (test only, not persisted): sensor DOL-HDR plane mode, VIF + ISP HDR type DOL. */
int main(int argc, char **argv)
{
    int secs = 0, hdr = -1;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "hdr"))
            hdr = 1;
        else
            secs = atoi(argv[i]);
    }
    struct sigaction sa = { .sa_handler = on_signal };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    setvbuf(stdout, NULL, _IOLBF, 0);   /* keep log lines if we crash */
    printf("camapp build " __DATE__ " " __TIME__ "\n");
    uint32_t vif_grp[7] = { 4, 0, 0, 2, 0, 0, 0 };             /* MIPI, work mode 2 */
    uint32_t vif_dev[5] = { 0x20, 0, 0x04380780, 0, 0 };        /* RG 10bpp, 1920x1080 */
    uint32_t vif_port[5] = { 0, 0x04380780, 0x04380780, 0x20, 0 };
    uint32_t isp_dev[2] = { 1, 0 };
    uint32_t isp_chn[20] = { 1 };                               /* sensor pad 0 */
    uint32_t scl_dev[1] = { 15 };               /* hw port mask: 0 display, 1 JPEG, 2 video encoder, 3 analysis */
    uint32_t scl_chn[1] = { 0 };
    uint32_t scl_param[1] = { 0 };
    uint32_t scl_port0[6] = { 0, 0x04380780, 0x04380780, 0x101, 0xb, 0 };   /* 1920x1080 NV12 */
    uint32_t disp_pub[13] = { 0x800080, 5, 0x10 };             /* bg, HDMI, 1080p60; zero sync info */
    uint32_t layer_attr[5] = { 0, 0x04380780, 0x780, 0x438, 0 };
    uint32_t port_attr[3] = { 0, 0x04380780, 0x04380780 };
    struct { HdmiEventCb cb; void *user; } hdmi_init = { hdmi_event, NULL };
    uint32_t hdmi_attr[12] = { 0, 1, 9, 0, 0, 0, 4 };          /* enable, timing 9 = 1080p60 */
    uint32_t scl_port1[6] = { 0, 0, 0x04380780, 0x101, 0xb, 0 };           /* no crop, 1920x1080 NV12 */
    uint32_t venc_dev[8] = { 0x2000, 0x2000 };                             /* max 8192x8192 */
    uint32_t venc_chn[19] = { 4, 1920, 1080, 0xc00000, 1, 1920, 1080,     /* JPEG, max size, buf, by frame, pic size */
                              0, 0, 0, 7, 30, 1, 70 };                    /* rc 7, 30/1 fps, quality 70 */
    int rc = 1, pipeline_ok = 0;

    settings_load(SETTINGS_PATH);                    /* first: the sensor mode (HDR) comes from it */
    dng_fbc_off();                                   /* before VIF: real raw for DNG photos */
    DO(MI_SYS_Init(0));                              UNDO(u_sys);
    DO(MI_SNR_InitDev(NULL));                        UNDO(u_snr_init);
    if (hdr < 0)
        hdr = settings.mode == MODE_HDR;
    hdr_on = hdr;
    DO(MI_SNR_SetPlaneMode(0, hdr));
    int res_idx = 0;                                 /* index 0 = 1920x1080@60 */
    if (hdr) {
        sensor_debug("hdr plane mode");
        res_idx = sensor_find_mode("HDR");
        printf("hdr: mode index %d\n", res_idx);
        if (res_idx < 0)
            goto fail;
        vif_grp[2] = 2;                              /* VIF HDR type DOL */
        vif_dev[0] = vif_port[3] = 0x2c;             /* DOL planes are RG 12bpp (plane info: precision 2) */
        isp_param[0] = 2;                            /* ISP HDR type DOL */
    }
    DO(MI_SNR_SetRes(0, res_idx));
    for (int plane = 0; hdr && plane < 2; plane++) {
        uint32_t info[18] = { 0 };
        int r = MI_SNR_GetPlaneInfo(0, plane, info);
        printf("hdr: plane %d info rc %d:", plane, r);
        for (int i = 0; i < 18; i++) printf(" %08x", info[i]);
        printf("\n");
    }
    DO(MI_SNR_Enable(0));                            UNDO(u_snr_en);
    if (!hdr)
        crop_load();                                 /* after settings_load */
    if (crop_w) {
        if (!OPEN_GATE)                              /* open gate: the sensor's default full output */
            crop_sensor();
        vif_dev[1] = OPEN_GATE ? 0 : ((1080 - crop_h) / 2) << 16;             /* dev crop rect: x | y << 16, w | h << 16 (x 14 made
                                                                 * the left-edge garbage at 110p worse) */
        vif_dev[2] = crop_w | crop_h << 16;
        vif_port[1] = vif_port[2] = crop_w | crop_h << 16;     /* port: whole dev rect, no offset */
        scl_port0[1] = crop_w | crop_h << 16;
    }
    DO(MI_VIF_CreateDevGroup(0, vif_grp));           UNDO(u_vif_grp);
    DO(MI_VIF_SetDevAttr(0, vif_dev));
    DO(MI_VIF_EnableDev(0));                         UNDO(u_vif_dev);
    DO(MI_VIF_SetOutputPortAttr(0, 0, vif_port));
    DO(MI_VIF_EnableOutputPort(0, 0));               UNDO(u_vif_port);
    DO(MI_ISP_CreateDevice(0, isp_dev));             UNDO(u_isp_dev);
    DO(MI_ISP_CreateChannel(0, 0, isp_chn));         UNDO(u_isp_chn);
    DO(MI_ISP_SetChnParam(0, 0, isp_param));
    if (crop_w) {
        uint16_t rect[4] = { 0 };
        int g = MI_ISP_GetInputPortCrop(0, 0, rect);
        printf("crop: isp input crop was %u,%u %ux%u (rc %d)\n", rect[0], rect[1], rect[2], rect[3], g);
        rect[0] = rect[1] = 0, rect[2] = crop_w, rect[3] = crop_h;
        printf("crop: isp input crop set: rc %d\n", MI_ISP_SetInputPortCrop(0, 0, rect));
    }
    DO(MI_ISP_StartChannel(0, 0));                   UNDO(u_isp_start);
    DO(MI_ISP_EnableOutputPort(0, 0, 0));            UNDO(u_isp_port);
    DO(MI_SYS_BindChnPort2(0, &vif_out, &isp_in, 30, 30, 1, 0)); UNDO(u_bind);
    DO(load_isp_api_bin());

    DO(MI_SCL_CreateDevice(0, scl_dev));             UNDO(u_scl_dev);
    DO(MI_SCL_CreateChannel(0, 0, scl_chn));         UNDO(u_scl_chn);
    DO(MI_SCL_SetChnParam(0, 0, scl_param));
    DO(MI_SCL_StartChannel(0, 0));                   UNDO(u_scl_start);
    DO(MI_SCL_SetOutputPortParam(0, 0, 0, scl_port0));
    DO(MI_SCL_EnableOutputPort(0, 0, 0));            UNDO(u_scl_port);
    DO(MI_SYS_SetChnOutputPortDepth(0, &scl_out, 0, 3));
    DO(MI_SYS_BindChnPort2(0, &isp_out, &scl_in, 30, 30, 4, 0)); UNDO(u_bind_isp_scl);

    DO(MI_DISP_InitDev(NULL));                       UNDO(u_disp_init);
    DO(MI_DISP_SetPubAttr(0, disp_pub));
    DO(MI_DISP_Enable(0));                           UNDO(u_disp_en);
    DO(MI_HDMI_Init(&hdmi_init));                    UNDO(u_hdmi_init);
    DO(MI_HDMI_Open(0));                             UNDO(u_hdmi_open);
    DO(MI_HDMI_SetAttr(0, hdmi_attr));
    DO(MI_HDMI_Start(0));                            UNDO(u_hdmi_start);   /* not waiting for hotplug */
    DO(MI_DISP_BindVideoLayer(0, 0));                UNDO(u_layer_bind);
    DO(MI_DISP_SetVideoLayerAttr(0, layer_attr));
    DO(MI_DISP_EnableVideoLayer(0));                 UNDO(u_layer_en);
    DO(MI_DISP_SetInputPortAttr(0, 0, port_attr));
    DO(MI_SYS_BindChnPort2(0, &scl_out, &disp_in, 30, 30, 1, 0)); UNDO(u_bind_scl_disp);
    DO(MI_DISP_EnableInputPort(0, 0));               UNDO(u_disp_port);
    DO(MI_DISP_SetInputPortSyncMode(0, 0, 2));

    DO(MI_SCL_SetOutputPortParam(0, 0, 1, scl_port1));
    DO(MI_SCL_EnableOutputPort(0, 0, 1));            UNDO(u_scl_port1);
    DO(MI_VENC_CreateDev(VENC_DEV_JPEG, venc_dev)); UNDO(u_venc_dev);
    DO(MI_VENC_CreateChn(VENC_DEV_JPEG, VENC_CHN, venc_chn)); UNDO(u_venc_chn);
    DO(MI_SYS_BindChnPort2(0, &scl_out1, &venc_in, 30, 30, 1, 0)); UNDO(u_bind_scl_venc);
    DO(MI_VENC_StartRecvPic(VENC_DEV_JPEG, VENC_CHN)); UNDO(u_venc_start);
    pipeline_ok = 1;

    pthread_t fifo;
    pthread_create(&fifo, NULL, fifo_thread, "/tmp/camapp_fifo");
    pthread_detach(fifo);

    pthread_t venc;
    venc_run = 1;
    pthread_create(&venc, NULL, venc_thread, NULL);

    step("uvc_start(/dev/video0)", uvc_start("/dev/video0"));

    printf("settings: mirror %u flip %u manual %u shutter %u gain %u rec_fps %u\n", settings.mirror,
           settings.flip, settings.manual, settings.shutter_us, settings.sensor_gain, settings.rec_fps);
    step("apply_orientation()", apply_orientation());
    step("apply_fps()", apply_fps());
    step("install_weight_tables()", install_weight_tables());
    step("apply_metering()", apply_metering());
    step("apply_nr3d()", apply_nr3d());
    step("apply_wdr()", apply_wdr());
    step("apply_isp_image()", apply_isp_image());
    step("apply_jpeg_quality()", apply_jpeg_quality());
    step("apply_wb()", apply_wb());
    step("apply_peaking()", apply_peaking());
    step("apply_ev_comp()", apply_ev_comp());
    if (settings.manual)
        apply_exposure();                           /* after apply_fps: the driver recomputes lines */
    step("input_start()", input_start(on_input_action));
    step("osd_start()", osd_start());
    osd_overlay(settings.overlay);
    led_set(1);

    long a = vif_isr_count();
    int s;
    for (s = 0; (secs == 0 || s < secs) && !stop_requested; s++) {
        if (secs || s % 10 == 0)                     /* daemon mode: status every 10 s */
            print_expo(s);
        led_set(rec_on ? s & 1 : 1);                /* blink while recording */
        sd_hotplug_check();
        if (rec_stop_pending && rec_on) {           /* timelapse frame limit reached */
            long n = settings.tl_limit_frames;
            rec_set(0, 0);
            char msg[48];
            snprintf(msg, sizeof(msg), "Timelapse done: %ld frames", n);
            notice_for(msg, OSD_WHITE, 5);
        }
        update_osd();
        if (settings_dirty_at && time(NULL) - settings_dirty_at >= SETTINGS_SAVE_DELAY) {
            settings_dirty_at = 0;
            printf("settings: saved, rc %d\n", settings_save(SETTINGS_PATH));
        }
        fflush(stdout);
        sleep(1);
    }
    if (stop_requested)
        printf("stop requested after %d s\n", s);
    secs = s ? s : 1;
    long b = vif_isr_count();
    printf("VIF IsrCnt %ld -> %ld over %d s (%.1f/s)\n", a, b, secs, (b - a) / (double)secs);
    input_stop();
    playback_stop();                                 /* rebinds the live preview before teardown */
    peak_stop();                                     /* uses the OSD and SCL port 3 */
    osd_stop();
    if (settings_dirty_at)
        printf("settings: saved at exit, rc %d\n", settings_save(SETTINGS_PATH));
    rec_set(0, 0);
    led_set(1);
    venc_run = 0;
    pthread_join(venc, NULL);
    uvc_stop();
    printf("encoder frames: %ld (%.1f/s)\n", venc_frames, venc_frames / (double)secs);
    rc = (a >= 0 && b > a) ? 0 : 2;
    printf("RESULT: %s\n", rc == 0 ? "PASS frames flowing" : "FAIL no frames");

fail:
    if (!pipeline_ok && crop_w && !restart_requested) {   /* a crop the pipeline refuses: start full frame */
        printf("crop: pipeline start failed at %ux%u, crop off, restarting\n", crop_w, crop_h);
        settings.crop_w = settings.crop_h = 0;
        settings_save(SETTINGS_PATH);
        restart_requested = 1;
    }
    while (n_undo > 0) {
        n_undo--;
        step(undo[n_undo].name, undo[n_undo].fn());
    }
    if (restart_requested) {                        /* sensor mode change: start over in a clean process */
        printf("restart: re-exec for sensor mode change\n");
        fflush(stdout);
        execl("/proc/self/exe", argv[0], (char *)NULL);
        printf("restart: exec failed\n");
    }
    return rc;
}
