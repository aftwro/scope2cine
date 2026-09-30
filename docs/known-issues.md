# Known issues and roadmap

Open bugs, parked work with its exact state, and the final roadmap.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 15, 16), the complete project reference. Section numbers match it.

## 15. Known bugs, limits, oddities and parked work

### 15.1 OSD into the USB/recorded picture (MI_RGN): parked after 7 attempts
Goal: burn the OSD into the UVC picture (optionally), via a hardware overlay region on the SCL port that feeds JPEG.

Facts established:
- The layouts are in [§13](internals.md) and in the `rgn.c` header.
- The SSD220 doc calls sequence: `InitDev({palette*})`, `Create`, `AttachToChn`, `GetCanvasInfo`, draw, `UpdateCanvas`.
- Formats: 0 ARGB1555, 1 ARGB4444, 2 I2, 3 I4, 4 I8, 5 RGB565, 6 ARGB8888. Width is 8-aligned, height 2-aligned.
- Tiramisu has OSD on ports 0, 1, 2, 5, 6 + DIVP + LDC, with 6 layers.
- cardv calls AttachToChn at 0x37394 and 0x37f12.
- cardv's ini attaches its OSD to SCL dev 0 chn 0 **port 0** (`[OSD] Attach0Port = 0`; its VENCs bind port 1). The stock boot trace shows only `MI_RGN_InitDev`, so there is no working reference.

Attempts (FIFO `rgn_test N`: a 256x64 red box at 100,100):
1. Plain attach on port 1: all rc 0, and `/proc/mi_modules/mi_rgn/mi_rgn0` shows region 0 attached to Mod 34 dev 0 chn 0 port 1, bShow 1, FgAlpha 255. No box in the UVC JPEG.
2. With `setRealtimeFlip 34 0 0 1 1`: no box.
3. SCL port 1 disable/enable after attach: no box.
4. cardv alpha 0/0: no box.

Root cause of one real bug found on the way: the kernel logged `_mi_rgn_drv_window_blitosd_front_buffer: Front buf state error ... read the note of api MI_RGN_UpdateCanvas`. GetCanvasInfo and UpdateCanvas must be a pair with nothing (no attach) between them. This was fixed with `rgn_begin`/`rgn_end`, and the error went away.

After the fix:

5. Port 1: still no box.
6. Port 0: an SCL port-0 frame dump showed Y 6-8 in the box area, so no box there either.
7. cardv's exact order (create, get, draw, update, then attach + realtime flip) on port 1: all rc 0, no kernel error, still no box.

Ideas not tried:
- SCL port param bit for GOP blending (`scl_port1` word 3 = 0x101).
- The SCL "UseSclId" hardware-scaler mapping: cardv ini `Scl0DevUseSclId = 0x03` vs our hardware port mask 15.
- ARGB4444 / I4 formats.
- Attaching to DIVP.

The code stays in `rgn.c` (FIFO only; nothing runs by default).

### 15.2 HDMI output modes: parked
- The user's LG FHD EDID: native 1920x1080@100 (228.8 MHz reduced blanking), 1080p75, 1080p60/50, 720p60/50, 576p, 480p, max 250 MHz.
- The HDMI TX has a fixed list: 480p60, 576p50, 720p50/60, 1080p24/25/30/50/60, 1440p30/50/60, 4K25/30 and PC modes. There is no 75 or 100 Hz. (`echo "Timing list" > /proc/mi_modules/mi_hdmi/mi_hdmi0`.)
- cardv's `astTimingArray` gives (name, disp enum, hdmi id): 720p60 9/5, 720p50 8/4, 1080p60 16/9, 1080p50 15/7, 1080p30 12/10, 1080p25 11/11, 1080p24 10/12.
- The implementation is in `git stash@{0}` and `archive/scratchpad/hdmi_output_modes_stash.patch`:
  - setting `hdmi_out`, SCL port 0 at the output size, layer/port/timing set;
  - System row "HDMI output"; FIFO `hdmiout N` (restarts) and `edid`.
  
  Apply it with `git stash apply` or `git apply` on top of `c65e10b`.
- Result: 720p60 and 1080p50 both give **no signal**. The disp/hdmi procs report the new timing, but the display engine never runs ("inputtask's fence is not finished", mi_fb timing 0x0), also after a cold boot. Any timing other than the boot 1080p60 fails. It probably needs a display clock setup that the library does not do from the enum.
- Low value: most monitors scale 1080p60 themselves.
- Warning: a stuck display floods the UART console, which breaks `uart_push` md5 checks. Restore `hdmiout 0` first, then deploy.

