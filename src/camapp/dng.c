/* RAW photos as DNG: one frame straight from the VIF output (the sensor's 10-bit Bayer data before
 * any ISP processing), unpacked to 16 bits and wrapped in a minimal DNG 1.4 file that raw
 * converters (Lightroom, Resolve, darktable, RawTherapee) open.
 *
 * The VIF output is only real raw with frame buffer compression off (mi_vif.ko parameter
 * g_bFBCMode = N, set by camapp before the pipeline starts): with FBC on the buffer is packed to
 * 8 bits per pixel. Uncompressed format RG_10BPP: a little-endian bit stream, 4 pixels in 5 bytes,
 * stride = width x 10 / 8 (measured 2026-09-29: 1920 wide -> 2400, values 55..123 in a dark scene
 * around the sensor black level 60).
 *
 * Colour: the ISP colour matrix (white-balanced camera RGB -> linear sRGB) and the AWB gains at the
 * moment of capture give ColorMatrix1 = inverse(sRGB->XYZ x CCM x WB), D65, and AsShotNeutral.
 *
 * The VIF buffer is the sensor readout rotated 180 degrees against the displayed picture (measured
 * 2026-09-29 with mirror/flip off: raw strips matched the USB picture only rotated), so the DNG
 * carries Orientation 3 and the pixels stay in readout order (RGGB at 0,0). */
#include "camapp.h"
#include "mi.h"
#include "sensordbg.h"
#include "osd.h"

#include <math.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FBC_PARAM "/sys/module/mi_vif/parameters/g_bFBCMode"

/* Frame buffer compression off, so the VIF output port carries real 10-bit raw. Before VIF setup. */
void dng_fbc_off(void)
{
    FILE *f = fopen(FBC_PARAM, "w");
    if (f) {
        fputs("N", f);
        fclose(f);
    }
    printf("dng: VIF frame buffer compression %s\n", f ? "off" : "unchanged (no parameter)");
}

/* Little-endian TIFF writer helpers. Tags must be written in ascending order. */
struct tag { uint16_t id, type; uint32_t count, value; };
enum { T_BYTE = 1, T_ASCII = 2, T_SHORT = 3, T_LONG = 4, T_RATIONAL = 5, T_SRATIONAL = 10 };

static void matmul(const double a[9], const double b[9], double out[9])
{
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            out[r * 3 + c] = a[r * 3] * b[c] + a[r * 3 + 1] * b[3 + c] + a[r * 3 + 2] * b[6 + c];
}

static int matinv(const double m[9], double out[9])
{
    double d = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) + m[2] * (m[3] * m[7] - m[4] * m[6]);
    if (fabs(d) < 1e-9)
        return -1;
    out[0] = (m[4] * m[8] - m[5] * m[7]) / d, out[1] = (m[2] * m[7] - m[1] * m[8]) / d, out[2] = (m[1] * m[5] - m[2] * m[4]) / d;
    out[3] = (m[5] * m[6] - m[3] * m[8]) / d, out[4] = (m[0] * m[8] - m[2] * m[6]) / d, out[5] = (m[2] * m[3] - m[0] * m[5]) / d;
    out[6] = (m[3] * m[7] - m[4] * m[6]) / d, out[7] = (m[1] * m[6] - m[0] * m[7]) / d, out[8] = (m[0] * m[4] - m[1] * m[3]) / d;
    return 0;
}

struct job {
    char path[96];
    uint8_t *packed;
    uint32_t stride, w, h;            /* full buffer */
    uint32_t x0, y0, cw, ch;          /* region to write (even offsets) */
    int cfa_phase;                    /* 0 RGGB, 1 GRBG, 2 GBRG, 3 BGGR at (0,0) of the buffer */
    double color[9], neutral[3];
    double ccm[9];                    /* ISP matrix (white-balanced camera -> sRGB), for the preview */
    int preview;                      /* write a 160x90 sRGB PPM of the raw instead of the DNG */
    uint32_t black;
};

