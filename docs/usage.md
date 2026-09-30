# Using the camera

Modes, recording, photos, exposure, colour, assists, the OSD, the menu and every button.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 9), the complete project reference. Section numbers match it.

## 9. Feature reference (what the user sees and does)

### 9.1 Camera modes (Main menu › Mode; FIFO `mode 0..3`)
Picking a mode and confirming it (OK, or remote play) saves the running mode's keys and loads the target mode's keys, then restarts camapp. The mode keys are `sensor_fps rec_fps play_fps tl_interval_ms tl_limit_frames crop_w crop_h manual ae_priority shutter_us shutter_angle_x10 sensor_gain ev_thirds rec_codec rec_mbps`. On the first visit to a mode the current values carry over. Everything else is shared. The switch is refused while recording.

| Mode | Clip prefix | What it does |
|---|---|---|
| Normal | `NORM_` | frame rate 24/25/30/48/50/60/72/75 (crop: up to 105p at 1280 wide, list continues 90/100/120 when the crop allows); Record fps 30 = every 2nd frame when the sensor runs 60 |
| HDR | `HDR_` | sensor DOL-HDR, 24/25/30 only, no crop, no DNG; status bar shows "HDR" |
| Slow-mo | `SLOW_` | capture any whole rate 6..max (max = crop ceiling), every frame kept, clip stamped to play at "Plays at" (5 up to capture − 1) in PTS, AVI header and H.265/H.264 VUI; sidecar `speed=N.NNx slower`. Use H.265: MJPEG drops frames above 60p |
| Timelapse | `TLAP_` | one frame every Interval (0.1 s .. 60 min, MJPEG only), plays at 5..60, optional Stop after N frames (auto stop, "Timelapse done: N frames"); manual shutter may reach the interval, max 2.1 s |

### 9.2 Recording
- Start/stop: the remote record button, a body MENU triple click, or FIFO `rec_toggle` / `rec 1 [fps]` / `rec 0`. Recording refuses to start without a mounted, verified SD card.
- Codec (Recording › Codec): H.265 in `.ts` (default), H.264 in `.ts`, or MJPEG in `.avi`.
  - `.ts` is crash-safe: every packet stands alone, so a power cut loses at most the last moment. Power-cut tests played to the cut point.
  - To remux for editors (DaVinci Resolve free does not like `.ts`): `ffmpeg -i X.ts -c copy -tag:v hvc1 X.mov`.
- Bitrate 2-100 Mbps (menu 10-100), default 40.
  - 40 Mbps is a good 1080p60 quality. 100 Mbps = 12.5 MB/s, borderline for the card.
  - Encoder page: GOP (0 = one second of capture), CBR/VBR, VBR QP range (defaults 48/12), JPEG quality (default 70).
- File names: `/app/bin/sd/<PREFIX>_<nnnn>.<ts|avi>`. One counter is shared across all prefixes and extensions, including old `REC_` files. The next number is shown as `STBY #nnnn`.
- Split: `.ts` at 1.9 GiB (32-bit offsets) at a keyframe; `.avi` at 1000 MiB (AVI 1.0). FAT32 limits any file to 4 GB anyway.
- Queue: the encoder threads copy each frame into a queue of up to 240 frames or 20 MB (`REC_QUEUE`, `REC_MEM_MAX`). Each frame gets a buffer of its own size, up to 6 MB (keyframes at 24p/40 Mbps exceed 2 MB). The `recwrite` thread writes them to the card.
  - A full queue drops the frame. Every drop is logged with its reason and size.
  - Status bar: `BUF n%` while the queue is over half full, `DROP n` once frames are lost.
- Sidecar `<clip>.txt` (key=value), written when the file opens (`status=recording`) and rewritten at close. Keys:
  - `file`, `camera` (build date, sensor), `resolution`, `open_gate` / `sensor_crop`, `mode`, `codec`, `bitrate_mbps`.
  - `file_fps`, `capture_fps`, `speed` (Slow-mo/Timelapse), `interval`.
  - `tc_start`, `tc_end`, `frames`, `duration_s`, `dropped`.
  - `exposure`, `shutter` (with angle), `gain` (dB and x), `ev_comp`, `metering`, `white_balance`, `gamma`.
  - `look_slot`, `temporal_nr`, `wdr`, `mirror`, `flip`.