### 15.3 Full-width 120p: parked
[§8.3](internals.md). It needs the SoC MIPI receiver configured for 891 Mbps/lane. The driver never does that; `padinfo` dumps the receiver words for a start.

### 15.4 Playback and gallery: parked
- Stage 1 works (FIFO `play`, pause, seek ±5 s, stop back to live). There is no gallery page, and no button starts playback (the gallery button cycles the guides).
- Blockers:
  - The bottom smear on 1080-high H.265 ([§8.11](internals.md)); the 1088 + SPS conformance window fix was reverted and never redone.
  - 60p plays at ~0.55x.
  - A killed decoder breaks VDEC.
- Seen in 2026-09-25 notes, not re-checked: recording could start while playback was shown.

### 15.5 Hot pixels: investigated, no ISP fix
- The vendor dynamic defect-pixel correction is on (auto) by default. FIFO `dp 0` vendor / `1` off / `2+` manual slope; `dpdump` dumps it.
- Dark-frame test, 2026-09-30 (lens covered, Timelapse 2 s, 16x gain, counting JPEG spikes):

  | DP setting | Spikes |
  |---|---|
  | vendor | 176 / 73 |
  | off | 147 |
  | manual slope 4 | 59 / 150 |
  | slope 1 | 69 |
  | slope 64 | 72 |

  The frame-to-frame swing dominates, so the DP setting has no consistent effect. Hot pixels at long exposure flicker (dark current).
- The real fix would be dark-frame subtraction on raw frames (idea).

### 15.6 Small bugs and oddities (open)
- **`R30` marker never shows.** In `update_osd` (camapp.c, around line 983), `rate` is written with ` R30` when recording decimates 60→30, but the following if/else always overwrites `rate`. Display only; the recording itself is right.
- **32x gain**: register 0x3014 stays at 80 (24 dB) above 16x, but the picture doubles again. The driver probably switches HCG (0x3009 bit 4). Unverified.
- **FIFO numbers are parsed unsigned** (`%lu`). Negative values like `wdr_level -1` work only because the 32-bit wrap turns ULONG_MAX back into −1 on this ARM target. Commands that re-parse the line (`evcomp`, `isp`, `wb`) are fine.
- **Body MENU double/triple click**: the first press already took a photo (accepted by the user: photo without delay).
- **EV meter** uses the AE statistics (linear, before gamma/WDR), so with Log gamma or strong WDR the EV number does not describe the displayed picture.
- **Spatial luma NR at 50 %** is noisier than Off (low thresholds add structure), as with temporal NR presets. Measured, not understood.
- **Windows UVC re-enumeration** occasionally stuck (udc state "default", full-speed): a camera reboot fixes it.
- **WB menu UX** felt "a bit weird" to the user (Auto / Kelvin / Manual + Set from scene + remote lock = three concepts). An idea for a camera-style flow was never done.
- **Stock cardv** cannot restart after camapp has run in the same boot: reboot instead.
- **DNG** with mirror/flip on or with a crop was never verified; opening in Lightroom/Resolve was left to the user.
- **UVC** exposes only 2 controls ([§8.9](internals.md)).
- **SD UHS** 1.8 V switch fails: a board limit ([§2.2](hardware.md)).

### 15.7 Blocked or not started
- **USB mass storage (MSC)**: impossible on this firmware. There is no `usb_f_mass_storage` module or configfs function (`usb-storage.ko` is the host side). 17 other function types were tested: none exist; the firmware has only `usb_f_uvc`.
- **Own gadget modules** (28f): building ECM/RNDIS (network: SD over network, telnetd, remote control), mass storage and ACM serial from Linux 4.9.227 source (no modversions; vermagic above). Risk: a struct mismatch crashes the kernel (a reboot recovers).
- **Custom kernel**: the blocker is ABI, not size. All MI/sensor/HDMI modules are prebuilt for 4.9.227, and no Mercury6 4.9.227 source was found (public SigmaStar vendor kernels are 4.9.84). HDMI only comes up when the modules load.
- **Console on HDMI**: needs a kernel with fbcon. An in-app console overlay replaying dmesg was an idea.
- **USB host SSD recording**: the EHCI host controllers exist; whether a port is wired needs a physical check.
- **RAM pre-trigger buffer (Phantom style)**: designed, not built. JPEG ~100-150 KB/frame gives ~1000 frames in ~150 MB of MMA (~8 s at 120 fps). Trigger, pre/post split, review, trim, save.