static int write_dng(struct job *j)
{
    FILE *f = fopen(j->path, "wb");
    if (!f)
        return -1;
    setvbuf(f, NULL, _IOFBF, 256 * 1024);
    /* CFA phase at the written region's origin: odd offsets swap columns / rows of the pattern. */
    int ph = j->cfa_phase ^ (j->x0 & 1) ^ ((j->y0 & 1) << 1);
    static const uint8_t pats[4][4] = { { 0, 1, 1, 2 }, { 1, 0, 2, 1 }, { 1, 2, 0, 1 }, { 2, 1, 1, 0 } };
    const char make[] = "SigmaStar camapp", model[] = "IMX307 camapp", unique[] = "camapp IMX307";
    enum { N_TAGS = 26 };
    uint32_t ifd = 8, extra = ifd + 2 + N_TAGS * 12 + 4, p = extra;
    uint32_t o_make = p;   p += sizeof(make) + (sizeof(make) & 1);
    uint32_t o_model = p;  p += sizeof(model) + (sizeof(model) & 1);
    uint32_t o_unique = p; p += sizeof(unique) + (sizeof(unique) & 1);
    uint32_t o_bits = p;   p += 0;                 /* single value inline */
    uint32_t o_color = p;  p += 9 * 8;
    uint32_t o_neutral = p; p += 3 * 8;
    uint32_t o_data = (p + 15) & ~15u;
    uint32_t bytes = j->cw * j->ch * 2;
    (void)o_bits;
    struct tag t[N_TAGS] = {
        { 254, T_LONG, 1, 0 },                         /* NewSubfileType: main image */
        { 256, T_LONG, 1, j->cw }, { 257, T_LONG, 1, j->ch },
        { 258, T_SHORT, 1, 16 }, { 259, T_SHORT, 1, 1 }, /* 16 bits, uncompressed */
        { 262, T_SHORT, 1, 32803 },                    /* CFA */
        { 271, T_ASCII, sizeof(make), o_make }, { 272, T_ASCII, sizeof(model), o_model },
        { 273, T_LONG, 1, o_data }, { 274, T_SHORT, 1, 3 }, { 277, T_SHORT, 1, 1 },
        { 278, T_LONG, 1, j->ch }, { 279, T_LONG, 1, bytes }, { 284, T_SHORT, 1, 1 },
        { 33421, T_SHORT, 2, 2 | 2 << 16 },            /* CFARepeatPatternDim 2x2 */
        { 33422, T_BYTE, 4, pats[ph][0] | pats[ph][1] << 8 | pats[ph][2] << 16 | (uint32_t)pats[ph][3] << 24 },
        { 50706, T_BYTE, 4, 1 | 4 << 8 },              /* DNGVersion 1.4 */
        { 50707, T_BYTE, 4, 1 | 1 << 8 },              /* DNGBackwardVersion 1.1 */
        { 50708, T_ASCII, sizeof(unique), o_unique },
        { 50710, T_BYTE, 3, 0 | 1 << 8 | 2 << 16 },    /* CFAPlaneColor R G B */
        { 50711, T_SHORT, 1, 1 },                      /* CFALayout rectangular */
        { 50714, T_LONG, 1, j->black },                /* BlackLevel */
        { 50717, T_LONG, 1, 1023 },                    /* WhiteLevel (10 bit) */
        { 50721, T_SRATIONAL, 9, o_color },            /* ColorMatrix1 */
        { 50728, T_RATIONAL, 3, o_neutral },           /* AsShotNeutral */
        { 50778, T_SHORT, 1, 21 },                     /* CalibrationIlluminant1 D65 */
    };
    uint8_t hdr[8] = { 'I', 'I', 42, 0, 8, 0, 0, 0 };
    fwrite(hdr, 1, 8, f);
    uint16_t n = N_TAGS;
    fwrite(&n, 2, 1, f);
    for (int i = 0; i < N_TAGS; i++)
        fwrite(&t[i], 12, 1, f);
    uint32_t next = 0;
    fwrite(&next, 4, 1, f);
    fwrite(make, 1, sizeof(make) + (sizeof(make) & 1), f);
    fwrite(model, 1, sizeof(model) + (sizeof(model) & 1), f);
    fwrite(unique, 1, sizeof(unique) + (sizeof(unique) & 1), f);
    for (int i = 0; i < 9; i++) {
        int32_t r[2] = { (int32_t)lround(j->color[i] * 10000), 10000 };
        fwrite(r, 4, 2, f);
    }
    for (int i = 0; i < 3; i++) {
        uint32_t r[2] = { (uint32_t)lround(j->neutral[i] * 10000), 10000 };
        fwrite(r, 4, 2, f);
    }
    while (ftell(f) < (long)o_data)
        fputc(0, f);
    uint16_t *row = malloc(j->cw * 2);
    for (uint32_t y = 0; row && y < j->ch; y++) {
        const uint8_t *src = j->packed + (j->y0 + y) * j->stride;
        for (uint32_t x = 0; x < j->cw; x++) {
            uint32_t px = j->x0 + x, bit = px * 10, byte = bit >> 3;
            row[x] = ((src[byte] | src[byte + 1] << 8) >> (bit & 7)) & 1023;
        }
        fwrite(row, 2, j->cw, f);
    }
    free(row);
    int ok = row && fflush(f) == 0 && fsync(fileno(f)) == 0;
    return fclose(f) == 0 && ok ? 0 : -1;
}