- Record-run timecode: it runs only while recording, at each clip's file rate, and continues across clips and power-offs (`tc_run_ms` in the settings). It is shown in the bottom-left corner as `TC hh:mm:ss:ff` and written to the sidecar. System › Timecode resets it (OK twice).
- Tally: a red 6 px frame around the OSD while recording, `* REC mm:ss #nnnn` in red, and the LED blinks at 0.5 Hz.
- Time left on the card: shown after the free space in the status bar, from the measured rate while recording, or a smoothed estimate in standby.
- Measured: H.265 at 75p and 100p crop, 0 dropped. MJPEG 60p needs ~8.9 MB/s at q70 and drops frames (card ~8 MB/s through FAT while the pipeline runs). MJPEG 30p is clean.

### 9.3 Photos
- Remote photo button, body MENU single click (fires at once on release), or FIFO `snap`. The JPEG comes from the JPEG encoder (1920x1080, current quality) and is saved as `/app/bin/sd/IMG_nnnn.jpg` (first free number).
- Feedback: a white 6 px border for 150 ms, the LED off-blink, and the notice `IMG_nnnn saved` (+ ` + DNG`). Errors in red: `PHOTO FAILED: no SD card` / `write error`. Without a card the photo goes to `/tmp/snap.jpg`.
- Recording › Photo = `JPEG + DNG` also writes `IMG_nnnn.DNG` (raw 10-bit, ~4 MB, [§8.6](internals.md)); not in HDR.
- FIFO `dng [x y w h]` writes a raw frame or region to `/tmp/test.dng` for tests.

### 9.4 Exposure (Exposure page)
- **Auto/Manual** row cycles the four modes (the remote AE button does the same, FIFO `prio 0..3`):
  - **P**: auto shutter and gain.
  - **S** (shutter priority): the user's shutter; AE moves only the gain.
  - **G** (gain priority): the user's gain; AE moves only the shutter.
  - **M**: both manual.
  
  S and G are made by pinning the AE ExposureLimit: S pins min/max shutter; G pins min/max sensor gain and ISP gain 1x. EV comp works in P, S and G. There is no aperture mode (no iris).
- **Shutter by**: Speed (1/x) or Angle (degrees; kept when the frame rate changes).
  - Speed arrows step through the camera scale 1/24, 25, 30, 40, 48, 50, 60, 80, 100, 120, 125, 160, 200 … 64000. The remote zoom keys fine-step (1/60 → 1/61).
  - Angle: arrows 1°, zoom 0.1° (e.g. 172.8°), 360 = the whole frame. Play resets to 180°.
  - The limit is 97 % of the frame; in Timelapse, up to the interval (max 2.1 s). Status shows `1/x` or seconds from 0.4 s, plus the angle.
