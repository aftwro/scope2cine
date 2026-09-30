/* Focus and exposure aids (see camapp.h): the loupe (display crop) and the analysis thread that
 * reads a 480x270 copy of the picture (SCL port 3) for peaking, zebra, false colour, the histogram
 * and the waveform. */
#include "camapp.h"

#include <pthread.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "osd.h"

/* Focus loupe: crops only the display output (SCL port 0) and scales it back to 1920x1080,
 * so recording (port 2) and JPEG/UVC (port 1) keep the full frame. Verified live, no restart. */
int loupe_on, loupe_z10 = 20, loupe_cx = 960, loupe_cy = 540;   /* zoom x10, centre in pixels */

static void peak_set_crop(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

int apply_loupe(void)
{
    uint32_t x = 0, y = 0, w = SRC_W, h = SRC_H;    /* loupe coordinates are in the SCL input (crop) */
    if (loupe_on) {
        w = (SRC_W * 10 / loupe_z10) & ~15;         /* scaler-friendly sizes */
        h = (SRC_H * 10 / loupe_z10) & ~1;
        loupe_cx = loupe_cx < (int)w / 2 ? (int)w / 2 : loupe_cx > (int)(SRC_W - w / 2) ? (int)(SRC_W - w / 2) : loupe_cx;
        loupe_cy = loupe_cy < (int)h / 2 ? (int)h / 2 : loupe_cy > (int)(SRC_H - h / 2) ? (int)(SRC_H - h / 2) : loupe_cy;
        x = (loupe_cx - w / 2) & ~1;
        y = (loupe_cy - h / 2) & ~1;
    }
    uint32_t p0[6] = { loupe_on ? x | y << 16 : 0, w | h << 16, 0x04380780, 0x101, 0xb, 0 };
    int r = MI_SCL_SetOutputPortParam(0, 0, 0, p0);
    peak_set_crop(loupe_on ? x : 0, loupe_on ? y : 0, w, h);   /* peaking follows the loupe view */
    printf("loupe: %s %d.%dx crop %u,%u %ux%u, set = %d\n", loupe_on ? "on" : "off", loupe_z10 / 10, loupe_z10 % 10,
           x, y, w, h, r);
    /* The arrows move the picture (user preference), so the map is mirrored to move with them. */
    osd_loupe_map(loupe_on, ((double)SRC_W - x - w) / SRC_W, ((double)SRC_H - y - h) / SRC_H, (double)w / SRC_W,
                  (double)h / SRC_H);
    return r;
}

/* Frame analysis: focus peaking, zebra, false colour. SCL output port 3 gives a 480x270 NV12 copy
 * of what the display shows (same crop as the loupe); a thread takes the newest frame ~33x a second,
 * classifies every luma sample and hands a palette-index map to the OSD (osd_cells). Priority:
 * peaking > zebra > false colour. BufInfo layout (measured with pk_probe 2026-09-25): +0x38 u16 width,
 * u16 height, +0x3c luma pointer, +0x60 luma stride, +0x24 pixel format (0xb NV12). */
#define PEAK_W 480
#define PEAK_H 270
static ChnPort scl_out3 = { MOD_SCL, 0, 0, 3 };
static volatile int peak_run;

volatile int view_request;               /* viewgrab: save the next analysis frame as /tmp/view.ppm */

/* 160x90 sRGB PPM of the processed picture (what the display shows), from the 480x270 NV12
 * analysis frame: a way to see the image over UART when neither HDMI nor USB can be watched. */
static void view_save(const uint8_t *y, uint32_t stride, const uint8_t *uv, uint32_t uv_stride, int w, int h)
{
    FILE *f = fopen("/tmp/view.ppm", "wb");
    if (!f)
        return;
    fprintf(f, "P6\n160 90\n255\n");
    for (int oy = 0; oy < 90; oy++)
        for (int ox = 0; ox < 160; ox++) {
            int i = ox * w / 160, j = oy * h / 90;
            int yy = y[j * stride + i], u = uv[(j / 2) * uv_stride + (i & ~1)] - 128, v = uv[(j / 2) * uv_stride + (i & ~1) + 1] - 128;
            int rgb[3] = { yy + (1436 * v >> 10), yy - ((352 * u + 731 * v) >> 10), yy + (1815 * u >> 10) };
            for (int c = 0; c < 3; c++)
                fputc(rgb[c] < 0 ? 0 : rgb[c] > 255 ? 255 : rgb[c], f);
        }
    fclose(f);
}
static pthread_t peak_tid;
static uint32_t peak_crop[4] = { 0, 0, 1920, 1080 };
static const uint32_t peak_colours[4] = { 0xffff3030, 0xffffe020, 0xffffffff, 0xff30a0ff };
const char *const peak_colour_names[4] = { "Red", "Yellow", "White", "Blue" };

static void peak_set_crop(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    peak_crop[0] = x;
    peak_crop[1] = y;
    peak_crop[2] = w;
    peak_crop[3] = h;
    if (!peak_run)
        return;
    uint32_t param[6] = { x | y << 16, (w == 1920 && h == 1080) ? 0 : (w | h << 16), PEAK_W | PEAK_H << 16, 0x101, 0xb, 0 };
    MI_SCL_SetOutputPortParam(0, 0, 3, param);
}

static void *peak_thread(void *arg)
{
    prctl(PR_SET_NAME, "scopes");                 /* thread name for top / cpustat */
    static uint8_t mask[PEAK_H * PEAK_W];
    uint8_t info[0x200], next[0x200];
    long n = 0, t_calc = 0, t_draw = 0;
    struct timespec a, b, c, t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (peak_run) {
        usleep(66000);                                  /* 15 updates/s: enough for the eye; 33/s took most of a core
                                                         * (measured 2026-09-29: 59% of a core, 48 ms drawing each) */
        uint32_t handle = 0, last = 0;
        int have = 0;
        while (MI_SYS_ChnOutputPortGetBuf(&scl_out3, next, &handle) == 0) {   /* keep only the newest */
            if (have)
                MI_SYS_ChnOutputPortPutBuf(last);
            memcpy(info, next, sizeof(info));
            last = handle;
            have = 1;
        }
        if (!have)
            continue;
        static int cleared;                             /* overlays blanked for the current clip */
        if (playing) {                                  /* live analysis means nothing over a clip */
            if (!cleared) {
                osd_cells(NULL, PEAK_W, PEAK_H, NULL, 0);
                osd_histogram(NULL);
                osd_waveform(NULL);
                cleared = 1;
            }
            MI_SYS_ChnOutputPortPutBuf(last);
            continue;
        }
        cleared = 0;
        const uint8_t *y = *(uint8_t **)(info + 0x3c);
        uint32_t stride = *(uint32_t *)(info + 0x60);
        int w = *(uint16_t *)(info + 0x38), h = *(uint16_t *)(info + 0x3a);
        if (view_request && y && *(uint8_t **)(info + 0x40)) {
            view_save(y, stride, *(uint8_t **)(info + 0x40), *(uint32_t *)(info + 0x64), w, h);
            view_request = 0;
        }
        clock_gettime(CLOCK_MONOTONIC, &a);
        if (w == PEAK_W && h == PEAK_H && y) {
            /* palette: 1 peaking, 2 zebra, 3.. false-colour bands */
            /* zebra and false colour are ~60 % opaque so the picture shows through (ARGB) */
            uint32_t pal[9] = { 0, peak_colours[settings.peak_colour & 3], 0xa0f0f0f0,
                                0xa09020ff, 0xa02050ff, 0xa020c040, 0xa0ff90c8, 0xa0ffe020, 0xa0ff2020 };
            uint8_t fc[256];                            /* false-colour lookup by luma */
            for (int v = 0; v < 256; v++) {
                int pc = v * 100 / 255;
                fc[v] = pc < 3 ? 3 : pc < 10 ? 4 : (pc >= 38 && pc <= 42) ? 5 : (pc >= 52 && pc <= 56) ? 6 :
                        pc >= 99 ? 8 : pc >= 97 ? 7 : 0;
            }
            /* luma histogram of the displayed picture, smoothed 50/50, drawn every 3rd update */
            static uint32_t hist[256], shown[256];
            static int hist_tick;
            if (settings.histogram) {
                memset(hist, 0, sizeof(hist));
                for (int j = 0; j < h; j += 2)
                    for (int i = 0; i < w; i += 2)
                        hist[y[j * stride + i]]++;
                if (++hist_tick % 3 == 0) {
                    for (int k = 0; k < 256; k++)
                        shown[k] = (shown[k] + hist[k]) / 2;
                    osd_histogram(shown);
                    if (!settings.histogram)        /* switched off while drawing: clear again */
                        osd_histogram(NULL);
                }
            }
            /* waveform: column bucket x level, every 3rd update. Luma, or R/G/B from Y + the NV12
             * chroma (BT.601 full range), overlaid or as a parade (each channel in its own third). */
            static uint16_t wf[WF_H * WF_W], wr[WF_H * WF_W], wg[WF_H * WF_W], wb[WF_H * WF_W];
            static int wf_tick;
            const uint8_t *uv = *(uint8_t **)(info + 0x40);
            uint32_t uv_stride = *(uint32_t *)(info + 0x64);
            if (settings.waveform >= 2 && uv && ++wf_tick % 3 == 0) {
                int parade = settings.waveform == 3;
                memset(wr, 0, sizeof(wr));
                memset(wg, 0, sizeof(wg));
                memset(wb, 0, sizeof(wb));
                uint16_t *ch[3] = { wr, wg, wb };
                for (int j = 0; j < h; j += 2)
                    for (int i = 0; i < w; i++) {
                        int yy = y[j * stride + i], u = uv[(j / 2) * uv_stride + (i & ~1)] - 128,
                            v = uv[(j / 2) * uv_stride + (i & ~1) + 1] - 128;
                        int rgb[3] = { yy + (1436 * v >> 10), yy - ((352 * u + 731 * v) >> 10), yy + (1815 * u >> 10) };
                        for (int c = 0; c < 3; c++) {
                            int lv = rgb[c] < 0 ? 0 : rgb[c] > 255 ? 255 : rgb[c];
                            int col = parade ? c * (WF_W / 3) + i * (WF_W / 3) / w : i * WF_W / w;
                            uint16_t *cnt = &ch[c][(lv * (WF_H - 1) / 255) * WF_W + col];
                            if (*cnt < 60000)
                                (*cnt)++;
                        }
                    }
                osd_waveform_rgb(wr, wg, wb, parade);
                if (!settings.waveform)
                    osd_waveform(NULL);
            } else if (settings.waveform == 1 && ++wf_tick % 3 == 0) {
                memset(wf, 0, sizeof(wf));
                for (int j = 0; j < h; j += 2)
                    for (int i = 0; i < w; i++) {
                        int lv = y[j * stride + i] * (WF_H - 1) / 255;
                        uint16_t *c = &wf[lv * WF_W + i * WF_W / w];
                        if (*c < 60000)
                            (*c)++;
                    }
                osd_waveform(wf);
                if (!settings.waveform)
                    osd_waveform(NULL);
            }
            int overlays = settings.peaking || settings.zebra || settings.false_colour;
            int thr = 110 - (int)(settings.peaking > 10 ? 10 : settings.peaking) * 10;
            int zthr = settings.zebra ? ((int)settings.zebra - 1) * 255 / 100 : 256;
            memset(mask, 0, sizeof(mask));
            for (int j = 1; overlays && j < h - 1; j++) {
                const uint8_t *r = y + j * stride;
                for (int i = 1; i < w - 1; i++) {
                    uint8_t v = 0;
                    if (settings.false_colour)
                        v = fc[r[i]];
                    if (r[i] >= zthr && (i + j) % 8 < 4)            /* 45 degree stripes, ~13 px on screen */
                        v = 2;
                    if (settings.peaking &&
                        abs(r[i + 1] - r[i - 1]) + abs(r[i + stride] - r[i - stride]) > thr)
                        v = 1;
                    mask[j * w + i] = v;
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &b);
            osd_cells(mask, w, h, pal, 9);
            clock_gettime(CLOCK_MONOTONIC, &c);
            t_calc += (b.tv_sec - a.tv_sec) * 1000000 + (b.tv_nsec - a.tv_nsec) / 1000;
            t_draw += (c.tv_sec - b.tv_sec) * 1000000 + (c.tv_nsec - b.tv_nsec) / 1000;
            n++;
        }
        MI_SYS_ChnOutputPortPutBuf(last);
        if (c.tv_sec - t0.tv_sec >= 5 && n) {         /* timing report every 5 s */
            printf("peaking: %.1f updates/s, edge calc %ld us, draw %ld us\n", n / 5.0, t_calc / n, t_draw / n);
            n = t_calc = t_draw = 0;
            t0 = c;
        }
    }
    osd_cells(NULL, PEAK_W, PEAK_H, NULL, 0);
    return NULL;
}

void peak_stop(void)
{
    if (!peak_run)
        return;
    peak_run = 0;
    pthread_join(peak_tid, NULL);
    printf("peaking: off, disable %d\n", MI_SCL_DisableOutputPort(0, 0, 3));
}

int apply_peaking(void)
{
    int want = settings.peaking || settings.zebra || settings.false_colour || settings.histogram || settings.waveform ||
               view_request;
    osd_scopes_layout(settings.histogram, settings.waveform != 0);
    if (want && !peak_run) {
        peak_run = 1;                                  /* before peak_set_crop, which writes the port */
        if (!loupe_on)                                 /* whole SCL input (the sensor crop, if any) */
            peak_crop[0] = peak_crop[1] = 0, peak_crop[2] = SRC_W, peak_crop[3] = SRC_H;
        peak_set_crop(peak_crop[0], peak_crop[1], peak_crop[2], peak_crop[3]);
        int r = MI_SCL_EnableOutputPort(0, 0, 3);
        r |= MI_SYS_SetChnOutputPortDepth(0, &scl_out3, 2, 4);
        pthread_create(&peak_tid, NULL, peak_thread, NULL);
        printf("analysis: on (peaking %u, zebra %u%%, false colour %u), rc %d\n", settings.peaking, settings.zebra,
               settings.false_colour, r);
        return r;
    }
    if (!want && peak_run)
        peak_stop();
    return 0;
}
