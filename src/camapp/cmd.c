/* Command dispatcher (see camapp.h): every button action and every line written to the command
 * pipe (/tmp/camapp_fifo) ends up in handle_cmd. The menu and playback get the first look; the
 * rest are camera functions and debug/measurement commands. */
#include "camapp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include "osd.h"
#include "player.h"
#include "uvc.h"
#include "sensordbg.h"

/* UVC camera controls (uvc.c): AE mode 1 manual / 2 auto / 4 shutter priority, brightness =
 * EV compensation in 1/3 stops. Run in the UVC thread, like FIFO commands run in theirs. */
int uvc_control_get(int ctrl)
{
    if (ctrl == UVC_CTRL_AE_MODE)
        return settings.manual ? 1 : settings.ae_priority == 1 ? 4 : 2;
    return settings.ev_thirds;
}

void uvc_control_set(int ctrl, int value)
{
    char line[32];
    if (ctrl == UVC_CTRL_AE_MODE)
        snprintf(line, sizeof(line), "prio %d\n", value & 1 ? 3 : value & 4 ? 1 : 0);
    else {
        value = value < -EV_THIRDS_MAX ? -EV_THIRDS_MAX : value > EV_THIRDS_MAX ? EV_THIRDS_MAX : value;
        snprintf(line, sizeof(line), "evcomp %d\n", value);
    }
    handle_cmd(line);
}

