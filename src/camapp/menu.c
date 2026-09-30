/* On-screen menu (see camapp.h): pages, rows, values, help text, and what each button does to the
 * selected row. MENU opens/closes, up/down select, left/right (or AE/OK) change, remote zoom -/+
 * change in fine steps, remote play resets the row (or confirms "Press OK" rows). */
#include "camapp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include "osd.h"
#include "sensordbg.h"

/* ---- Menu: MENU opens/closes, up/down select, left/right (or AE/OK) change the value. ----
 * The visible rows depend on state: Manual shows Shutter + Gain, Auto shows EV comp. */
enum { MI_MODE, MI_FRAMERATE, MI_CROP_W, MI_CROP_H, MI_TL_INTERVAL, MI_TL_LIMIT, MI_PLAY_FPS, MI_REC_FPS, MI_REC_CODEC, MI_REC_MBPS, MI_PHOTO_RAW, MI_EXPOSURE, MI_SHUTTER_BY, MI_SHUTTER, MI_GAIN, MI_EVCOMP, MI_AE_LOCK, MI_AE_SPEED, MI_AE_FLICKER, MI_AE_MAXSH, MI_AE_MAXGAIN, MI_METERING, MI_SPOT_SIZE, MI_WB, MI_WB_K, MI_WB_R, MI_WB_G, MI_WB_B, MI_WB_GRAB, MI_LOUPE, MI_PEAKING, MI_PEAK_COLOUR, MI_ZEBRA, MI_FALSECOL, MI_WAVEFORM, MI_GUIDES, MI_FRAME, MI_HISTOGRAM,
       MI_MIRROR, MI_FLIP, MI_EXPERT, MI_SYSTEM, MI_SEC_REC, MI_SEC_EXPO, MI_SEC_COLOUR, MI_SEC_ASSIST, XI_BACK,
       /* System page */
       SI_SD, SI_FORMAT, SI_BUILD, SI_UPTIME, SI_CPU, SI_RAM, SI_FLASH, SI_TC, SI_LOGS, SI_RESET, SI_BACK,
       /* Expert > Sensor page (read-only live hardware values) */
       EI_NR3D, EI_WDR, EI_FPS, EI_VMAX, EI_HMAX, EI_SHS, EI_GAIN, EI_TEMP, EI_REGS, EI_ISP, EI_ENC, EI_BACK,
       /* Expert > Registers page (read-only IMX307 register viewer) */
       RI_ADDR, RI_V0, RI_V1, RI_V2, RI_V3, RI_V4, RI_V5, RI_V6, RI_V7, RI_ACCESS, RI_UNDO, RI_BACK,
       /* Expert > Image page (ISP image controls) */
       II_LOOK_SLOT, II_LOOK_LOAD, II_LOOK_SAVE, II_LOOK_EXPORT, II_LOOK_IMPORT, II_BRIGHT, II_CONTRAST, II_LIGHT, II_GAMMA, II_SHARP, II_SAT, II_CCM, II_BLACK, II_BW, II_MATRIX, II_HSV, II_NOISE, II_RESET, II_BACK,
       /* Expert > Image > Matrix page (custom colour matrix) */
       CI_MODE, CI_COPY, CI_M0, CI_M1, CI_M2, CI_M3, CI_M4, CI_M5, CI_M6, CI_M7, CI_M8, CI_RESET, CI_BACK,
       /* Expert > Image > Hue tweaks page (ISP HSV) */
       HI_RH, HI_RS, HI_YH, HI_YS, HI_GH, HI_GS, HI_CH, HI_CS, HI_BH, HI_BS, HI_MH, HI_MS, HI_RESET, HI_BACK,
       /* Expert > Image > Noise page */
       NI_LUMA, NI_CHROMA, NI_DESPIKE, NI_FALSE, NI_DEFOG, NI_RESET, NI_BACK,
       /* Expert > Encoder page */
       GI_JPEGQ, GI_GOP, GI_RC, GI_QPMAX, GI_QPMIN, GI_BACK, MI_ALL };
static const char *const menu_labels_all[MI_ALL] = {
    "Mode", "Frame rate", "Crop width", "Crop height", "Interval", "Stop after", "Plays at", "Record fps", "Codec", "Bitrate", "Photo", "Auto/Manual", "Shutter by", "Shutter", "Gain", "EV comp", "AE lock", "AE speed", "Anti-flicker", "Max shutter", "Max gain", "Metering", "Spot size", "White bal.", "Kelvin", "Red gain", "Green gain", "Blue gain", "Set from scene", "Magnify", "Peaking", "Peak colour", "Zebra", "False colour", "Waveform", "Guides", "Frame", "Histogram", "Mirror", "Flip",
    "Expert", "System", "Recording", "Exposure", "Colour", "Assist", "Back", "SD card", "Format SD", "Build", "Uptime", "CPU", "RAM", "Flash", "Timecode", "Save logs", "Reset settings", "Back",
    "Temporal NR", "WDR", "Real fps", "Frame length", "Line length", "Exposure", "Sensor gain", "SoC temp", "Registers", "Image & looks", "Encoder", "Back",
    "Address", "", "", "", "", "", "", "", "", "Write access", "Undo writes", "Back",
    "Look slot", "Load look", "Save look", "Export looks", "Import looks", "Brightness", "Contrast", "Lightness", "Gamma", "Sharpness", "Saturation", "Colour matrix", "Black level", "Black & white", "Matrix edit", "Hue tweaks", "Noise & clean-up", "Reset page", "Back",
    "Matrix", "Copy current", "R from R", "R from G", "R from B", "G from R", "G from G", "G from B",
    "B from R", "B from G", "B from B", "Reset page", "Back",
    "Red hue", "Red sat", "Yellow hue", "Yellow sat", "Green hue", "Green sat", "Cyan hue", "Cyan sat",
    "Blue hue", "Blue sat", "Magenta hue", "Magenta sat", "Reset page", "Back",
    "Luma NR", "Chroma NR", "De-spike", "False colour", "Defog", "Reset page", "Back",
    "JPEG quality", "H.265 GOP", "Rate control", "Max QP", "Min QP", "Back",
};
enum { PAGE_MAIN, PAGE_SYSTEM, PAGE_EXPERT, PAGE_REGS, PAGE_ISP, PAGE_CCM, PAGE_HSV, PAGE_NOISE, PAGE_ENC,
       PAGE_REC, PAGE_EXPO, PAGE_COLOUR, PAGE_ASSIST, N_PAGES };
/* Page tree: parent page and the parent's row that opens it (Back / MENU return to that row). */
static const int page_parent[N_PAGES] = { PAGE_MAIN, PAGE_MAIN, PAGE_MAIN, PAGE_EXPERT, PAGE_COLOUR, PAGE_ISP,
                                          PAGE_ISP, PAGE_ISP, PAGE_EXPERT, PAGE_MAIN, PAGE_MAIN, PAGE_MAIN, PAGE_MAIN };
static const int page_entry[N_PAGES] = { -1, MI_SYSTEM, MI_EXPERT, EI_REGS, EI_ISP, II_MATRIX, II_HSV, II_NOISE, EI_ENC,
                                         MI_SEC_REC, MI_SEC_EXPO, MI_SEC_COLOUR, MI_SEC_ASSIST };
/* Section accent (title + left stripe): Recording red, Exposure yellow, Colour green, Assist cyan,
 * Expert magenta, System grey, main menu amber. The selection bar stays amber everywhere. */
static const uint32_t page_accent[N_PAGES] = { 0xffffb020, 0xff9aa6ab, 0xffd050d0, 0xffd050d0, 0xff40c860, 0xff40c860,
                                               0xff40c860, 0xff40c860, 0xffd050d0, 0xffff4040, 0xffffd820, 0xff40c860,
                                               0xff30c8e0 };
/* Rows of the section pages and the pages that are not one enum range. */
static const int rows_main[] = { MI_MODE, MI_SEC_REC, MI_SEC_EXPO, MI_SEC_COLOUR, MI_SEC_ASSIST, MI_EXPERT, MI_SYSTEM, -1 };
static const int rows_rec[] = { MI_FRAMERATE, MI_CROP_W, MI_CROP_H, MI_TL_INTERVAL, MI_TL_LIMIT, MI_PLAY_FPS, MI_REC_FPS,
                                MI_REC_CODEC, MI_REC_MBPS, MI_PHOTO_RAW, XI_BACK, -1 };
static const int rows_expo[] = { MI_EXPOSURE, MI_SHUTTER_BY, MI_SHUTTER, MI_GAIN, MI_EVCOMP, MI_METERING, MI_SPOT_SIZE, MI_AE_LOCK,
                                 MI_AE_SPEED, MI_AE_FLICKER, MI_AE_MAXSH, MI_AE_MAXGAIN, XI_BACK, -1 };
static const int rows_colour[] = { MI_WB, MI_WB_K, MI_WB_R, MI_WB_G, MI_WB_B, MI_WB_GRAB, EI_ISP, XI_BACK, -1 };
static const int rows_assist[] = { MI_LOUPE, MI_PEAKING, MI_PEAK_COLOUR, MI_ZEBRA, MI_FALSECOL, MI_WAVEFORM, MI_HISTOGRAM,
                                   MI_GUIDES, MI_FRAME, XI_BACK, -1 };
static const int rows_system[] = { MI_MIRROR, MI_FLIP, SI_SD, SI_FORMAT, SI_BUILD, SI_UPTIME, SI_CPU, SI_RAM, SI_FLASH, SI_TC, SI_LOGS, SI_RESET, SI_BACK, -1 };
static const int rows_expert[] = { EI_NR3D, EI_WDR, EI_FPS, EI_VMAX, EI_HMAX, EI_SHS, EI_GAIN, EI_TEMP, EI_REGS, EI_ENC,
                                   EI_BACK, -1 };
/* Register viewer: 8 registers from reg_base. IMX307 registers live at 0x3000-0x34FF. */
#define REG_FIRST 0x3000
#define REG_LAST 0x34f8
static unsigned reg_base = 0x3018;
/* Register writes: Safe = only sensor_reg_safe() registers, All = anything (volatile; worst case the
 * sensor stops until camapp restarts). The first write to each address records its original value
 * so "Undo writes" (FIFO regundo) can put everything back, newest first. */
static int reg_write_all;
static struct { uint16_t reg; uint8_t orig; } reg_undo[128];
static int reg_n_undo;

int reg_write(uint16_t reg, uint8_t val)
{
    uint8_t old = 0;
    if (sensor_read_block(reg, &old, 1) != 0)
        return -1;
    int seen = 0;
    for (int i = 0; i < reg_n_undo; i++)
        seen |= reg_undo[i].reg == reg;
    if (!seen && reg_n_undo < (int)(sizeof(reg_undo) / sizeof(reg_undo[0]))) {
        reg_undo[reg_n_undo].reg = reg;
        reg_undo[reg_n_undo++].orig = old;
    }
    int r = sensor_write_reg(reg, val);
    printf("regw 0x%04X %s: 0x%02X -> 0x%02X rc %d%s\n", reg, sensor_reg_name(reg), old, val, r,
           sensor_reg_safe(reg) ? "" : " (UNSAFE register)");
    return r;
}

void reg_undo_all(void)
{
    while (reg_n_undo > 0) {
        reg_n_undo--;
        int r = sensor_write_reg(reg_undo[reg_n_undo].reg, reg_undo[reg_n_undo].orig);
        printf("regundo 0x%04X -> 0x%02X rc %d\n", reg_undo[reg_n_undo].reg, reg_undo[reg_n_undo].orig, r);
    }
}
static void menu_back(void);
static const char *codec_short(void);
static int menu_page, menu_confirm = -1;    /* row id waiting for a second OK, -1 = none */
static int mode_pick = -1;                  /* Mode row: picked, not yet confirmed; -1 = none */

/* Crop rows: settings.crop_w/h hold the size picked in the menu, crop_w/h the size this run started
 * with; they differ until OK/play restarts, and leaving the row puts the running size back. */
static uint32_t crop_set_w(void)
{
    return settings.crop_w == OPEN_GATE_W ? OPEN_GATE_W : !settings.crop_w || settings.crop_w > 1920 ? 1920 : settings.crop_w;
}
static uint32_t crop_set_h(void)
{
    return settings.crop_w == OPEN_GATE_W ? OPEN_GATE_H : !settings.crop_h || settings.crop_h > 1080 ? 1080 : settings.crop_h;
}
static int crop_pending(void) { return crop_set_w() != SRC_W || crop_set_h() != SRC_H; }
static void crop_cancel(void)
{
    if (crop_pending())
        settings.crop_w = crop_w, settings.crop_h = crop_h;
}
static const char *const guide_names[] = { "Off", "Thirds", "Cross", "Both" };