- **Gain**: half-stop grid 1x (0 dB) to 16x (+24 dB) in the menu (1024 = 1x internally). FIFO `gain N` takes any value; 32x works through the limit. The status bar shows the total gain (sensor × ISP), with ` d` when more than 5 % of it is ISP digital gain.
- **EV comp**: ±3 EV in 1/3 steps (remote zoom +/− outside the menu in P/S/G; FIFO `evcomp N` in thirds, `ev0` resets). Log gamma adds −2 EV internally.
- **Metering**: AVG (camapp's uniform table), CTR (the vendor centre-weighted table) or SPOT (the remote + key cycles them).
  - The spot is S × 2S blocks of the 16x16 AE grid (S = Spot size 2/4/6/8, square on screen). It is drawn as a yellow box and moved with the arrows outside the menu. It keeps its centre when resized; FIFO `spot_centre` recentres it. Spot size 2 = 240x270 sensor pixels (12 % of the width).
  - Semantics: the meter renders the metered area as mid grey.
- **AE lock**: remote contrast button or the menu row (auto only). `AE-L` in the status bar. Not saved.
- **AE speed**: 5 %, 10 %, 20 %, 30 %, 50 %, 75 %, Vendor, Instant (= vendor/2, no overshoot), 150 %, 200 %, 300 %. Above 100 % it overshoots and hunts (the sensor applies exposure 2-3 frames late).
- **Anti-flicker**: Vendor (50 Hz), Off, 50 Hz, 60 Hz (`MI_ISP_AE_SetFlicker` 2/0/2/1; the mapping was verified by read-back).
- **Max shutter** (Auto = one frame, shown as `Auto (1/fps)`, or a shorter 1/x limit) and **Max gain** (Vendor 8x, or 1x … 32x): the AE limits. Vendor max shutter was 1/200 real, which is why stock went dark in dim light.

### 9.5 Colour and image (Colour page, Colour › Image & looks)
- **White balance**: Auto, Kelvin (2000-10000 K, step 100, default 5600), Manual gains (R/G/B 0.25-8x in 16/1024 steps). **Set from scene** grabs the current gains into Manual (a persistent WB lock). The remote AWB button toggles lock/auto. The corner block shows `AWB 5600K` (the live AWB temperature), `WB 3200K` or `WB M`.
- **Image & looks**:
  - **Look slot** 1-5, **Load look**, **Save look** (OK twice), **Export looks** (all slots to `LOOKS/LOOK1..5.TXT` on the SD card), **Import looks** (OK twice, image keys only, values clamped; format in docs/looks.md).
  - Brightness, Contrast, Lightness, Saturation: Auto (vendor tuning) or 0-100.
  - Gamma: Auto, Log (experimental, S-Log3-like, AE −2 EV, grade in post), 1.0-3.0 (Rec.709-style toe).
  - Sharpness: Auto, Off, 10-400 % of vendor.
  - Colour matrix strength: Auto, 0-200 % (0 = sensor native).
  - Black level: offset in sensor codes.
  - Black & white.
  - Sub-pages: Matrix edit, Hue tweaks, Noise & clean-up.
  - Reset page (OK twice).
  - Play or a long OK on a number row sets Auto/Default.
- **Matrix edit**: Vendor (live matrix, read-only) or Custom (9 coefficients, 1.000 = 1024, step 8/1024). Copy current takes the vendor matrix for the current light. The ISP forces each row to sum to 1.0, so the diagonal follows the other two coefficients and is shown "(auto)". Reset page.
- **Hue tweaks**: hue shift (±128 ISP units) and saturation (0-300 %) for the red, yellow, green, cyan, blue and magenta bands, blended with triangle weights over neighbouring points. Reset page.
- **Noise & clean-up**: Luma NR (Auto/Off/10-400 % of the NRLuma_Adv thresholds), Chroma NR, De-spike and False colour (Auto/Off/On each), Defog (Off/5-100). Reset page.
- **Expert › Temporal NR**: the NR3D motion gain, Off / 125…16000 (vendor 1000). Higher is stronger with more ghosting. **Expert › WDR**: Auto (vendor table) / Off (the vendor boot state) / 8…255 manual strength.
- A look = every image key (`isp_*`, `ccm_*`, `hsv_*`, `nr_*`, `defog`, `wdr`, `nr3d_gain`). Exposure, WB, frame rate and recording settings are never part of a look.

### 9.6 Assists (Assist page) and the live buttons
- **Magnify / loupe**: display only, 1.2x to 8x.
  - Remote mirror = on/off; zoom +/− = ±0.5x; arrows pan by 1/8 of the view (they move the picture).
  - A position map shows in the bottom left, and `LOUPE n.nx` in the status bar. Not saved.
- **Peaking**: 1-10 (higher = more edges); Peak colour red/yellow/white/blue. The remote flip button toggles it (default level 5).
- **Zebra**: stripes at or above 0-100 % of full scale (leaving Off starts at 95 %).
- **False colour**: ARRI-like (<3 % purple, <10 % blue, 38-42 % green = mid grey, 52-56 % pink = skin, 97-98 % yellow, ≥99 % red).
- **Waveform**: Off / Luma / RGB overlay / RGB parade, 240x100 at the bottom centre.
- **Histogram** of the displayed luma, 256 columns, red marker when >0.1 % of the samples are at 255. The remote freeze button toggles it. With both panels on, the waveform sits left of centre and the histogram right; a lone panel is centred.
- **Guides**: Off / Thirds / Cross / Both (the remote gallery button cycles them).
- **Frame**: Off / 2.39 / 2.00 / 1.85 / 1.66 / 4:3 / 1:1 / 9:16 frame lines.
- Guides run under the status bar and corner text; menu and panels cover them. The analysis loop runs at ~15 updates/s only while peaking, zebra, false colour, histogram or waveform is on.
- **OSD hide**: body MENU double click or FIFO `osd_toggle` hides the whole fb layer for a clean HDMI picture; opening the menu brings it back. No remote button is assigned.
- Live buttons outside the menu, in this precedence:
  1. The loupe (zoom, pan) while it is on.
  2. In M/S the shutter: remote ◀/▶ and body AE/OK.
  3. In M/G the gain: ▲/▼ on the remote or the body.
  4. In M the remote zoom keys fine-step the shutter.
  5. With spot metering the arrows move the spot.
  6. In P/S/G the remote zoom keys change EV comp.

### 9.7 OSD layout (1600x900 framebuffer, scaled 1.2x to HDMI)
- **Status bar** (top):
  - Left: `STBY #nnnn`, or red `* REC mm:ss #nnnn` (+ ` BUF n%` / ` DROP n`; Timelapse adds `Nfr=x.xs`), or `PLAY`.
  - Centre: `<AUTO|S|G|M|AE-L>[ ±ev] <shutter>[ angle°]  <gain>x[ d]  <AVG|CTR|SPOT> EV ±x.x  LV y.y`. It turns yellow when |EV| > 1 or the meter saturates (`EV >+x.x`).
  - Right: `<rate>[ HDR][ LOUPE n.nx][ MIR][ FLIP]  <SD free>G[ time left]` or `NO SD`. The rate is `60p`, `100p>25` (Slow-mo capture > play) or `TL 1.0s>25`.
- **Bottom-left corner**: codec (`H265 40M` / `H264 40M` / `MJPG`), WB, `TC hh:mm:ss:ff`.
- **Bottom-right corner**: `TNR <gain|off> NR <auto|off|n%>`, `WDR <auto|off|n|->` + AE speed when not vendor, and `SoC n°C` when the SoC is at 80 °C or more (the status turns yellow).
- **Notices** under the status bar (2 s): photo saved, WB locked, AE locked, SD state, mode messages.
- **Menu**: a panel at (40,70), 470 px wide, 40 px rows, at most 14 visible (▲/▼ scroll arrows). Amber selection bar; each section has its accent colour on the title and left stripe (Recording red, Exposure yellow, Colour green, Assist cyan, Expert magenta, System grey, Main amber). The help panel (500 px) sits to the right.

### 9.8 Menu tree
```
MAIN: Mode | Recording > | Exposure > | Colour > | Assist > | Expert > | System >
  RECORDING: Frame rate, Crop width, Crop height, Interval*, Stop after*, Plays at*, Record fps*,
             Codec, Bitrate*, Photo (JPEG / JPEG + DNG), Back
  EXPOSURE:  Auto/Manual (P/S/G/M), Shutter by*, Shutter*, Gain*, EV comp*, Metering, Spot size*,
             AE lock*, AE speed*, Anti-flicker*, Max shutter*, Max gain*, Back
  COLOUR:    White bal., Kelvin*, Red/Green/Blue gain*, Set from scene*, Image & looks >, Back
    IMAGE & LOOKS: Look slot, Load look, Save look, Export looks, Import looks, Brightness, Contrast,
             Lightness, Gamma, Sharpness, Saturation, Colour matrix, Black level, Black & white,
             Matrix edit >, Hue tweaks >, Noise & clean-up >, Reset page, Back
      MATRIX: Matrix (Vendor/Custom), Copy current, R from R/G/B, G from R/G/B, B from R/G/B, Reset page, Back
      HUE:    Red/Yellow/Green/Cyan/Blue/Magenta hue + sat (12 rows), Reset page, Back
      NOISE:  Luma NR, Chroma NR, De-spike, False colour, Defog, Reset page, Back
  ASSIST:    Magnify, Peaking, Peak colour*, Zebra, False colour, Waveform, Histogram, Guides, Frame, Back
  EXPERT:    Temporal NR, WDR, Real fps, Frame length (VMAX), Line length (HMAX), Exposure (lines/us),
             Sensor gain (reg 0x3014), SoC temp, Registers >, Encoder >, Back
    REGISTERS: Address (steps 8, 0x3000-0x34F8), 8 register rows (name, value, bits; left/right edit
             when writable), Write access Safe/All (All needs OK twice), Undo writes, Back
    ENCODER: JPEG quality, H.265 GOP, Rate control (CBR/VBR), Max QP, Min QP, Back
  SYSTEM:    Mirror, Flip, SD card (free/size), Format SD (OK twice), Build, Uptime, CPU, RAM, Flash,
             Timecode (reset OK twice), Save logs, Reset settings (OK twice), Back
(* = shown only when relevant: mode, exposure mode, WB mode, feature on)
```
- Keys in the menu:
  - MENU closes the menu and remembers the page and row; MENU again reopens there.
  - Up/down select.
  - Left/right change the value (body AE = left, body OK = right). The remote zoom −/+ make fine steps.
  - Remote play/pause = reset the row to Auto/Default, or confirm "Press OK" / "OK twice" rows. Body OK held 0.8 s = reset.
  - Remote mirror = back one level.
  - Moving off an armed "OK twice" row cancels it.
- Locked while recording: Mode, Frame rate, Crop, Plays at, Record fps, Codec, Bitrate, the Encoder page (GOP, rate control, QP), Timecode reset and Format SD.
- Crop rows: arrows step 32/18 px, the remote zoom 8/2 px. The rows show the running size until OK/play applies both (restart). Leaving the rows puts the running size back.
- Registers page: Safe allows 0x3001 REGHOLD, 0x300A/0x300B BLKLEVEL, 0x3014 GAIN and 0x308C PGCTRL (test pattern). All allows anything. Writes are volatile: a restart re-inits the sensor. Undo restores newest first (128 slots). The help text explains each register (`sensor_reg_desc()`); the full guide is docs/imx307_registers.md. Fun test: `regw 0x308c 0x11` (test pattern 1 = fine vertical stripes); `regw 0x308c 0x21` gives colour bars.

### 9.9 Body buttons and remote (full map)
| Input | Outside the menu | In the menu |
|---|---|---|
| Body MENU: 1 click | photo (at once, on release) | close menu |
| Body MENU: hold 0.8 s | open menu | close menu |
| Body MENU: 2 clicks | OSD hide/show (fires 450 ms after the 2nd click) | — |
| Body MENU: 3 clicks | record start/stop (press 1 also took a photo: accepted) | — |
| Body UP / DOWN | gain in M/G, else spot up/down in SPOT | select row |
| Body AE / OK | shutter −/+ in M/S | value −/+ ; OK held 0.8 s = reset |
| Remote zoom+ / zoom− | EV ±1/3 (P/S/G), fine shutter (M), loupe zoom | fine step |
| Remote ▲ ▼ ◀ ▶ | loupe pan / shutter (◀▶) and gain (▲▼) in M/S/G / spot move | select / change |
| Remote MENU | open menu | close menu (remembers) |
| Remote mirror | loupe on/off | back one level |
| Remote AE | exposure mode P → S → G → M | — |
| Remote flip | peaking on/off | — |
| Remote + | metering AVG → CTR → SPOT | — |
| Remote AWB | WB lock (grab gains) / back to auto | — |
| Remote contrast | AE lock toggle | — |
| Remote freeze | histogram on/off | — |
| Remote play/pause | (playback: pause) | reset row / confirm |
| Remote photo | photo | photo |
| Remote record | record start/stop | record start/stop |
| Remote gallery | guides Off → Thirds → Cross → Both | — |

- Remote repeats: a scancode after a gap of more than 120 ms (or a different code) is a new press. Holding repeats only navigation and stepping actions: after 400 ms, one step per NEC repeat frame (~108 ms), 4 steps per frame after 2 s.
- The body keys have no kernel autorepeat. The input thread repeats them every 110 ms with the same delay and acceleration.
- Record and photo work with the menu open; other function buttons are ignored while it is open.

### 9.10 System page
- Mirror and Flip: sensor readout direction (`MI_SNR_SetOrien`, or register 0x3007 in crop mode), instant and saved.
- SD card: free / size. Format SD: [§8.12](internals.md).
- Build: the compile date and time (also the first log line, `camapp build ...`). Uptime.
- CPU: both cores over the last second. RAM: available of total, plus camapp's own use (video buffers live in the ~170 MB MMA heap). Flash: free space on `/customer`.
- Timecode reset.
- Save logs: copies `/tmp/camapp.log` and `dmesg` to `LOGS/LOG_nnnn_camapp.txt` and `LOGS/LOG_nnnn_dmesg.txt` on the SD card.
- Reset settings (OK twice): `settings_reset()` + re-apply everything.