void handle_cmd(char *line)
{
    char cmd[32];
    unsigned long val = 0, arg2 = 0;
    int n = sscanf(line, "%31s %lu %lu", cmd, &val, &arg2);
    if (n < 1)
        return;
    /* Body MENU gestures (input.c): hold = menu, 1 click = photo (or close the menu), 2 = OSD
     * hide/show, 3 = record. Translated to the plain commands before anything else sees them. */
    if (!strcmp(cmd, "menu_long"))
        strcpy(cmd, "menu");
    else if (!strcmp(cmd, "menu_short"))
        strcpy(cmd, menu_open ? "menu" : "snap");
    else if (!strcmp(cmd, "menu_double"))
        strcpy(cmd, menu_open ? "nop" : "osd_toggle");
    else if (!strcmp(cmd, "menu_triple"))
        strcpy(cmd, menu_open ? "nop" : "rec_toggle");
    if (!strcmp(cmd, "nop"))
        return;
    if (!strcmp(cmd, "osd_toggle")) {               /* hide / show the whole OSD layer (clean HDMI picture) */
        osd_hidden ^= 1;
        osd_show(!osd_hidden);
        printf("osd: %s\n", osd_hidden ? "hidden" : "shown");
        return;
    }
    if (playback_input(cmd))
        return;
    if (!strcmp(cmd, "play")) {                     /* play [PATH]: default the newest clip */
        char path[160];
        if (sscanf(line, "%*s %159s", path) != 1 && !newest_clip(path, sizeof(path))) {
            notice_for("No clips on the card", OSD_YELLOW, 2);
            return;
        }
        playback_start(path);
        return;
    }
    if (menu_input(cmd))
        return;
    if (!strcmp(cmd, "dp")) {         /* dp 0 vendor | 1 off | 2 SLOPE: manual, hot slope SLOPE (test) */
        static uint8_t vendor[0x2f4], d[0x2f4];
        static int have;
        if (!have && MI_ISP_IQ_GetDynamicDP(0, 0, vendor) == 0)
            have = 1;
        memcpy(d, vendor, sizeof(d));
        if (val == 1)
            *(uint32_t *)d = 0;
        if (val == 2) {
            memcpy(d + 8 + 16 * 44, d + 8 + 15 * 44, 44);      /* manual = the highest-gain entry */
            *(uint16_t *)(d + 8 + 16 * 44 + 4) = (uint16_t)arg2;
            *(uint32_t *)(d + 4) = 1;
        }
        printf("dp: mode %lu slope %lu: set %d\n", val, arg2, have ? MI_ISP_IQ_SetDynamicDP(0, 0, d) : -1);
        return;
    }
    if (!strcmp(cmd, "dpdump")) {     /* debug: MI_ISP_IQ_GetDynamicDP (0x2f4 = 8 + 17 x 44 bytes) */
        static uint8_t d[0x2f4];
        int r = MI_ISP_IQ_GetDynamicDP(0, 0, d);
        printf("dpdump: rc %d enable %u op %u\n", r, *(uint32_t *)d, *(uint32_t *)(d + 4));
        for (int e = 0; e < 17; e += (e == 0 || e == 7 || e == 15) ? (e == 15 ? 1 : e == 0 ? 7 : 8) : 1) {
            printf("dpdump %s%2d:", e == 16 ? "manual" : "auto", e);
            for (int i = 0; i < 44; i += 2) printf(" %u", *(uint16_t *)(d + 8 + e * 44 + i));
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "evcomp")) {     /* evcomp N: EV compensation in 1/3 stops (signed), auto exposure */
        int v = 0;
        sscanf(line, "%*s %d", &v);
        settings.ev_thirds = v < -EV_THIRDS_MAX ? -EV_THIRDS_MAX : v > EV_THIRDS_MAX ? EV_THIRDS_MAX : v;
        apply_ev_comp();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "spotsize") && n == 2) {
        spot_resize((int)val);
        settings_changed();
        printf("cmd spotsize: %d at block %u,%u\n", spot_w(), settings.spot_x, settings.spot_y);
        return;
    }
    if (!strcmp(cmd, "spot_centre")) {       /* spot back to the exact centre */
        settings.spot_x = SPOT_CX;
        settings.spot_y = SPOT_CY;
        install_weight_tables();
        show_spot_area();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "zebra")) {              /* zebra PCT (0..100) | zebra off */
        settings.zebra = n == 2 ? (val > 100 ? 100 : val) + 1 : 0;
        apply_peaking();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "waveform") && n == 2) {   /* waveform 0 off, 1 luma, 2 RGB, 3 parade */
        settings.waveform = val > 3 ? 1 : val;
        if (!settings.waveform)
            osd_waveform(NULL);
        apply_peaking();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "falsecolour") && n == 2) {
        settings.false_colour = val ? 1 : 0;
        apply_peaking();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "peaking")) {    /* remote flip button: peaking on (last level, default 5) / off */
        static uint32_t last_level = 5;
        if (settings.peaking) {
            last_level = settings.peaking;
            settings.peaking = 0;
        } else {
            settings.peaking = n == 2 && val <= 10 ? val : last_level;
        }
        apply_peaking();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "loupe")) {      /* remote mirror button: loupe on/off */
        loupe_on ^= 1;
        apply_loupe();
        return;
    }
    if (!strcmp(cmd, "magnify") && n == 2) {  /* magnify Z10: 10 = off, 12..80 = 1.25x .. 8x */
        loupe_on = val > 10;
        if (loupe_on)
            loupe_z10 = val > 80 ? 80 : val;
        apply_loupe();
        return;
    }
    if (loupe_on && !menu_open && (!strcmp(cmd, "ev_up") || !strcmp(cmd, "ev_down"))) {   /* zoom */
        loupe_z10 += !strcmp(cmd, "ev_up") ? 5 : -5;       /* 0.5x steps */
        loupe_z10 = loupe_z10 < 12 ? 12 : loupe_z10 > 80 ? 80 : loupe_z10;
        apply_loupe();
        return;
    }
    if (loupe_on && !menu_open &&
        (!strcmp(cmd, "left") || !strcmp(cmd, "right") || !strcmp(cmd, "up") || !strcmp(cmd, "down"))) {
        int step_x = SRC_W * 10 / loupe_z10 / 8, step_y = SRC_H * 10 / loupe_z10 / 8;   /* 1/8 of the view */
        /* user: the first mapping felt flipped on both axes, so the arrows move the picture */
        loupe_cx += (!strcmp(cmd, "left") - !strcmp(cmd, "right")) * step_x;
        loupe_cy += (!strcmp(cmd, "up") - !strcmp(cmd, "down")) * step_y;
        apply_loupe();
        return;
    }
    if (!strcmp(cmd, "fps") && n == 2) {
        sensor_debug("before");
        printf("cmd fps %lu: SetFps = %d\n", val, MI_SNR_SetFps(0, val));
        usleep(500000);
        sensor_debug("after");
        return;
    }
    if (!strcmp(cmd, "framerate") && n == 2) {
        if (rec_on)
            return;
        settings.sensor_fps = val;
        settings.sensor_fps = sensor_fps_valid();
        apply_exposure();                           /* clamps shutter to the new frame, then sets fps */
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "h265") && !rec_on) {        /* h265 GOP RC QPMAX QPMIN (next clip) */
        unsigned g, rc, qa, qi;
        if (sscanf(line, "%*s %u %u %u %u", &g, &rc, &qa, &qi) == 4) {
            settings.h265_gop = g > 600 ? 600 : g;
            settings.h265_rc = rc ? 1 : 0;
            settings.h265_qp_max = qa > 51 ? 51 : qa;
            settings.h265_qp_min = qi > settings.h265_qp_max ? settings.h265_qp_max : qi;
            settings_changed();
        }
        printf("cmd h265: gop %u rc %u qp %u-%u\n", settings.h265_gop, settings.h265_rc, settings.h265_qp_min, settings.h265_qp_max);
        return;
    }
    if (!strcmp(cmd, "jpegparam")) {  /* debug: jpegparam [Q]: dump MI_VENC_GetJpegParam, optionally set word 0 */
        static uint32_t p[64];
        memset(p, 0, sizeof(p));
        int r = MI_VENC_GetJpegParam(VENC_DEV_JPEG, VENC_CHN, p);
        printf("jpegparam: get rc %d: %u %u %u %u | %08x %08x ... %u %u %u\n", r, p[0], p[1], p[2], p[3], p[1], p[2],
               p[33], p[34], p[35]);
        if (n == 2) {
            p[0] = val;
            printf("jpegparam: set q %lu rc %d\n", val, MI_VENC_SetJpegParam(VENC_DEV_JPEG, VENC_CHN, p));
        }
        return;
    }
    if (!strcmp(cmd, "vui")) {       /* debug: raw MI_VENC_GetH265Vui / GetH264Vui of the recording encoder (dev 0) */
        static uint8_t v[256];
        memset(v, 0, sizeof(v));
        int r = MI_VENC_GetH265Vui(0, 0, v);
        printf("vui h265: rc %d:", r);
        for (int i = 0; i < 96; i++) printf(" %02x", v[i]);
        printf("\n");
        memset(v, 0, sizeof(v));
        r = MI_VENC_GetH264Vui(0, 0, v);
        printf("vui h264: rc %d:", r);
        for (int i = 0; i < 96; i++) printf(" %02x", v[i]);
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "padinfo")) {   /* debug: raw MI_SNR_GetPadInfo words (MIPI receiver settings) */
        static uint32_t w[64];
        memset(w, 0, sizeof(w));
        int r = MI_SNR_GetPadInfo(0, w);
        printf("padinfo: rc %d:", r);
        for (int i = 0; i < 64; i++) printf(" %x", w[i]);
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "sensor")) {
        sensor_debug("now");
        return;
    }
    if (!strcmp(cmd, "rec") && n >= 2) {
        rec_set(val != 0, n == 3 ? (int)arg2 : (int)settings.rec_fps);
        return;
    }
    if (!strcmp(cmd, "tllimit") && n == 2 && !rec_on) {   /* Timelapse: stop after N frames, 0 = off */
        settings.tl_limit_frames = val > 100000 ? 100000 : val;
        printf("cmd tllimit: %u\n", settings.tl_limit_frames);
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "tlint") && n == 2 && !rec_on) {     /* Timelapse interval in ms, 100 .. 3600000 */
        settings.tl_interval_ms = val < 100 ? 100 : val > 3600000 ? 3600000 : val;
        printf("cmd tlint: %u ms\n", settings.tl_interval_ms);
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "playfps") && n == 2 && !rec_on) {   /* Slow-mo playback rate (clamped by play_fps()) */
        settings.play_fps = val;
        settings.play_fps = play_fps();
        printf("cmd playfps: %u\n", settings.play_fps);
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "codec") && n == 2 && !rec_on) {   /* codec 0 MJPEG | 1 H.265 | 2 H.264 */
        settings.rec_codec = val > 2 ? 1 : val;    /* 0 MJPEG, 1 H.265, 2 H.264 */
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "mbps") && n == 2 && !rec_on) {
        settings.rec_mbps = val < 2 ? 2 : val > 100 ? 100 : val;
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "rec_toggle")) {
        rec_set(!rec_on, settings.rec_fps);
        return;
    }
    if (!strcmp(cmd, "recfps") && n == 2 && (val == 30 || val == 60)) {
        settings.rec_fps = val;
        settings_changed();
        printf("cmd recfps: %u\n", settings.rec_fps);
        return;
    }
    if (!strcmp(cmd, "mirror") || !strcmp(cmd, "flip")) {
        /* Sensor readout direction; MI_ISP_SetChnParam refuses changes while the channel runs. */
        if (cmd[0] == 'm')
            settings.mirror ^= 1;
        else
            settings.flip ^= 1;
        printf("cmd %s: mirror %u flip %u, SetOrien = %d\n", cmd, settings.mirror, settings.flip, apply_orientation());
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "meter")) {
        uint32_t info[0x23c / 4] = { 0 };
        MI_ISP_AE_QueryExposureInfo(0, 0, info);
        printf("meter: avgY %u lumY %u target %u LVx10 %u BV %d stable %u -> EV %+.2f (shutter %u us, gain %u)\n",
               info[AE_AVG_Y], info[AE_AVG_Y - 1], info[AE_TARGET], info[AE_LV_X10], (int32_t)info[AE_BV],
               info[0], ev_offset(info), info[5], info[3]);
        return;
    }
    if ((!strcmp(cmd, "metering") && n == 2 && val <= 2) || !strcmp(cmd, "metering_next")) {
        settings.metering = !strcmp(cmd, "metering_next") ? (settings.metering + 1) % 3 : val;
        printf("cmd metering: %s, SetWinWgtType = %d\n", metering_names[settings.metering], apply_metering());
        settings_changed();
        return;
    }
    int own_shutter = settings.manual || settings.ae_priority == 1, own_gain = settings.manual || settings.ae_priority == 2;
    if ((own_shutter || own_gain) && !playing) {    /* M / S / G: the user's values straight from the buttons */
        int sd = !own_shutter ? 0 : !strcmp(cmd, "right") || !strcmp(cmd, "ok") ? 1 : !strcmp(cmd, "left") || !strcmp(cmd, "ae") ? -1 : 0;
        int gd = !own_gain ? 0 : !strcmp(cmd, "up") ? 1 : !strcmp(cmd, "down") ? -1 : 0;
        int fd = !settings.manual ? 0 : !strcmp(cmd, "ev_up") ? 1 : !strcmp(cmd, "ev_down") ? -1 : 0;
        if (sd || fd) {
            shutter_step(sd ? sd : fd, !sd);
            settings_changed();
            return;
        }
        if (gd) {
            gain_step(gd, 0);
            settings_changed();
            return;
        }
    }
    if (!strcmp(cmd, "left") || !strcmp(cmd, "right") || !strcmp(cmd, "up") || !strcmp(cmd, "down")) {
        if (settings.metering != 2) {           /* arrows only move the spot for now; menus later */
            printf("cmd ignored: %s (not in spot metering)\n", cmd);
            return;
        }
        int x = settings.spot_x, y = settings.spot_y;
        x += !strcmp(cmd, "right") - !strcmp(cmd, "left");
        y += !strcmp(cmd, "down") - !strcmp(cmd, "up");
        settings.spot_x = x < 0 ? 0 : x > WGT_GRID - SPOT_W ? WGT_GRID - SPOT_W : x;
        settings.spot_y = y < 0 ? 0 : y > WGT_GRID - SPOT_H ? WGT_GRID - SPOT_H : y;
        printf("cmd %s: spot at block %u,%u, SetWinWgt = %d\n", cmd, settings.spot_x, settings.spot_y,
               install_weight_tables());
        show_spot_area();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "ev_up") || !strcmp(cmd, "ev_down") || (!strcmp(cmd, "ev0"))) {
        int v = !strcmp(cmd, "ev0") ? 0 : settings.ev_thirds + (cmd[3] == 'u' ? 1 : -1);
        settings.ev_thirds = v < -EV_THIRDS_MAX ? -EV_THIRDS_MAX : v > EV_THIRDS_MAX ? EV_THIRDS_MAX : v;
        apply_ev_comp();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "overlay_next") || (!strcmp(cmd, "overlay") && n == 2 && val <= 3)) {
        unsigned g = !strcmp(cmd, "overlay_next") ? (settings.overlay + 1) & 3 : val;   /* frame guide kept */
        settings.overlay = (settings.overlay & ~3u) | g;
        osd_overlay(settings.overlay);
        printf("cmd overlay: %s%s\n", settings.overlay & 1 ? "thirds " : "", settings.overlay & 2 ? "crosshair" : "");
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "histogram_toggle")) {
        settings.histogram ^= 1;
        if (!settings.histogram)
            osd_histogram(NULL);
        apply_peaking();
        printf("cmd histogram: %s\n", settings.histogram ? "on" : "off");
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "jpgavg")) {     /* debug: average JPEG frame size over 3 s (noise proxy) */
        unsigned long long b0 = venc_bytes;
        long f0 = venc_frames;
        sleep(3);
        long nf = venc_frames - f0;
        printf("jpgavg: %llu bytes/frame over %ld frames\n", nf ? (venc_bytes - b0) / nf : 0, nf);
        return;
    }
    if (!strcmp(cmd, "nr3d_exp") && n == 2) {   /* debug: isolated NR3D field experiments */
        static uint8_t nr[NR3D_SIZE];
        if (!nr3d_have_vendor && MI_ISP_IQ_GetNR3D(0, 0, nr3d_vendor) == 0)
            nr3d_have_vendor = 1;
        memcpy(nr, nr3d_vendor, sizeof(nr));
        for (int e = 0; e < NR3D_ENTRIES; e++) {
            uint8_t *q = nr + 8 + e * NR3D_ENTRY;
            uint16_t *tf = (uint16_t *)(q + 54);
            for (int i = 0; i < 16; i++) {
                if (val == 3) tf[i] /= 2;                           /* curve halved */
                if (val == 4) tf[i] = tf[i] * 3 / 2 > 0x0fff ? 0x0fff : tf[i] * 3 / 2;
                if (val == 6) tf[i] = 0x0fff;                       /* curve flat max */
                if (val == 7) tf[i] = 0x0100;                       /* curve flat low */
            }
            if (val == 2) q[4] /= 2;                                /* byte +4 halved */
            if (val == 8) q[0] = q[0] * 2 > 255 ? 255 : q[0] * 2;   /* motion threshold x2 */
            if (val == 9) q[0] /= 2;                                /* motion threshold /2 */
            if (val == 10) *(uint16_t *)(q + 2) = *(uint16_t *)(q + 2) * 2;   /* motion gain x2 */
            if (val == 11) *(uint16_t *)(q + 2) /= 2;               /* motion gain /2 */
            if (val == 12) q[0] = q[0] * 4 > 255 ? 255 : q[0] * 4;  /* motion threshold x4 */
            if (val == 5) q[4] = 0x7f;                              /* byte +4 raised */
        }
        if (val == 1)
            *(uint32_t *)nr = 0;
        printf("nr3d_exp %lu: SetNR3D = %d\n", val, MI_ISP_IQ_SetNR3D(0, 0, nr));
        return;
    }
    if (!strcmp(cmd, "reg")) {        /* reg ADDR [COUNT]: read IMX307 registers (hex ok), read-only */
        long addr = 0, count = 1;
        if (sscanf(line, "%*s %li %li", &addr, &count) < 1 || count < 1 || count > 64) {
            printf("reg: usage reg 0x3018 [count 1-64]\n");
            return;
        }
        uint8_t v[64];
        int r = sensor_read_block(addr, v, count);
        printf("reg 0x%04lX:", addr);
        for (int i = 0; i < count; i++) printf(" %02X", v[i]);
        printf("%s\n", r ? " (read error)" : "");
        return;
    }
    if (!strcmp(cmd, "regw")) {       /* regw ADDR VAL: write any IMX307 register (logged, undoable) */
        long addr = 0, v = 0;
        if (sscanf(line, "%*s %li %li", &addr, &v) != 2 || addr < 0x3000 || addr > 0x34ff || v < 0 || v > 255) {
            printf("regw: usage regw 0x308c 0x21\n");
            return;
        }
        reg_write(addr, v);
        return;
    }
    if (!strcmp(cmd, "isp")) {        /* isp brightness|contrast|lightness|saturation -1..100, black OFFSET, bw 0/1 */
        char what[16];
        long v;
        if (sscanf(line, "%*s %15s %ld", what, &v) != 2) {
            printf("isp: usage isp contrast 60 | isp black -32 | isp bw 1 (-1 = auto)\n");
            return;
        }
        long c = v < -1 ? -1 : v > 100 ? 100 : v;
        if (!strcmp(what, "saturation")) {
            settings.isp_saturation = c;
            apply_saturation();
        } else if (!strcmp(what, "black")) {
            settings.isp_black = v < -4096 ? -4096 : v > 4096 ? 4096 : v;
            apply_black();
        } else if (!strncmp(what, "hsv_", 4) && strlen(what) == 7 && strchr("rygcbm", what[4])) {
            int b = strchr("rygcbm", what[4]) - "rygcbm";   /* isp hsv_r_h -10 | isp hsv_g_s 150 */
            if (what[6] == 'h')
                settings.hsv_hue[b] = v < -128 ? -128 : v > 128 ? 128 : v;
            else
                settings.hsv_sat[b] = v < 0 ? 0 : v > 300 ? 300 : v;
            apply_hsv();
        } else if (!strcmp(what, "luma_nr")) {     /* -1 vendor, 0 off, % */
            settings.nr_luma = v < -1 ? -1 : v > 400 ? 400 : v;
            apply_noise();
        } else if (!strcmp(what, "chroma_nr") || !strcmp(what, "despike") || !strcmp(what, "falsecolor")) {
            int32_t *p = what[0] == 'c' ? &settings.nr_chroma : what[0] == 'd' ? &settings.nr_despike : &settings.nr_falsecolor;
            *p = v < 0 ? -1 : v ? 1 : 0;
            apply_noise();
        } else if (!strcmp(what, "defog")) {
            settings.defog = v < 0 ? 0 : v > 100 ? 100 : v;
            apply_noise();
        } else if (!strcmp(what, "ccm_custom")) {  /* 0/1 */
            settings.ccm_custom = v ? 1 : 0;
            apply_ccm();
        } else if (!strcmp(what, "ccm_copy")) {
            ccm_copy_current();
            apply_ccm();
        } else if (!strncmp(what, "ccm_m", 5) && what[5] >= '0' && what[5] <= '8' && !what[6]) {   /* x1024 */
            int i = what[5] - '0';                    /* set off-diagonal, diagonal balances the row */
            v = v < -4096 ? -4096 : v > 4095 ? 4095 : v;
            ccm_step(i, v - settings.ccm_m[i]);
            apply_ccm();
        } else if (!strcmp(what, "ccm")) {         /* -1 vendor, % */
            settings.isp_ccm = v < -1 ? -1 : v > 300 ? 300 : v;
            apply_ccm();
        } else if (!strcmp(what, "sharpness")) {   /* -1 vendor, 0 off, % */
            settings.isp_sharpness = v < -1 ? -1 : v > 1000 ? 1000 : v;
            apply_sharpness();
        } else if (!strcmp(what, "gamma")) {       /* x 10, -1 = vendor, 1 = log */
            settings.isp_gamma = v == GAMMA_LOG ? GAMMA_LOG : v < 10 ? -1 : v > 30 ? 30 : v;
            apply_gamma();
        } else if (!strcmp(what, "bw")) {
            settings.isp_bw = v ? 1 : 0;
            apply_bw();
        } else {
            int k = -1;
            for (int i = 0; i < 3; i++)
                if (!strcmp(what, iq_simple[i].name))
                    k = i;
            if (k < 0) {
                printf("isp: unknown control %s\n", what);
                return;
            }
            *iq_simple[k].value = c;
            apply_iq_simple(k);
        }
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "crop") && n == 3) {           /* crop W H: sensor window (0 0 = full), restarts */
        settings.crop_w = val, settings.crop_h = arg2;
        printf("crop: set %lux%lu, max %u fps, restarting\n", val, arg2, crop_max_fps(val));
        settings_changed();
        if (!rec_on)
            restart_requested = stop_requested = 1;
        return;
    }
    if (!strcmp(cmd, "restart")) {    /* debug: re-exec camapp (re-reads start-time files like /tmp/crop_probe) */
        if (!rec_on)
            restart_requested = stop_requested = 1;
        printf("restart: %s\n", rec_on ? "refused while recording" : "requested");
        return;
    }
    if (!strcmp(cmd, "regundo")) {
        reg_undo_all();
        return;
    }
    if (!strcmp(cmd, "mode") && n == 2) {           /* camera mode (MODE_*): restarts camapp */
        printf("mode %lu: rc %d\n", (unsigned long)val, mode_switch(val));
        return;
    }
    if (!strcmp(cmd, "wdr_level") && n == 2) {    /* -1 auto, 0..255 manual strength */
        settings.wdr = (long)val < -1 ? -1 : (long)val > 255 ? 255 : (long)val;
        apply_wdr();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "nr3d_gain") && n == 2) {    /* 0 off, 1..65535 motion gain (vendor 1000) */
        settings.nr3d_gain = val > 0xffff ? 0xffff : val;
        apply_nr3d();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "wdr_exp") && n == 2) {    /* debug: WDR strength experiments */
        static uint8_t wv[0x668], w[0x668];
        static int have;
        if (!have && MI_ISP_IQ_GetWDR(0, 0, wv) == 0)
            have = 1;
        memcpy(w, wv, sizeof(w));
        if (val <= 255) {                       /* manual strength = val */
            w[0x608 + 0x2b] = val;
            *(uint32_t *)w = 1;
            *(uint32_t *)(w + 4) = 1;
        } else if (val == 1000) {               /* auto table (vendor strengths by ISO) */
            *(uint32_t *)(w + 4) = 0;
        } else if (val == 1001) {               /* disabled */
            *(uint32_t *)w = 0;
        }                                       /* anything else: vendor boot state */
        printf("wdr_exp %lu: SetWDR = %d\n", val, MI_ISP_IQ_SetWDR(0, 0, w));
        return;
    }
    if (!strcmp(cmd, "awb_exp") && n >= 2) {   /* debug: awb_exp MODE [ARG] experiments */
        static uint8_t vendor[0x5b8], a[0x5b8];
        static int have;
        if (!have && MI_ISP_AWB_GetAttr(0, 0, vendor) == 0)
            have = 1;
        memcpy(a, vendor, sizeof(a));
        uint16_t *g = (uint16_t *)(a + 8);
        int r = 0;
        if (val == 1) ((uint32_t *)a)[0] = 1;                          /* state = 1 (pause?) */
        if (val >= 2 && val <= 5) {                                    /* manual, one gain word = 2048 */
            ((uint32_t *)a)[1] = 1;
            g[val - 2] = 2048;
        }
        if (val == 6) {                                                /* CT manual: op type ARG, CT 3000 */
            uint32_t ct = arg2 ? arg2 : 3000;
            r = MI_ISP_AWB_SetCTMwbAttr(0, 0, &ct);
            ((uint32_t *)a)[1] = 2;
        }
        if (val == 7) ((uint32_t *)a)[1] = 1;                          /* manual, unity gains */
        int r2 = MI_ISP_AWB_SetAttr(0, 0, a);
        usleep(300000);
        uint8_t info[0x18] = { 0 };
        MI_ISP_AWB_QueryInfo(0, 0, info);
        printf("awb_exp %lu: ct rc %d set %d | info", val, r, r2);
        for (int i = 0; i < 0x18; i += 2) printf(" %u", *(uint16_t *)(info + i));
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "awb")) {        /* remote AWB button: lock (grab current gains) / back to auto */
        if (settings.wb_mode == 0)
            wb_grab();
        else {
            settings.wb_mode = 0;
            apply_wb();
        }
        notice_for(settings.wb_mode ? "WB locked" : "WB auto", OSD_WHITE, 2);
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "wb")) {         /* wb auto | wb kelvin K | wb manual R G B (x1024) | wb grab */
        char what[16];
        long a1 = 0, a2 = 0, a3 = 0;
        int k = sscanf(line, "%*s %15s %ld %ld %ld", what, &a1, &a2, &a3);
        if (k >= 1 && !strcmp(what, "auto")) {
            settings.wb_mode = 0;
        } else if (k >= 2 && !strcmp(what, "kelvin")) {
            settings.wb_mode = 1;
            settings.wb_kelvin = a1 < 2000 ? 2000 : a1 > 10000 ? 10000 : a1;
        } else if (k >= 4 && !strcmp(what, "manual")) {
            settings.wb_mode = 2;
            settings.wb_r = a1 < 256 ? 256 : a1 > 8191 ? 8191 : a1;
            settings.wb_g = a2 < 256 ? 256 : a2 > 8191 ? 8191 : a2;
            settings.wb_b = a3 < 256 ? 256 : a3 > 8191 ? 8191 : a3;
        } else if (k >= 1 && !strcmp(what, "grab")) {
            wb_grab();
            settings_changed();
            return;
        } else {
            printf("wb: usage wb auto | wb kelvin 5600 | wb manual 1300 1024 2900 | wb grab\n");
            return;
        }
        apply_wb();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "nrl_exp") && n == 2) {   /* debug: NRLuma_Adv field experiments (entry 146 bytes) */
        static uint8_t v[0x9bc], la[0x9bc];
        static int have;
        if (!have && MI_ISP_IQ_GetNRLuma_Adv(0, 0, v) == 0)
            have = 1;
        memcpy(la, v, sizeof(la));
        for (int e = 0; e < 17; e++) {
            uint8_t *q = la + 8 + e * 146;
            uint16_t *t = (uint16_t *)(q + 2);                  /* two u16[8] tables */
            for (int i = 0; i < 16; i++) {
                if (val == 1) t[i] = t[i] * 2 > 1023 ? 1023 : t[i] * 2;
                if (val == 2) t[i] /= 2;
                if (val == 7) ((uint16_t *)(q + 36))[i] /= 2;   /* 1023 limit table */
                if (val == 5) q[68 + i] /= 2;                   /* falloff curve */
            }
            for (int i = 130; i <= 144; i++) {                  /* u8 per-level table */
                if (val == 3) q[i] = q[i] * 2 > 255 ? 255 : q[i] * 2;
                if (val == 4) q[i] = 0;
            }
            if (val == 6) *(uint16_t *)(q + 34) = 0;
        }
        printf("nrl_exp %lu: set %d\n", val, MI_ISP_IQ_SetNRLuma_Adv(0, 0, la));
        return;
    }
    if (!strcmp(cmd, "nradump")) {    /* debug: NRLuma_Adv raw dump (read-only), 32 bytes a line */
        static uint8_t la[0x9bc];
        printf("nradump: rc %d\n", MI_ISP_IQ_GetNRLuma_Adv(0, 0, la));
        for (int row = 0; row < 0x9bc; row += 32) {
            printf("nradump +%03x", row);
            for (int i = row; i < row + 32 && i < 0x9bc; i++) printf("%s%02x", i % 4 ? "" : " ", la[i]);
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "nra_exp") && n == 2) {   /* debug: advanced NR + clean-up on/off experiments */
        static uint8_t la[0x9bc], ca[0x308], d[0x1c], f[0x90], s[0x24c];
        int r1 = MI_ISP_IQ_GetNRLuma_Adv(0, 0, la), r2 = MI_ISP_IQ_GetNRChroma_Adv(0, 0, ca);
        int r3 = MI_ISP_IQ_GetDefog(0, 0, d), r4 = MI_ISP_IQ_GetFalseColor(0, 0, f), r5 = MI_ISP_IQ_GetNRDeSpike(0, 0, s);
        printf("nra: get %d/%d/%d/%d/%d | luma_adv en %u op %u | chroma_adv en %u op %u\n", r1, r2, r3, r4, r5,
               ((uint32_t *)la)[0], ((uint32_t *)la)[1], ((uint32_t *)ca)[0], ((uint32_t *)ca)[1]);
        int r = 0;
        if (val == 1 || val == 2) { ((uint32_t *)la)[0] = val == 2; r = MI_ISP_IQ_SetNRLuma_Adv(0, 0, la); }
        if (val == 3 || val == 4) { ((uint32_t *)ca)[0] = val == 4; r = MI_ISP_IQ_SetNRChroma_Adv(0, 0, ca); }
        if (val == 5 || val == 6) {                                         /* defog on (strength 50) / off */
            ((uint32_t *)d)[0] = val == 5;
            r = MI_ISP_IQ_SetDefog(0, 0, d);
        }
        if (val == 7 || val == 8) { ((uint32_t *)f)[0] = val == 8; r = MI_ISP_IQ_SetFalseColor(0, 0, f); }
        if (val == 9 || val == 10) { ((uint32_t *)s)[0] = val == 10; r = MI_ISP_IQ_SetNRDeSpike(0, 0, s); }
        printf("nra_exp %lu: set %d\n", val, r);
        return;
    }
    if (!strcmp(cmd, "nr_exp") && n == 2) {    /* debug: isolated luma/chroma NR field experiments */
        static uint8_t lv[0x24c], l[0x24c], cv[0x118], c[0x118];
        static int have;
        if (!have && MI_ISP_IQ_GetNRLuma(0, 0, lv) == 0 && MI_ISP_IQ_GetNRChroma(0, 0, cv) == 0)
            have = 1;
        memcpy(l, lv, sizeof(l));
        memcpy(c, cv, sizeof(c));
        for (int e = 0; e < 17; e++) {
            uint8_t *q = l + 8 + e * 34;
            uint16_t *w = (uint16_t *)(c + 8 + e * 16);
            if (val == 2) q[0] = q[0] * 2 > 255 ? 255 : q[0] * 2;
            if (val == 3) q[0] /= 2;
            if (val == 4) for (int i = 2; i < 34; i++) q[i] = q[i] * 2 > 255 ? 255 : q[i] * 2;
            if (val == 6) for (int i = 0; i < 2; i++) w[i] = w[i] * 2;
            if (val == 7) for (int i = 0; i < 2; i++) w[i] /= 2;
            if (val == 8) for (int i = 4; i < 6; i++) w[i] = w[i] * 2;
        }
        if (val == 1) ((uint32_t *)l)[0] = 0;
        if (val == 5) ((uint32_t *)c)[0] = 0;
        printf("nr_exp %lu: set %d/%d\n", val, MI_ISP_IQ_SetNRLuma(0, 0, l), MI_ISP_IQ_SetNRChroma(0, 0, c));
        return;
    }
    if (!strcmp(cmd, "remote_ae")) {  /* remote AE button: exposure mode P > S > G > M */
        expo_mode_step(1);
        settings_changed();
        static const char *const names[] = { "Exposure: P auto", "Exposure: S shutter priority", "Exposure: G gain priority" };
        notice_for(settings.manual ? "Exposure: M manual" : names[settings.ae_priority % 3], OSD_WHITE, 2);
        return;
    }
    if (!strcmp(cmd, "prio") && n == 2) {   /* prio 0-3: P S G M */
        settings.manual = val == 3;
        settings.ae_priority = val < 3 ? val : 0;
        apply_exposure();
        settings_changed();
        printf("cmd prio: %s\n", expo_mode_short());
        return;
    }
    if (!strcmp(cmd, "ae_lock")) {    /* remote contrast button: toggle AE lock (auto only) */
        if (settings.manual) {
            notice_for("AE lock: exposure is manual", OSD_YELLOW, 2);
            return;
        }
        ae_locked ^= 1;
        apply_exposure();
        notice_for(ae_locked ? "AE locked" : "AE unlocked", OSD_WHITE, 2);
        return;
    }
    if (!strcmp(cmd, "aeconv") && n >= 2) {    /* debug: aeconv IDX VAL (IDX 10 = print only) */
        uint32_t c[10];
        if (MI_ISP_AE_GetConverge(0, 0, c) != 0)
            return;
        if (val < 10)
            c[val] = arg2;
        int r = val < 10 ? MI_ISP_AE_SetConverge(0, 0, c) : 0;
        printf("aeconv: set %d |", r);
        for (int i = 0; i < 10; i++) printf(" %u", c[i]);
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "aestep")) {     /* debug: aestep [THIRDS]: EV step (default -3), exposure x gain every 50 ms */
        int d = -3;
        sscanf(line, "%*s %d", &d);
        int32_t ev[2] = { settings.ev_thirds + d, 3 };
        uint32_t info[0x23c / 4];
        double e[48];
        MI_ISP_AE_SetEVComp(0, 0, ev);
        for (int i = 0; i < 48; i++) {
            MI_ISP_AE_QueryExposureInfo(0, 0, info);
            e[i] = (double)info[5] * info[3] / 1024.0 * info[4] / 1024.0;
            usleep(50000);
        }
        apply_ev_comp();                               /* back to the user's EV comp */
        int t90 = -1;
        for (int i = 0; i < 48 && t90 < 0; i++)
            if (fabs(e[i] - e[0]) >= 0.9 * fabs(e[47] - e[0]))
                t90 = i * 50;
        printf("aestep: start %.0f end %.0f (x%.2f), 90%% after %d ms |", e[0], e[47], e[47] / (e[0] ? e[0] : 1), t90);
        for (int i = 0; i < 48; i += 3) printf(" %.2f", e[i] / (e[0] ? e[0] : 1));
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "dng")) {        /* dng [x y w h]: raw frame (or region) to /tmp/test.dng (FIFO test) */
        unsigned x = 0, y = 0, w = 0, h = 0;
        sscanf(line, "%*s %u %u %u %u", &x, &y, &w, &h);
        printf("dng: capture %d\n", dng_capture(sd_mounted() && !w ? REC_DIR "/TEST.DNG" : "/tmp/test.dng", x, y, w, h));
        return;
    }
    if (!strcmp(cmd, "rgn_test") && n == 2) {
        printf("rgn_test: %d\n", rgn_test((int)val));
        return;
    }
    if (!strcmp(cmd, "viewgrab")) {   /* processed picture as /tmp/view.ppm (160x90), see scopes.c */
        view_request = 1;
        apply_peaking();                            /* starts the analysis port if no scope is on */
        for (int i = 0; i < 40 && view_request; i++)
            usleep(50000);
        int ok = !view_request;
        view_request = 0;
        apply_peaking();                            /* stops it again if nothing else needs it */
        printf("viewgrab: %s\n", ok ? "/tmp/view.ppm" : "timeout");
        return;
    }
    if (!strcmp(cmd, "dngview")) {    /* raw frame developed to /tmp/dngview.ppm (160x90) to check DNG data */
        printf("dng: preview %d\n", dng_capture("/tmp/dngview.ppm", 0, 0, 0, 0));
        return;
    }
    if (!strcmp(cmd, "photoraw") && n == 2) {
        settings.photo_raw = val ? 1 : 0;
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "raw_probe")) {  /* debug: one buffer from the VIF output (Bayer), dump BufInfo + first bytes */
        static ChnPort vo = { MOD_VIF, 0, 0, 0 };
        int rd = MI_SYS_SetChnOutputPortDepth(0, &vo, 1, 4);
        uint8_t info[0x200];
        uint32_t handle = 0;
        int r = -1;
        for (int i = 0; i < 50 && r != 0; i++) {
            memset(info, 0, sizeof(info));
            r = MI_SYS_ChnOutputPortGetBuf(&vo, info, &handle);
            if (r != 0)
                usleep(20000);
        }
        printf("raw_probe: depth %d getbuf %d handle %u\n", rd, r, handle);
        for (int row = 0; r == 0 && row < 0xc0; row += 16) {
            printf("raw_probe +%03x", row);
            for (int i = row; i < row + 16; i += 4) printf(" %08x", *(uint32_t *)(info + i));
            printf("\n");
        }
        if (r == 0) {
            const uint8_t *p = *(uint8_t **)(info + 0x3c);
            uint32_t stride = *(uint32_t *)(info + 0x60);
            printf("raw_probe: w %u h %u fmt %u stride %u ptr %p\n", *(uint16_t *)(info + 0x38),
                   *(uint16_t *)(info + 0x3a), *(uint32_t *)(info + 0x24), stride, (void *)p);
            if (p) {
                printf("raw_probe: row 540 bytes:");
                for (int i = 0; i < 32; i++) printf(" %02x", p[540 * stride + 960 * 2 + i]);
                printf("\n");
                FILE *f = fopen("/tmp/raw.bin", "wb");
                if (f) {
                    fwrite(p, 1, stride * *(uint16_t *)(info + 0x3a), f);
                    fclose(f);
                }
            }
            printf("raw_probe: putbuf %d\n", MI_SYS_ChnOutputPortPutBuf(handle));
        }
        printf("raw_probe: depth off %d\n", MI_SYS_SetChnOutputPortDepth(0, &vo, 0, 4));
        return;
    }
    if (!strcmp(cmd, "pk_probe")) {   /* debug: SCL port 3 at 480x270 NV12, grab one frame, dump its BufInfo */
        static ChnPort p3 = { MOD_SCL, 0, 0, 3 };
        uint32_t param[6] = { 0, 0, 480 | 270 << 16, 0x101, 0xb, 0 };
        int r1 = MI_SCL_SetOutputPortParam(0, 0, 3, param), r2 = MI_SCL_EnableOutputPort(0, 0, 3);
        int r3 = MI_SYS_SetChnOutputPortDepth(0, &p3, 2, 4);
        printf("pk_probe: param %d enable %d depth %d\n", r1, r2, r3);
        uint8_t info[0x200];
        uint32_t handle = 0;
        int r = -1;
        for (int i = 0; i < 50 && r != 0; i++) {
            memset(info, 0, sizeof(info));
            r = MI_SYS_ChnOutputPortGetBuf(&p3, info, &handle);
            if (r != 0)
                usleep(20000);
        }
        printf("pk_probe: getbuf %d handle %u\n", r, handle);
        for (int row = 0; r == 0 && row < 0xc0; row += 16) {
            printf("pk_probe +%03x", row);
            for (int i = row; i < row + 16; i += 4) printf(" %08x", *(uint32_t *)(info + i));
            printf("\n");
        }
        if (r == 0)
            printf("pk_probe: putbuf %d\n", MI_SYS_ChnOutputPortPutBuf(handle));
        printf("pk_probe: disable %d\n", MI_SCL_DisableOutputPort(0, 0, 3));
        return;
    }
    if (!strcmp(cmd, "h265test")) {   /* debug: H.265 CBR on VENC dev 0 next to JPEG, 10 s, then tear down */
        /* Channel attr as cardv's initVideoEncoder (logs/dec_initVideoEncoder.c): type 3 = H.265,
         * max W/H, buffer W*H/2, profile 0, by-frame 1, W, H, 0, 0, rc 8 = H.265 CBR, GOP, stat
         * time, fps num/den, bitrate, fluctuate. Device 0 max size 4608x2592 as cardv. */
        uint32_t dev_attr[8] = { 0x1200, 0xa20 };
        uint32_t fps = sensor_fps_valid(), mbps = n >= 2 && val ? val : 20;
        uint32_t attr[19] = { 3, 1920, 1080, 1920 * 1080 / 2, 0, 1, 1920, 1080, 0, 0,
                              8, fps, 0, fps, 1, mbps * 1000000, 0, 0, 0 };
        ChnPort in = { MOD_VENC, 0, 0, 0 };
        /* h265test 20 2: feed the encoder from its own SCL output port 2 instead of sharing port 1 */
        int port = n >= 3 && arg2 == 2 ? 2 : 1;
        ChnPort src = { MOD_SCL, 0, 0, port };
        int rp = 0;
        if (port == 2) {
            uint32_t p2[6] = { 0, 0, 0x04380780, 0x101, 0xb, 0 };   /* no crop, 1920x1080 NV12 */
            rp = MI_SCL_SetOutputPortParam(0, 0, 2, p2);
            rp |= MI_SCL_EnableOutputPort(0, 0, 2) << 8;
            printf("h265test: scl port 2 param/enable rc %d\n", rp);
        }
        int nojpeg = 0;                                         /* h265test 20 2 1: pause JPEG meanwhile */
        sscanf(line, "%*s %*s %*s %d", &nojpeg);
        if (nojpeg)
            printf("h265test: jpeg paused %d\n", MI_VENC_StopRecvPic(VENC_DEV_JPEG, VENC_CHN));
        int r1 = MI_VENC_CreateDev(0, dev_attr);
        int r2 = MI_VENC_CreateChn(0, 0, attr);
        int r3 = MI_SYS_BindChnPort2(0, &src, &in, 30, 30, 1, 0);
        int r4 = MI_VENC_StartRecvPic(0, 0);
        printf("h265test: dev %d chn %d bind %d start %d (%u fps, %u Mbps)\n", r1, r2, r3, r4, fps, mbps);
        long frames = 0, idr = 0;
        unsigned long long bytes = 0;
        FILE *f = fopen("/tmp/h265test.h265", "wb");
        int fd = MI_VENC_GetFd(0, 0);
        struct timespec ts0, ts1;
        clock_gettime(CLOCK_MONOTONIC, &ts0);
        long jpeg0 = venc_frames, first_ms = -1;
        long frames_after_first = 0;
        for (;;) {
            clock_gettime(CLOCK_MONOTONIC, &ts1);
            long ms = (ts1.tv_sec - ts0.tv_sec) * 1000 + (ts1.tv_nsec - ts0.tv_nsec) / 1000000;
            if (!(r2 == 0 && r4 == 0 && fd >= 0 && ms < 10000))
                break;
            fd_set fds;
            struct timeval tv = { 0, 500000 };
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
                continue;
            uint32_t stat[0x28 / 4] = { 0 };
            if (MI_VENC_Query(0, 0, stat) != 0 || stat[4] == 0)
                continue;
            uint8_t packs[PACK_SIZE * MAX_PACKS] = { 0 };
            uint32_t stream[0x40 / 4] = { 0 };
            stream[0] = (uint32_t)packs;
            stream[1] = stat[4] < MAX_PACKS ? stat[4] : MAX_PACKS;
            if (MI_VENC_GetStream(0, 0, stream, -1) != 0)
                continue;
            for (uint32_t i = 0; i < stream[1]; i++) {
                uint32_t len = *(uint32_t *)(packs + i * PACK_SIZE + 0xc);
                const uint8_t *d = *(uint8_t **)(packs + i * PACK_SIZE + 8);
                bytes += len;
                if (len > 5 && ((d[4] >> 1) & 0x3f) == 32)    /* VPS NAL = start of an IDR group */
                    idr++;
                if (f && ftell(f) < 400000)
                    fwrite(d, 1, len, f);
            }
            frames++;
            if (first_ms < 0)
                first_ms = ms;                          /* rate measured from the first frame on */
            else
                frames_after_first++;
            MI_VENC_ReleaseStream(0, 0, stream);
        }
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        long end_ms = (ts1.tv_sec - ts0.tv_sec) * 1000 + (ts1.tv_nsec - ts0.tv_nsec) / 1000000;
        if (f)
            fclose(f);
        if (fd >= 0)
            MI_VENC_CloseFd(0, 0);
        double secs = end_ms / 1000.0, rate = first_ms >= 0 && end_ms > first_ms ?
                      frames_after_first * 1000.0 / (end_ms - first_ms) : 0;
        printf("h265test: %ld frames (%ld with VPS) %llu bytes in %.2f s: %.2f fps after first frame (%ld ms), %.2f Mbps; jpeg %.2f fps\n",
               frames, idr, bytes, secs, rate, first_ms, bytes * 8 / secs / 1e6, (venc_frames - jpeg0) / secs);
        printf("h265test: teardown stop %d unbind %d chn %d dev %d\n", MI_VENC_StopRecvPic(0, 0),
               MI_SYS_UnBindChnPort(0, &src, &in), MI_VENC_DestroyChn(0, 0), MI_VENC_DestroyDev(0));
        if (port == 2)
            printf("h265test: scl port 2 disable %d\n", MI_SCL_DisableOutputPort(0, 0, 2));
        if (nojpeg)
            printf("h265test: jpeg resumed %d\n", MI_VENC_StartRecvPic(VENC_DEV_JPEG, VENC_CHN));
        return;
    }
    if (!strcmp(cmd, "aetrace")) {    /* debug: aetrace [SAMPLES]: avgY/shutter/gain every 50 ms (read-only) */
        int count = n >= 2 && val > 0 && val <= 200 ? val : 100;
        uint32_t info[0x23c / 4];
        for (int i = 0; i < count; i++) {
            MI_ISP_AE_QueryExposureInfo(0, 0, info);
            printf("aet %3d y %3u sh %5u sg %5u ig %4u st %u\n", i, info[AE_AVG_Y], info[5], info[3], info[4], info[0]);
            usleep(50000);
        }
        return;
    }
    if (!strcmp(cmd, "aestate") && n == 2) {   /* debug: AE state 0 run / 1 pause */
        uint32_t st = val;
        printf("aestate %u: set %d\n", st, MI_ISP_AE_SetState(0, 0, &st));
        return;
    }
    if (!strcmp(cmd, "flicker") && n == 2) {   /* flicker 0 vendor | 1 off | 2 50 Hz | 3 60 Hz */
        settings.ae_flicker = val & 3;
        apply_ae_flicker();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "aespeed") && n == 2) {   /* aespeed PCT (0 = vendor) */
        settings.ae_speed = val > 1000 ? 1000 : val;
        apply_ae_speed();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "aemax") && n == 3) {     /* aemax SHUTTER_US GAIN: auto limits, 0 = vendor */
        settings.ae_max_shutter_us = val > 1000000 ? 1000000 : val;
        settings.ae_max_gain = arg2 > 32768 ? 32768 : arg2;
        apply_exposure();
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "aelim") && n >= 2) {    /* debug: aelim INDEX VALUE: change one exposure-limit word */
        uint32_t lim[8];
        if (MI_ISP_AE_GetExposureLimit(0, 0, lim) != 0 || val > 7)
            return;
        uint32_t old = lim[val];
        lim[val] = arg2;
        printf("aelim [%lu] %u -> %lu: set %d\n", val, old, arg2, MI_ISP_AE_SetExposureLimit(0, 0, lim));
        return;
    }
    if (!strcmp(cmd, "aeinfo")) {     /* debug: AE limits / speed / state / flicker / target (read-only) */
        uint32_t lim[8] = { 0 }, conv[10] = { 0 }, st = 0, fl = 0, tgt[0x84 / 4] = { 0 };
        int r1 = MI_ISP_AE_GetExposureLimit(0, 0, lim), r2 = MI_ISP_AE_GetConverge(0, 0, conv);
        int r3 = MI_ISP_AE_GetState(0, 0, &st), r4 = MI_ISP_AE_GetFlicker(0, 0, &fl), r5 = MI_ISP_AE_GetTarget(0, 0, tgt);
        printf("aeinfo: rc %d/%d/%d/%d/%d | limit", r1, r2, r3, r4, r5);
        for (int i = 0; i < 8; i++) printf(" %u", lim[i]);
        printf(" | converge");
        for (int i = 0; i < 10; i++) printf(" %u", conv[i]);
        printf(" | state %u flicker %u | target", st, fl);
        for (int i = 0; i < 0x84 / 4; i++) printf(" %u", tgt[i]);
        printf("\n");
        return;
    }
    if (!strcmp(cmd, "nr")) {         /* debug: noise / clean-up structs (read-only), hex, 36 bytes a line */
        static const struct { const char *name; int (*get)(uint32_t, uint32_t, void *); int size; } t[] = {
            { "luma", MI_ISP_IQ_GetNRLuma, 0x24c }, { "chroma", MI_ISP_IQ_GetNRChroma, 0x118 },
            { "defog", MI_ISP_IQ_GetDefog, 0x1c }, { "falsecolor", MI_ISP_IQ_GetFalseColor, 0x90 },
            { "despike", MI_ISP_IQ_GetNRDeSpike, 0x24c },
        };
        static uint8_t b[0x24c];
        for (unsigned k = 0; k < sizeof(t) / sizeof(t[0]); k++) {
            memset(b, 0, sizeof(b));
            printf("nr %s: rc %d\n", t[k].name, t[k].get(0, 0, b));
            for (int row = 0; row < t[k].size; row += 36) {
                printf("nr %s +%03x", t[k].name, row);
                for (int i = row; i < row + 36 && i < t[k].size; i++) printf("%s%02x", i % 4 ? "" : " ", b[i]);
                printf("\n");
            }
        }
        return;
    }
    if (!strcmp(cmd, "hsv")) {        /* debug: HSV struct dump (read-only), s16 words, zero rows skipped */
        static uint8_t h[0x134c];
        int r = MI_ISP_IQ_GetHSV(0, 0, h);
        printf("hsv: rc %d\n", r);
        for (int row = 0; row < 0x134c; row += 48) {
            int nz = 0;
            for (int i = row; i < row + 48 && i < 0x134c; i++) nz |= h[i];
            if (!nz)
                continue;
            printf("hsv: +%04x", row);
            for (int i = row; i < row + 48 && i < 0x134c; i += 2) printf(" %d", *(int16_t *)(h + i));
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "wbinfo")) {     /* debug: white balance state dump (read-only) */
        static uint8_t attr[0x5b8];
        uint8_t info[0x18] = { 0 }, stab[0xc] = { 0 };
        uint32_t ct = 0;
        int r1 = MI_ISP_AWB_QueryInfo(0, 0, info), r2 = MI_ISP_AWB_GetAttr(0, 0, attr);
        int r3 = MI_ISP_AWB_GetCTMwbAttr(0, 0, &ct), r4 = MI_ISP_AWB_GetStabilizer(0, 0, stab);
        printf("awb: rc %d/%d/%d/%d | ctmwb %u | info", r1, r2, r3, r4, ct);
        for (int i = 0; i < 0x18; i += 2) printf(" %u", *(uint16_t *)(info + i));
        printf(" | stab");
        for (int i = 0; i < 0xc; i += 2) printf(" %u", *(uint16_t *)(stab + i));
        printf("\n");
        for (int row = 0; row < 0x5b8; row += 64) {
            int nz = 0;
            for (int i = row; i < row + 64 && i < 0x5b8; i++) nz |= attr[i];
            if (!nz)
                continue;
            printf("awb: attr +%03x", row);
            for (int i = row; i < row + 64 && i < 0x5b8; i += 2) printf(" %u", *(uint16_t *)(attr + i));
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "ccm")) {        /* debug: colour matrix dump (read-only), raw u16 words */
        static uint8_t m[0x1e8], q[0x1a];
        int r1 = MI_ISP_IQ_GetRGBMatrix(0, 0, m), r2 = MI_ISP_IQ_QueryCCMInfo(0, 0, q);
        printf("ccm: rc %d/%d | info", r1, r2);
        for (int i = 0; i < 0x1a; i += 2) printf(" %d", *(int16_t *)(q + i));
        printf("\n");
        for (int row = 0; row < 0x1e8; row += 32) {
            printf("ccm: +%03x", row);
            for (int i = row; i < row + 32 && i < 0x1e8; i += 2) printf(" %6d", *(int16_t *)(m + i));
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "shp_exp") && n == 2) {   /* debug: isolated sharpness field experiments */
        static uint8_t vendor[0x1328], s[0x1328];
        static int have;
        if (!have && MI_ISP_IQ_GetSharpness(0, 0, vendor) == 0)
            have = 1;
        memcpy(s, vendor, sizeof(s));
        for (int e = 0; e < 17; e++) {
            uint8_t *q = s + 8 + e * 288;
            for (int i = 0; i < 16; i++) {
                if (val == 5) q[0x98 + i] = 0;                                      /* table off */
                if (val == 6) q[0x98 + i] = q[0x98 + i] * 2 > 255 ? 255 : q[0x98 + i] * 2;
            }
            if (val == 2) q[0] = q[0] * 2 > 255 ? 255 : q[0] * 2;
            if (val == 3) q[1] = q[1] * 2 > 255 ? 255 : q[1] * 2;
            if (val == 4) q[2] = q[2] * 2 > 255 ? 255 : q[2] * 2;
            if (val == 7) q[0] = 0;
            if (val == 8) q[1] = q[2] = 0;
        }
        if (val == 1)
            ((uint32_t *)s)[0] = 0;                                                 /* disabled */
        printf("shp_exp %lu: set = %d\n", val, MI_ISP_IQ_SetSharpness(0, 0, s));
        return;
    }
    if (!strcmp(cmd, "shp")) {        /* debug: sharpness struct dump (read-only) */
        static uint8_t s[0x1328];
        int r = MI_ISP_IQ_GetSharpness(0, 0, s);
        printf("shp: rc %d enable %u optype %u\n", r, ((uint32_t *)s)[0], ((uint32_t *)s)[1]);
        for (int e = 0; e < 17; e += (e == 0 ? 8 : e == 8 ? 7 : 1)) {   /* entries 0, 8, 15, manual */
            const uint8_t *q = s + 8 + e * 288;
            for (int row = 0; row < 288; row += 48) {
                printf("shp: %s%2d +%03x", e < 16 ? "auto" : "man ", e < 16 ? e : 0, row);
                for (int i = row; i < row + 48; i++) printf("%s%02x", i % 4 ? "" : " ", q[i]);
                printf("\n");
            }
        }
        return;
    }
    if (!strcmp(cmd, "gamma")) {      /* debug: RGB gamma LUT summary (read-only) */
        static uint8_t g[0x6608];
        int r = MI_ISP_IQ_GetRGBGamma(0, 0, g);
        printf("gamma: rc %d enable %u optype %u\n", r, ((uint32_t *)g)[0], ((uint32_t *)g)[1]);
        for (int e = 0; e < 17; e++) {                /* entry = 1536 bytes: u16 R[256], G[256], B[256]? */
            const uint16_t *q = (const uint16_t *)(g + 8 + e * 1536);
            printf("gamma: %s%2d R", e < 16 ? "auto" : "man ", e < 16 ? e : 0);
            for (int i = 0; i < 256; i += 16) printf(" %u", q[i]);
            printf(" %u | G", q[255]);
            for (int i = 0; i < 256; i += 64) printf(" %u", q[256 + i]);
            printf(" %u | B %u %u\n", q[511], q[512 + 128], q[767]);
        }
        return;
    }
    if (!strcmp(cmd, "iq")) {         /* debug: dump basic ISP image controls (read-only) */
        static const struct { const char *name; int (*get)(uint32_t, uint32_t, void *); int size, entry; } t[] = {
            { "brightness", MI_ISP_IQ_GetBrightness, 0x4c, 4 }, { "contrast", MI_ISP_IQ_GetContrast, 0x4c, 4 },
            { "lightness", MI_ISP_IQ_GetLightness, 0x4c, 4 }, { "saturation", MI_ISP_IQ_GetSaturation, 0x1a0, 24 },
            { "obc", MI_ISP_IQ_GetOBC, 0x90, 8 }, { "colortogray", MI_ISP_IQ_GetColorToGray, 4, 0 },
        };
        static uint8_t b[0x1a0];
        for (unsigned k = 0; k < sizeof(t) / sizeof(t[0]); k++) {
            memset(b, 0, sizeof(b));
            int r = t[k].get(0, 0, b);
            printf("iq %s: rc %d |", t[k].name, r);
            for (int i = 0; i < t[k].size; i++) printf("%s%02x", i % 4 ? "" : " ", b[i]);
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "wdr")) {        /* debug: dump WDR (local tone mapping) state, read-only */
        static uint8_t w[0x668];
        int r = MI_ISP_IQ_GetWDR(0, 0, w);
        printf("wdr: rc %d enable %u optype %u\n", r, *(uint32_t *)w, *(uint32_t *)(w + 4));
        for (int e = 0; e < 17; e++) {
            const uint8_t *q = w + 8 + e * 96;
            printf("wdr: %s%2d +2b=%3u |", e < 16 ? "auto" : "man ", e < 16 ? e : 0, q[0x2b]);
            for (int i = 0; i < 96; i++) printf("%s%02x", i % 4 ? "" : " ", q[i]);
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "nr3d")) {       /* debug: dump temporal NR state (read-only) */
        static uint8_t nr[0x844];
        uint8_t ex[0x24] = { 0 }, p1[4] = { 0 };
        int r1 = MI_ISP_IQ_GetNR3D(0, 0, nr), r2 = MI_ISP_IQ_GetNR3D_EX(0, 0, ex), r3 = MI_ISP_IQ_GetNR3D_P1(0, 0, p1);
        printf("nr3d: rc %d/%d/%d enable %u optype %u | EX", r1, r2, r3, *(uint32_t *)nr, *(uint32_t *)(nr + 4));
        for (int i = 0; i < 0x24; i++) printf(" %02x", ex[i]);
        printf(" | P1 %02x %02x %02x %02x\n", p1[0], p1[1], p1[2], p1[3]);
        for (int e = 0; e < 17; e++) {                /* 16 auto entries (by ISO), then manual */
            const uint8_t *q = nr + 8 + e * 124;
            printf("nr3d: %s%2d", e < 16 ? "auto" : "man ", e < 16 ? e : 0);
            for (int i = 0; i < 124; i++) printf("%s%02x", i % 4 ? "" : " ", q[i]);
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "histo")) {      /* debug: compare the two histogram sources */
        uint16_t hw[128] = { 0 };
        uint32_t wy[130] = { 0 };
        int r1 = MI_ISP_AE_GetHisto0HwStats(0, 0, hw), r2 = MI_ISP_AE_GetHistoWghtY(0, 0, wy);
        unsigned long s1 = 0, s2 = 0;
        for (int i = 0; i < 128; i++) { s1 += hw[i]; s2 += wy[2 + i]; }
        printf("histo: hw rc %d sum %lu | wght rc %d lumY %u avgY %u sum %lu\n", r1, s1, r2, wy[0], wy[1], s2);
        for (int row = 0; row < 8; row++) {
            printf("histo: %3d-%3d hw", row * 16, row * 16 + 15);
            for (int i = 0; i < 16; i++) printf(" %5u", hw[row * 16 + i]);
            printf(" | w");
            for (int i = 0; i < 16; i++) printf(" %5u", wy[2 + row * 16 + i]);
            printf("\n");
        }
        return;
    }
    if (!strcmp(cmd, "wgt")) {
        static struct win_weights w;
        printf("wgt: GetWinWgt = %d, mode %u\n", MI_ISP_AE_GetWinWgt(0, 0, &w), w.mode);
        for (int i = 0; i < 3; i++) {
            struct wgt_box b;
            int any = weight_box(&w, i, &b);
            int cx = any ? (b.x0 + b.x1) / 2 : 0, cy = any ? (b.y0 + b.y1) / 2 : 0;
            printf("wgt: %s blocks x %d..%d y %d..%d, centre weight %u, corner weight %u\n", metering_names[i],
                   b.x0, b.x1, b.y0, b.y1, w.tbl[i][cy][cx], w.tbl[i][0][0]);
            for (int y = 0; y < WGT_GRID; y++) {
                printf("wgt:  %s", metering_names[i]);
                for (int x = 0; x < WGT_GRID; x++)
                    printf(" %3u", w.tbl[i][y][x]);
                printf("\n");
            }
        }
        return;
    }
    if (!strcmp(cmd, "snap")) {
        snap_request = 1;
        return;
    }
    if (!strcmp(cmd, "auto")) {
        settings.manual = 0;
    } else if (!strcmp(cmd, "shutter") && n == 2) {
        settings.manual = 1;
        settings.shutter_us = val;
        settings.shutter_angle_x10 = 0;
    } else if (!strcmp(cmd, "angle") && n == 2) {  /* angle 172.8: manual shutter by angle, 0.1 degree */
        double a = 0;
        sscanf(line, "%*s %lf", &a);
        settings.manual = 1;
        settings.shutter_angle_x10 = a < 0.1 ? 1 : a > 360 ? 3600 : (uint32_t)(a * 10 + 0.5);
    } else if (!strcmp(cmd, "gain") && n == 2) {
        settings.manual = 1;
        settings.sensor_gain = val;
    } else {
        printf("cmd ignored: %s", line);
        return;
    }
    apply_exposure();
    settings_changed();
}