/* Shutter speed scale for the arrows (1/x): third stops like a stills/video camera plus the cine
 * values 24, 48 and 120; the fine buttons step between them. */
static const unsigned speed_scale[] = {
    24, 25, 30, 40, 48, 50, 60, 80, 100, 120, 125, 160, 200, 250, 320, 400, 500, 640, 800, 1000, 1250, 1600,
    2000, 2500, 3200, 4000, 5000, 6400, 8000, 10000, 12800, 16000, 20000, 25600, 32000, 40000, 51200, 64000,
};
#define N_SPEED (int)(sizeof(speed_scale) / sizeof(speed_scale[0]))

/* Half-stop gain steps (1024 = 1x, label in dB). */
static const struct { unsigned gain, db; } gain_steps[] = {
    { 1024, 0 }, { 1448, 3 }, { 2048, 6 }, { 2896, 9 }, { 4096, 12 }, { 5793, 15 }, { 8192, 18 },
    { 11585, 21 }, { 16384, 24 },
};
#define N_GAIN (int)(sizeof(gain_steps) / sizeof(gain_steps[0]))

int menu_open, menu_sel;
static int menu_rows[MI_ALL], menu_nrows;

#define MENU_VISIBLE 14                     /* rows on screen at once; longer pages scroll */
static int menu_want_id = -1;               /* row to select after the next rebuild, -1 = keep current */

/* Rebuild the visible rows. Rows appear and disappear with state (exposure mode, WB mode), so the
 * cursor follows the selected row by id, not by position. */
static void menu_build_rows(void)
{
    int keep = menu_want_id >= 0 ? menu_want_id : menu_nrows && menu_sel < menu_nrows ? menu_rows[menu_sel] : -1;
    menu_want_id = -1;
    menu_nrows = 0;
    static const int *const lists[N_PAGES] = { rows_main, rows_system, rows_expert, NULL, NULL, NULL, NULL, NULL, NULL,
                                               rows_rec, rows_expo, rows_colour, rows_assist };
    static const int first_row[N_PAGES] = { 0, 0, 0, RI_ADDR, II_LOOK_SLOT, CI_MODE, HI_RH, NI_LUMA, GI_JPEGQ };
    static const int last_row[N_PAGES] = { 0, 0, 0, RI_BACK, II_BACK, CI_BACK, HI_BACK, NI_BACK, GI_BACK };
    const int *list = lists[menu_page];
    int n_ids = 0, ids[MI_ALL];
    if (list)
        while (list[n_ids] >= 0)
            ids[n_ids] = list[n_ids], n_ids++;
    else
        for (int i = first_row[menu_page]; i <= last_row[menu_page]; i++)
            ids[n_ids++] = i;
    for (int k = 0; k < n_ids; k++) {
        int i = ids[k];
        if ((i == MI_SHUTTER_BY || i == MI_SHUTTER) && !settings.manual && settings.ae_priority != 1)
            continue;
        if (i == MI_GAIN && !settings.manual && settings.ae_priority != 2)
            continue;
        if ((i == MI_AE_MAXSH && settings.ae_priority == 1) || (i == MI_AE_MAXGAIN && settings.ae_priority == 2))
            continue;                               /* pinned by the priority mode */
        if ((i == MI_EVCOMP || i == MI_AE_LOCK || i == MI_AE_SPEED || i == MI_AE_FLICKER || i == MI_AE_MAXSH
             || i == MI_AE_MAXGAIN)
            && settings.manual)
            continue;
        if (i == MI_REC_MBPS && settings.rec_codec == 0)
            continue;
        if ((i == MI_CROP_W || i == MI_CROP_H) && settings.mode != MODE_NORMAL && settings.mode != MODE_SLOWMO)
            continue;
        if (i == MI_PLAY_FPS && settings.mode != MODE_SLOWMO && settings.mode != MODE_TIMELAPSE)
            continue;
        if ((i == MI_TL_INTERVAL || i == MI_TL_LIMIT) && settings.mode != MODE_TIMELAPSE)
            continue;
        if (i == MI_REC_FPS && (settings.mode == MODE_SLOWMO || settings.mode == MODE_TIMELAPSE))
            continue;
        if ((i == MI_REC_CODEC || i == MI_REC_MBPS) && settings.mode == MODE_TIMELAPSE)
            continue;                               /* timelapse records MJPEG for now */
        if (i == MI_SPOT_SIZE && settings.metering != 2)
            continue;
        if (i == MI_PEAK_COLOUR && !settings.peaking)
            continue;
        if (i == MI_WB_K && settings.wb_mode != 1)
            continue;
        if ((i == MI_WB_R || i == MI_WB_G || i == MI_WB_B) && settings.wb_mode != 2)
            continue;
        if (i == MI_WB_GRAB && settings.wb_mode == 2)
            continue;
        if ((i == GI_QPMAX || i == GI_QPMIN) && settings.h265_rc != 1)
            continue;
        if (i == keep)
            menu_sel = menu_nrows;
        menu_rows[menu_nrows++] = i;
    }
    if (menu_sel >= menu_nrows)
        menu_sel = menu_nrows - 1;
}

