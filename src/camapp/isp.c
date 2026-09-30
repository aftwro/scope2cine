/* ISP image processing (see camapp.h): temporal NR, WDR, the Expert > Image controls (brightness,
 * contrast, lightness, saturation, black level, B&W, gamma, sharpening, colour matrix, HSV, noise
 * page), white balance and the look slots. Every setting is written from a vendor snapshot taken
 * once, so applying the same value twice gives the same result. */
#include "camapp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Temporal noise reduction (3DNR). MI_ISP_IQ_NR3D is 0x844 bytes: u32 enable, u32 op type, then
 * 16 auto entries (by ISO) and 1 manual entry of 124 bytes each. Measured 2026-09-24 on a static
 * scene at 24 dB with JPEG size as the noise proxy: the byte at +4 and the u16[16] curve at +54 are
 * tuning curves (changing either way adds noise, halving them doubles it), while the u16 motion
 * gain at +2 (vendor 1000) orders the noise monotonically: /2 more noise, x2 less. Off 99k,
 * gain/2 95k, vendor 90k, gain x2 88.5k bytes/frame. So the setting is that motion gain, written
 * to every entry (vendor: 1000 in all 16 auto entries, op type auto); 0 disables temporal NR. */
uint8_t nr3d_vendor[NR3D_SIZE];
int nr3d_have_vendor;

int apply_nr3d(void)
{
    static uint8_t nr[NR3D_SIZE];
    if (!nr3d_have_vendor) {
        if (MI_ISP_IQ_GetNR3D(0, 0, nr3d_vendor) != 0)
            return -1;
        nr3d_have_vendor = 1;
    }
    memcpy(nr, nr3d_vendor, sizeof(nr));
    unsigned gain = settings.nr3d_gain > 0xffff ? 0xffff : settings.nr3d_gain;
    if (gain == 0)
        *(uint32_t *)nr = 0;                          /* disable temporal NR */
    else
        for (int e = 0; e < NR3D_ENTRIES; e++)
            *(uint16_t *)(nr + 8 + e * NR3D_ENTRY + 2) = gain;
    int r = MI_ISP_IQ_SetNR3D(0, 0, nr);
    printf("nr3d: motion gain %u, SetNR3D = %d\n", gain, r);
    return r;
}

/* WDR (digital local tone mapping, stock cardv's "hdr"). MI_ISP_IQ_WDR is 0x668 bytes: u32 enable,
 * u32 op type (0 auto, 1 manual), then 16 auto entries (by ISO) and 1 manual entry of 96 bytes each;
 * strength is the byte at +0x2B. After isp_api.bin load the vendor state is manual with strength 0
 * (no effect). Vendor auto table: 64 at low gain fading to 0 from ISO entry 6. Measured 2026-09-24
 * (JPEG bytes/frame, static scene, 1.8x gain): 0 258k, 64 352k, 128 390k, 255 483k; AE is unaffected
 * (its statistics are taken before WDR). Setting: -1 vendor auto table, 0..255 manual strength. */
