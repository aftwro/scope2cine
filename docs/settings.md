# Settings and device paths

The settings file keys and the files and kernel paths on the camera.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 11, 12), the complete project reference. Section numbers match it.

## 11. Settings file reference (`/customer/camapp/settings`)

- The file holds one `key=value` per line, as signed integers.
- Missing keys keep their defaults, and unknown keys are ignored.
- Legacy keys: `hdr=1` maps to the HDR mode; the old `nr3d=` preset key is ignored.
- Mode files (`modes/<Name>`) hold only the mode keys (marked M).
- Look files (`looks/lookN`, `LOOKS/LOOKN.TXT`) hold only the image keys (marked L).

| Key | Default | Meaning |
|---|---|---|
| `mode` | 0 | 0 Normal, 1 HDR, 2 Slow-mo, 3 Timelapse (applied at start) |
| `sensor_fps` (M) | 60 | 24 25 30 48 50 60 72 75 (90 100 120 with a crop); Slow-mo any 6..max; HDR ≤30 |
| `rec_fps` (M) | 30 | 30 or 60: MJPEG decimation when the sensor runs 60 |
| `play_fps` (M) | 25 | Slow-mo 5..capture−1, Timelapse 5..60 |
| `tl_interval_ms` (M) | 1000 | 100..3600000 |
| `tl_limit_frames` (M) | 0 | Timelapse stop after N frames, 0 = off |
| `crop_w`, `crop_h` (M) | 0, 0 | sensor crop (0 = full; 1944 = open gate) |
| `manual` (M) | 0 | 1 = M mode |
| `ae_priority` (M) | 0 | auto only: 0 P, 1 S, 2 G |
| `shutter_us` (M) | 10000 | manual shutter, real µs |
| `shutter_angle_x10` (M) | 0 | 0 = by speed, else angle × 10 |
| `sensor_gain` (M) | 1024 | manual gain, 1024 = 1x |
| `ev_thirds` (M) | 0 | −9..9 |
| `rec_codec` (M) | 1 | 0 MJPEG, 1 H.265, 2 H.264 |
| `rec_mbps` (M) | 40 | 2..100 |
| `photo_raw` | 0 | 1 = JPEG + DNG |
| `mirror`, `flip` | 0 | sensor readout direction |
| `metering` | 0 | 0 AVG, 1 CTR, 2 SPOT |
| `spot_x`, `spot_y` | 7, 6 | spot top-left block (16x16 grid) |
| `spot_size` | 0 | 2/4/6/8 (0 = 2) |
| `overlay` | 0 | bit 0 thirds, bit 1 cross, higher bits = frame line index |
| `histogram` | 0 | histogram panel |
| `waveform` | 0 | 0 off, 1 luma, 2 RGB, 3 parade |
| `peaking` | 0 | 0 off, 1..10 |
| `peak_colour` | 0 | 0 red, 1 yellow, 2 white, 3 blue |
| `zebra` | 0 | 0 off, else threshold % + 1 |
| `false_colour` | 0 | 0/1 |
| `ae_max_shutter_us` | 0 | 0 = Auto (one frame) |
| `ae_max_gain` | 0 | 0 = vendor (8x) |
| `ae_speed` | 0 | % of vendor, 0 = vendor, 1000 = Instant |
| `ae_flicker` | 0 | 0 vendor, 1 off, 2 50 Hz, 3 60 Hz |
| `wb_mode` | 0 | 0 auto, 1 Kelvin, 2 manual |
| `wb_kelvin` | 5600 | 2000..10000 |
| `wb_r`, `wb_g`, `wb_b` | 1024 | manual gains |
| `jpeg_quality` | 70 | 1..100 |
| `h265_gop` | 0 | frames, 0 = one second |
| `h265_rc` | 0 | 0 CBR, 1 VBR |
| `h265_qp_max`, `h265_qp_min` | 48, 12 | VBR QP range |
| `tc_run_ms` | 0 | record-run timecode position |
| `look_slot` | 1 | selected look slot |
| `isp_brightness` `isp_contrast` `isp_lightness` `isp_saturation` (L) | −1 | −1 auto, 0..100 |
| `isp_gamma` (L) | −1 | −1 vendor, 1 log, 10..30 = gamma × 10 |
| `isp_sharpness` (L) | −1 | −1 vendor, 0 off, % |
| `isp_ccm` (L) | −1 | −1 vendor, % strength |
| `isp_black` (L) | 0 | OBC offset, ISP units (16 = 1 sensor code) |
| `isp_bw` (L) | 0 | black and white |
| `ccm_custom` (L) | 0 | 1 = custom matrix |
| `ccm_rr` … `ccm_bb` (L) | identity | custom matrix, 1024 = 1.0 |
| `hsv_<r|y|g|c|b|m>_hue` (L) | 0 | ±128 |
| `hsv_<r|y|g|c|b|m>_sat` (L) | 100 | 0..300 % |
| `nr_luma` (L) | −1 | −1 vendor, 0 off, % |
| `nr_chroma` `nr_despike` `nr_falsecolor` (L) | −1 | −1 vendor, 0 off, 1 on |
| `defog` (L) | 0 | 0..100 |
| `wdr` (L) | 0 | −1 auto, 0 off, 1..255 |
| `nr3d_gain` (L) | 1000 | 0 off, vendor 1000 |