void menu_draw(void)
{
    static char buf[MI_ALL][32];
    const char *labels[MI_ALL], *values[MI_ALL];
    struct sensor_regs regs = { -1, -1, -1, -1, 0, 0, 0 };
    int regs_ok = 0;
    static char reg_label[8][24];
    uint8_t reg_val[8];
    int reg_ok = 0;
    menu_build_rows();
    if (menu_page == PAGE_EXPERT)
        regs_ok = sensor_read_regs(&regs) == 0;
    if (menu_page == PAGE_REGS)
        reg_ok = sensor_read_block(reg_base, reg_val, 8) == 0;
    for (int r = 0; r < menu_nrows; r++) {
        int id = menu_rows[r];
        char *b = buf[r];
        switch (id) {
        case MI_MODE:
            if (mode_pick >= 0 && mode_pick != (int)settings.mode)
                snprintf(b, 32, "%s: %s", mode_names[mode_pick], rec_on ? "stop rec first" : "OK/Play = go");
            else
                snprintf(b, 32, "%s", mode_names[settings.mode]);
            break;
        case MI_FRAMERATE: snprintf(b, 32, "%up", sensor_fps_valid()); break;
        case MI_PLAY_FPS:
            if (settings.mode == MODE_TIMELAPSE)
                snprintf(b, 32, "%up (%.0fx faster)", play_fps(), settings.tl_interval_ms * play_fps() / 1000.0);
            else
                snprintf(b, 32, "%up (%.1fx slower)", play_fps(), sensor_fps_valid() / (double)play_fps());
            break;
        case MI_CROP_W: case MI_CROP_H: {
            uint32_t w = crop_set_w(), h = crop_set_h();
            if (crop_pending())
                snprintf(b, 32, "%u (now %u): %s", id == MI_CROP_W ? w : h, id == MI_CROP_W ? SRC_W : SRC_H,
                         rec_on ? "stop rec" : "Play = go");
            else if (id == MI_CROP_W && w == OPEN_GATE_W)
                snprintf(b, 32, "Open gate %u (exp.)", w);
            else if (id == MI_CROP_W)
                snprintf(b, 32, "%u%s (max %up)", w, w == 1920 ? " full" : "", crop_max_fps(w));
            else
                snprintf(b, 32, "%u%s (%.2f:1)", h, h == 1080 ? " full" : "", (double)w / h);
            break;
        }
        case MI_TL_INTERVAL: snprintf(b, 32, "%s", tl_interval_text()); break;
        case MI_SPOT_SIZE:
            snprintf(b, 32, "%d (%d%% of width)%s", spot_w(), spot_w() * 100 / WGT_GRID, spot_w() == 2 ? " default" : "");
            break;
        case MI_PHOTO_RAW: snprintf(b, 32, "%s", settings.photo_raw ? "JPEG + DNG (raw)" : "JPEG"); break;
        case MI_TL_LIMIT:
            if (!settings.tl_limit_frames)
                snprintf(b, 32, "Off (until stopped)");
            else
                snprintf(b, 32, "%u fr = %.1fs clip", settings.tl_limit_frames, settings.tl_limit_frames / (double)play_fps());
            break;
        case MI_REC_FPS:
            if (sensor_fps_valid() == 60)
                snprintf(b, 32, "%up", settings.rec_fps);
            else
                snprintf(b, 32, "= %up", sensor_fps_valid());
            break;
        case MI_EXPOSURE:
            snprintf(b, 32, "%s", settings.manual ? "M manual" : settings.ae_priority == 1 ? "S shutter prio"
                                  : settings.ae_priority == 2 ? "G gain prio" : "P auto");
            break;
        case MI_SHUTTER_BY: snprintf(b, 32, "%s", settings.shutter_angle_x10 ? "Angle" : "Speed"); break;
        case MI_SHUTTER: {
            uint32_t us = manual_shutter_us();
            const char *over = us > max_manual_us() ? " (max)" : us > frame_us() ? " (long)" : "";
            if (settings.shutter_angle_x10 % 10)
                snprintf(b, 32, "%u.%u" OSD_DEGREE " = 1/%u%s", settings.shutter_angle_x10 / 10,
                         settings.shutter_angle_x10 % 10, (1000000 + us / 2) / us, over);
            else if (settings.shutter_angle_x10)
                snprintf(b, 32, "%u" OSD_DEGREE " = 1/%u%s", settings.shutter_angle_x10 / 10, (1000000 + us / 2) / us, over);
            else
                snprintf(b, 32, "%s%s", shutter_text(us), over);
            break;
        }
        case MI_GAIN: {
            double db = 20 * log10(settings.sensor_gain / 1024.0);
            snprintf(b, 32, "+%.1f dB  %.2fx", db < 0.05 ? 0 : db, settings.sensor_gain / 1024.0);
            break;
        }
        case MI_EVCOMP: snprintf(b, 32, "%+.1f EV", settings.ev_thirds / 3.0); break;
        case MI_METERING:
            snprintf(b, 32, "%s", settings.metering == 1 ? "Centre" : settings.metering == 2 ? "Spot" : "Average");
            break;
        case MI_AE_LOCK: snprintf(b, 32, "%s", ae_locked ? "Locked" : "Off"); break;
        case MI_AE_FLICKER: snprintf(b, 32, "%s", flicker_names[settings.ae_flicker & 3]); break;
        case MI_AE_SPEED:
            if (!settings.ae_speed) snprintf(b, 32, "Vendor");
            else if (settings.ae_speed >= 1000) snprintf(b, 32, "Instant");
            else snprintf(b, 32, "%u%%", settings.ae_speed);
            break;
        case MI_AE_MAXSH:
            if (!settings.ae_max_shutter_us || settings.ae_max_shutter_us >= frame_us() * 97 / 100)
                snprintf(b, 32, "Auto (1/%u)", sensor_fps_valid());
            else
                snprintf(b, 32, "1/%u", (1000000 + settings.ae_max_shutter_us / 2) / settings.ae_max_shutter_us);
            break;
        case MI_AE_MAXGAIN:
            if (!settings.ae_max_gain) snprintf(b, 32, "Vendor 8x");
            else snprintf(b, 32, "%ux", settings.ae_max_gain / 1024);
            break;
        case MI_REC_CODEC:
            snprintf(b, 32, "%s", settings.rec_codec == 1 ? "H.265 (.ts)" : settings.rec_codec == 2 ? "H.264 (.ts)" : "MJPEG (.avi)");
            break;
        case MI_REC_MBPS: snprintf(b, 32, "%u Mbps", settings.rec_mbps); break;
        case MI_WB: snprintf(b, 32, "%s", wb_names[settings.wb_mode % 3]); break;
        case MI_WB_K: snprintf(b, 32, "%u K", settings.wb_kelvin); break;
        case MI_WB_R: case MI_WB_G: case MI_WB_B: {
            uint32_t v = id == MI_WB_R ? settings.wb_r : id == MI_WB_G ? settings.wb_g : settings.wb_b;
            snprintf(b, 32, "%.3fx", v / 1024.0);
            break;
        }
        case MI_WB_GRAB: snprintf(b, 32, "Press OK"); break;
        case MI_PEAKING:
            if (!settings.peaking) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%u", settings.peaking);
            break;
        case MI_PEAK_COLOUR: snprintf(b, 32, "%s", peak_colour_names[settings.peak_colour & 3]); break;
        case MI_ZEBRA:
            if (!settings.zebra) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%u%%", settings.zebra - 1);
            break;
        case MI_FALSECOL: snprintf(b, 32, "%s", settings.false_colour ? "On" : "Off"); break;
        case MI_WAVEFORM: {
            static const char *const wf_names[] = { "Off", "Luma", "RGB", "Parade" };
            snprintf(b, 32, "%s", wf_names[settings.waveform & 3]);
            break;
        }
        case MI_LOUPE:
            if (!loupe_on) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%d.%02dx", loupe_z10 / 10, loupe_z10 % 10 * 10);
            break;
        case MI_GUIDES: snprintf(b, 32, "%s", guide_names[settings.overlay & 3]); break;
        case MI_FRAME: snprintf(b, 32, "%s", osd_frame_names[(settings.overlay & OSD_FRAME_MASK) >> OSD_FRAME_SHIFT]); break;
        case MI_HISTOGRAM: snprintf(b, 32, "%s", settings.histogram ? "On" : "Off"); break;
        case MI_MIRROR: snprintf(b, 32, "%s", settings.mirror ? "On" : "Off"); break;
        case MI_FLIP: snprintf(b, 32, "%s", settings.flip ? "On" : "Off"); break;
        case MI_SYSTEM: {
            struct statvfs sv;
            if (sd_mounted() && statvfs(REC_DIR, &sv) == 0)
                snprintf(b, 32, "SD %.1fG free", (double)sv.f_bavail * sv.f_frsize / (1u << 30));
            else
                snprintf(b, 32, "No SD card");
            break;
        }
        case MI_SEC_REC:
            if (settings.mode == MODE_TIMELAPSE)
                snprintf(b, 32, "TL %s>%up", tl_interval_text(), play_fps());
            else if (settings.mode == MODE_SLOWMO)
                snprintf(b, 32, "%up>%u %s", sensor_fps_valid(), play_fps(), codec_short());
            else
                snprintf(b, 32, "%up %s%s", sensor_fps_valid(), codec_short(), crop_w ? " crop" : "");
            break;
        case MI_SEC_EXPO:
            if (settings.manual)
                snprintf(b, 32, "M %s %.1fx", shutter_text(manual_shutter_us()), settings.sensor_gain / 1024.0);
            else
                snprintf(b, 32, "Auto EV %+.1f", settings.ev_thirds / 3.0);
            break;
        case MI_SEC_COLOUR:
            if (settings.wb_mode == 1)
                snprintf(b, 32, "WB %uK", settings.wb_kelvin);
            else
                snprintf(b, 32, "WB %s, look %u", settings.wb_mode == 2 ? "manual" : "auto", settings.look_slot);
            break;
        case MI_SEC_ASSIST:
            snprintf(b, 32, "%s%s%s%s%s", settings.peaking ? "Peak " : "", settings.zebra ? "Zebra " : "",
                     settings.false_colour ? "FC " : "", settings.waveform ? "Wave " : "", settings.histogram ? "Hist" : "");
            if (!b[0])
                snprintf(b, 32, "all off");
            break;
        case XI_BACK: snprintf(b, 32, "Main menu"); break;
        case SI_SD: {
            struct statvfs st;
            if (sd_mounted() && statvfs(REC_DIR, &st) == 0)
                snprintf(b, 32, "%.1f / %.1f GB free", (double)st.f_bavail * st.f_frsize / (1u << 30),
                         (double)st.f_blocks * st.f_frsize / (1u << 30));
            else
                snprintf(b, 32, "No card");
            break;
        }
        case SI_FORMAT:
            if (sd_fmt_state != FMT_IDLE)
                snprintf(b, 32, "%s", sd_fmt_msg);
            else
                snprintf(b, 32, "%s", menu_confirm == SI_FORMAT ? "OK again = ERASE" : rec_on ? "Stop rec first" : "Press OK");
            break;
        case SI_BUILD: snprintf(b, 32, "%s", __DATE__ " " __TIME__); break;
        case SI_CPU: {                                  /* busy % of both cores since the last redraw (~1 s) */
            static unsigned long long pbusy, ptotal;
            unsigned long long v[8] = { 0 }, busy, total = 0;
            FILE *f = fopen("/proc/stat", "r");
            if (f) {
                if (fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
                           &v[6], &v[7]) != 8)
                    memset(v, 0, sizeof(v));
                fclose(f);
            }
            for (int k = 0; k < 8; k++)
                total += v[k];
            busy = total - v[3] - v[4];                 /* minus idle and iowait */
            if (total > ptotal && ptotal)
                snprintf(b, 32, "%llu%% busy (2 cores)", (busy - pbusy) * 100 / (total - ptotal));
            else
                snprintf(b, 32, "measuring");
            pbusy = busy, ptotal = total;
            break;
        }
        case SI_RAM: {
            long tot = 0, avail = 0, rss = 0, val;
            char key[40];
            FILE *f = fopen("/proc/meminfo", "r");
            while (f && fscanf(f, "%39s %ld kB ", key, &val) == 2) {
                if (!strcmp(key, "MemTotal:")) tot = val;
                if (!strcmp(key, "MemAvailable:")) avail = val;
            }
            if (f) fclose(f);
            f = fopen("/proc/self/status", "r");
            while (f && fscanf(f, "%39s", key) == 1)
                if (!strcmp(key, "VmRSS:") && fscanf(f, "%ld", &rss) == 1)
                    break;
            if (f) fclose(f);
            snprintf(b, 32, "%ld/%ld MB free, app %ld", avail / 1024, tot / 1024, rss / 1024);
            break;
        }
        case SI_FLASH: {
            struct statvfs sv;
            if (statvfs("/customer", &sv) == 0)
                snprintf(b, 32, "%.1f MB free", (double)sv.f_bavail * sv.f_frsize / (1024 * 1024));
            break;
        }
        case SI_UPTIME: {
            double up = 0;
            FILE *f = fopen("/proc/uptime", "r");
            if (f) { if (fscanf(f, "%lf", &up) != 1) up = 0; fclose(f); }
            snprintf(b, 32, "%dh %02dm", (int)up / 3600, (int)up / 60 % 60);
            break;
        }
        case SI_LOGS: snprintf(b, 32, "Press OK"); break;
        case SI_TC:
            if (menu_confirm == SI_TC)
                snprintf(b, 32, "OK again = 00:00:00:00");
            else
                snprintf(b, 32, "%s", tc_text(rec_tc_ms(), file_fps()));
            break;
        case SI_RESET: snprintf(b, 32, "%s", menu_confirm == SI_RESET ? "OK again = RESET" : "Press OK"); break;
        case SI_BACK: snprintf(b, 32, "Main menu"); break;
        case MI_EXPERT: snprintf(b, 32, "Sensor, regs, encoder"); break;
        case EI_NR3D:
            if (settings.nr3d_gain == 0) snprintf(b, 32, "Off");
            else snprintf(b, 32, settings.nr3d_gain == 1000 ? "%lu (default)" : "%lu", (unsigned long)settings.nr3d_gain);
            break;
        case EI_WDR:
            if (settings.wdr < 0) snprintf(b, 32, "Auto");
            else if (settings.wdr == 0) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%ld", (long)settings.wdr);
            break;
        case EI_FPS:                                /* HDR: 2 sensor frames (exposures) per output frame */
            if (hdr_on)
                snprintf(b, 32, "%.2f (sensor %.2f)", regs_ok ? regs.fps / 2 : 0, regs_ok ? regs.fps : 0);
            else
                snprintf(b, 32, "%.2f", regs_ok ? regs.fps : 0);
            break;
        case EI_VMAX: snprintf(b, 32, regs_ok ? "%ld lines" : "-", regs.vmax); break;
        case EI_HMAX: snprintf(b, 32, regs_ok ? "%ld (%.2f us)" : "-", regs.hmax, regs.line_us); break;
        case EI_SHS: snprintf(b, 32, regs_ok ? "%ld ln %.0f us" : "-", regs.vmax - regs.shs1 - 1, regs.exposure_us); break;
        case EI_GAIN: snprintf(b, 32, regs.gain >= 0 ? "0x%02lx %.1f dB" : "-", regs.gain, regs.gain * 0.3); break;
        case EI_TEMP: {
            int c = soc_temp_c();
            snprintf(b, 32, c >= 0 ? "%d C" : "-", c);
            break;
        }
        case EI_BACK: snprintf(b, 32, "Main menu"); break;
        case EI_REGS: snprintf(b, 32, "View"); break;
        case EI_ISP: snprintf(b, 32, "Adjust"); break;
        case EI_ENC: snprintf(b, 32, "Open"); break;
        case GI_JPEGQ: snprintf(b, 32, "%u%s", settings.jpeg_quality, settings.jpeg_quality == 70 ? " (default)" : ""); break;
        case GI_BACK: snprintf(b, 32, "Expert"); break;
        case GI_GOP:
            if (!settings.h265_gop)
                snprintf(b, 32, "Auto (1 s = %u fr)", sensor_fps_valid());
            else
                snprintf(b, 32, "%u fr = %.2fs", settings.h265_gop, settings.h265_gop / (double)sensor_fps_valid());
            break;
        case GI_RC: snprintf(b, 32, "%s", settings.h265_rc == 1 ? "VBR (bitrate = max)" : "CBR"); break;
        case GI_QPMAX: snprintf(b, 32, "%u%s", settings.h265_qp_max, settings.h265_qp_max == 48 ? " (default)" : ""); break;
        case GI_QPMIN: snprintf(b, 32, "%u%s", settings.h265_qp_min, settings.h265_qp_min == 12 ? " (default)" : ""); break;
        case II_BRIGHT: case II_CONTRAST: case II_LIGHT: case II_SAT: {
            int32_t v = id == II_SAT ? settings.isp_saturation : *iq_simple[id - II_BRIGHT].value;
            if (v < 0) snprintf(b, 32, "Auto");
            else snprintf(b, 32, "%ld", (long)v);
            break;
        }
        case II_BLACK:
            if (settings.isp_black == 0) snprintf(b, 32, "Default");
            else snprintf(b, 32, "%+ld", (long)settings.isp_black / 16);
            break;
        case II_BW: snprintf(b, 32, "%s", settings.isp_bw ? "On" : "Off"); break;
        case II_MATRIX: snprintf(b, 32, "%s", settings.ccm_custom ? "Custom" : "Edit"); break;
        case CI_MODE: snprintf(b, 32, "%s", settings.ccm_custom ? "Custom" : "Vendor (auto)"); break;
        case CI_COPY: snprintf(b, 32, "Press OK"); break;
        case CI_M0: case CI_M1: case CI_M2: case CI_M3: case CI_M4: case CI_M5: case CI_M6: case CI_M7: case CI_M8: {
            static int32_t live[9];                 /* vendor mode: what the ISP uses now, read-only */
            if (!settings.ccm_custom && id == CI_M0 && ccm_current(live) < 0)
                memset(live, 0, sizeof(live));
            int i = id - CI_M0, v = settings.ccm_custom ? settings.ccm_m[i] : live[i], row = i / 3;
            snprintf(b, 32, "%s%d.%03d%s", v < 0 ? "-" : "", abs(v) / 1024, abs(v) % 1024 * 1000 / 1024,
                     !settings.ccm_custom ? " vendor" : i == row * 4 ? " (auto)" : "");
            break;
        }
        case CI_BACK: snprintf(b, 32, "Image"); break;
        case HI_BACK: snprintf(b, 32, "Image"); break;
        case II_NOISE: snprintf(b, 32, "Edit"); break;
        case NI_BACK: snprintf(b, 32, "Image"); break;
        case NI_LUMA:
            if (settings.nr_luma < 0) snprintf(b, 32, "Auto");
            else if (settings.nr_luma == 0) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%ld%%", (long)settings.nr_luma);
            break;
        case NI_CHROMA: case NI_DESPIKE: case NI_FALSE: {
            int32_t v = id == NI_CHROMA ? settings.nr_chroma : id == NI_DESPIKE ? settings.nr_despike : settings.nr_falsecolor;
            snprintf(b, 32, "%s", v < 0 ? "Auto" : v ? "On" : "Off");
            break;
        }
        case NI_DEFOG:
            if (settings.defog <= 0) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%ld", (long)settings.defog);
            break;
        case II_RESET: case CI_RESET: case HI_RESET: case NI_RESET:
            snprintf(b, 32, "%s", menu_confirm == id ? "OK again = RESET" : "Press OK");
            break;
        case II_LOOK_SLOT: snprintf(b, 32, "%u (%s)", settings.look_slot, look_exists(settings.look_slot) ? "saved" : "empty"); break;
        case II_LOOK_LOAD: snprintf(b, 32, "%s", look_exists(settings.look_slot) ? "Press OK" : "slot empty"); break;
        case II_LOOK_SAVE: snprintf(b, 32, "%s", menu_confirm == id ? "OK again = SAVE" : "Press OK"); break;
        case II_LOOK_EXPORT: snprintf(b, 32, "Press OK"); break;
        case II_LOOK_IMPORT: snprintf(b, 32, "%s", menu_confirm == id ? "OK again = IMPORT" : "Press OK"); break;
        case II_HSV: snprintf(b, 32, "Edit"); break;
        case HI_RH: case HI_YH: case HI_GH: case HI_CH: case HI_BH: case HI_MH: {
            int32_t v = settings.hsv_hue[(id - HI_RH) / 2];
            snprintf(b, 32, v ? "%+ld" : "0", (long)v);
            break;
        }
        case HI_RS: case HI_YS: case HI_GS: case HI_CS: case HI_BS: case HI_MS:
            snprintf(b, 32, "%ld%%", (long)settings.hsv_sat[(id - HI_RS) / 2]);
            break;
        case II_CCM:
            if (settings.ccm_custom) snprintf(b, 32, "(custom)");
            else if (settings.isp_ccm < 0) snprintf(b, 32, "Auto");
            else snprintf(b, 32, "%ld%%", (long)settings.isp_ccm);
            break;
        case II_SHARP:
            if (settings.isp_sharpness < 0) snprintf(b, 32, "Auto");
            else if (settings.isp_sharpness == 0) snprintf(b, 32, "Off");
            else snprintf(b, 32, "%ld%%", (long)settings.isp_sharpness);
            break;
        case II_GAMMA:
            if (settings.isp_gamma < 0) snprintf(b, 32, "Auto");
            else if (settings.isp_gamma == GAMMA_LOG) snprintf(b, 32, "Log (exp.)");
            else snprintf(b, 32, "%ld.%ld", (long)settings.isp_gamma / 10, (long)settings.isp_gamma % 10);
            break;
        case II_BACK: snprintf(b, 32, "Colour"); break;
        case RI_ADDR: snprintf(b, 32, "0x%04X-0x%04X", reg_base, reg_base + 7); break;
        case RI_V0: case RI_V1: case RI_V2: case RI_V3: case RI_V4: case RI_V5: case RI_V6: case RI_V7: {
            int k = id - RI_V0;
            snprintf(reg_label[k], sizeof(reg_label[k]), "%04X %s", reg_base + k, sensor_reg_name(reg_base + k));
            if (reg_ok) {
                char bits[9];
                for (int i = 0; i < 8; i++)
                    bits[i] = reg_val[k] & (0x80 >> i) ? '1' : '0';
                bits[8] = 0;
                snprintf(b, 32, "0x%02X %3u %s", reg_val[k], reg_val[k], bits);
            } else {
                snprintf(b, 32, "read error");
            }
            break;
        }
        case RI_ACCESS:
            if (menu_confirm == RI_ACCESS)
                snprintf(b, 32, "OK again = ALL");
            else
                snprintf(b, 32, "%s", reg_write_all ? "All (!)" : "Safe");
            break;
        case RI_UNDO: snprintf(b, 32, reg_n_undo ? "%d reg%s" : "nothing", reg_n_undo, reg_n_undo == 1 ? "" : "s"); break;
        case RI_BACK: snprintf(b, 32, "Expert"); break;
        }
        labels[r] = id >= RI_V0 && id <= RI_V7 ? reg_label[id - RI_V0] : menu_labels_all[id];
        values[r] = b;
    }
    static const char *const titles[N_PAGES] = { "MENU", "MENU > SYSTEM", "MENU > EXPERT", "MENU > EXPERT > REGISTERS",
                                                 "MENU > COLOUR > IMAGE", "COLOUR > IMAGE > MATRIX",
                                                 "COLOUR > IMAGE > HUE", "COLOUR > IMAGE > NOISE", "MENU > EXPERT > ENCODER",
                                                 "MENU > RECORDING", "MENU > EXPOSURE", "MENU > COLOUR", "MENU > ASSIST" };
    int sel_id = menu_nrows ? menu_rows[menu_sel] : -1;
    const char *help = NULL;
    if (sel_id >= RI_V0 && sel_id <= RI_V7) {
        static char hbuf[200];
        uint16_t reg = reg_base + sel_id - RI_V0;
        snprintf(hbuf, sizeof(hbuf), "%s %s", reg_write_all || sensor_reg_safe(reg) ? "[left/right edits]" : "[read-only]",
                 sensor_reg_desc(reg));
        help = hbuf;
    } else if (sel_id == RI_ACCESS)
        help = "Safe: only gain, black level, test pattern and hold can be changed. All: any register. "
               "Nothing is permanent: a restart restores the sensor. Worst case the picture stops.";
    else if (sel_id == RI_UNDO)
        help = "OK: put every register changed since start back to its original value, newest first.";
    else if (sel_id == II_LOOK_SLOT)
        help = "Look = all image settings (this page, matrix, hue, noise, WDR, temporal NR). Pick a slot 1-5 to load or save.";
    else if (sel_id == II_LOOK_LOAD)
        help = "OK: load the look in this slot (replaces the current image settings).";
    else if (sel_id == II_LOOK_SAVE)
        help = "OK twice: save the current image settings into this slot (overwrites it).";
    else if (sel_id == II_LOOK_EXPORT)
        help = "OK: copy all saved slots to the SD card as LOOKS/LOOK1.TXT .. LOOK5.TXT (text, edit on a PC; keys in docs/looks.md).";
    else if (sel_id == II_LOOK_IMPORT)
        help = "OK twice: read LOOKS/LOOKn.TXT from the SD card into the slots (overwrites them). Only image settings are taken.";
    else if (sel_id == II_BRIGHT)
        help = "ISP brightness, 0-100: shifts the whole tone curve up or down. Play = Auto (vendor tuning).";
    else if (sel_id == II_CONTRAST)
        help = "ISP contrast, 0-100: steepness of the tone curve around mid-grey. Play = Auto (vendor, lower in low light).";
    else if (sel_id == II_GAMMA)
        help = "ISP tone curve (RGB gamma LUT). Log (experimental): -2 EV in auto, flat, 18% grey at ~41%, most highlight range (grade in post). 1.0 = linear, 2.2 = video, higher = flatter and brighter. Play = Auto.";
    else if (sel_id == II_RESET)
        help = "OK twice: every row on this page back to Auto / Default (Matrix edit and Hue tweaks keep their own reset).";
    else if (sel_id == CI_RESET)
        help = "OK twice: back to the vendor matrix, custom numbers cleared to identity.";
    else if (sel_id == HI_RESET)
        help = "OK twice: all hue shifts 0, all saturations 100%.";
    else if (sel_id == MI_REC_CODEC)
        help = "H.265 (.ts): smallest files, clean 75p, plays to the last frame after a power cut. H.264 (.ts): same but opens in every editor/player, a little bigger. MJPEG (.avi): every frame a JPEG, big files.";
    else if (sel_id == MI_REC_MBPS)
        help = "H.265 bitrate. 40 Mbps is a good 1080p60 quality; 80-100 needs a fast card. Remux for editors: ffmpeg -i REC.ts -c copy -tag:v hvc1 REC.mov";
    else if (sel_id == MI_PEAKING)
        help = "Focus peaking: marks sharp edges in colour. 1 = only the sharpest, 10 = many. Remote flip button = on/off. Works inside the loupe.";
    else if (sel_id == MI_ZEBRA)
        help = "Zebra stripes where brightness is at or above this level (100% = clipped white). 95% warns before clipping; 70% is a common skin/face check.";
    else if (sel_id == GI_GOP)
        help = "H.265 keyframe interval in captured frames (next clip). Auto = one second. Short GOP = easier scrubbing/editing and more robust, bigger files; long GOP = smaller files. Zoom -/+: 1 frame. Play = Auto.";
    else if (sel_id == GI_RC)
        help = "CBR: constant bitrate (Codec page bitrate), steady file size. VBR: the bitrate is a ceiling; simple scenes use far less, quality held by the QP range. Next clip.";
    else if (sel_id == GI_QPMAX || sel_id == GI_QPMIN)
        help = "VBR quantiser range 0-51 (lower = finer). Max QP caps the worst quality, Min QP the best (stops wasting bits on easy frames). Defaults 48 / 12. Next clip.";
    else if (sel_id == GI_JPEGQ)
        help = "JPEG quality 1-100 for MJPEG clips, photos and the USB webcam (one encoder). 70 = default (~70 KB/frame here), 95 = ~3x bigger. Above ~85 at 60p MJPEG needs a fast card. Play = 70.";
    else if (sel_id == SI_TC)
        help = "Record-run timecode: runs only while recording and continues across clips and power-offs (at each clip's file rate); written to the clip's .txt sidecar. OK twice (remote: play twice) = back to 00:00:00:00.";
    else if (sel_id == MI_TL_LIMIT)
        help = "Timelapse: stop recording by itself after this many frames (the clip length at the play rate is shown). Off = until you press record again. Remote zoom -/+: 1 frame.";
    else if (sel_id == MI_TL_INTERVAL)
        help = "Timelapse: one frame is kept every interval (0.1 s - 60 min). Arrows: 0.5, 1, 2, 5 s ... 60 min; remote zoom -/+: fine steps. Records MJPEG for now.";
    else if (sel_id == MI_PLAY_FPS && settings.mode == MODE_TIMELAPSE)
        help = "Timelapse: the clip is stamped to play at this rate (5-60). 1 s interval at 25p = 25x faster.";
    else if (sel_id == MI_PLAY_FPS)
        help = "Slow-mo: every captured frame is kept and the clip is stamped to play at this rate (5 up to one below the capture rate), so it plays slower in any player or editor (75 / 25 = 3x). Use H.265: MJPEG drops frames above 60p.";
    else if (sel_id == MI_CROP_W || sel_id == MI_CROP_H)
        help = "Sensor window, centred. Narrower = faster: 1280 wide runs up to 105p (full width 75p). Height changes only the framing, not the speed. Above full width: Open gate (experimental) = the whole 1948x1097 sensor readout (1944x1096 used), ~1.3% more picture, scaled to 1920x1080, 75p max. Arrows 32 / 18 px, remote zoom 8 / 2 px. OK/Play applies both (restart); leaving the two rows without it cancels.";
    else if (sel_id == MI_MODE)
        help = "Left/right pick, OK (remote: play) switches and restarts. Each mode keeps its own frame rate, exposure and codec. HDR: 30p max. Slow-mo: capture 6-75p, plays at any lower rate (min 5). Timelapse: interval + play rate.";
    else if (sel_id == MI_FALSECOL)
        help = "False colour: purple crushed, blue near black, green 38-42% (mid grey), pink 52-56% (skin), yellow 97-99%, red clipped.";
    else if (sel_id == MI_FRAME)
        help = "Frame lines for a target aspect ratio (the picture is 16:9): 2.39 scope, 1.85 flat, 4:3, 1:1, 9:16 vertical. Play = Off.";
    else if (sel_id == MI_WAVEFORM)
        help = "Waveform: left-right = picture position, bottom-top = level 0-100% (lines at 25/50/75), top edge = clipping. Luma = brightness; RGB = the three channels overlaid; Parade = R, G, B side by side.";
    else if (sel_id == MI_LOUPE)
        help = "Focus loupe (display only, recording keeps the full frame). Remote: mirror = on/off, zoom +/- = zoom, arrows = move.";
    else if (sel_id == MI_AE_SPEED)
        help = "How fast auto exposure follows light changes. Instant = fastest without overshoot (settles in ~0.3 s). Below: slower, smoother glides. Above 100%: overshoots and hunts.";
    else if (sel_id == MI_AE_MAXSH)
        help = "Longest shutter auto exposure may use. Auto = one full frame (1/fps), the longest the sensor can do without dropping the frame rate.";
    else if (sel_id == MI_AE_FLICKER)
        help = "Keeps auto shutter times in steps of the mains flicker so lamps don't band. Use your mains frequency (50 or 60 Hz).";
    else if (sel_id == II_NOISE)
        help = "Spatial noise reduction and clean-up blocks of the ISP (temporal NR is on the Sensor page).";
    else if (sel_id == NI_LUMA)
        help = "Spatial luma noise reduction: % of the vendor thresholds. Higher = smoother, softer. Off = raw grain. Play = Auto.";
    else if (sel_id == NI_CHROMA)
        help = "Colour-noise reduction (removes coloured blotches in shadows). Auto = vendor (on).";
    else if (sel_id == NI_DESPIKE)
        help = "Removes single bright or dark pixels (impulse noise, hot pixels). Auto = vendor (on).";
    else if (sel_id == NI_FALSE)
        help = "Suppresses coloured moire / false colour on fine detail. Auto = vendor (on).";
    else if (sel_id == MI_SHUTTER_BY)
        help = "Speed: shutter as 1/x seconds. Angle: shutter as a share of the frame (180 = half), kept when the frame rate changes.";
    else if (sel_id == MI_SHUTTER && settings.shutter_angle_x10)
        help = "Shutter angle. Arrows: 1 degree. Remote zoom -/+: 0.1 degree (172.8). 360 = whole frame. Play = 180.";
    else if (sel_id == MI_SHUTTER)
        help = "1/x seconds. Arrows: camera scale (1/50, 1/60, 1/125 ...). Remote zoom -/+: fine (1/60 > 1/61). Play = 180 degrees.";
    else if (sel_id == NI_DEFOG)
        help = "Haze removal: local contrast boost for foggy or low-contrast scenes. Off = vendor.";
    else if (sel_id == NI_RESET)
        help = "OK twice: all rows on this page back to Auto / Off.";
    else if (sel_id == II_HSV)
        help = "Per-colour hue and saturation (ISP HSV): change one colour range without touching the others.";
    else if (sel_id >= HI_RH && sel_id < HI_BACK && (sel_id - HI_RH) % 2 == 0)
        help = "Hue shift for this colour band (ISP units, + and - turn it around the colour wheel). Neighbouring bands blend smoothly. Play = 0.";
    else if (sel_id >= HI_RS && sel_id < HI_BACK && (sel_id - HI_RS) % 2 == 0)
        help = "Saturation of this colour band only, 100% = vendor. 0% greys it out. Play = 100%.";
    else if (sel_id == II_MATRIX)
        help = "Edit the 9 colour matrix numbers yourself (ISP manual matrix).";
    else if (sel_id == SI_CPU || sel_id == SI_RAM || sel_id == SI_FLASH)
        help = "Live system status. CPU: both cores busy over the last second. RAM: Linux memory available of total, and what camapp itself uses (video buffers live in a separate ~170 MB area). Flash: free space where camapp and settings live.";
    else if (sel_id == SI_LOGS)
        help = "OK: copy the camapp log (recording drops, errors, settings) and the kernel log (dmesg) to LOGS/ on the SD card, numbered LOG_nnnn_camapp.txt / _dmesg.txt.";
    else if (sel_id == CI_MODE)
        help = "Vendor: calibrated matrices blended by light colour. Custom: your 9 numbers, fixed for every light.";
    else if (sel_id == CI_COPY)
        help = "OK: take the vendor matrix for the current light as your Custom matrix (switches to Custom).";
    else if (sel_id >= CI_M0 && sel_id <= CI_M8)
        help = "Output colour = inputs x numbers, 1.000 = 100%. Minus pulls that colour out. Each row always sums to 1.0 (the ISP enforces it), so the diagonal follows. Vendor mode shows the live vendor matrix, read-only; set Mode to Custom to edit (starts from it). Custom default is identity = no correction (raw, dull colour). Play = back to the vendor matrix.";
    else if (sel_id == II_CCM)
        help = "Colour matrix strength: 0% = sensor-native colour (muted), 100% = vendor calibration, more = stronger colour separation. Play = Auto.";
    else if (sel_id == II_SHARP)
        help = "ISP edge sharpening, % of the vendor edge gain. Off = no sharpening (softest, filmic). Over 100% = crisper, more halos. Play = Auto.";
    else if (sel_id == II_LIGHT)
        help = "ISP lightness, 0-100: brightens or darkens midtones while keeping black and white. Play = Auto (vendor: off).";
    else if (sel_id == II_SAT)
        help = "ISP colour saturation, 0-100. Play = Auto: vendor, which fades colour as gain rises (42 down to 20).";
    else if (sel_id == II_BLACK)
        help = "ISP black level (OBC), steps of 1 sensor code. + crushes shadows, - lifts them (milky). Play = Default (vendor).";
    else if (sel_id == II_BW)
        help = "ISP colour-to-gray: true black and white from the ISP's luma.";
    else if (sel_id == RI_ADDR)
        help = "Left/right: move 8 registers. Values refresh every second. Full guide: docs/imx307_registers.md";
    /* Show at most MENU_VISIBLE rows; the window follows the cursor, arrows mark hidden rows. */
    static int menu_top;
    if (menu_sel < menu_top)
        menu_top = menu_sel;
    if (menu_sel >= menu_top + MENU_VISIBLE)
        menu_top = menu_sel - MENU_VISIBLE + 1;
    if (menu_top > menu_nrows - MENU_VISIBLE)
        menu_top = menu_nrows > MENU_VISIBLE ? menu_nrows - MENU_VISIBLE : 0;
    int shown = menu_nrows - menu_top < MENU_VISIBLE ? menu_nrows - menu_top : MENU_VISIBLE;
    if (!help && sel_id == MI_SEC_REC)
        help = "Frame rate, sensor crop, timelapse interval, playback rate, codec and bitrate.";
    else if (sel_id == MI_SPOT_SIZE)
        help = "Spot metering area: 2 (12% of the width) to 8 (half the frame) in steps of 2, always square on screen and centred where it was. Arrows outside the menu move it. Play = 2 (default).";
    else if (sel_id == MI_PHOTO_RAW)
        help = "JPEG + DNG: every photo also saves IMG_nnnn.DNG, the sensor's 10-bit raw data before any processing (4 MB), for Lightroom, Resolve, darktable. Not in HDR mode.";
    else if (!help && sel_id == MI_EXPOSURE)
        help = "P: auto shutter and gain. S: you set the shutter (angle), auto exposure moves only the gain. G: you set the gain, auto exposure moves only the shutter. M: both manual. EV comp works in P, S and G. No aperture mode: the lens has no iris.";
    else if (!help && sel_id == MI_SEC_EXPO)
        help = "Auto or manual exposure: shutter, gain, EV compensation, metering and the auto-exposure limits.";
    else if (!help && sel_id == MI_SEC_COLOUR)
        help = "White balance, looks, and the full image pages: tone, matrix, hue, noise.";
    else if (!help && sel_id == MI_SEC_ASSIST)
        help = "Focus and exposure aids: magnify, peaking, zebra, false colour, waveform, histogram, guides, frame lines.";
    else if (!help && sel_id == MI_EXPERT)
        help = "Sensor timing and noise reduction, raw sensor registers, encoder settings.";
    else if (!help && sel_id == MI_SYSTEM)
        help = "Mirror/flip, SD card and format, timecode, build, reset settings.";
    osd_menu(titles[menu_page], labels + menu_top, values + menu_top, shown, menu_sel - menu_top, help,
             menu_top > 0, menu_top + shown < menu_nrows, page_accent[menu_page]);
}