/* Check image: the raw frame developed the way a raw converter would read the DNG (black, as-shot
 * WB, the matrix, sRGB gamma, Orientation 3), one 2x2 Bayer block per output pixel. */
static int write_preview(struct job *j)
{
    enum { PW = 160, PH = 90 };
    FILE *f = fopen(j->path, "wb");
    if (!f)
        return -1;
    fprintf(f, "P6\n%d %d\n255\n", PW, PH);
    for (int oy = 0; oy < PH; oy++)
        for (int ox = 0; ox < PW; ox++) {
            uint32_t bx = ((PW - 1 - ox) * j->w / PW) & ~1u, by = ((PH - 1 - oy) * j->h / PH) & ~1u;   /* 180 deg */
            double c[4];
            for (int k = 0; k < 4; k++) {
                uint32_t px = bx + (k & 1), py = by + (k >> 1), bit = px * 10, byte = bit >> 3;
                const uint8_t *src = j->packed + py * j->stride;
                int v = (((src[byte] | src[byte + 1] << 8) >> (bit & 7)) & 1023) - (int)j->black;
                c[k] = v < 0 ? 0 : v / (1023.0 - j->black);
            }
            double cam[3] = { c[0] / j->neutral[0], (c[1] + c[2]) / 2, c[3] / j->neutral[2] }, rgb[3];
            for (int r2 = 0; r2 < 3; r2++) {
                double v = (j->ccm[r2 * 3] * cam[0] + j->ccm[r2 * 3 + 1] * cam[1] + j->ccm[r2 * 3 + 2] * cam[2]) * 4;  /* +2 EV */
                v = v <= 0 ? 0 : v >= 1 ? 1 : v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1 / 2.4) - 0.055;
                rgb[r2] = v;
            }
            for (int k = 0; k < 3; k++)
                fputc((int)(rgb[k] * 255 + 0.5), f);
        }
    return fclose(f);
}

static void *dng_thread(void *arg)
{
    prctl(PR_SET_NAME, "dng");                 /* thread name for top / cpustat */
    struct job *j = arg;
    int r = j->preview ? write_preview(j) : write_dng(j);
    printf("dng: %s %ux%u at %u,%u, CFA phase %d, black %u: %s\n", j->path, j->cw, j->ch, j->x0, j->y0, j->cfa_phase,
           j->black, r == 0 ? "ok" : "write error");
    if (r != 0)
        notice_for("RAW (DNG) save failed", OSD_RED, 3);
    free(j->packed);
    free(j);
    return NULL;
}

/* Grab the current VIF frame and write it (or a region of it, w/h 0 = whole frame) as a DNG in a
 * background thread. Returns 0 when the frame was captured. */
