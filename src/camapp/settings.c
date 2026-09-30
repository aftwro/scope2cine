/* Settings file (see settings.h). */
#include "settings.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const struct settings defaults = { .shutter_us = 10000, .sensor_gain = 1024, .rec_fps = 30, .play_fps = 25, .tl_interval_ms = 1000, .jpeg_quality = 70, .h265_qp_max = 48, .h265_qp_min = 12,
                                          .sensor_fps = 60, .spot_x = 7, .spot_y = 6, .look_slot = 1, .nr3d_gain = 1000, .rec_codec = 1, .rec_mbps = 40,
                                          .isp_brightness = -1, .isp_contrast = -1, .isp_lightness = -1,
                                          .isp_saturation = -1, .isp_gamma = -1, .isp_sharpness = -1, .isp_ccm = -1,
                                          .hsv_sat = { 100, 100, 100, 100, 100, 100 },
                                          .nr_luma = -1, .nr_chroma = -1, .nr_despike = -1, .nr_falsecolor = -1,
                                          .wb_kelvin = 5600, .wb_r = 1024, .wb_g = 1024, .wb_b = 1024,
                                          .ccm_m = { 1024, 0, 0, 0, 1024, 0, 0, 0, 1024 } };
struct settings settings = defaults;
const char *const mode_names[N_MODES] = { "Normal", "HDR", "Slow-mo", "Timelapse" };

void settings_reset(void) { settings = defaults; }