static void apply_all_settings(void)
{
    apply_orientation();
    install_weight_tables();
    apply_metering();
    apply_ev_comp();
    apply_exposure();
    osd_overlay(settings.overlay);
    if (!settings.histogram)
        osd_histogram(NULL);
    apply_peaking();
    show_spot_area();
}

/* "reset" action (body OK held, remote play/pause): selected number row back to Auto/Default. */
/* New spot size (1..8), keeping the spot centred where it was. */
void spot_resize(int size)
{
    /* The centre (in half blocks) is kept from the first resize on, so odd sizes (which cannot sit on
     * it exactly) do not make the spot drift; moving the spot with the arrows sets a new centre. */
    static int want_cx2 = -1, want_cy2, last_x = -1, last_y, last_w;
    size = size < 2 ? 2 : size > 8 ? 8 : size & ~1;  /* even widths only (see SPOT_W) */
    if ((int)settings.spot_x != last_x || (int)settings.spot_y != last_y || SPOT_W != last_w || want_cx2 < 0)
        want_cx2 = 2 * (int)settings.spot_x + SPOT_W, want_cy2 = 2 * (int)settings.spot_y + SPOT_H;
    int cx2 = want_cx2, cy2 = want_cy2;
    settings.spot_size = size;
    int x = (cx2 - SPOT_W) / 2, y = (cy2 - SPOT_H) / 2;
    settings.spot_x = x < 0 ? 0 : x > WGT_GRID - SPOT_W ? WGT_GRID - SPOT_W : x;
    settings.spot_y = y < 0 ? 0 : y > WGT_GRID - SPOT_H ? WGT_GRID - SPOT_H : y;
    last_x = settings.spot_x, last_y = settings.spot_y, last_w = SPOT_W;
    install_weight_tables();
    show_spot_area();
}