#define WDR_SIZE 0x668
#define WDR_MANUAL 0x608
int apply_wdr(void)
{
    static uint8_t vendor[WDR_SIZE], w[WDR_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetWDR(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(w, vendor, sizeof(w));
    *(uint32_t *)w = 1;                               /* enable */
    if (settings.wdr < 0) {
        *(uint32_t *)(w + 4) = 0;                     /* auto: vendor strengths by ISO */
    } else {
        *(uint32_t *)(w + 4) = 1;                     /* manual */
        w[WDR_MANUAL + 0x2b] = settings.wdr > 255 ? 255 : settings.wdr;
    }
    int r = MI_ISP_IQ_SetWDR(0, 0, w);
    printf("wdr: %ld, SetWDR = %d\n", (long)settings.wdr, r);
    return r;
}

/* ISP image controls (Expert > Image). Every setting is either the vendor's own tuning (Auto,
 * snapshot taken after isp_api.bin load) or a manual value in the ISP's own manual slot; nothing
 * is emulated in software. Brightness/Contrast/Lightness structs are 0x4c bytes: u32 enable,
 * u32 op type (0 auto, 1 manual), 16 auto u32 (by ISO), 1 manual u32 (0..100). Saturation is
 * 0x1a0: 17 entries of 24 bytes, byte 0 = strength. OBC (black level) is 0x90: 17 entries of
 * 4 u16 (one per Bayer channel, 16-bit scale). Dump: FIFO iq (logs/iq_dump.txt). */
#define IQ_SAT_SIZE 0x1a0
#define IQ_SAT_MANUAL (8 + 16 * 24)
#define IQ_OBC_SIZE 0x90

const struct iq_simple iq_simple[] = {
    { "brightness", MI_ISP_IQ_GetBrightness, MI_ISP_IQ_SetBrightness, &settings.isp_brightness },
    { "contrast", MI_ISP_IQ_GetContrast, MI_ISP_IQ_SetContrast, &settings.isp_contrast },
    { "lightness", MI_ISP_IQ_GetLightness, MI_ISP_IQ_SetLightness, &settings.isp_lightness },
};
uint8_t iq_vendor[3][IQ_SIMPLE_SIZE];
static uint8_t sat_vendor[IQ_SAT_SIZE], obc_vendor[IQ_OBC_SIZE];
static int iq_have_vendor;

static int iq_snapshot(void)
{
    if (iq_have_vendor)
        return 0;
    for (int k = 0; k < 3; k++)
        if (iq_simple[k].get(0, 0, iq_vendor[k]) != 0)
            return -1;
    if (MI_ISP_IQ_GetSaturation(0, 0, sat_vendor) != 0 || MI_ISP_IQ_GetOBC(0, 0, obc_vendor) != 0)
        return -1;
    iq_have_vendor = 1;
    return 0;
}

int apply_iq_simple(int k)
{
    uint8_t b[IQ_SIMPLE_SIZE];
    if (iq_snapshot() != 0)
        return -1;
    memcpy(b, iq_vendor[k], sizeof(b));
    int32_t v = *iq_simple[k].value;
    if (v >= 0) {
        ((uint32_t *)b)[0] = 1;                       /* enable */
        ((uint32_t *)b)[1] = 1;                       /* manual */
        ((uint32_t *)b)[18] = v > 100 ? 100 : v;
    }
    int r = iq_simple[k].set(0, 0, b);
    printf("isp %s: %ld, set = %d\n", iq_simple[k].name, (long)v, r);
    return r;
}

int apply_saturation(void)
{
    uint8_t b[IQ_SAT_SIZE];
    if (iq_snapshot() != 0)
        return -1;
    memcpy(b, sat_vendor, sizeof(b));
    if (settings.isp_saturation >= 0) {
        ((uint32_t *)b)[0] = 1;
        ((uint32_t *)b)[1] = 1;
        b[IQ_SAT_MANUAL] = settings.isp_saturation > 100 ? 100 : settings.isp_saturation;
    }
    int r = MI_ISP_IQ_SetSaturation(0, 0, b);
    printf("isp saturation: %ld, set = %d\n", (long)settings.isp_saturation, r);
    return r;
}

int apply_black(void)                        /* offset added to every vendor entry */
{
    uint8_t b[IQ_OBC_SIZE];
    if (iq_snapshot() != 0)
        return -1;
    memcpy(b, obc_vendor, sizeof(b));
    uint16_t *ch = (uint16_t *)(b + 8);
    for (int i = 0; i < 17 * 4; i++) {
        long v = ch[i] + (long)settings.isp_black;
        ch[i] = v < 0 ? 0 : v > 0xffff ? 0xffff : v;
    }
    int r = MI_ISP_IQ_SetOBC(0, 0, b);
    printf("isp black: offset %ld (manual %u), set = %d\n", (long)settings.isp_black, ch[16 * 4], r);
    return r;
}

int apply_bw(void)
{
    uint32_t en = settings.isp_bw ? 1 : 0;
    int r = MI_ISP_IQ_SetColorToGray(0, 0, &en);
    printf("isp bw: %u, set = %d\n", en, r);
    return r;
}

/* Leaving Auto starts from the vendor's own manual value, not from 0. */
int32_t iq_start_value(int sat)
{
    if (iq_snapshot() != 0)
        return 50;
    return sat ? sat_vendor[IQ_SAT_MANUAL] : 50;
}

/* RGB gamma LUT (MI_ISP_IQ_RGBGamma, 0x6608 bytes): u32 enable, u32 op type, 17 entries (16 auto
 * by ISO + manual) of u16 R[256], G[256], B[256], output 10-bit 0..1023 over the input range.
 * Vendor: manual curve ~ gamma 2.4 with a soft shoulder (logs/gamma_dump.txt). Our curve is a
 * pure power 1/g with a Rec.709-style linear toe (slope capped at 4.5) so shadow noise is not
 * multiplied without limit. Written to every entry, so auto/manual mode does not matter. */
#define GAMMA_SIZE 0x6608
#define GAMMA_ENTRY 1536

int apply_gamma(void)
{
    static uint8_t vendor[GAMMA_SIZE], g[GAMMA_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetRGBGamma(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(g, vendor, sizeof(g));
    if (settings.isp_gamma > 0) {
        double p = 10.0 / (settings.isp_gamma < 10 ? 10 : settings.isp_gamma > 30 ? 30 : settings.isp_gamma);
        uint16_t lut[256];
        for (int i = 0; i < 256; i++) {
            double x = i / 255.0, y = pow(x, p);
            if (settings.isp_gamma == GAMMA_LOG)    /* log: 18 % grey -> ~41 %, soft highlight roll-off */
                y = log(1 + LOG_A * x) / log(1 + LOG_A);
            else if (4.5 * x < y)
                y = 4.5 * x;
            lut[i] = (uint16_t)(y * 1023 + 0.5);
        }
        ((uint32_t *)g)[0] = 1;
        for (int e = 0; e < 17; e++)
            for (int c = 0; c < 3; c++)
                memcpy(g + 8 + e * GAMMA_ENTRY + c * 512, lut, sizeof(lut));
    }
    int r = MI_ISP_IQ_SetRGBGamma(0, 0, g);
    printf("isp gamma: %ld, set = %d\n", (long)settings.isp_gamma, r);
    apply_ev_comp();                                /* log adds its exposure offset */
    return r;
}

/* Sharpness (MI_ISP_IQ_Sharpness, 0x1328 bytes: u32 enable, u32 op type, 17 entries of 288 bytes,
 * layout undocumented; logs/shp_dump.txt). Measured 2026-09-24 with JPEG size, one field at a
 * time (static scene, vendor ~270k): disabled 204k; byte 1 x2 302k; bytes 1+2 = 0 235k; byte 0
 * x2 or 0 both softer (tuning, not strength); byte 2 x2 and the 16-byte table at +0x98 ~ no
 * change. Bytes 1 and 2 act as the edge gains, so the setting scales both in every entry. */
#define SHP_SIZE 0x1328
#define SHP_ENTRY 288
int apply_sharpness(void)
{
    static uint8_t vendor[SHP_SIZE], s[SHP_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetSharpness(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(s, vendor, sizeof(s));
    int32_t pc = settings.isp_sharpness;
    if (pc == 0) {
        ((uint32_t *)s)[0] = 0;                       /* sharpening off */
    } else if (pc > 0) {
        for (int e = 0; e < 17; e++)
            for (int i = 1; i <= 2; i++) {
                uint8_t *b = s + 8 + e * SHP_ENTRY + i;
                long v = (long)*b * pc / 100;
                *b = v > 255 ? 255 : v;
            }
    }
    int r = MI_ISP_IQ_SetSharpness(0, 0, s);
    printf("isp sharpness: %ld, set = %d\n", (long)pc, r);
    return r;
}

/* Colour matrix (MI_ISP_IQ_RGBMatrix, 0x1e8 bytes; logs/ccm_dump.txt): u32 enable, u32 op type,
 * u32 (0), 16 auto entries of 28 bytes {u16 CCT, u16 m[3][4], u16 saturation %}, then the manual
 * entry {u16 m[3][4], u16 saturation, pad} at 0x1cc. Coefficients: 1024 = 1.0, 13-bit two's
 * complement; column 3 of each row holds the row sum. Vendor: auto, calibrated at 3000/4000/6500 K
 * (other slots CCT 0), blended by the AWB colour temperature (QueryCCMInfo: current matrix + CCT).
 * The setting scales every calibrated matrix toward identity: m' = I + k (m - I). */
#define CCM_SIZE 0x1e8
#define CCM_AUTO 12
#define CCM_ENTRY 28
#define CCM_MANUAL 0x1cc
static int ccm_get(uint16_t v) { return v & 0x1000 ? (int)v - 0x2000 : v; }
static uint16_t ccm_put(int v) { v = v < -4096 ? -4096 : v > 4095 ? 4095 : v; return v & 0x1fff; }

int apply_ccm(void)
{
    static uint8_t vendor[CCM_SIZE], m[CCM_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetRGBMatrix(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(m, vendor, sizeof(m));
    if (settings.ccm_custom) {                        /* manual CCM: our 9 coefficients */
        uint16_t *q = (uint16_t *)(m + CCM_MANUAL);
        for (int row = 0; row < 3; row++) {
            int sum = 0;
            for (int col = 0; col < 3; col++) {
                q[row * 4 + col] = ccm_put(settings.ccm_m[row * 3 + col]);
                sum += settings.ccm_m[row * 3 + col];
            }
            q[row * 4 + 3] = ccm_put(sum);
        }
        ((uint32_t *)m)[0] = 1;
        ((uint32_t *)m)[1] = 1;
    } else if (settings.isp_ccm >= 0) {
        for (int e = 0; e < 16; e++) {
            uint16_t *q = (uint16_t *)(m + CCM_AUTO + e * CCM_ENTRY);
            if (q[0] == 0)                            /* unused slot */
                continue;
            for (int row = 0; row < 3; row++) {
                int sum = 0;
                for (int col = 0; col < 3; col++) {
                    int id = row == col ? 1024 : 0;
                    int v = id + (ccm_get(q[1 + row * 4 + col]) - id) * settings.isp_ccm / 100;
                    q[1 + row * 4 + col] = ccm_put(v);
                    sum += ccm_get(q[1 + row * 4 + col]);
                }
                q[1 + row * 4 + 3] = ccm_put(sum);
            }
        }
    }
    int r = MI_ISP_IQ_SetRGBMatrix(0, 0, m);
    printf("isp ccm: %s %ld, set = %d\n", settings.ccm_custom ? "custom" : "strength", (long)settings.isp_ccm, r);
    return r;
}

/* Per-hue adjustment (MI_ISP_IQ_HSV, 0x134c bytes; logs/hsv_dump.txt): u32 enable, u32 op type,
 * 17 entries (16 auto by ISO + manual) of 290 bytes = s16[145]: three pairs of 24-point tables
 * {s16 hue shift[24] (0 = none), u16 saturation[24] (64 = 1.0x)} at [0], [48], [96], plus one u16.
 * The 24 points are 15 degrees apart around the hue circle (point 0 = red). The pairs are probably
 * variants for different pipeline modes, so all three are written the same. Each of our 6 bands
 * (R, Y, G, C, B, M at points 0, 4, 8, 12, 16, 20) spreads with a triangle of half-width 4 points,
 * so neighbouring bands blend and their weights sum to 1. */
#define HSV_SIZE 0x134c
#define HSV_ENTRY 290
static const char *const hsv_band_names[6] = { "Red", "Yellow", "Green", "Cyan", "Blue", "Magenta" };

int apply_hsv(void)
{
    static uint8_t vendor[HSV_SIZE], h[HSV_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetHSV(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(h, vendor, sizeof(h));
    for (int e = 0; e < 17; e++) {
        int16_t *q = (int16_t *)(h + 8 + e * HSV_ENTRY);
        for (int pair = 0; pair < 3; pair++)
            for (int i = 0; i < 24; i++) {
                double hue = 0, sat = 0;
                for (int b = 0; b < 6; b++) {
                    int d = abs(i - b * 4);
                    d = d > 12 ? 24 - d : d;                  /* around the circle */
                    double w = d >= 4 ? 0 : 1 - d / 4.0;
                    hue += w * settings.hsv_hue[b];
                    sat += w * (settings.hsv_sat[b] - 100);
                }
                int hv = q[pair * 48 + i] + (int)lround(hue);
                int sv = (int)lround(((uint16_t *)q)[pair * 48 + 24 + i] * (100 + sat) / 100.0);
                q[pair * 48 + i] = hv < -512 ? -512 : hv > 511 ? 511 : hv;
                ((uint16_t *)q)[pair * 48 + 24 + i] = sv < 0 ? 0 : sv > 255 ? 255 : sv;
            }
    }
    int r = MI_ISP_IQ_SetHSV(0, 0, h);
    printf("isp hsv: hue %ld %ld %ld %ld %ld %ld sat %ld %ld %ld %ld %ld %ld, set = %d\n",
           (long)settings.hsv_hue[0], (long)settings.hsv_hue[1], (long)settings.hsv_hue[2], (long)settings.hsv_hue[3],
           (long)settings.hsv_hue[4], (long)settings.hsv_hue[5], (long)settings.hsv_sat[0], (long)settings.hsv_sat[1],
           (long)settings.hsv_sat[2], (long)settings.hsv_sat[3], (long)settings.hsv_sat[4], (long)settings.hsv_sat[5], r);
    return r;
}

/* Noise and clean-up (Expert > Image > Noise). Measured 2026-09-24 at 16x gain with JPEG size
 * as the noise proxy (logs/nr_dump.txt, logs/nrluma_adv_dump.txt): the basic NRLuma/NRChroma
 * structs have no effect in this pipeline; NRLuma_Adv is the active spatial luma NR (off: +5 %
 * bytes). Its strength is the two u16[8] threshold tables at entry +2 (entry 146 bytes, 17
 * entries): x2 -> -5 %, /2 -> +12 %. NRChroma_Adv, false colour: no measurable size change;
 * de-spike off: -13 %. Defog (0x1c: u32 enable, u32 op, u8 strength[17]) is off by vendor.
 * Blocks without a known strength field are vendor / off / on. */
#define NRL_SIZE 0x9bc
#define NRL_ENTRY 146
static int apply_nr_luma(void)
{
    static uint8_t vendor[NRL_SIZE], b[NRL_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_IQ_GetNRLuma_Adv(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(b, vendor, sizeof(b));
    if (settings.nr_luma == 0) {
        ((uint32_t *)b)[0] = 0;
    } else if (settings.nr_luma > 0) {
        ((uint32_t *)b)[0] = 1;
        for (int e = 0; e < 17; e++) {
            uint16_t *t = (uint16_t *)(b + 8 + e * NRL_ENTRY + 2);
            for (int i = 0; i < 16; i++) {
                long v = (long)t[i] * settings.nr_luma / 100;
                t[i] = v > 1023 ? 1023 : v;
            }
        }
    }
    int r = MI_ISP_IQ_SetNRLuma_Adv(0, 0, b);
    printf("isp luma nr: %ld, set = %d\n", (long)settings.nr_luma, r);
    return r;
}

/* Vendor / off / on for a block whose first u32 is its enable flag. */
static int apply_enable(const char *name, int (*get)(uint32_t, uint32_t, void *),
                        int (*set)(uint32_t, uint32_t, void *), uint8_t *vendor, int *have, int size, int32_t value)
{
    uint8_t b[0x308];
    if (!*have) {
        if (get(0, 0, vendor) != 0)
            return -1;
        *have = 1;
    }
    memcpy(b, vendor, size);
    if (value >= 0)
        ((uint32_t *)b)[0] = value ? 1 : 0;
    int r = set(0, 0, b);
    printf("isp %s: %ld, set = %d\n", name, (long)value, r);
    return r;
}

int apply_noise(void)
{
    static uint8_t chroma[0x308], despike[0x24c], falsecolor[0x90];
    static int have_c, have_d, have_f;
    int r = apply_nr_luma();
    r |= apply_enable("chroma nr", MI_ISP_IQ_GetNRChroma_Adv, MI_ISP_IQ_SetNRChroma_Adv, chroma, &have_c, 0x308,
                      settings.nr_chroma);
    r |= apply_enable("despike", MI_ISP_IQ_GetNRDeSpike, MI_ISP_IQ_SetNRDeSpike, despike, &have_d, 0x24c,
                      settings.nr_despike);
    r |= apply_enable("false colour", MI_ISP_IQ_GetFalseColor, MI_ISP_IQ_SetFalseColor, falsecolor, &have_f, 0x90,
                      settings.nr_falsecolor);
    uint8_t d[0x1c];
    if (MI_ISP_IQ_GetDefog(0, 0, d) == 0) {
        ((uint32_t *)d)[0] = settings.defog > 0;
        if (settings.defog > 0)
            memset(d + 8, settings.defog > 100 ? 100 : settings.defog, 17);
        int rd = MI_ISP_IQ_SetDefog(0, 0, d);
        printf("isp defog: %ld, set = %d\n", (long)settings.defog, rd);
        r |= rd;
    }
    return r;
}

/* White balance (MI_ISP_AWB_Attr, 0x5b8 bytes; logs/awb_dump.txt). Verified 2026-09-24:
 * +0 u32 state (0 run, 1 pause = hold current gains), +4 u32 op type (0 auto, 1 manual gains,
 * 2 colour temperature via MI_ISP_AWB_SetCTMwbAttr(u32 K)), +8 u16 manual gains R, Gr, Gb, B
 * (1024 = 1.0x); the rest is auto-algorithm tuning. QueryInfo (0x18): u16 stable, ?, R, Gr, Gb, B
 * gains in use, CCT. E.g. tungsten lamp: R 1221 B 3615 at 2381 K; CT 3000 K -> R 1356 B 3303. */
#define AWB_ATTR_SIZE 0x5b8
const char *const wb_names[] = { "Auto", "Kelvin", "Manual" };

int apply_wb(void)
{
    static uint8_t vendor[AWB_ATTR_SIZE], a[AWB_ATTR_SIZE];
    static int have_vendor;
    if (!have_vendor) {
        if (MI_ISP_AWB_GetAttr(0, 0, vendor) != 0)
            return -1;
        have_vendor = 1;
    }
    memcpy(a, vendor, sizeof(a));
    uint16_t *g = (uint16_t *)(a + 8);
    int r = 0;
    ((uint32_t *)a)[0] = 0;
    if (settings.wb_mode == 1) {
        uint32_t k = settings.wb_kelvin < 2000 ? 2000 : settings.wb_kelvin > 10000 ? 10000 : settings.wb_kelvin;
        r = MI_ISP_AWB_SetCTMwbAttr(0, 0, &k);
        ((uint32_t *)a)[1] = 2;
    } else if (settings.wb_mode == 2) {
        ((uint32_t *)a)[1] = 1;
        g[0] = settings.wb_r;
        g[1] = g[2] = settings.wb_g;
        g[3] = settings.wb_b;
    }
    r |= MI_ISP_AWB_SetAttr(0, 0, a);
    printf("wb: %s K %u gains %u/%u/%u, set = %d\n", wb_names[settings.wb_mode % 3], settings.wb_kelvin,
           settings.wb_r, settings.wb_g, settings.wb_b, r);
    return r;
}

/* Take the gains the ISP uses right now (auto or Kelvin) into manual mode: a white balance lock
 * that survives restarts. */
int wb_grab(void)
{
    uint16_t info[12];
    if (MI_ISP_AWB_QueryInfo(0, 0, info) != 0)
        return -1;
    settings.wb_r = info[2];
    settings.wb_g = (info[3] + info[4]) / 2;
    settings.wb_b = info[5];
    settings.wb_mode = 2;
    printf("wb: grabbed %u/%u/%u at %u K\n", info[2], settings.wb_g, info[5], info[6]);
    return apply_wb();
}

/* Copy the matrix the ISP is using right now (vendor auto, blended for the current light) into
 * the custom matrix, as a starting point for editing. */
/* The 9 coefficients the ISP uses right now (vendor blend for this light, or the custom matrix). */
int ccm_current(int32_t out[9])
{
    uint8_t q[0x1a];
    if (MI_ISP_IQ_QueryCCMInfo(0, 0, q) != 0)
        return -1;
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 3; col++)
            out[row * 3 + col] = ccm_get(((uint16_t *)q)[row * 4 + col]);
    return ((uint16_t *)q)[12];                       /* colour temperature, K */
}

int ccm_copy_current(void)
{
    int cct = ccm_current(settings.ccm_m);
    if (cct < 0)
        return -1;
    printf("ccm: copied current matrix (CCT %d K)\n", cct);
    return 0;
}

/* Step one custom coefficient. The ISP forces every row to sum to 1.0 (measured 2026-09-24: an
 * unbalanced row came back with its last coefficient changed), so an off-diagonal change is
 * balanced by the row's diagonal, and the diagonal itself follows from the other two. */
void ccm_step(int i, int delta)
{
    int row = i / 3, diag = row * 3 + row;
    if (i == diag)
        return;
    settings.ccm_m[i] += delta;
    settings.ccm_m[diag] -= delta;
    for (int k = row * 3; k < row * 3 + 3; k++)
        settings.ccm_m[k] = settings.ccm_m[k] < -4096 ? -4096 : settings.ccm_m[k] > 4095 ? 4095 : settings.ccm_m[k];
}

int apply_isp_image(void)
{
    int r = 0;
    r |= apply_ccm();
    r |= apply_hsv();
    r |= apply_noise();
    r |= apply_sharpness();
    r |= apply_gamma();
    for (int k = 0; k < 3; k++)
        r |= apply_iq_simple(k);
    r |= apply_saturation();
    r |= apply_black();
    r |= apply_bw();
    return r;
}


/* Looks: image settings snapshots in /customer/camapp/looks/lookN (see settings_save_look). */
#define LOOK_DIR "/customer/camapp/looks"
static void look_path(char *buf, int size, unsigned slot) { snprintf(buf, size, LOOK_DIR "/look%u", slot); }
int look_exists(unsigned slot)
{
    char p[64];
    look_path(p, sizeof(p), slot);
    return access(p, F_OK) == 0;
}
int look_save(unsigned slot)
{
    char p[64];
    mkdir(LOOK_DIR, 0755);
    look_path(p, sizeof(p), slot);
    int r = settings_save_look(p);
    sync();
    printf("look: saved slot %u = %d\n", slot, r);
    return r;
}
int look_load(unsigned slot)
{
    char p[64];
    look_path(p, sizeof(p), slot);
    if (access(p, F_OK) != 0)
        return -1;
    settings_load(p);                               /* only the image keys are in the file */
    apply_nr3d();
    apply_wdr();
    apply_isp_image();
    settings_changed();
    printf("look: loaded slot %u\n", slot);
    return 0;
}

/* Export / import all slots as LOOKS/LOOKn.TXT on the SD card (plain key=value, PC-editable,
 * see docs/looks.md). Import keeps only image keys. Returns the number of slots copied, -1 = no card. */
#define LOOK_SD_DIR REC_DIR "/LOOKS"
int looks_export(void)
{
    if (!sd_mounted())
        return -1;
    mkdir(LOOK_SD_DIR, 0755);
    int n = 0;
    for (unsigned k = 1; k <= LOOK_SLOTS; k++) {
        char src[64], dst[96];
        look_path(src, sizeof(src), k);
        snprintf(dst, sizeof(dst), LOOK_SD_DIR "/LOOK%u.TXT", k);
        if (access(src, F_OK) == 0 && settings_filter_look(src, dst) == 0)
            n++;
    }
    sync();
    printf("look: exported %d\n", n);
    return n;
}
int looks_import(void)
{
    if (!sd_mounted())
        return -1;
    mkdir(LOOK_DIR, 0755);
    int n = 0;
    for (unsigned k = 1; k <= LOOK_SLOTS; k++) {
        char src[96], dst[64];
        snprintf(src, sizeof(src), LOOK_SD_DIR "/LOOK%u.TXT", k);
        look_path(dst, sizeof(dst), k);
        if (access(src, F_OK) == 0 && settings_filter_look(src, dst) == 0)
            n++;
    }
    sync();
    printf("look: imported %d\n", n);
    return n;
}
