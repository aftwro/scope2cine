#ifndef CAMAPP_CAMAPP_H
#define CAMAPP_CAMAPP_H
/* Internal interface between camapp's source files. camapp.c: pipeline bring-up, exposure, metering,
 * OSD status, playback glue, main loop. isp.c: image processing and looks. rec.c: recording and SD
 * card. menu.c: on-screen menu. cmd.c: button/FIFO command dispatcher. scopes.c: loupe and the
 * analysis thread (peaking, zebra, false colour, histogram, waveform). Not a public API. */
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include "avi.h"
#include "mi.h"
#include "settings.h"

#define REC_DIR "/app/bin/sd"          /* SD card mount point: clips, photos, LOOKS */

enum { VENC_DEV_JPEG = 8, VENC_CHN = 0 };

/* AE exposure info (0x23c bytes, MI_ISP_AE_EXPO_INFO layout): +0x2c average Y (linear,
 * saturates near 255), +0x230 scene light value x10, +0x234 brightness value, +0x238 AE target Y
 * (same units as average Y). Verified 2026-09-24: avgY scales with shutter time. */
enum { AE_AVG_Y = 0x2c / 4, AE_LV_X10 = 0x230 / 4, AE_BV = 0x234 / 4, AE_TARGET = 0x238 / 4 };
#define AE_AVG_Y_CLIP 250          /* at or above this the meter is saturated */

/* AE window weights (0xc04 bytes): u32 mode, then 32x32 u8 tables for average, centre, spot.
 * Only the top-left 16x16 of each table is used: the AE grid is 16x16 blocks over the frame.
 * Vendor tables (read 2026-09-24): "average" is a dashcam road pattern (sky ~0, bottom heavy),
 * "centre" is a proper centre-weighted bump, "spot" is flat. We install our own average and spot. */
#define WGT_N 32
#define WGT_GRID 16
/* Spot = S x 2S blocks of the 16x16 grid (S = settings.spot_size 2, 4, 6, 8, default 2 = 240 x 270
 * sensor pixels): square on screen on a 16:9 frame. Odd widths cannot sit on the centre line of a
 * 16-column grid (size 1 sat half a block off the crosshair, user 2026-09-30), so only even ones. */
int spot_w(void);
void spot_resize(int size);            /* menu.c: 1..8, keeps the centre */
#define SPOT_W spot_w()
#define SPOT_H (2 * spot_w())
#define SPOT_CX ((WGT_GRID - SPOT_W) / 2)
#define SPOT_CY ((WGT_GRID - SPOT_H) / 2)
struct win_weights { uint32_t mode; uint8_t tbl[3][WGT_N][WGT_N]; };
struct wgt_box { int x0, y0, x1, y1; };      /* inclusive block bounds of non-zero weights */

/* camapp.c */
extern const char *const metering_names[];
extern volatile int  playing;
extern volatile int  snap_request, venc_run;
extern volatile unsigned long long  venc_bytes;
extern volatile long  venc_frames;
int weight_box(const struct win_weights *w, int t, struct wgt_box *b);
void playback_start(const char *path);
int playback_input(const char *cmd);
int newest_clip(char *path, int size);
int clip_number(const char *name);
extern uint32_t live_shutter_us;       /* real exposure (sensor registers), updated every second */
extern double live_gain;               /* total gain (sensor x ISP), updated every second */
/* Record-run timecode HH:MM:SS:FF of ms at fps (static buffer). */
const char *tc_text(uint64_t ms, unsigned fps);
int apply_jpeg_quality(void);
unsigned file_fps(void);                /* rate clips of the current mode are stamped with */
uint64_t rec_tc_ms(void);               /* record-run timecode now (running while recording) */
double ev_offset(const uint32_t *info);
#define EV_THIRDS_MAX 9
const char *expo_mode_short(void);
void expo_mode_step(int dir);   /* P > S > G > M */
unsigned max_fps(void);
#define N_SENSOR_FPS 11
extern const unsigned sensor_fps_steps[N_SENSOR_FPS];   /* 24 25 30 48 50 60 72 75 90 100 120 */
extern uint32_t crop_w, crop_h;        /* sensor window crop of this run, 0 = full frame */
#define OPEN_GATE_W 1944                /* crop_w value meaning open gate: the whole 1948x1097 readout, */
#define OPEN_GATE_H 1096                /* VIF-aligned (width 8, height 2), scaled to 1920x1080 */
#define OPEN_GATE (crop_w == OPEN_GATE_W)
#define SRC_W (crop_w ? crop_w : 1920)  /* picture size entering ISP/SCL */
#define SRC_H (crop_w ? crop_h : 1080)
unsigned crop_max_fps(uint32_t w);
extern volatile sig_atomic_t  stop_requested, restart_requested;
extern int  hdr_on;
/* Switch camera mode: stores the current mode's keys, loads the target's, restarts camapp.
 * -1 (nothing done) while recording or for the running mode. */