static void menu_reset(void)
{
    int id = menu_rows[menu_sel];
    int reset = 1;
    switch (id) {
    case II_BRIGHT: case II_CONTRAST: case II_LIGHT:
        *iq_simple[id - II_BRIGHT].value = -1;
        apply_iq_simple(id - II_BRIGHT);
        break;
    case II_SAT: settings.isp_saturation = -1; apply_saturation(); break;
    case II_BLACK: settings.isp_black = 0; apply_black(); break;
    case II_GAMMA: settings.isp_gamma = -1; apply_gamma(); break;
    case II_SHARP: settings.isp_sharpness = -1; apply_sharpness(); break;
    case NI_LUMA: settings.nr_luma = -1; apply_noise(); break;
    case NI_CHROMA: settings.nr_chroma = -1; apply_noise(); break;
    case NI_DESPIKE: settings.nr_despike = -1; apply_noise(); break;
    case NI_FALSE: settings.nr_falsecolor = -1; apply_noise(); break;
    case NI_DEFOG: settings.defog = 0; apply_noise(); break;
    case II_CCM: settings.isp_ccm = -1; apply_ccm(); break;
    case HI_RH: case HI_YH: case HI_GH: case HI_CH: case HI_BH: case HI_MH:
        settings.hsv_hue[(id - HI_RH) / 2] = 0; apply_hsv(); break;
    case HI_RS: case HI_YS: case HI_GS: case HI_CS: case HI_BS: case HI_MS:
        settings.hsv_sat[(id - HI_RS) / 2] = 100; apply_hsv(); break;
    case MI_WB: case MI_WB_K: case MI_WB_R: case MI_WB_G: case MI_WB_B:
        settings.wb_mode = 0; apply_wb(); break;
    case MI_AE_MAXSH: settings.ae_max_shutter_us = 0; apply_exposure(); break;
    case MI_SHUTTER:                                /* 180 degrees, or its speed (1/(2 x fps)) */
        if (settings.shutter_angle_x10)
            settings.shutter_angle_x10 = 1800;
        else
            settings.shutter_us = 1000000 / (2 * sensor_fps_valid());
        apply_exposure();
        break;
    case MI_LOUPE: loupe_on = 0; apply_loupe(); return;
    case MI_PEAKING: settings.peaking = 0; apply_peaking(); break;
    case MI_ZEBRA: settings.zebra = 0; apply_peaking(); break;
    case MI_FRAME: settings.overlay &= ~OSD_FRAME_MASK; osd_overlay(settings.overlay); break;
    case MI_FALSECOL: settings.false_colour = 0; apply_peaking(); break;
    case MI_SPOT_SIZE: spot_resize(2); break;
    case MI_AE_SPEED: settings.ae_speed = 0; apply_ae_speed(); break;
    case MI_AE_FLICKER: settings.ae_flicker = 0; apply_ae_flicker(); break;
    case MI_AE_MAXGAIN: settings.ae_max_gain = 0; apply_exposure(); break;
    case CI_M0: case CI_M1: case CI_M2: case CI_M3: case CI_M4: case CI_M5: case CI_M6: case CI_M7: case CI_M8:
        if (settings.ccm_custom) {                  /* play: custom back to the vendor matrix of this light */
            settings.ccm_custom = 0;
            apply_ccm();
            ccm_copy_current();
            settings.ccm_custom = 1;
            apply_ccm();
        }
        break;
    case EI_WDR: settings.wdr = 0; apply_wdr(); break;
    case GI_JPEGQ: settings.jpeg_quality = 70; apply_jpeg_quality(); break;
    case GI_GOP: settings.h265_gop = 0; break;
    case GI_RC: settings.h265_rc = 0; break;
    case GI_QPMAX: settings.h265_qp_max = 48; break;
    case GI_QPMIN: settings.h265_qp_min = 12; break;
    case EI_NR3D: settings.nr3d_gain = 1000; apply_nr3d(); break;
    default: reset = 0;
    }
    if (reset)
        settings_changed();
}

/* Manual shutter one step: right/up = bigger number (wider angle / faster speed). Arrows use the
 * camera scale (or 1 degree), fine = the remote zoom buttons (1/60 > 1/61, or 0.1 degree). Used by the
 * menu row and by the live-view buttons in manual exposure. */