---

## 16. Final roadmap table (corrected; supersedes the table in docs/notes/COMPACT.md)

Corrections against docs/notes/COMPACT.md:
- 22c is skipped by the user, not open.
- 27 is done (the user confirmed the System page on screen).
- The "NEXT: OSD INJECTION" section header is stale (28a is parked).
- The helper tools now live in `tools/` (brio.sh, hset.sh and hsweep*.sh are in `archive/scratchpad/`).
- The UART tools now use gzip/stream.
- Rows 16 and 37 and the last ideas were missing.

| # | Area | Item | Status |
|---|---|---|---|
| 1-3 | Foundation | flash backup + recovery, own pipeline, full exposure control | done |
| 1b | Input | body MENU gestures (1 photo, hold menu, 2 OSD hide, 3 record) | done |
| 3b | Exposure | P / S / G / M priority modes | done |
| 4 | Exposure | resizable spot 2/4/6/8, centred | done |
| 5 | Exposure | manual exposure in HDR; anti-flicker mapping | done |
| 6-8 | Colour / image / sensor | WB, CCM (vendor + custom), HSV, noise, TNR, WDR, DOL-HDR, register viewer/writer | done |
| 9-11 | Recording | H.265 / H.264 / MJPEG, 240-frame / 20 MB queue, time left, tally, BUF/DROP | done |
| 12 | Looks | 5 slots, SD export/import, Log gamma | done |
| 13-15 | Aids | loupe, peaking, zebra, false colour, histogram, waveform | done |
| 14 | Aids | focus meter from AF stats | removed ("pretty bad") |
| 16 | Aids | animated zebra; histogram of the loupe area only | idea |
| 17 | OSD | sections menu, accent colours, help panel, MENU remembers, mirror = back | done |
| 18 | OSD | frame guides under the status text | done (look could be polished: shade-style guides = idea) |
| 19-20 | Playback | play/seek, gallery, photo viewer | parked ([§15.4](known-issues.md)) |
| 21 | Modes | Normal / HDR / Slow-mo / Timelapse | done |
| 22 / 22b | Modes | sensor crop up to 105p at 1280; open gate | done |
| 22c | Speed | above 120 fps at narrow crops | skipped by the user |
| 22d | Speed | full-width 120p | parked ([§15.3](known-issues.md)) |
| 23 | Slow-mo | RAM pre-trigger ring buffer | open (optional) |
| 24 | Timelapse | MJPEG, interval, stop after, shutter to 2.1 s | done |
| 24b | Timelapse | shutter beyond 2.1 s, H.265 timelapse | skipped |
| 25 | Metadata | sidecar, record-run timecode, clip names, crop in sidecar | done |
| 26 | Encoder | JPEG quality, GOP, CBR/VBR + QP, H.264, slow-mo VUI | done |
| 27 | Hardware | System page CPU/RAM/flash/SD, save logs | done |
| 28 | USB | own configfs UVC gadget 1080p 30/60, paced, reconnect | done |
| 28a | USB | OSD injection into UVC (MI_RGN) | parked ([§15.1](known-issues.md)) |
| 28b | USB | record to PC over USB | idea |
| 28c | OSD | OSD hide (body double click, FIFO); no remote button assigned | done |
| 28d | USB | mass storage | blocked |
| 28e | USB | UVC controls: AE mode + brightness | done (2 controls) |
| 28f | USB | own gadget modules (network, MSC, serial) | experimental, not started |
| 28g | USB | host mode + SSD | investigate (physical) |
| 29 | Host control | USB network control, remote preview | planned, needs 28f |
| 30 | HDMI | output modes | parked ([§15.2](known-issues.md)) |
| 31 | Performance | scopes 15 Hz, capped recomposite, named threads | done |
| 32-33 | Firmware | own image / OTA (U-Boot SD upgrade path); custom kernel | planned / blocked |
| 34 | RAW | DNG photo | done (RAW video removed) |
| 35-36 | Tools | flicker meter, slit-scan / photo finish; stabilisation | idea |
| 37 | Hardware (user) | C-mount lens mount + IR-cut, 32 MB NOR, microphone | user's plan |
| 38 | Oddities | hot pixels; 32x gain | investigated, no ISP fix |
| 39-41 | Looks | replicate other cameras, full profiles, PC tuning files | idea / parked |
| — | Ideas | dark-frame subtraction for night timelapse; shade-style frame guides; GPIO hunt page; console overlay; WB flow rework; persistent log in the KMSGLOG flash area; HCG check | ideas |
