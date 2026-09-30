# FIFO command reference

Every command camapp accepts on its command pipe.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 10), the complete project reference. Section numbers match it.

## 10. FIFO command reference

Write one line to the pipe: `echo "<command> [args]" > /tmp/camapp_fifo`. Results go to `/tmp/camapp.log` (read it with `tail /tmp/camapp.log` over UART). The parser takes `%31s %lu %lu`, so numeric arguments are unsigned except where a command re-parses the line itself (`evcomp`, `isp`, `wb`, `h265`, `crop`, `play`, `dng`, `angle`). Commands that change settings save them (debounced). "(debug)" commands are measurement tools kept from development; most are read-only dumps.

### 10.1 Navigation and buttons (same names the buttons send)
| Command | Effect |
|---|---|
| `menu` | open/close menu |
| `up` `down` `left` `right` | arrows (menu / loupe pan / live shutter/gain / spot move) |
| `ae` `ok` | body AE (−) and OK (+) |
| `reset` | remote play/pause: reset row / confirm / pause playback |
| `ev_up` `ev_down` | remote zoom +/−: EV comp, fine steps, loupe zoom |
| `fine_up` `fine_down` | fine step in the menu |
| `ev0` | EV comp to 0 |
| `menu_short` `menu_long` `menu_double` `menu_triple` | body MENU gestures (photo-or-close, menu, OSD hide, record) |
| `nop` | nothing |
| `osd_toggle` | hide/show the OSD layer |
| `snap` | photo (+ DNG if Photo = JPEG + DNG) |
| `rec_toggle` / `rec 1 [fps]` / `rec 0` | record start/stop (fps 30/60 decimation for MJPEG at 60p) |
| `loupe` | loupe on/off (in the menu: back) |
| `peaking` | peaking on (last level, default 5) / off |
| `metering_next` | AVG → CTR → SPOT |
| `remote_ae` | exposure mode P → S → G → M |
| `awb` | WB lock / auto |
| `ae_lock` | AE lock toggle (auto only) |
| `histogram_toggle` | histogram on/off |
| `overlay_next` | guides cycle |
| `mirror` / `flip` | toggle sensor mirror / flip |

### 10.2 Settings and features
| Command | Effect |
|---|---|
| `mode N` | 0 Normal, 1 HDR, 2 Slow-mo, 3 Timelapse (restarts) |
| `framerate N` | sensor frame rate (validated for the mode/crop; not while recording) |
| `crop W H` | sensor crop, `crop 0 0` = full, `crop 1944 1096` = open gate (restarts) |
| `restart` | re-exec camapp (debug) |
| `playfps N` | Slow-mo/Timelapse play rate |
| `tlint MS` | Timelapse interval 100 .. 3600000 ms |
| `tllimit N` | Timelapse stop after N frames (0 = off) |
| `codec N` | 0 MJPEG, 1 H.265, 2 H.264 (not while recording) |
| `mbps N` | bitrate 2..100 |
| `recfps 30|60` | MJPEG record decimation at 60p |
| `h265 GOP RC QPMAX QPMIN` | encoder settings for the next clip (RC 0 CBR, 1 VBR) |
| `photoraw 0|1` | Photo JPEG / JPEG + DNG |
| `auto` | P mode (auto exposure) |
| `shutter US` | manual shutter in real µs (M) |
| `angle DEG` | manual shutter by angle, e.g. `angle 172.8` |
| `gain N` | manual sensor gain, 1024 = 1x |
| `prio N` | 0 P, 1 S, 2 G, 3 M |
| `evcomp N` | EV comp in thirds, −9..9 |
| `metering N` | 0 average, 1 centre, 2 spot |
| `spotsize N` | 2/4/6/8 (odd values round down) |
| `spot_centre` | spot back to the centre |
| `aemax US GAIN` | AE max shutter (real µs) and max gain (1024 = 1x); 0 = Auto/vendor |
| `aespeed PCT` | AE speed % of vendor, 0 = vendor, 1000 = Instant |
| `flicker N` | 0 vendor, 1 off, 2 50 Hz, 3 60 Hz |
| `wb auto` / `wb kelvin K` / `wb manual R G B` / `wb grab` | white balance (gains 1024 = 1x) |
| `isp <name> <value>` | image controls: `brightness`, `contrast`, `lightness`, `saturation` (−1 auto, 0..100); `gamma` (−1 vendor, 1 log, 10..30); `sharpness` (−1, 0, %); `ccm` (−1, %); `black OFFSET`; `bw 0|1`; `luma_nr` (−1, 0, %); `chroma_nr`/`despike`/`falsecolor` (−1/0/1); `defog N`; `ccm_custom 0|1`; `ccm_copy 0`; `ccm_m0..8 X` (×1024); `hsv_<r|y|g|c|b|m>_h N` / `_s PCT` |
| `wdr_level N` | −1 auto, 0 off, 1..255 |
| `nr3d_gain N` | 0 off, 1..65535 (vendor 1000) |
| `zebra PCT` / `zebra` | zebra threshold 0..100 / off |
| `falsecolour 0|1` | false colour |
| `waveform N` | 0 off, 1 luma, 2 RGB, 3 parade |
| `overlay N` | guides bits: 1 thirds, 2 cross |
| `magnify Z10` | loupe zoom ×10 (10 = off, 12..80) |
| `dp N` | defect-pixel correction: 0 vendor, 1 off, 2+ = manual with hot slope N (test) |
| `regw ADDR VAL` | write any IMX307 register (hex ok, logged, undoable; "(UNSAFE register)" tag) |
| `regundo` | undo all register writes, newest first |
| `rgn_test N` | MI_RGN overlay probe: 1 box on port 1, 2 + port re-enable, 3 alpha 0, 4 port 0, 5 cardv order, 0 remove (parked) |
| `play [PATH]` / `playstop` / `playpause` | playback (parked; default = newest clip) |