void shutter_step(int dir, int fine)
{
    if (settings.shutter_angle_x10 && fine) {   /* fine: 0.1 degree */
        int a = (int)settings.shutter_angle_x10 + dir;
        settings.shutter_angle_x10 = a < 1 ? 1 : a > 3600 ? 3600 : a;
    } else if (settings.shutter_angle_x10) {    /* 1 degree steps; a fractional angle snaps to whole */
        int a = (int)settings.shutter_angle_x10 / 10 + (dir > 0 || settings.shutter_angle_x10 % 10 == 0 ? dir : 0);
        settings.shutter_angle_x10 = (a < 1 ? 1 : a > 360 ? 360 : a) * 10;
    } else if (settings.mode == MODE_TIMELAPSE && (settings.shutter_us > 41667 || (settings.shutter_us >= 41000 && dir < 0))) {
        /* timelapse long shutter beyond 1/24: stops to seconds; fine = 0.1 s. Shorter than 1/24: normal scale */
        static const unsigned long_us[] = { 41667, 66667, 125000, 250000, 500000, 1000000, 1500000, 2000000,
                                            2147000 };   /* cap: max_manual_us() */
        const int n = sizeof(long_us) / sizeof(long_us[0]);
        long us = settings.shutter_us;
        if (fine) {
            us -= dir * 100000L;                    /* right = shorter */
        } else if (dir < 0) {                       /* longer */
            int i = 0;
            while (i < n - 1 && long_us[i] <= (unsigned)us)
                i++;
            us = long_us[i];
        } else {
            int i = n - 1;
            while (i > 0 && long_us[i] >= (unsigned)us)
                i--;
            us = long_us[i];
        }
        uint32_t cap = max_manual_us();
        settings.shutter_us = us < 41667 ? 41667 : (uint32_t)us > cap ? cap : (uint32_t)us;
    } else if (!fine) {                    /* arrows: camera scale, next value above / below */
        unsigned x = (1000000 + settings.shutter_us / 2) / settings.shutter_us;
        int i = dir > 0 ? 0 : N_SPEED - 1;
        if (dir > 0)
            while (i < N_SPEED - 1 && speed_scale[i] <= x)
                i++;
        else
            while (i > 0 && speed_scale[i] >= x)
                i--;
        settings.shutter_us = (1000000 + speed_scale[i] / 2) / speed_scale[i];
    } else {                                    /* fine: 1/x with x in steps of 1, 10, 100, 1000 by decade */
        unsigned x = (1000000 + settings.shutter_us / 2) / settings.shutter_us;
        unsigned ref = dir > 0 ? x : x - 1;
        unsigned step = ref < 100 ? 1 : ref < 1000 ? 10 : ref < 10000 ? 100 : 1000;
        x = dir > 0 ? (x / step + 1) * step : ((x - 1) / step) * step;
        x = x < 24 ? 24 : x > 64000 ? 64000 : x;
        settings.shutter_us = (1000000 + x / 2) / x;
    }
    apply_exposure();
}

/* Manual sensor gain one step: 3 dB on the half-stop grid, fine = 0.3 dB (the sensor's own step). */
void gain_step(int dir, int fine)
{
    if (fine) {
        double db = 20 * log10(settings.sensor_gain / 1024.0);
        db = round(db / 0.3) * 0.3 + dir * 0.3;
        db = db < 0 ? 0 : db > 24 ? 24 : db;
        settings.sensor_gain = (uint32_t)(1024 * pow(10, db / 20) + 0.5);
    } else {                                    /* next grid point strictly above / below (grid values
                                                 * are half stops, not exact dB: use them as they are) */
        int g = dir > 0 ? 0 : N_GAIN - 1;
        if (dir > 0)
            while (g < N_GAIN - 1 && gain_steps[g].gain <= settings.sensor_gain + 16)
                g++;
        else
            while (g > 0 && gain_steps[g].gain + 16 >= settings.sensor_gain)
                g--;
        settings.sensor_gain = gain_steps[g].gain;
    }
    apply_exposure();
}

/* Set while a fine-step button (remote zoom+/zoom-) is handled; rows without a finer step ignore it. */
static int menu_fine;
/* Set while OK (body) or remote play confirms; the Mode row switches only on this, not on the arrows. */
static int menu_ok;

/* Copy /tmp/camapp.log and dmesg to REC_DIR/LOGS/LOG_nnnn_*.txt. Returns the number or -1. */
static int save_logs(void)
{
    char dir[64], cmd[256];
    if (!sd_mounted())
        return -1;
    snprintf(dir, sizeof(dir), "%s/LOGS", REC_DIR);
    mkdir(dir, 0755);
    int n = 1;
    for (; n < 10000; n++) {
        char p[96];
        snprintf(p, sizeof(p), "%s/LOG_%04d_camapp.txt", dir, n);
        if (access(p, F_OK) != 0)
            break;
    }
    fflush(stdout);
    snprintf(cmd, sizeof(cmd), "cp /tmp/camapp.log %s/LOG_%04d_camapp.txt && dmesg > %s/LOG_%04d_dmesg.txt && sync",
             dir, n, dir, n);
    int r = system(cmd);
    printf("logs: saved LOG_%04d, rc %d\n", n, r);
    return r == 0 ? n : -1;
}

/* Up one level, back on the row that opened this page. */
static void menu_back(void)
{
    menu_want_id = page_entry[menu_page];
    menu_page = page_parent[menu_page];
    menu_sel = 0;
}

static const char *codec_short(void)
{
    return settings.rec_codec == 1 ? "H.265" : settings.rec_codec == 2 ? "H.264" : "MJPEG";
}