---

## 12. Device paths reference

| Path | Use |
|---|---|
| `/tmp/camapp.log` | camapp log; first line `camapp build <date> <time>`; status every 10 s; `uvc_start` marks the end of bring-up |
| `/tmp/camapp_fifo` | command pipe |
| `/proc/mi_modules/mi_vif/mi_vif0` | VIF state; the line after the `IsrCnt` header = frame interrupt count (the frame counter used everywhere to prove frames flow) |
| `/proc/mi_modules/mi_scl/mi_scl0` | SCL state; `echo 'dump_buffer 0 oport 0 UsrGetFifo /tmp/sd0 bufnum=1' > ...` dumps a port frame |
| `/proc/mi_modules/mi_rgn/mi_rgn0` | RGN state; `setRealtimeFlip MOD DEV CHN PORT 1` |
| `/proc/mi_modules/mi_hdmi/mi_hdmi0` | HDMI state; `echo "Timing list" > ...` prints the supported timings |
| `/proc/mi_modules/mi_fb/mi_fb0` | framebuffer state |
| `/proc/mi_modules/mi_disp/...` | display state |
| `/sys/class/mstar/csi0/csi_ints` | MIPI receiver interrupt/error counters |
| `/sys/devices/virtual/mstar/msys/TEMP_R` | SoC temperature |
| `/sys/devices/virtual/mstar/msys/CHIP_ID` | chip ID (0xF4) |
| `/sys/module/mi_vif/parameters/g_bFBCMode` | VIF frame buffer compression (camapp writes N) |
| `/sys/class/gpio/gpio38/value` | LED |
| `/sys/class/udc/<udc>/soft_connect`, `.../state` | USB pull-up toggle, gadget state (`configured` when enumerated) |
| `/sys/kernel/debug/mmc0/ios` | SD bus clock/width/timing (mount debugfs first) |
| `/sys/block/mmcblk0/mmcblk0p1` | SD card presence |
| `/dev/i2c-1` (addr 0x1a) | IMX307 registers |
| `/dev/input/event0`, `event1` | body buttons, IR remote |
| `/dev/video0` | UVC gadget (opening it = USB pull-up on) |
| `/dev/fb0` | OSD framebuffer |
| `/customer/config/usb_gadget/cam` | our configfs gadget |
| `/bootconfig/iqfile/isp_api.bin` | ISP API bin loaded at start |
| `/bootconfig/bin/riu_r`, `riu_w` | raw SoC register access (dangerous) |
| `/customer/modules/4.9.227/` | kernel modules (vendor) |