int dng_capture(const char *path, uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
    int preview = !strcmp(strrchr(path, '.') ? strrchr(path, '.') : "", ".ppm");
    if (hdr_on)
        return -1;                                      /* DOL planes are 12-bit pairs: not handled */
    static ChnPort vo = { MOD_VIF, 0, 0, 0 };
    uint8_t info[0x200];
    uint32_t handle = 0;
    int r = MI_SYS_SetChnOutputPortDepth(0, &vo, 1, 4);
    for (int i = 0; i < 25 && (r = MI_SYS_ChnOutputPortGetBuf(&vo, info, &handle)) != 0; i++)
        usleep(20000);
    struct job *j = r == 0 ? calloc(1, sizeof(*j)) : NULL;
    if (j) {
        j->w = *(uint16_t *)(info + 0x38);
        j->h = *(uint16_t *)(info + 0x3a);
        j->stride = *(uint32_t *)(info + 0x60);
        const uint8_t *p = *(uint8_t **)(info + 0x3c);
        int packed10 = *(uint32_t *)(info + 0x24) == 0x20 && j->stride >= j->w * 10 / 8;
        j->packed = p && packed10 ? malloc(j->stride * j->h) : NULL;
        if (j->packed)
            memcpy(j->packed, p, j->stride * j->h);
        else
            printf("dng: VIF buffer is not uncompressed RG_10BPP (fmt %u, stride %u for %u px)\n",
                   *(uint32_t *)(info + 0x24), j->stride, j->w);
    }
    if (r == 0)
        MI_SYS_ChnOutputPortPutBuf(handle);
    MI_SYS_SetChnOutputPortDepth(0, &vo, 0, 4);
    if (!j || !j->packed) {
        free(j);
        printf("dng: capture failed (getbuf %d)\n", r);
        return -1;
    }
    j->cw = w && x0 + w <= j->w ? w & ~1u : j->w;
    j->ch = h && y0 + h <= j->h ? h & ~1u : j->h;
    j->x0 = w ? x0 : 0;
    j->y0 = h ? y0 : 0;
    snprintf(j->path, sizeof(j->path), "%s", path);
    j->preview = preview;
    /* RGGB at the sensor's (0,0); mirror / flip / an odd VIF crop line shift the phase. */
    j->cfa_phase = (settings.mirror ? 1 : 0) ^ (settings.flip ? 2 : 0) ^ (crop_w && !OPEN_GATE && ((1080 - crop_h) / 2) & 1 ? 2 : 0);
    uint8_t bl[2] = { 60, 0 };
    sensor_read_block(0x300a, bl, 2);
    j->black = (bl[0] | (bl[1] & 1) << 8);
    /* Colour: XYZ -> camera = inverse(sRGB(D65)->XYZ x CCM x WB gains), scaled so D65 white peaks at 1. */
    int32_t ccm[9];
    uint16_t awb[12] = { 0 };
    double m[9], wbm[9] = { 0 }, s2x[9] = { 0.4124, 0.3576, 0.1805, 0.2126, 0.7152, 0.0722, 0.0193, 0.1192, 0.9505 }, a[9], b[9];
    if (ccm_current(ccm) < 0)
        for (int i = 0; i < 9; i++)
            ccm[i] = i % 4 == 0 ? 1024 : 0;
    for (int i = 0; i < 9; i++)
        j->ccm[i] = m[i] = ccm[i] / 1024.0;
    MI_ISP_AWB_QueryInfo(0, 0, awb);
    double gr = awb[2] ? awb[2] : 1024, gg = awb[3] && awb[4] ? (awb[3] + awb[4]) / 2.0 : 1024, gb = awb[5] ? awb[5] : 1024;
    wbm[0] = gr / gg, wbm[4] = 1, wbm[8] = gb / gg;
    matmul(m, wbm, a);
    matmul(s2x, a, b);
    if (matinv(b, j->color) != 0)
        memcpy(j->color, (double[9]){ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, sizeof(j->color));
    double d65[3] = { 0.9505, 1.0, 1.089 }, peak = 0;
    for (int r2 = 0; r2 < 3; r2++) {
        double v = j->color[r2 * 3] * d65[0] + j->color[r2 * 3 + 1] * d65[1] + j->color[r2 * 3 + 2] * d65[2];
        peak = v > peak ? v : peak;
    }
    for (int i = 0; i < 9 && peak > 0; i++)
        j->color[i] /= peak;
    j->neutral[0] = gg / gr, j->neutral[1] = 1, j->neutral[2] = gg / gb;
    pthread_t th;
    pthread_attr_t at;
    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &at, dng_thread, j) != 0) {
        free(j->packed);
        free(j);
        return -1;
    }
    return 0;
}