static void menu_change(int dir)
{
    int id = menu_rows[menu_sel];
    if (id != menu_confirm)
        menu_confirm = -1;
    switch (id) {
    case MI_SYSTEM:
        menu_page = PAGE_SYSTEM;
        menu_sel = 0;
        return;
    case MI_EXPERT:
        menu_page = PAGE_EXPERT;
        menu_sel = 0;
        return;
    case MI_SEC_REC: case MI_SEC_EXPO: case MI_SEC_COLOUR: case MI_SEC_ASSIST:
        menu_page = id == MI_SEC_REC ? PAGE_REC : id == MI_SEC_EXPO ? PAGE_EXPO : id == MI_SEC_COLOUR ? PAGE_COLOUR : PAGE_ASSIST;
        menu_sel = 0;
        return;
    case SI_BACK: case EI_BACK: case XI_BACK: case II_BACK: case CI_BACK: case HI_BACK: case NI_BACK: case RI_BACK:
    case GI_BACK:
        menu_back();
        return;
    case EI_NR3D:
    {                                               /* Off < 125 .. 2000 (step 125) .. 16000 (step 500) */
        long g = settings.nr3d_gain, step = g > 2000 || (g == 2000 && dir > 0) ? 500 : 125;
        g = dir > 0 ? g / step * step + step : (g + step - 1) / step * step - step;
        settings.nr3d_gain = g < 0 ? 0 : g > 16000 ? 16000 : g;
        apply_nr3d();
    }
        break;
    case EI_WDR:                                    /* Auto < Off < 8 < 16 ... < 248 < 255 */
        if (dir > 0)
            settings.wdr = settings.wdr < 0 ? 0 : settings.wdr / 8 * 8 + 8;
        else
            settings.wdr = settings.wdr <= 0 ? -1 : (settings.wdr + 7) / 8 * 8 - 8;
        if (settings.wdr > 255)
            settings.wdr = 255;
        apply_wdr();
        break;
    case MI_CROP_W: case MI_CROP_H:                 /* arrows change the pending size, OK / play restarts */
        if (menu_ok) {
            if (crop_pending() && !rec_on) {
                printf("menu: crop %ux%u, restarting\n", crop_set_w(), crop_set_h());
                settings_changed();
                restart_requested = stop_requested = 1;
            }
            return;
        }
        if (id == MI_CROP_W) {                      /* ... 1888 < 1920 full < Open gate 1944 */
            int cur = (int)crop_set_w(), w = cur == OPEN_GATE_W ? (dir > 0 ? OPEN_GATE_W : 1920)
                                                 : cur == 1920 && dir > 0 ? OPEN_GATE_W : cur + dir * (menu_fine ? 8 : 32);
            settings.crop_h = w == OPEN_GATE_W ? OPEN_GATE_H : cur == OPEN_GATE_W ? 1080 : crop_set_h();
            if (w > 1920 && w != OPEN_GATE_W)
                w = 1920;
            settings.crop_w = w < 320 ? 320 : w;
        } else if (crop_set_w() != OPEN_GATE_W) {  /* open gate has a fixed height */
            int h = (int)crop_set_h() + dir * (menu_fine ? 2 : 18);
            settings.crop_h = h < 180 ? 180 : h > 1080 ? 1080 : h;
            settings.crop_w = crop_set_w();
        }
        return;
    case MI_MODE:                                   /* arrows pick, OK / remote play switches (restart) */
        if (menu_ok && mode_pick >= 0 && mode_pick != (int)settings.mode) {
            mode_switch(mode_pick);
            return;
        }
        mode_pick = ((mode_pick < 0 ? (int)settings.mode : mode_pick) + N_MODES + dir) % N_MODES;
        return;
    case EI_REGS:
        menu_page = PAGE_REGS;
        menu_sel = 0;
        return;
    case EI_ISP:
        menu_page = PAGE_ISP;
        menu_sel = 0;
        return;
    case EI_ENC:
        menu_page = PAGE_ENC;
        menu_sel = 0;
        return;
    case GI_GOP: {                                  /* Auto < 1 < 2 ... < 600 frames; zoom = 1 frame */
        static const unsigned steps[] = { 0, 1, 2, 5, 10, 12, 15, 24, 25, 30, 48, 50, 60, 75, 120, 150, 240, 300, 600 };
        const int n = sizeof(steps) / sizeof(steps[0]);
        long g = settings.h265_gop;
        if (rec_on)
            return;
        if (menu_fine) {
            g += dir;
        } else if (dir > 0) {
            int i = 0;
            while (i < n - 1 && steps[i] <= (unsigned)g)
                i++;
            g = steps[i];
        } else {
            int i = n - 1;
            while (i > 0 && steps[i] >= (unsigned)g)
                i--;
            g = steps[i];
        }
        settings.h265_gop = g < 0 ? 0 : g > 600 ? 600 : g;
        break;
    }
    case GI_RC:
        if (rec_on)
            return;
        settings.h265_rc ^= 1;
        break;
    case GI_QPMAX: case GI_QPMIN: {
        if (rec_on)
            return;
        uint32_t *q = id == GI_QPMAX ? &settings.h265_qp_max : &settings.h265_qp_min;
        int v = (int)*q + dir;
        *q = v < 0 ? 0 : v > 51 ? 51 : v;
        if (settings.h265_qp_min > settings.h265_qp_max)   /* keep min <= max: move the other one */
            *(id == GI_QPMAX ? &settings.h265_qp_min : &settings.h265_qp_max) = *q;
        break;
    }
    case GI_JPEGQ: {                                /* 5 per press, remote zoom = 1 */
        int q = (int)settings.jpeg_quality + dir * (menu_fine ? 1 : 5);
        settings.jpeg_quality = q < 1 ? 1 : q > 100 ? 100 : q;
        apply_jpeg_quality();
        break;
    }
    case II_MATRIX:
        menu_page = PAGE_CCM;
        menu_sel = 0;
        return;
    case II_HSV:
        menu_page = PAGE_HSV;
        menu_sel = 0;
        return;
    case II_NOISE:
        menu_page = PAGE_NOISE;
        menu_sel = 0;
        return;
    case NI_LUMA:                                   /* Auto < Off < 10% .. 400% in 10% steps; leaving Auto = 100% */
        if (settings.nr_luma < 0)
            settings.nr_luma = dir > 0 ? 100 : -1;
        else
            settings.nr_luma = settings.nr_luma / 10 * 10 + dir * 10;
        settings.nr_luma = settings.nr_luma > 400 ? 400 : settings.nr_luma < -1 ? -1 : settings.nr_luma;
        apply_noise();
        break;
    case NI_CHROMA: case NI_DESPIKE: case NI_FALSE: {   /* Auto -> Off -> On -> Auto */
        int32_t *v = id == NI_CHROMA ? &settings.nr_chroma : id == NI_DESPIKE ? &settings.nr_despike : &settings.nr_falsecolor;
        *v = dir > 0 ? (*v < 0 ? 0 : *v == 0 ? 1 : -1) : (*v < 0 ? 1 : *v == 1 ? 0 : -1);
        apply_noise();
        break;
    }
    case NI_DEFOG:                                  /* Off < 5 .. 100 in steps of 5 */
        settings.defog = settings.defog / 5 * 5 + dir * 5;
        settings.defog = settings.defog < 0 ? 0 : settings.defog > 100 ? 100 : settings.defog;
        apply_noise();
        break;
    case II_LOOK_SLOT:
        settings.look_slot = (settings.look_slot - 1 + LOOK_SLOTS + dir) % LOOK_SLOTS + 1;
        break;
    case II_LOOK_LOAD:
        if (dir > 0)
            notice_for(look_load(settings.look_slot) == 0 ? "Look loaded" : "Look slot is empty", OSD_WHITE, 2);
        break;
    case II_LOOK_SAVE:                              /* first OK arms, second saves (overwrites) */
        if (dir < 0)
            return;
        if (menu_confirm != id) {
            menu_confirm = id;
            return;
        }
        menu_confirm = -1;
        notice_for(look_save(settings.look_slot) == 0 ? "Look saved" : "Look save failed", OSD_WHITE, 2);
        break;
    case II_LOOK_EXPORT: {
        if (dir < 0)
            return;
        int n = looks_export();
        char msg[48];
        snprintf(msg, sizeof(msg), n < 0 ? "No SD card" : "Exported %d look(s) to SD LOOKS", n);
        notice_for(msg, n < 0 ? OSD_YELLOW : OSD_WHITE, 3);
        break;
    }
    case II_LOOK_IMPORT: {                          /* first OK arms, second imports (overwrites slots) */
        if (dir < 0)
            return;
        if (menu_confirm != id) {
            menu_confirm = id;
            return;
        }
        menu_confirm = -1;
        int n = looks_import();
        char msg[48];
        snprintf(msg, sizeof(msg), n < 0 ? "No SD card" : "Imported %d look(s) from SD", n);
        notice_for(msg, n < 0 ? OSD_YELLOW : OSD_WHITE, 3);
        break;
    }
    case II_RESET: case CI_RESET: case HI_RESET: case NI_RESET:   /* first OK arms, second resets the page */
        if (dir < 0)
            return;
        if (menu_confirm != id) {
            menu_confirm = id;
            return;
        }
        menu_confirm = -1;
        if (id == II_RESET) {
            settings.isp_brightness = settings.isp_contrast = settings.isp_lightness = -1;
            settings.isp_saturation = settings.isp_gamma = settings.isp_sharpness = settings.isp_ccm = -1;
            settings.isp_black = 0;
            settings.isp_bw = 0;
            apply_isp_image();
        } else if (id == NI_RESET) {
            settings.nr_luma = settings.nr_chroma = settings.nr_despike = settings.nr_falsecolor = -1;
            settings.defog = 0;
            apply_noise();
        } else if (id == CI_RESET) {
            settings.ccm_custom = 0;
            for (int i = 0; i < 9; i++)
                settings.ccm_m[i] = i % 4 == 0 ? 1024 : 0;
            apply_ccm();
        } else {
            for (int b = 0; b < 6; b++) {
                settings.hsv_hue[b] = 0;
                settings.hsv_sat[b] = 100;
            }
            apply_hsv();
        }
        printf("menu: page reset\n");
        break;
    case HI_RH: case HI_YH: case HI_GH: case HI_CH: case HI_BH: case HI_MH: {
        int32_t *v = &settings.hsv_hue[(id - HI_RH) / 2];
        *v += dir;
        *v = *v < -128 ? -128 : *v > 128 ? 128 : *v;
        apply_hsv();
        break;
    }
    case HI_RS: case HI_YS: case HI_GS: case HI_CS: case HI_BS: case HI_MS: {
        int32_t *v = &settings.hsv_sat[(id - HI_RS) / 2];
        *v += dir * 5;
        *v = *v < 0 ? 0 : *v > 300 ? 300 : *v;
        apply_hsv();
        break;
    }
    case CI_MODE: {
        /* Entering Custom with the default (identity) or an empty table starts from the vendor
         * matrix the ISP uses right now, so the picture does not jump. */
        int identity = 1, empty = 1;
        for (int i = 0; i < 9; i++) {
            identity &= settings.ccm_m[i] == (i % 4 == 0 ? 1024 : 0);
            empty &= settings.ccm_m[i] == 0;
        }
        if (!settings.ccm_custom && (identity || empty))
            ccm_copy_current();
        settings.ccm_custom ^= 1;
        apply_ccm();
        break;
    }
    case CI_COPY:
        if (settings.ccm_custom) {                  /* the vendor matrix for this light, not our own */
            settings.ccm_custom = 0;
            apply_ccm();
        }
        ccm_copy_current();
        settings.ccm_custom = 1;
        apply_ccm();
        break;
    case CI_M0: case CI_M1: case CI_M2: case CI_M3: case CI_M4: case CI_M5: case CI_M6: case CI_M7: case CI_M8:
        if (!settings.ccm_custom) {                 /* vendor values are read-only */
            notice_for("Vendor matrix is read-only: set Mode to Custom", OSD_YELLOW, 2);
            return;
        }
        ccm_step(id - CI_M0, dir * 8);              /* 8/1024 ~ 0.008 per step */
        apply_ccm();
        break;
    case II_BRIGHT: case II_CONTRAST: case II_LIGHT: case II_SAT: {   /* Auto < 0 < 1 .. 100 */
        int32_t *v = id == II_SAT ? &settings.isp_saturation : iq_simple[id - II_BRIGHT].value;
        int32_t n = *v + dir;
        if (*v < 0 && dir > 0)
            n = id == II_SAT ? iq_start_value(1)
                : (int32_t)((uint32_t *)iq_vendor[id - II_BRIGHT])[18];
        *v = n < -1 ? -1 : n > 100 ? 100 : n;
        if (id == II_SAT)
            apply_saturation();
        else
            apply_iq_simple(id - II_BRIGHT);
        break;
    }
    case II_CCM:                                    /* Auto < 0% .. 200% in 10% steps; leaving Auto = 100% */
        if (settings.isp_ccm < 0)
            settings.isp_ccm = dir > 0 ? 100 : -1;
        else
            settings.isp_ccm = settings.isp_ccm / 10 * 10 + dir * 10;
        if (settings.isp_ccm > 200) settings.isp_ccm = 200;
        if (settings.isp_ccm < -1) settings.isp_ccm = -1;
        apply_ccm();
        break;
    case II_SHARP:                                  /* Auto < Off < 10% .. 400% in 10% steps; leaving Auto = 100% */
        if (settings.isp_sharpness < 0)
            settings.isp_sharpness = dir > 0 ? 100 : -1;
        else
            settings.isp_sharpness = settings.isp_sharpness / 10 * 10 + dir * 10;
        if (settings.isp_sharpness > 400) settings.isp_sharpness = 400;
        if (settings.isp_sharpness < -1) settings.isp_sharpness = -1;
        apply_sharpness();
        break;
    case II_GAMMA:                                  /* Auto < Log < 1.0 .. 3.0 in 0.1 steps */
        if (settings.isp_gamma < 0)
            settings.isp_gamma = dir > 0 ? GAMMA_LOG : -1;
        else if (settings.isp_gamma == GAMMA_LOG)
            settings.isp_gamma = dir > 0 ? 10 : -1;
        else
            settings.isp_gamma += dir;
        if (settings.isp_gamma > 30) settings.isp_gamma = 30;
        if (settings.isp_gamma < 10 && settings.isp_gamma > GAMMA_LOG) settings.isp_gamma = GAMMA_LOG;
        apply_gamma();
        break;
    case II_BLACK:
        settings.isp_black += dir * 16;
        if (settings.isp_black < -4096) settings.isp_black = -4096;
        if (settings.isp_black > 4096) settings.isp_black = 4096;
        apply_black();
        break;
    case II_BW:
        settings.isp_bw ^= 1;
        apply_bw();
        break;
    case RI_ADDR:                                   /* steps of 8 registers, wraps */
        reg_base = reg_base / 8 * 8 + (dir > 0 ? 8 : -8);
        if (reg_base < REG_FIRST || reg_base > REG_LAST)
            reg_base = dir > 0 ? REG_FIRST : REG_LAST;
        return;
    case RI_V0: case RI_V1: case RI_V2: case RI_V3: case RI_V4: case RI_V5: case RI_V6: case RI_V7: {
        uint16_t reg = reg_base + id - RI_V0;       /* left/right = value -/+ 1 (wraps), if writable */
        uint8_t v;
        if ((!reg_write_all && !sensor_reg_safe(reg)) || sensor_read_block(reg, &v, 1) != 0)
            return;
        reg_write(reg, (uint8_t)(v + dir));
        return;
    }
    case RI_ACCESS:                                 /* Safe -> All needs OK twice; back to Safe at once */
        if (reg_write_all) {
            reg_write_all = 0;
        } else if (menu_confirm != RI_ACCESS) {
            menu_confirm = RI_ACCESS;
        } else {
            menu_confirm = -1;
            reg_write_all = 1;
        }
        return;
    case RI_UNDO:
        reg_undo_all();
        return;
    case EI_FPS:
    case EI_VMAX:
    case EI_HMAX:
    case EI_SHS:
    case EI_GAIN:
    case EI_TEMP:
        return;                                     /* read-only */
    case SI_FORMAT:
        if (dir < 0 || rec_on || sd_fmt_state == FMT_RUNNING)
            return;
        if (menu_confirm != SI_FORMAT) {          /* first press arms, second press formats */
            menu_confirm = SI_FORMAT;
            sd_fmt_state = FMT_IDLE;
            return;
        }
        menu_confirm = -1;
        sd_format_start();
        return;
    case SI_RESET:
        if (dir < 0)
            return;
        if (menu_confirm != SI_RESET) {
            menu_confirm = SI_RESET;
            return;
        }
        menu_confirm = -1;
        settings_reset();
        apply_all_settings();
        apply_nr3d();
        apply_wdr();
        apply_isp_image();
        apply_wb();
        printf("settings: reset to defaults\n");
        break;
    case SI_LOGS: {                                 /* camapp log + kernel log to the SD card */
        char msg[64];
        int n = save_logs();
        snprintf(msg, sizeof(msg), n < 0 ? "Save logs failed (SD card?)" : "Logs saved: LOGS/LOG_%04d", n);
        notice_for(msg, n < 0 ? OSD_YELLOW : OSD_WHITE, 3);
        break;
    }
    case SI_TC:                                     /* record-run timecode back to zero: OK twice */
        if (dir < 0 || rec_on)
            return;
        if (menu_confirm != SI_TC) {
            menu_confirm = SI_TC;
            return;
        }
        menu_confirm = -1;
        settings.tc_run_ms = 0;
        printf("timecode: reset\n");
        break;
    case SI_SD:
    case SI_BUILD:
    case SI_UPTIME:
        return;                                     /* information only */
    case MI_FRAMERATE: {
        if (rec_on)                                 /* never change the rate mid-recording */
            return;
        if (settings.mode == MODE_SLOWMO) {         /* any whole rate, 1 fps per press */
            int f = (int)sensor_fps_valid() + dir;
            settings.sensor_fps = f < SLOWMO_MIN_FPS ? SLOWMO_MIN_FPS : f > (int)max_fps() ? max_fps() : (unsigned)f;
            apply_exposure();
            break;
        }
        int lo = 0, n = 0;                          /* rates available in this mode: steps lo .. n-1 */
        while (lo < N_SENSOR_FPS && sensor_fps_steps[lo] < min_fps())
            lo++;
        n = lo;
        while (n < N_SENSOR_FPS && sensor_fps_steps[n] <= max_fps())
            n++;
        int i = lo;
        while (i < n - 1 && sensor_fps_steps[i] != sensor_fps_valid())
            i++;
        i = lo + (i - lo + (n - lo) + dir) % (n - lo);
        settings.sensor_fps = sensor_fps_steps[i];
        apply_exposure();                           /* clamps shutter to the new frame, then sets fps */
        break;
    }
    case MI_TL_INTERVAL: {                          /* arrows: camera-like steps; fine: 0.1 s / 1 s / 10 s */
        static const unsigned steps[] = { 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 15000, 20000, 30000,
                                          60000, 120000, 180000, 300000, 600000, 900000, 1200000, 1800000,
                                          2700000, 3600000 };
        const int n = sizeof(steps) / sizeof(steps[0]);
        long ms = settings.tl_interval_ms;
        if (rec_on)
            return;
        if (menu_fine) {
            long step = ms < 10000 || (ms == 10000 && dir < 0) ? 100 : ms < 300000 || (ms == 300000 && dir < 0) ? 1000 : 10000;
            ms += dir * step;
        } else if (dir > 0) {
            int i = 0;
            while (i < n - 1 && steps[i] <= (unsigned)ms)
                i++;
            ms = steps[i];
        } else {
            int i = n - 1;
            while (i > 0 && steps[i] >= (unsigned)ms)
                i--;
            ms = steps[i];
        }
        settings.tl_interval_ms = ms < 100 ? 100 : ms > 3600000 ? 3600000 : ms;
        break;
    }
    case MI_TL_LIMIT: {                             /* Off < 25 < 50 ... < 100000 frames; fine: 1 frame */
        static const unsigned steps[] = { 0, 25, 50, 100, 150, 250, 500, 750, 1000, 1500, 2500, 5000, 7500,
                                          10000, 25000, 50000, 100000 };
        const int n = sizeof(steps) / sizeof(steps[0]);
        long f = settings.tl_limit_frames;
        if (rec_on)
            return;
        if (menu_fine) {
            f += dir;
        } else if (dir > 0) {
            int i = 0;
            while (i < n - 1 && steps[i] <= (unsigned)f)
                i++;
            f = steps[i];
        } else {
            int i = n - 1;
            while (i > 0 && steps[i] >= (unsigned)f)
                i--;
            f = steps[i];
        }
        settings.tl_limit_frames = f < 0 ? 0 : f > 100000 ? 100000 : f;
        break;
    }
    case MI_PLAY_FPS:                               /* 1 fps per press */
        if (rec_on)
            return;
        settings.play_fps = play_fps() + dir;       /* play_fps() keeps it inside 10 .. capture - 1 */
        settings.play_fps = play_fps();
        break;
    case MI_REC_FPS:
        if (!rec_on && sensor_fps_valid() == 60)
            settings.rec_fps = settings.rec_fps == 30 ? 60 : 30;
        break;
    case MI_EXPOSURE:
        expo_mode_step(dir);
        break;
    case MI_PHOTO_RAW:
        settings.photo_raw ^= 1;
        break;
    case MI_SPOT_SIZE:
        spot_resize(spot_w() + 2 * dir);
        break;
    case MI_SHUTTER_BY:                             /* switch keeps the current exposure */
        if (settings.shutter_angle_x10) {
            settings.shutter_us = manual_shutter_us();
            settings.shutter_angle_x10 = 0;
        } else {
            uint32_t a = (uint32_t)(settings.shutter_us * 360.0 * sensor_fps_valid() / 1e6 + 0.5);
            settings.shutter_angle_x10 = (a < 1 ? 1 : a > 360 ? 360 : a) * 10;
        }
        apply_exposure();
        break;
    case MI_SHUTTER:
        shutter_step(dir, menu_fine);
        break;
    case MI_GAIN:
        gain_step(dir, menu_fine);
        break;
    case MI_EVCOMP: {
        int v = settings.ev_thirds + dir;
        settings.ev_thirds = v < -EV_THIRDS_MAX ? -EV_THIRDS_MAX : v > EV_THIRDS_MAX ? EV_THIRDS_MAX : v;
        apply_ev_comp();
        break;
    }
    case MI_AE_LOCK:
        ae_locked ^= 1;
        apply_exposure();
        break;
    case MI_AE_FLICKER:
        settings.ae_flicker = (settings.ae_flicker + 4 + dir) % 4;
        apply_ae_flicker();
        break;
    case MI_AE_SPEED: {                             /* Vendor < 5% 10% 20% 30% 50% 75% 150% 200% 300% */
        static const uint32_t steps[] = { 5, 10, 20, 30, 50, 75, 0, 150, 200, 300, 1000 };
        int n = sizeof(steps) / sizeof(steps[0]), i = 0;
        while (i < n - 1 && steps[i] != settings.ae_speed)
            i++;
        i += dir;
        settings.ae_speed = steps[i < 0 ? 0 : i >= n ? n - 1 : i];
        apply_ae_speed();
        break;
    }
    case MI_AE_MAXSH: {                             /* 1/1000 .. 1/24, Auto = one frame (last) */
        static const uint32_t steps[] = { 1000, 2000, 4000, 5000, 8333, 10000, 16667, 20000, 20833,
                                          33333, 40000, 41667, 0 };
        int n = sizeof(steps) / sizeof(steps[0]), i = 0;
        while (i < n - 1 && steps[i] != settings.ae_max_shutter_us)
            i++;
        i += dir;
        settings.ae_max_shutter_us = steps[i < 0 ? 0 : i >= n ? n - 1 : i];
        apply_exposure();
        break;
    }
    case MI_AE_MAXGAIN: {                           /* Vendor < 1x 2x 4x 8x 16x 32x */
        static const uint32_t steps[] = { 0, 1024, 2048, 4096, 8192, 16384, 32768 };
        int n = sizeof(steps) / sizeof(steps[0]), i = 0;
        while (i < n - 1 && steps[i] != settings.ae_max_gain)
            i++;
        i += dir;
        settings.ae_max_gain = steps[i < 0 ? 0 : i >= n ? n - 1 : i];
        apply_exposure();
        break;
    }
    case MI_REC_CODEC:                              /* never mid-recording */
        if (!rec_on)
            settings.rec_codec = (settings.rec_codec + 3 + dir) % 3;   /* MJPEG > H.265 > H.264 */
        break;
    case MI_REC_MBPS: {                             /* 10 .. 100 Mbit/s */
        static const uint32_t steps[] = { 10, 20, 30, 40, 50, 60, 80, 100 };
        int n = sizeof(steps) / sizeof(steps[0]), i = 0;
        if (rec_on)
            break;
        while (i < n - 1 && steps[i] < settings.rec_mbps)
            i++;
        i += dir;
        settings.rec_mbps = steps[i < 0 ? 0 : i >= n ? n - 1 : i];
        break;
    }
    case MI_WB:
        settings.wb_mode = (settings.wb_mode + 3 + dir) % 3;
        apply_wb();
        break;
    case MI_WB_K:
        settings.wb_kelvin += dir * (menu_fine ? 10 : 100);
        settings.wb_kelvin = settings.wb_kelvin < 2000 ? 2000 : settings.wb_kelvin > 10000 ? 10000 : settings.wb_kelvin;
        apply_wb();
        break;
    case MI_WB_R: case MI_WB_G: case MI_WB_B: {       /* 0.25x .. 8x in steps of 16/1024 (fine: 1/1024) */
        uint32_t *v = id == MI_WB_R ? &settings.wb_r : id == MI_WB_G ? &settings.wb_g : &settings.wb_b;
        long n = (long)*v + dir * (menu_fine ? 1 : 16);
        *v = n < 256 ? 256 : n > 8191 ? 8191 : n;
        apply_wb();
        break;
    }
    case MI_PEAKING:                                /* Off < 1 .. 10 (sensitivity) */
        settings.peaking = (int)settings.peaking + dir < 0 ? 0 : settings.peaking + dir > 10 ? 10 : settings.peaking + dir;
        apply_peaking();
        break;
    case MI_PEAK_COLOUR:
        settings.peak_colour = (settings.peak_colour + 4 + dir) % 4;
        break;
    case MI_ZEBRA: {                                /* Off < 0% .. 100% in 1% steps; leaving Off = 95% */
        int z = settings.zebra ? (int)settings.zebra - 1 : -1;
        z = z < 0 ? (dir > 0 ? 95 : -1) : z + dir;
        z = z > 100 ? 100 : z;
        settings.zebra = z < 0 ? 0 : z + 1;
        apply_peaking();
        break;
    }
    case MI_FALSECOL:
        settings.false_colour ^= 1;
        apply_peaking();
        break;
    case MI_WAVEFORM:                               /* Off, Luma, RGB, Parade */
        settings.waveform = (settings.waveform + 4 + dir) % 4;
        if (!settings.waveform)
            osd_waveform(NULL);
        apply_peaking();
        break;
    case MI_LOUPE: {                                /* Off < 1.2x < 1.5x .. 8x in 0.25x steps */
        int z = loupe_on ? loupe_z10 : 10;
        z += dir * (z < 20 ? 2 : 5);
        if (z <= 10) {
            loupe_on = 0;
        } else {
            loupe_on = 1;
            loupe_z10 = z > 80 ? 80 : z;
        }
        apply_loupe();
        return;                                     /* view aid, not a saved setting */
    }
    case MI_WB_GRAB:                                /* row disappears in Manual: land on White bal. */
        wb_grab();
        menu_want_id = MI_WB;
        break;
    case MI_METERING:
        settings.metering = (settings.metering + 3 + dir) % 3;
        apply_metering();
        show_spot_area();
        break;
    case MI_GUIDES:                                 /* thirds / crosshair bits; the frame guide stays */
        settings.overlay = (settings.overlay & ~3u) | ((settings.overlay + 4 + dir) & 3);
        osd_overlay(settings.overlay);
        break;
    case MI_FRAME: {
        unsigned f = ((settings.overlay & OSD_FRAME_MASK) >> OSD_FRAME_SHIFT) + OSD_FRAMES + dir;
        settings.overlay = (settings.overlay & ~OSD_FRAME_MASK) | (f % OSD_FRAMES) << OSD_FRAME_SHIFT;
        osd_overlay(settings.overlay);
        break;
    }
    case MI_HISTOGRAM:
        settings.histogram ^= 1;
        if (!settings.histogram)
            osd_histogram(NULL);
        apply_peaking();                            /* the histogram comes from the analysis thread */
        break;
    case MI_MIRROR:
    case MI_FLIP:
        if (id == MI_MIRROR)
            settings.mirror ^= 1;
        else
            settings.flip ^= 1;
        apply_orientation();
        break;
    }
    settings_changed();
}