int mode_switch(unsigned mode);
extern const char *const peak_colour_names[4];
extern const char *const flicker_names[];
extern int  ae_locked;
int apply_orientation(void);
unsigned frame_us(void);
uint32_t max_manual_us(void);
const char *shutter_text(uint32_t us);
int soc_temp_c(void);
void show_spot_area(void);
uint32_t manual_shutter_us(void);
int install_weight_tables(void);
int apply_metering(void);
void apply_exposure(void);
int apply_ae_speed(void);
int apply_ae_flicker(void);
int sd_mounted(void);
void settings_changed(void);           /* schedule a debounced settings save */
int apply_ev_comp(void);
#define SLOWMO_MIN_FPS 6               /* driver floor: 3000 mfps request = 6 fps */
#define PLAY_MIN_FPS 5
unsigned min_fps(void);
unsigned play_fps(void);
/* Timelapse interval as shown on screen ("0.5s", "10s", "5m", "2m30s"); static buffer. */
const char *tl_interval_text(void);
unsigned sensor_fps_valid(void);     /* output frame rate the sensor runs */
void tally_update(void);              /* recording tally frame follows rec_on */
#define NOTICE_SECS 2
void notice_for(const char *text, uint32_t colour, int secs);   /* one-line OSD notice for secs */
extern int osd_hidden;
extern volatile int view_request;      /* scopes.c: viewgrab pending */                 /* OSD layer hidden (body MENU double click, FIFO osd_toggle) */
int rgn_test(int on);                  /* rgn.c: step R2 red box in the USB picture */
void dng_fbc_off(void);                /* dng.c: VIF output uncompressed (before pipeline start) */
int dng_capture(const char *path, uint32_t x0, uint32_t y0, uint32_t w, uint32_t h);
/* Encoder stream fetch (layout from cardv's thumbnail grabber @0x494ec): stream = {Pack *packs,
 * u32 pack_count, ...} (0x40 bytes); Pack = 0xb8 bytes, +8 data ptr, +0xc length. */
#define PACK_SIZE 0xb8
#define MAX_PACKS 2               /* EV compensation (+ the log gamma offset) */

/* Log tone curve for the ISP gamma LUT: y = log(1 + a x) / log(1 + a). Auto exposure runs 2 stops
 * under with it (LOG_EV_THIRDS, see apply_ev_comp), so 18 % grey sits at 4.5 % linear and the sensor
 * keeps 2 more stops of highlight headroom; a = 155 lifts that 4.5 % back to ~41 % (S-Log3-like). */
#define LOG_A 155.0
#define LOG_EV_THIRDS (-6)

/* rec.c */
#define REC_QUEUE 240                   /* frames the writer may lag behind (see rec.c) */
#define REC_MEM_MAX (20u * 1024 * 1024)  /* ... and the bytes they may hold */
extern uint32_t rec_queued_bytes;
enum { FMT_IDLE, FMT_RUNNING, FMT_OK, FMT_FAILED };   /* sd_fmt_state */
int clip_next_no(void);
void rec_frame(const void *p1, uint32_t n1, const void *p2, uint32_t n2);
int rec_open_next(void);
void rec_set(int on, int fps);
void sd_format_start(void);
void sd_hotplug_check(void);
extern int rec_head, rec_count;
extern avi_t *rec;
extern volatile int rec_on, rec_writer_run;
extern int rec_file_no, rec_fps, rec_skip, rec_decimate;
extern long rec_frames_total, rec_dropped, rec_errors;
extern volatile uint64_t rec_bytes_total;
extern long rec_dropped_at_start, rec_frames_at_start;
extern volatile int rec_stop_pending;
extern time_t rec_started;
extern volatile int sd_fmt_state;
extern char sd_fmt_msg[40];

/* isp.c */
#define NR3D_SIZE 0x844
#define NR3D_ENTRIES 17
#define NR3D_ENTRY 124
#define IQ_SIMPLE_SIZE 0x4c
struct iq_simple {                      /* Brightness / Contrast / Lightness: same struct layout */
    const char *name;
    int (*get)(uint32_t, uint32_t, void *);
    int (*set)(uint32_t, uint32_t, void *);
    int32_t *value;
};
extern const struct iq_simple iq_simple[];
extern const char *const wb_names[];
extern uint8_t nr3d_vendor[NR3D_SIZE];
extern int nr3d_have_vendor;
extern uint8_t iq_vendor[3][IQ_SIMPLE_SIZE];
int apply_black(void);
int apply_bw(void);
int apply_ccm(void);
int apply_gamma(void);
int apply_hsv(void);
int apply_iq_simple(int k);
int apply_isp_image(void);
int apply_noise(void);
int apply_nr3d(void);
int apply_saturation(void);
int apply_sharpness(void);
int apply_wb(void);
int apply_wdr(void);
int ccm_copy_current(void);
int ccm_current(int32_t out[9]);
void ccm_step(int i, int delta);
int32_t iq_start_value(int sat);
int look_exists(unsigned slot);
int look_load(unsigned slot);
int look_save(unsigned slot);
int looks_export(void);
int looks_import(void);
int wb_grab(void);

/* menu.c: menu */
int reg_write(uint16_t reg, uint8_t val);
void reg_undo_all(void);
void menu_draw(void);
void shutter_step(int dir, int fine);
void gain_step(int dir, int fine);
int menu_input(const char *cmd);
extern int  menu_open, menu_sel;

/* cmd.c: command dispatcher */
void handle_cmd(char *line);

/* scopes.c: focus and exposure aids */
extern int loupe_on, loupe_z10, loupe_cx, loupe_cy;   /* loupe: on, zoom x10, centre (px) */
int apply_loupe(void);
int apply_peaking(void);          /* start/stop the analysis thread for the enabled aids */
void peak_stop(void);

#endif
