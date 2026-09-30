#ifndef CAMAPP_SETTINGS_H
#define CAMAPP_SETTINGS_H
#include <stdint.h>

/* User settings persisted across reboots as key=value text. */
#define GAMMA_LOG 1                 /* isp_gamma value for the log tone curve */
struct settings {
    uint32_t mirror, flip;          /* sensor readout direction */
    uint32_t manual;                /* 0 = auto exposure, 1 = manual */
    uint32_t photo_raw;             /* photos also as DNG (raw 10-bit Bayer) */
    uint32_t ae_priority;           /* auto only: 0 P (program), 1 S (shutter fixed), 2 G (gain fixed) */
    uint32_t shutter_us, sensor_gain;   /* manual exposure (gain: 1024 = 1x) */
    uint32_t shutter_angle_x10;     /* manual shutter by angle: 0 = by speed (shutter_us), else degrees x 10 */
    uint32_t rec_fps;               /* 30 or 60; only used while the sensor runs 60 fps */
    uint32_t play_fps;              /* Slow-mo / Timelapse: clips are stamped to play at this rate (Slow-mo 5 .. capture - 1, Timelapse 5 .. 60) */
    uint32_t tl_interval_ms;        /* Timelapse: one frame every this many ms (100 .. 3600000) */
    uint32_t tl_limit_frames;       /* Timelapse: stop after this many frames, 0 = until stopped */
    uint32_t crop_w, crop_h;        /* sensor window crop (Normal, Slow-mo), 0 = full 1920 / 1080; applies at start */
    uint32_t peaking;               /* focus peaking: 0 off, 1..10 sensitivity (higher = more edges) */
    uint32_t peak_colour;           /* 0 red, 1 yellow, 2 white, 3 blue */
    uint32_t zebra;                 /* 0 off, else threshold + 1 (1..101 = 0..100 % of full scale) */
    uint32_t false_colour;          /* 1 = exposure false colour */
    uint32_t waveform;              /* waveform panel: 0 off, 1 luma, 2 RGB overlay, 3 RGB parade */
    uint32_t rec_codec;             /* 0 MJPEG (.avi), 1 H.265 (.ts), 2 H.264 (.ts) */
    uint32_t rec_mbps;              /* H.265 bitrate, Mbit/s */
    uint32_t sensor_fps;            /* 24, 25, 30, 48, 50, 60, 72 or 75 (HDR: up to 30) */
    uint32_t metering;              /* AE window weighting: 0 average, 1 centre, 2 spot */
    uint32_t spot_x, spot_y;        /* spot top-left block on the 16x16 AE grid */
    uint32_t spot_size;             /* spot = size x 2*size blocks (1..8, 0 = default 2) */
    int32_t ev_thirds;              /* exposure compensation in 1/3 EV steps, -9..+9 */
    uint32_t overlay;               /* OSD guides: bit 0 rule of thirds, bit 1 crosshair */
    uint32_t histogram;             /* OSD histogram panel on/off */
    uint32_t nr3d_gain;             /* temporal NR motion gain: 0 off, vendor 1000, higher = stronger */
    uint32_t mode;                  /* camera mode (MODE_*), applied at start; switching restarts camapp */
    int32_t isp_brightness, isp_contrast, isp_lightness, isp_saturation;   /* -1 vendor (auto), 0..100 manual */
    int32_t isp_black;              /* ISP black level (OBC) offset from the vendor value, ISP units (16 = 1 LSB 12-bit) */
    uint32_t isp_bw;                /* 1 = black and white (ISP colour-to-gray) */
    int32_t isp_gamma;              /* -1 vendor curve, GAMMA_LOG = log curve, else gamma x 10 (10..30) */
    int32_t isp_sharpness;          /* -1 vendor, 0 off, else % of the vendor edge gains (bytes 1,2) */
    int32_t isp_ccm;                /* -1 vendor, else colour matrix strength % (0 = identity / sensor native) */
    uint32_t ccm_custom;            /* 1 = use the custom matrix below (ISP manual CCM) */
    int32_t ccm_m[9];               /* custom matrix, row-major R,G,B out from R,G,B in; 1024 = 1.0 */
    int32_t hsv_hue[6], hsv_sat[6]; /* per colour band R,Y,G,C,B,M: hue shift (ISP units), sat % (100 = vendor) */
    int32_t nr_luma;                /* spatial luma NR: -1 vendor, 0 off, else % of vendor thresholds */
    int32_t nr_chroma, nr_despike, nr_falsecolor;   /* -1 vendor, 0 off, 1 on */
    int32_t defog;                  /* 0 off (vendor), 1..100 strength */
    uint32_t ae_max_shutter_us;     /* auto exposure: longest shutter (real us), 0 = Auto (one frame) */
    uint32_t ae_max_gain;           /* auto exposure: highest sensor gain (1024 = 1x), 0 = vendor (8x) */
    uint32_t ae_speed;              /* auto exposure speed, % of vendor (0 = vendor, 1000 = instant) */
    uint32_t ae_flicker;            /* anti-flicker: 0 vendor (50 Hz), 1 off, 2 50 Hz, 3 60 Hz */
    uint32_t wb_mode;               /* white balance: 0 auto, 1 Kelvin, 2 manual gains */
    uint32_t wb_kelvin;             /* Kelvin mode colour temperature, 2000..10000 */
    uint32_t wb_r, wb_g, wb_b;      /* manual gains, 1024 = 1.0x */
    int32_t wdr;                    /* WDR strength: -1 vendor auto table, 0 off (vendor boot state), 1..255 manual */
    uint32_t look_slot;             /* selected look slot 1..LOOK_SLOTS (Expert > Image) */
    uint32_t jpeg_quality;          /* JPEG encoder quality factor 1..100 (MJPEG clips, photos, USB) */
    uint32_t h265_gop;              /* H.265 keyframe interval in frames, 0 = one second of capture */
    uint32_t h265_rc;               /* H.265 rate control: 0 CBR, 1 VBR */
    uint32_t h265_qp_max, h265_qp_min;  /* VBR quantiser range 0..51 (lower = better) */
    uint32_t tc_run_ms;             /* record-run timecode: total timeline length of all clips so far, ms */
};

extern struct settings settings;

/* Camera modes. Each mode keeps its own frame rate, exposure and recording settings (the mode keys)
 * in its own file; everything else is shared. */
enum { MODE_NORMAL, MODE_HDR, MODE_SLOWMO, MODE_TIMELAPSE, N_MODES };
extern const char *const mode_names[N_MODES];

/* Load from path; missing file or keys keep the defaults. */
void settings_load(const char *path);
/* Restore every setting to its default. */
void settings_reset(void);
/* Atomic save: path.tmp + fsync + rename. 0 on success. */
int settings_save(const char *path);
/* Save only the mode keys (sensor_fps, rec_fps, play_fps, exposure, codec, bitrate); load with settings_load. */
int settings_save_mode(const char *path);
/* Looks: the image-processing subset (isp_*, ccm_*, hsv_*, nr_*, defog, wdr, nr3d_gain) saved to its
 * own file with the same atomic write; loading one is settings_load(path) (only keys present change). */
#define LOOK_SLOTS 5
int settings_save_look(const char *path);
/* Copy a look file keeping only image keys (import from a PC-edited file). 0 on success. */
int settings_filter_look(const char *src, const char *dst);

#endif