/* Rows that act on OK ("Press OK" rows and Mode): remote play confirms them, since the remote has no
 * OK button. Everywhere else play resets the row to its default. */
static int row_confirms(int id)
{
    switch (id) {
    case MI_MODE: case MI_CROP_W: case MI_CROP_H: case MI_WB_GRAB: case SI_FORMAT: case SI_RESET: case SI_TC: case SI_LOGS: case CI_COPY:
    case II_LOOK_LOAD: case II_LOOK_SAVE: case II_LOOK_EXPORT: case II_LOOK_IMPORT:
    case II_RESET: case CI_RESET: case HI_RESET: case NI_RESET:
        return 1;
    }
    return 0;
}

/* Returns 1 if the menu consumed the command. */
int menu_input(const char *cmd)
{
    if (!menu_open) {
        if (strcmp(cmd, "menu"))
            return 0;
        menu_open = 1;                              /* back on the page and row it was closed on */
        if (osd_hidden) {                           /* a hidden OSD comes back with the menu */
            osd_hidden = 0;
            osd_show(1);
        }
        menu_confirm = mode_pick = -1;
        menu_draw();
        printf("menu: open\n");
        return 1;
    }
    /* Remote mirror button (sends "loupe") = back one level; MENU always closes. */
    if (!strcmp(cmd, "loupe")) {
        if (menu_page != PAGE_MAIN) {
            menu_confirm = mode_pick = -1;
            crop_cancel();
            menu_back();
            menu_draw();
            printf("menu: back\n");
        }
        return 1;
    }
    /* Menu open: function buttons do nothing (only navigation, value changes, record and photo). */
    static const char *const blocked[] = { "loupe", "peaking", "metering_next", "awb", "ae_lock",
                                           "histogram_toggle", "overlay_next", "remote_ae" };
    for (unsigned i = 0; i < sizeof(blocked) / sizeof(blocked[0]); i++)
        if (!strcmp(cmd, blocked[i])) {
            printf("menu: %s ignored while the menu is open\n", cmd);
            return 1;
        }
    if (!strcmp(cmd, "menu")) {
        menu_open = 0;
        crop_cancel();
        osd_menu_close();
        printf("menu: closed\n");
    } else if (!strcmp(cmd, "up") || !strcmp(cmd, "down")) {
        menu_confirm = mode_pick = -1;
        menu_build_rows();
        menu_sel = (menu_sel + menu_nrows + (cmd[0] == 'd' ? 1 : -1)) % menu_nrows;
        if (menu_rows[menu_sel] != MI_CROP_W && menu_rows[menu_sel] != MI_CROP_H)
            crop_cancel();                          /* width and height can be set together */
        menu_draw();
    } else if (!strcmp(cmd, "left") || !strcmp(cmd, "ae") || !strcmp(cmd, "right") || !strcmp(cmd, "ok")
               || (!strcmp(cmd, "reset") && row_confirms(menu_rows[menu_sel]))) {
        menu_ok = !strcmp(cmd, "ok") || !strcmp(cmd, "reset");
        menu_change(!strcmp(cmd, "left") || !strcmp(cmd, "ae") ? -1 : 1);
        menu_ok = 0;
        menu_draw();
        printf("menu: %s changed\n", menu_labels_all[menu_rows[menu_sel]]);
    } else if (!strcmp(cmd, "ev_up") || !strcmp(cmd, "ev_down") || !strcmp(cmd, "fine_up") || !strcmp(cmd, "fine_down")) {
        menu_fine = 1;                              /* remote zoom+/zoom-: precise steps */
        menu_change(strstr(cmd, "up") ? 1 : -1);
        menu_fine = 0;
        menu_draw();
        printf("menu: %s changed (fine)\n", menu_labels_all[menu_rows[menu_sel]]);
    } else if (!strcmp(cmd, "reset")) {
        menu_reset();
        menu_draw();
        printf("menu: %s reset\n", menu_labels_all[menu_rows[menu_sel]]);
    } else {
        return 0;                                   /* other buttons (record, photo...) still work */
    }
    return 1;
}