static const struct { const char *key; uint32_t *val; } fields[] = {
    { "mirror", &settings.mirror }, { "flip", &settings.flip }, { "manual", &settings.manual }, { "ae_priority", &settings.ae_priority }, { "photo_raw", &settings.photo_raw },
    { "shutter_us", &settings.shutter_us }, { "sensor_gain", &settings.sensor_gain },
    { "shutter_angle_x10", &settings.shutter_angle_x10 },
    { "peaking", &settings.peaking }, { "peak_colour", &settings.peak_colour },
    { "zebra", &settings.zebra }, { "false_colour", &settings.false_colour }, { "waveform", &settings.waveform },
    { "rec_fps", &settings.rec_fps }, { "play_fps", &settings.play_fps }, { "tl_interval_ms", &settings.tl_interval_ms }, { "tl_limit_frames", &settings.tl_limit_frames }, { "crop_w", &settings.crop_w }, { "crop_h", &settings.crop_h }, { "rec_codec", &settings.rec_codec }, { "rec_mbps", &settings.rec_mbps }, { "sensor_fps", &settings.sensor_fps },
    { "metering", &settings.metering },
    { "spot_x", &settings.spot_x }, { "spot_size", &settings.spot_size }, { "spot_y", &settings.spot_y },
    { "ev_thirds", (uint32_t *)&settings.ev_thirds }, { "overlay", &settings.overlay },
    { "histogram", &settings.histogram }, { "nr3d_gain", &settings.nr3d_gain },
    { "wdr", (uint32_t *)&settings.wdr }, { "mode", &settings.mode },
    { "isp_brightness", (uint32_t *)&settings.isp_brightness }, { "isp_contrast", (uint32_t *)&settings.isp_contrast },
    { "isp_lightness", (uint32_t *)&settings.isp_lightness }, { "isp_saturation", (uint32_t *)&settings.isp_saturation },
    { "isp_black", (uint32_t *)&settings.isp_black }, { "isp_bw", &settings.isp_bw },
    { "isp_gamma", (uint32_t *)&settings.isp_gamma }, { "isp_sharpness", (uint32_t *)&settings.isp_sharpness },
    { "isp_ccm", (uint32_t *)&settings.isp_ccm },
    { "ccm_custom", &settings.ccm_custom },
    { "hsv_r_hue", (uint32_t *)&settings.hsv_hue[0] }, { "hsv_r_sat", (uint32_t *)&settings.hsv_sat[0] },
    { "hsv_y_hue", (uint32_t *)&settings.hsv_hue[1] }, { "hsv_y_sat", (uint32_t *)&settings.hsv_sat[1] },
    { "hsv_g_hue", (uint32_t *)&settings.hsv_hue[2] }, { "hsv_g_sat", (uint32_t *)&settings.hsv_sat[2] },
    { "hsv_c_hue", (uint32_t *)&settings.hsv_hue[3] }, { "hsv_c_sat", (uint32_t *)&settings.hsv_sat[3] },
    { "hsv_b_hue", (uint32_t *)&settings.hsv_hue[4] }, { "hsv_b_sat", (uint32_t *)&settings.hsv_sat[4] },
    { "hsv_m_hue", (uint32_t *)&settings.hsv_hue[5] }, { "hsv_m_sat", (uint32_t *)&settings.hsv_sat[5] },
    { "nr_luma", (uint32_t *)&settings.nr_luma }, { "nr_chroma", (uint32_t *)&settings.nr_chroma },
    { "nr_despike", (uint32_t *)&settings.nr_despike }, { "nr_falsecolor", (uint32_t *)&settings.nr_falsecolor },
    { "defog", (uint32_t *)&settings.defog }, { "look_slot", &settings.look_slot }, { "tc_run_ms", &settings.tc_run_ms }, { "jpeg_quality", &settings.jpeg_quality }, { "h265_gop", &settings.h265_gop },
    { "h265_rc", &settings.h265_rc }, { "h265_qp_max", &settings.h265_qp_max }, { "h265_qp_min", &settings.h265_qp_min },
    { "ae_max_shutter_us", &settings.ae_max_shutter_us }, { "ae_max_gain", &settings.ae_max_gain },
    { "ae_speed", &settings.ae_speed }, { "ae_flicker", &settings.ae_flicker },
    { "wb_mode", &settings.wb_mode }, { "wb_kelvin", &settings.wb_kelvin },
    { "wb_r", &settings.wb_r }, { "wb_g", &settings.wb_g }, { "wb_b", &settings.wb_b },
    { "ccm_rr", (uint32_t *)&settings.ccm_m[0] }, { "ccm_rg", (uint32_t *)&settings.ccm_m[1] },
    { "ccm_rb", (uint32_t *)&settings.ccm_m[2] }, { "ccm_gr", (uint32_t *)&settings.ccm_m[3] },
    { "ccm_gg", (uint32_t *)&settings.ccm_m[4] }, { "ccm_gb", (uint32_t *)&settings.ccm_m[5] },
    { "ccm_br", (uint32_t *)&settings.ccm_m[6] }, { "ccm_bg", (uint32_t *)&settings.ccm_m[7] },
    { "ccm_bb", (uint32_t *)&settings.ccm_m[8] },
};
#define N_FIELDS (sizeof(fields) / sizeof(fields[0]))

void settings_load(const char *path)
{
    FILE *f = fopen(path, "r");
    char key[32];
    long v;
    if (!f)
        return;
    while (fscanf(f, " %31[^=]=%ld", key, &v) == 2) {
        if (!strcmp(key, "hdr"))                    /* before camera modes: hdr=1 is the HDR mode */
            settings.mode = v ? MODE_HDR : MODE_NORMAL;
        for (unsigned i = 0; i < N_FIELDS; i++)
            if (!strcmp(key, fields[i].key))
                *fields[i].val = v;
    }
    fclose(f);
    if (settings.mode >= N_MODES)
        settings.mode = MODE_NORMAL;
}

static int is_look_key(const char *k)
{
    return !strncmp(k, "isp_", 4) || !strncmp(k, "ccm_", 4) || !strncmp(k, "hsv_", 4) || !strncmp(k, "nr_", 3) ||
           !strcmp(k, "defog") || !strcmp(k, "wdr") || !strcmp(k, "nr3d_gain");
}