### 10.3 Measurement and debug (read-only unless noted)
| Command | Output |
|---|---|
| `sensor` | driver view (fps, mode list) + VMAX/HMAX/SHS1/FRSEL registers with computed fps/exposure + VIF-measured fps |
| `fps N` | raw `MI_SNR_SetFps(0,N)` with before/after dump (changes timing; debug) |
| `reg ADDR [COUNT]` | read 1..64 sensor registers |
| `padinfo` | raw MI_SNR_GetPadInfo (MIPI receiver words) |
| `meter` | AE avgY, target, LV, BV, stable, EV |
| `aeinfo` | AE limits, converge speeds, state, flicker, target table |
| `aetrace [N]` | avgY/shutter/gains every 50 ms |
| `aestep [THIRDS]` | EV step response trace (changes EV comp temporarily) |
| `aeconv IDX VAL` / `aelim IDX VAL` / `aestate 0|1` | poke one AE converge word / limit word / pause AE (debug writes) |
| `wgt` | AE weight tables |
| `histo` | compare ISP histogram sources |
| `iq` `gamma` `shp` `ccm` `hsv` `nr` `nr3d` `wdr` `wbinfo` `dpdump` `nradump` | struct dumps ([§13](internals.md)) |
| `jpgavg` | average JPEG frame size over 3 s (noise proxy used for every NR/sharpness experiment) |
| `jpegparam [Q]` | dump JPEG param (optionally set word 0) |
| `vui` | raw H.265/H.264 VUI of the recording encoder |
| `nr3d_exp N` `wdr_exp N` `awb_exp M [A]` `nrl_exp N` `nra_exp N` `nr_exp N` `shp_exp N` | field experiments from development (write ISP structs; restart to undo) |
| `h265test [Mbps]` | 10 s H.265 test on VENC dev 0 next to JPEG, first 400 KB to /tmp/h265test.h265 |
| `raw_probe` | one VIF output buffer: BufInfo + first bytes |
| `pk_probe` | one SCL port 3 buffer: BufInfo |
| `dng [x y w h]` | raw frame/region to /tmp/test.dng |
| `dngview` | raw developed to /tmp/dngview.ppm (160x90) |
| `viewgrab` | processed picture to /tmp/view.ppm (160x90) |