static int is_mode_key(const char *k)
{
    static const char *const keys[] = { "sensor_fps", "rec_fps", "play_fps", "tl_interval_ms", "tl_limit_frames", "crop_w", "crop_h", "manual", "ae_priority", "shutter_us", "shutter_angle_x10",
                                        "sensor_gain", "ev_thirds", "rec_codec", "rec_mbps" };
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (!strcmp(k, keys[i]))
            return 1;
    return 0;
}

enum { SAVE_ALL, SAVE_LOOK, SAVE_MODE };
static int save_filtered(const char *path, int which)
{
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f)
        return -1;
    for (unsigned i = 0; i < N_FIELDS; i++)
        if (which == SAVE_ALL || (which == SAVE_LOOK ? is_look_key(fields[i].key) : is_mode_key(fields[i].key)))
            fprintf(f, "%s=%d\n", fields[i].key, (int32_t)*fields[i].val);   /* signed: ev_thirds < 0 */
    if (fflush(f) != 0 || fsync(fileno(f)) != 0) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return rename(tmp, path);
}

int settings_save(const char *path) { return save_filtered(path, SAVE_ALL); }
int settings_save_look(const char *path) { return save_filtered(path, SAVE_LOOK); }
int settings_save_mode(const char *path) { return save_filtered(path, SAVE_MODE); }

/* Valid range of each look key (docs/looks.md); imported values are clamped into it. Keys not
 * listed here are accepted as they are. */
static const struct { const char *prefix; long min, max; } look_ranges[] = {
    { "isp_brightness", -1, 100 }, { "isp_contrast", -1, 100 }, { "isp_lightness", -1, 100 },
    { "isp_saturation", -1, 100 }, { "isp_gamma", -1, 30 }, { "isp_sharpness", -1, 1000 },
    { "isp_ccm", -1, 200 }, { "isp_black", -1024, 1024 }, { "isp_bw", 0, 1 }, { "ccm_custom", 0, 1 },
    { "ccm_", -4096, 4096 }, { "hsv_", -128, 300 }, { "nr_luma", -1, 400 }, { "nr_", -1, 1 },
    { "defog", 0, 100 }, { "wdr", -1, 255 }, { "nr3d_gain", 0, 16000 },
};

static long look_clamp(const char *key, long v)
{
    for (unsigned i = 0; i < sizeof(look_ranges) / sizeof(look_ranges[0]); i++)
        if (!strncmp(key, look_ranges[i].prefix, strlen(look_ranges[i].prefix))) {
            long lo = look_ranges[i].min, hi = look_ranges[i].max;
            if (!strncmp(key, "hsv_", 4) && strstr(key, "_hue"))
                hi = 128;                                   /* hue shift +-128, saturation 0..300 */
            else if (!strncmp(key, "hsv_", 4))
                lo = 0;
            return v < lo ? lo : v > hi ? hi : v;
        }
    return v;
}

int settings_filter_look(const char *src, const char *dst)
{
    FILE *in = fopen(src, "r");
    if (!in)
        return -1;
    char tmp[128], key[32];
    long v;
    snprintf(tmp, sizeof(tmp), "%s.tmp", dst);
    FILE *out = fopen(tmp, "w");
    if (!out) {
        fclose(in);
        return -1;
    }
    int n = 0;
    while (fscanf(in, " %31[^=]=%ld", key, &v) == 2)
        for (unsigned i = 0; i < N_FIELDS; i++)
            if (!strcmp(key, fields[i].key) && is_look_key(key)) {
                long c = look_clamp(key, v);
                if (c != v)
                    printf("look: %s=%ld clamped to %ld\n", key, v, c);
                fprintf(out, "%s=%ld\n", key, c);
                n++;
            }
    fclose(in);
    if (fflush(out) != 0 || fsync(fileno(out)) != 0 || n == 0) {
        fclose(out);
        unlink(tmp);
        return -1;
    }
    fclose(out);
    return rename(tmp, dst);
}

