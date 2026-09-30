# FINAL HANDOFF: scope2cine (camapp), the AliExpress "microscope" turned cinema camera

Written 2026-09-30, the day the project was declared done. This is the self-contained reference for picking the project up again (a bug fix, a new feature, a recovery). It describes the final state of the hardware, the firmware, the code, the tools and every known quirk.

Other documents in the repository and how they relate to this one:
- `docs/notes/COMPACT.md`: the working state document used during development (resume block, roadmap, key findings). It was authoritative until now. A few of its rows are stale; §16 lists the corrections.
- `docs/notes/handoff-log.md`: frozen chronological log (2026-09-16 to 2026-09-26) with every experiment, dead end, struct dump and measurement. Use it for archaeology; do not act on its "next steps".
- `docs/imx307_registers.md` (sensor register guide), `docs/lenses.md` (lens guide), `docs/looks.md` (look file format).
- `archive/scratchpad/`: every scratch file of the development sessions that still exists or could be rebuilt (§19.3).

Contents:
0. Quick start
1. What the project is, final state
2. Hardware
3. Access, backup and recovery
4. The stock firmware (what camapp replaced)
5. Boot integration and on-device files
6. Development environment on the PC
7. camapp architecture
8. Pipeline, sensor and driver facts (the hard-won knowledge)
9. Feature reference (what the user sees)
10. FIFO command reference
11. Settings file reference
12. Device paths reference
13. ISP / 3A / encoder struct layouts
14. Measurements and performance
15. Known bugs, limits, oddities and parked work
16. Final roadmap table (corrected)
17. Debugging playbook
18. Working rules and lessons learned
19. Repository map, logs index, scratchpad dump index
20. Timeline

---

## 0. Quick start

1. The camera boots into `camapp` on its own. HDMI shows the live picture with the OSD at 1080p60. USB is a UVC webcam (1920x1080 MJPEG, 30 or 60 fps).
2. To talk to it, use the UART at `/dev/ttyUSB0`, 115200 baud, from WSL: `python3 tools/sh.py "" "your command"`. Output often arrives late: re-read with `python3 tools/sh.py "" "" "true"`. Only one program may hold the port at a time.
3. camapp's log is `/tmp/camapp.log` on the camera (RAM, lost at power-off). **System › Save logs** copies it and `dmesg` to the SD card.
4. Commands go to `/tmp/camapp_fifo`: `echo "framerate 50" > /tmp/camapp_fifo`. Full list in §10.
5. To build and install: `tools/deploy.sh`. It cross-compiles, checks symbols against the device libraries, pushes over UART and installs with automatic rollback. Success output: `update: OK, new build running` and `UPDATE_RC=0`.
6. Settings live in `/customer/camapp/settings` (key=value, §11). Deleting the file gives defaults on the next start.
7. To get the stock firmware back: `rm /customer/camapp/enable` then reboot (stock cardv through the wrapper), or `rm /config/cardv` for the exact stock boot.
8. If the camera is bricked: the golden 16 MB dump `microscope_C22018_0xE41B1D54.bin` plus the CH341A programmer and SOP8 clip restore everything (§3).
9. Before changing anything, read §18 (working rules). The short version: one small step at a time; build, deploy, verify on the device, commit; prove that frames advance; stop after two failed hypotheses.
10. The repository is `/home/aftwr/microscopeproject` (git, no remote). Branch `master` holds the full development history; the GitHub branch `main` is one clean commit of the final tree. Last code commit: `c65e10b`. `git stash@{0}` holds the parked HDMI output-mode code (also saved as `archive/scratchpad/hdmi_output_modes_stash.patch`).

---

## 1. What the project is, final state

### 1.1 Goal
A cheap AliExpress USB/HDMI "microscope" camera is really a SigmaStar Mercury6 dashcam board with a Sony IMX307 sensor. The goal was to turn it into a hackable cinema camera with "unmatched control": every value is a real sensor, ISP or encoder parameter that can be set as a free number, not a preset. It must be reliable enough for real film use.

The approach: keep the vendor Linux kernel, drivers and SigmaStar MI libraries, and replace the vendor application `cardv` with our own application `camapp` (C, about 9,400 lines in `src/camapp/`).

### 1.2 What works (final state, 2026-09-30)
- The full pipeline without cardv: sensor → VIF → ISP → SCL → HDMI display, JPEG encoder, H.265/H.264 encoder and an analysis port. It boots faster than stock and the motion is smoother.
- Camera modes, each with its own settings: Normal, HDR (sensor DOL-HDR, 30p max), Slow-mo (capture 6 to 105 fps, clips stamped to play slower) and Timelapse (interval 0.1 s to 60 min).
- Frame rates of 24, 25, 30, 48, 50, 60, 72 and 75p at full width. With a sensor crop the rate can go higher: up to 105p at 1280 wide. The code caps it at 120. An experimental open-gate setting uses the whole 1944x1096 readout.
- Recording:
  - Codecs: H.265 or H.264 in MPEG-TS (crash-safe), or MJPEG in AVI.
  - Bitrate 2 to 100 Mbps, with CBR/VBR, GOP and QP range settings.
  - A 240-frame / 20 MB write queue and per-mode clip names.
  - A sidecar `.txt` with the full metadata, and a record-run timecode.
  - A red tally frame, blinking LED, BUF/DROP warnings and the time left on the card.
- Photos: JPEG, optionally plus a 10-bit raw DNG.
- Exposure:
  - Modes P / S (shutter priority) / G (gain priority) / M.
  - Shutter by speed or by angle, from 1 line (~1/67,000 s) to 2.1 s in Timelapse. Gain from 1x to 32x.
  - EV compensation of ±3 EV in 1/3 steps.
  - Metering average / centre / spot, with a movable and resizable spot.
  - AE lock, AE speed, anti-flicker, and AE maximum shutter and gain limits.
- White balance: Auto, Kelvin, manual gains and "Set from scene".
- Image processing on ISP controls only, no software filters:
  - brightness, contrast, lightness, saturation, gamma (including Log), sharpness and black level;
  - colour matrix strength and a custom 9-number matrix;
  - per-colour hue and saturation;
  - luma NR, chroma NR, de-spike, false colour, defog, WDR and temporal NR;
  - black and white.
  - Looks: 5 slots, exportable to and importable from the SD card as text.
- Assists: loupe (magnify 1.2x to 8x, movable), focus peaking, zebra, false colour, histogram, waveform (luma, RGB or parade), guides (thirds, cross), frame lines (2.39, 2.00, 1.85, 1.66, 4:3, 1:1, 9:16) and OSD hide.
- The OSD menu:
  - Sections Recording / Exposure / Colour / Assist / Expert / System, each with its accent colour, and a help side panel.
  - MENU closes the menu and remembers the place; the mirror button goes back.
  - The Expert pages show live sensor registers, a register viewer and writer with undo, and encoder settings.
  - The System page shows CPU, RAM, flash, SD, build and uptime, and has format SD, timecode reset, save logs and reset settings.
- Controls:
  - 5 body buttons and an 18-key IR remote.
  - Body MENU gestures: 1 click = photo, hold = menu, 2 clicks = OSD hide, 3 clicks = record.
- USB: an own configfs UVC gadget (honest 1920x1080 MJPEG at 30/60 fps; works in Windows Camera, VLC and Android UVC apps). The host can set two controls: AE mode and brightness (EV comp).
- Safety:
  - A boot counter falls back to stock cardv after 3 bad boots.
  - Deploy has a health check and automatic rollback.
  - Settings are saved atomically.
  - A crop the pipeline refuses restarts at full frame.

### 1.3 What does not work or was parked
- OSD burned into the USB/recorded picture (MI_RGN overlay accepted but never blended). Parked after 7 attempts (§15.1).
- HDMI output timings other than 1080p60: no signal. Parked; the code is in the stash (§15.2).
- Full-width 120p: the SoC MIPI receiver garbles at the 891 Mbps sensor setting. Parked (§15.3).
- Above 120 fps at narrow crops: skipped by the user.
- Playback and gallery on the camera: stage 1 worked (FIFO `play`), but the camera's H.265 decoder smears the bottom of 1080-high streams. Parked (§15.4).
- RAW video: removed (SD too slow).
- USB mass storage: blocked (no kernel module).
- Custom kernel: blocked (no matching source).
- Hot pixels at long exposures: the ISP correction has no consistent effect. A dark-frame subtraction would be the fix (idea).
- Focus meter: built from the AF statistics, judged "pretty bad", removed (the loupe covers focusing).

---

## 2. Hardware

### 2.1 Identity
| Item | Value |
|---|---|
| Product | AliExpress USB/HDMI "microscope" camera, ex-dashcam board |
| Board silkscreen | `V522_W_XX`, `DGECAM` |
| SoC | SigmaStar Mercury6 (SDK codename Tiramisu), chip ID 0xF4, 2x ARM Cortex-A53 (CPU part 0xd03) running a 32-bit ARMv7 kernel. Top marking ground off. Best-fit part: SSC8826Q (dual A53 @ 1.2 GHz, 256 MB DDR3 in package); also possible SSD268G, SSC359G |
| RAM | 256 MB DRAM (IPL). Linux sees ~72 MB (`LX_MEM=0xffe0000` minus reservations). MMA video heap 172 MB (`mma_heap=...sz=0xAC00000`), ~51 MB used by camapp. `cma=2M@0x23800000` |
| Flash | Macronix MX25L12835F, SPI-NOR, 16 MB, 3.3 V, JEDEC ID C2 20 18, SOIC-8 |
| Sensor | Sony IMX307, 1/2.8", back-illuminated, 2.9 µm pixels, 1945x1097 active, readout 1948x1097, 1920x1080 used. 16:9 native. I2C bus 1 (`/dev/i2c-1`) address 0x1a (0x34 8-bit). 4-lane MIPI at 445.5 Mbps/lane. 37.125 MHz input clock, 148.5 MHz pixel clock |
| Lens | Stock M12 holder with a very slow lens (~F12, very dark). The user plans a C-mount conversion (docs/lenses.md) |
| Kernel / bootloader | Linux 4.9.227 (built 2024-04-30, vermagic "4.9.227 SMP preempt mod_unload ARMv7 thumb2 p2v8"), U-Boot 2015.01, SigmaStar IPL. glibc 2.30, BusyBox userland |
| Power | USB powered |

### 2.2 Inputs and outputs
- **HDMI**: SigmaStar HDMI TX. It runs only the boot timing 1080p60 (§15.2). The user's monitor is an LG FHD, 1920x1080 at up to 100 Hz; its EDID is in `archive/scratchpad/project_scratch/edid.bin`.
- **USB**: device port on the SigmaStar UDC (`Sstar-udc`), used as the UVC gadget. The kernel also registers two EHCI USB 2.0 host controllers (`Sstar-ehci-1/2`, 1 port each, nothing attached) and prints `SStar USB3 PHY probe, base:fd2a4600`. Whether any host port is wired to a connector is unknown (physical check never done).
- **SD slot**: `mmc0`, 4-bit, 50 MHz, "SD high-speed" timing, 3.3 V. The 1.8 V UHS switch fails on this board ("Single Volt doesn't ready" / "cannot verify signal voltage switch"), so the ceiling is ~25 MB/s. Measured: ~9.5 to 14 MB/s write, 18.7 MB/s read. Only vfat in the kernel (no exFAT), so 4 GB per file.
- **Body buttons** (ADC ladder, `soc:adc-keys`, `/dev/input/event0`, EV_KEY): UP 103, AE 105 (KEY_LEFT), MENU 139, OK 28 (KEY_ENTER), DOWN 108. No kernel autorepeat (EVIOCSREP fails).
- **IR remote** (`mstar ir`, `/dev/input/event1`, NEC; the kernel keymap rc-mstar-dtv only translates 5 keys, so camapp reads the raw EV_MSC/MSC_SCAN scancodes). 18 keys in reading order: zoom+ 0x45, ▲ 0x46, zoom− 0x47 / ◀ 0x44, MENU 0x40, ▶ 0x43 / mirror 0x07, ▼ 0x15, AE 0x09 / flip 0x16, + 0x19, AWB 0x0d / contrast 0x0c, freeze 0x18, play/pause 0x5e / photo 0x08, record 0x1c, gallery 0x5a. **The remote has no OK button and no long press.** It sends no repeat codes while a key is held (tapping is used instead). `event2` is an MCE IR keyboard/mouse device (unused).
- **LED**: red status LED on GPIO 38 (active high).
- **UART**: root console, 115200 8N1. The user built a permanent UART port into the chassis. From WSL the adapter is `/dev/ttyUSB0`; on Windows it was COM18 (SmarTTY).
- No FPC/LCD connector, no audio input on the board (the stock `uac_function_enable` gadget parameter is ignored).

### 2.3 GPIO map
| GPIO | State | Role |
|---|---|---|
| 4, 5 | out high | anti-clone auth chip (bit-banged). Do not touch |
| 10, 11, 12 | out lo/lo/hi, owned by stock cardv | no visible effect when toggled; likely IR-cut H-bridge (10/11) and IR LED enable (12), unpopulated |
| 38 | out high | status LED (confirmed by blink test) |
| 124 | out high | set by U-Boot, probably power hold. Do not touch |
| 131 | in low | unknown input (USB or card detect?) |

### 2.4 Anti-clone authentication chip
- The boot log shows `AUTH_GPIO_INIT!:5-4`: a challenge/response chip on GPIO 5 and 4, probably bit-banged I²C.
- The challenge is always `67 c6 69 73 51 ff 4a ec`. These are the low bytes of unseeded glibc `rand()` (seed 1): the developer forgot `srand()`.
- The 10-byte response differs every boot (the chip adds its own nonce), but it decodes to the same payload.
- Whether a replay of one recorded response would pass was never tested. It does not matter while the original chip is on the board.

### 2.5 User's planned hardware (not done)
- C-mount (or short-flange D-mount) lens mount with a 650 nm IR-cut filter and shims (docs/lenses.md).
- Flash upgrade to 32 MB NOR: buy the 3.3 V `W25Q256JV`, not the 1.8 V `JW`/`FW` parts. Needs 4-byte addressing in IPL, U-Boot and the kernel; SigmaStar dashcam SDKs often lack it. Verify before buying (docs/notes/handoff-log.md "STORAGE UPGRADE").
- A microphone (mentioned 2026-09-29/30; no audio work was done, no audio input exists on the board).
- Whether the board already has an IR-cut glass over the sensor was never checked.

---

## 3. Access, backup and recovery

### 3.1 UART shell
- Ctrl+C during boot interrupts the boot scripts and gives a root BusyBox shell. The whole userland is BusyBox.
- `busybox telnetd` also runs from `rcS` (unauthenticated), but there is no network interface (only `lo`), so it is unreachable today.
- **One owner rule**: only one program may open `/dev/ttyUSB0` at a time. A monitor and a command sender on the same port break each other.
- **Line limit**: BusyBox's line editor caps an input line at 1024 characters. Longer lines are cut, which is why `uart_push.py` sends 900-character chunks.
- The kernel and camapp print to the same console, so their output can interleave with transfers. Both UART tools verify md5 checksums.

### 3.2 Golden flash dump (the undo button)
- `microscope_C22018_0xE41B1D54.bin` (project root, gitignored): the full 16,777,216-byte flash image, CRC32 0xE41B1D54. It was verified by two identical reads with a CH341A "black" programmer plus a SOP8 clip, using NeoProgrammer.
- NeoProgrammer settings:
  - Mode: SPI 25-series, chip MX25L12835F.
  - Header block: `BIOS 25 SPI`, jumper 1-2.
  - Do not use the 1.8 V adapter from the kit.
  - Clip: coloured stripe to pin 1, the chip's dotted corner.
  - This CH341A unit tested fine for the known 5 V-on-VCC defect.
- `microscope2_C22018_0xE41B1D54.bin`: a second good read, identical.
- `microscopeCorrupted_C22018_0xDEE27A1E.bin` and `microscopeCorrupted2_...`: early bad reads. Keep them only as a warning.
- `microscope_C22018_0xE41B1D54_extracted-claude.tar.gz`: a binwalk extraction of the dump. It is unpacked in `extracted/` (rootfs from the XZ CPIO at 0x290000, plus two uImages; see `extracted/README.txt`).
- `live_system.tar.gz` (7.4 MB): a tar of the running filesystem (`/customer`, `/config` etc.). It is truncated inside the tar stream, but the gzip layer is fine. It is partly unpacked in `live_system/`, including the stock UI `customer/UI/bin/gkuui`.

### 3.3 Flash partition map (MXP table at flash offset 0x20000)
| mtd | Name | Start | Size | Content |
|---|---|---|---|---|
| mtd0 | BOOT | 0x000000 | 0x070000 | IPL + U-Boot; **U-Boot env at 0x6F000-0x70000** (last 4 KB) |
| mtd1 | KERNEL | 0x070000 | 0x220000 | Linux uImage (LZMA) |
| mtd2 | rootfs | 0x290000 | 0x5A0000 | XZ CPIO, loaded as initrd into a RAM rootfs |
| mtd3 | miservice | 0x830000 | 0x050000 | JFFS2 mounted at `/config` (320 KB): our `/config/cardv` wrapper lives here |
| mtd4 | customer | 0x880000 | 0x6F0000 | JFFS2 mounted at `/customer` (6.9 MB, ~3.2 MB free): vendor app/ini/modules and all our files under `/customer/camapp` |
| mtd5 | MISC | 0xF70000 | 0x060000 | "firmwarefs" (FUSE fwfs at `/misc`): `config.ini` (display timings, logo settings), `upgrade.jpg` |
| mtd6 | KMSGLOG | 0xFD0000 | 0x020000 | named kernel log, but completely erased (never written); free for our own persistent log |
| — | (not in MTD) | 0xFF0000 | 0x010000 | top 64 KB, erased, unused |

To split the dump into partitions on the PC:
```
dd if=dump.bin of=0_boot.bin      bs=1 skip=$((0x000000)) count=$((0x070000))
dd if=dump.bin of=1_kernel.bin    bs=1 skip=$((0x070000)) count=$((0x220000))
dd if=dump.bin of=2_rootfs.bin    bs=1 skip=$((0x290000)) count=$((0x5A0000))
dd if=dump.bin of=3_miservice.bin bs=1 skip=$((0x830000)) count=$((0x050000))
dd if=dump.bin of=4_customer.bin  bs=1 skip=$((0x880000)) count=$((0x6F0000))
dd if=dump.bin of=5_misc.bin      bs=1 skip=$((0xF70000)) count=$((0x060000))
dd if=dump.bin of=6_kmsglog.bin   bs=1 skip=$((0xFD0000)) count=$((0x020000))
```
Extracted pieces are in `logs/flash/` (boot.bin, kmsglog.bin, misc.bin, misc_config.ini, top64k.bin, upgrade.jpg, misc images).

- U-Boot env:
  - `bootcmd=sf probe 0;sf read 0x22000000 ${sf_kernel_start} ${sf_kernel_size}; mxp r.info rootfs; sf read 0x21000000 ${sf_part_start} ${sf_part_size}; bootm 0x22000000`
  - `bootdelay 0`, `wmSdUpgradeImage=ImageUpgradeSD.bin`, `sysVer SMC_20240430111451`.
  - U-Boot does no display init.
- The kernel command line:
  - `console=ttyS0,115200 rootfstype=ramfs initrd=0x21000000,0x5A0000 LX_MEM=0xffe0000 mma_heap=mma_heap_name0,miu=0,sz=0xAC00000 ... cma=2M@0x23800000 mmap_reserved=fb,...`
  - There is no VT/fbcon (no `/dev/tty0`) and no `/proc/config.gz`.
- **An SD-card upgrade path exists in U-Boot.** Every boot it probes for `ImageUpgradeSD.bin`; the boot log shows "VER_HEAD len =7 / Version head error. It is VerVS20". This is the basis for a future own flasher: build an image with the right version header. It was never tried.

### 3.4 Escape hatches, from mildest to strongest
1. `tools/deploy.sh` health check: a new build that does not come up within 15 s is rolled back automatically.
2. `/customer/camapp/rollback.sh`: swaps `camapp` and `camapp.prev` and restarts.
3. Boot counter: 3 boots without 30 s of healthy camapp fall back to stock cardv automatically.
4. UART Ctrl+C during boot, then fix files by hand.
5. `rm /customer/camapp/enable`: the wrapper runs stock cardv.
6. `rm /config/cardv`: the exact stock boot.
7. CH341A + SOP8 clip: write the golden dump back.

Everything we install is a file on the JFFS2 partitions; no vendor file was ever edited.

---

## 4. The stock firmware (what camapp replaced)

### 4.1 Stock boot chain
- **Boot stages:** IPL, then IPL_CUST, then U-Boot, then the kernel (load 0x20008000). The rootfs initrd is unpacked into a ramfs. "Initramfs unpacking failed: junk in compressed archive" is printed and harmless.
- **Init:** inittab `::respawn:-/bin/sh` runs `/etc/profile`. `/etc/profile` sources `/bootconfig/demo.sh`, and it sets PATH with **`/config` before `/bootconfig/bin`**. That PATH order is what lets our wrapper shadow cardv.
- **`/bootconfig/demo.sh` does, in order:**
  1. Loads `/customer/modules/4.9.227/*.ko` (sensor driver `imx307_MIPI.ko` with `lane_num=4`, `mi_*` modules, `usb-storage.ko`, ...).
  2. Runs `/customer/gadget/udc-uvc.sh`, which loads libcomposite + usb_f_uvc, mounts configfs at `/customer/config`, then loads `g_webcam.ko streaming_maxpacket=3072`.
  3. Exports GPIO 4/5.
  4. Starts `cardv /customer/cardvini/one_bayer_4k.ini &` (the 4k ini, because `/misc/config.ini` lacks `FB_WIDTH = 1920`).
  5. Runs `wait_rec.sh` (waits up to 2 s for `/tmp/cardv_rec`), then `riu_w 0x1154 16 0x0613`, then `/customer/gadget/vif.sh &`.
- `vif.sh`: a sensor watchdog. Every ~20 ms it compares the VIF ISR count, and if it is frozen it toggles IMX307 standby (reg 0x3000) through `/customer/i2c_read_write`. It burns CPU and must never run without cardv (it pokes the sensor behind camapp's back); our boot.sh kills it.
- `gkuui` (`/customer/UI/bin/gkuui 1920 1080`): the stock OSD/UI process, built on LVGL, started by cardv.
- Stock missing modules (harmless): `nfsv3`, `media`, `videodev`, `v4l2-common`. "AF motor params invalid" (no focus motor). "fail direction[out] gpio[11]".
- Stock cardv segfaults while tearing down on `reboot` (only on the way down).

### 4.2 Stock stop/start without reboot (tools/device/cam_stop.sh, cam_start.sh)
- **Stop:**
  1. Kill `vif.sh`, `gkuui`, `cardv`.
  2. cardv's exit handler hangs on `system("cat /proc/kmsg | tee /mnt/mmc/kmsg_info_*.txt")`: kill that `cat`.
  3. Delete `/mnt/mmc/kmsg_info_*.txt` and `/mnt/mmc/pid*_maps.txt` (RAM).

  Free RAM goes from 27 MB to 43 MB. Stopping cardv drops the USB gadget (PULL_UP off).
- **Start:**
  1. `cd / && cardv /customer/cardvini/one_bayer_4k.ini &`
  2. `riu_w 0x1154 16 0x0613`
  3. `/customer/gadget/vif.sh &`

  Stock cardv does not survive a restart after camapp ran (it hangs, no HDMI); reboot instead.
- `/customer/gadget/udc-msdc.sh` is the vendor's USB mass-storage gadget script (a mode-switch precedent; its function module is missing on this kernel).

### 4.3 cardv control surfaces (for reference)
- **FIFO**: `echo "<cmd> <args>" > /tmp/cardv_fifo`. A wrong argument count is silently ignored. Registered commands:
  - `ctmwb playback frequency mirror seamless rec capture capraw lock res vidres capres loop timelapse rm format sdrename thumb tt bri con sat sha wb scene ev flicker shutter iso gamma effect winweight night audioplay aplaystop audiorec pbvolume park gsensor zoom video hdmiset voice bitrate hdmitiming muxdebug dcf srcchg timezone hdr snr quit help datelogoStamp timeformat str reset lcdbri lcdcon lcdhue lcdsat recvol sclrotate ispget decrypt hdmi dispscal`
  - `format`, `rm`, `reset` and `quit` are destructive.
  - `shutter N`: 1 = 1/60 ... 5 = 1/1000, other = auto.
  - `iso N`: 0 auto, 1-6 = 1x-32x, −2 = AE lock, −3 restore, other = raw gain with 1024 = 1x.
  - Any new UVC stream start resets cardv's exposure.
- **Internal ISP command IDs** (`isp_process_cmd` @0x31571):

  | ID | Controls |
  |---|---|
  | 0x403 | saturation |
  | 0x404 | sharpness |
  | 0x405 | lightness |
  | 0x407 | EV comp |
  | 0x40a | AE ExposureLimit |
  | 0x40b | shutter |
  | 0x412 | contrast / iso |
  | 0x413 | brightness |
  | 0x415 | flicker |
  | 0x41a | AWB attr |
  | 0x41c | RGB gamma + expo mode |
  | 0x41f | metering WinWgtType |
  | 0x420 | WDR |
  | 0x421 | AWB CTM (Kelvin) |
- **Stock UVC**:
  - It advertised fake sizes up to 3840x2160, all max 30 fps, plus a still-image pin (Windows Camera timed out on it).
  - Its extension unit (GUID `{63610682-5070-49ab-b8cc-b3855e8d221d}`) only does white balance. `exposure_time_absolute` is mapped to EV comp, not shutter.
  - Every stream stop re-inits HDMI.
- **Stock HDMI bug**: cardv starts HDMI only from the hotplug callback. With the cable already plugged at boot, the first EDID read fails ("Check Edid Data Fail") and HDMI never starts ("Check Device Power" on the monitor) until a replug. camapp calls `MI_HDMI_Start` directly and does not have this bug.
- Stock stills were fake 7488x4212 (31 MP) upscales of 1080p. The "4K" in the stock menu is interpolation.
- `/bootconfig/bin/riu_r`, `riu_w`: raw SoC register (RIU bus) read/write. Powerful and dangerous.
- `/customer/i2c_read_write -b 1 -a 0x34 -f A16D8`: direct sensor register access.
- `libmi_iqserver.so` (the SigmaStar PC IQ tuning server) is linked into cardv. `libmi_ipu.so` (NPU) is present.
- IQ files:
  - `/bootconfig/iqfile/iqfile0.bin` (minor version mismatch warning, harmless).
  - `/bootconfig/iqfile/isp_api.bin`: the API bin that camapp loads; without it the picture is very dark.
  - `isp_api.xml`.

### 4.4 cardv's MI bring-up (reference, captured with our mitrace shim)
The order below is from `logs/mi_trace_boot1.log` (no HDMI) and `logs/mi_trace_boot2.log` (HDMI and replug):
1. `MI_SYS_Init`.
2. Sensor: `MI_SNR_InitDev`, `SetPlaneMode(0,0)`, `QueryResCount`, `GetRes`, `SetRes(0,0)`, `Enable(0)`, `GetPadInfo`.
3. VIF: `CreateDevGroup`, `SetDevAttr`, `EnableDev`, `SetOutputPortAttr`, `EnableOutputPort`.
4. ISP: `CreateDevice`, `CreateChannel`, `SetChnParam`, `StartChannel`, `EnableOutputPort`.
5. SCL dev 0 (ports 0/1) and dev 3.
6. Four `MI_SYS_BindChnPort2` binds.
7. `MI_ISP_API_CmdLoadBinFile`.
8. DISP.
9. HDMI (Start only from the hotplug callback).
10. VENC dev 8.

camapp copies this recipe (§8.1). Decompiles of the relevant cardv functions are in `research/decompiles/dec_*.c`.

---

## 5. Boot integration and on-device files

### 5.1 How camapp boots
1. demo.sh runs `cardv <ini> &`. Because of the PATH order, this runs our wrapper `/config/cardv` (source `tools/device/flash/cardv`).
2. The wrapper checks for `/customer/camapp/enable` and an executable `/customer/camapp/boot.sh`:
   - If they are missing, it runs `exec /bootconfig/bin/cardv "$@"` (stock).
   - If they exist, it reads `boot_attempts`. Below 3, it increments the counter, syncs and runs `exec boot.sh`. At 3 it prints a message and runs stock cardv.
3. `boot.sh` (source `tools/device/flash/boot.sh`):
   1. Touches `/tmp/cardv_rec` (demo.sh's `wait_rec.sh` waits for it).
   2. Starts a background loop that kills `vif.sh` (up to 60 s; demo.sh starts it right after).
   3. Starts a background loop that mounts the SD card (`mount -t vfat /dev/mmcblk0p1 /app/bin/sd`, retries for 20 s). cardv used to do this.
   4. Runs `uvc-gadget.sh`.
   5. Starts `camapp > /tmp/camapp.log 2>&1 &`.
   6. Waits 30 s. If camapp is still alive, it resets `boot_attempts` to 0.
4. `uvc-gadget.sh` (source `tools/device/flash/uvc-gadget.sh`):
   1. `rmmod g_webcam`.
   2. Builds a configfs gadget at `/customer/config/usb_gadget/cam`:
      - VID 0x0AC8, PID 0x3420, bcdDevice 0x0500. The new revision makes hosts re-read the descriptors.
      - Device class 0xEF/0x02/0x01 (IAD).
      - Strings "camapp" / "camapp IMX307" / "0001".
      - One UVC function with a single MJPEG format at 1920x1080. Frame intervals 333333 (30 fps) and 166666 (60 fps), default 30. Max frame buffer 2 MB, `streaming_maxpacket` 3072, MaxPower 100.
   3. Binds to the UDC.
   4. On any failure it reloads the vendor `g_webcam.ko streaming_maxpacket=3072 uac_function_enable=0`, so the camera always keeps a webcam.

### 5.2 Files on the camera
| Path | What |
|---|---|
| `/config/cardv` | boot wrapper (only file outside /customer) |
| `/customer/camapp/enable` | flag: boot camapp instead of stock |
| `/customer/camapp/boot_attempts` | boot counter (reset to 0 after 30 s healthy) |
| `/customer/camapp/boot.sh`, `uvc-gadget.sh`, `update.sh`, `rollback.sh` | scripts (sources in `tools/device/flash/`) |
| `/customer/camapp/camapp` | the running build |
| `/customer/camapp/camapp.prev` | previous build (rollback target) |
| `/customer/camapp/camapp.good` | first boot-integrated build (old, 2026-09-24) |
| `/customer/camapp/camapp.bad` | last build that failed its health check (if any) |
| `/customer/camapp/settings` | all settings, key=value (§11) |
| `/customer/camapp/modes/<Normal|HDR|Slow-mo|Timelapse>` | per-mode keys of the modes not running |
| `/customer/camapp/looks/look1..5` | look slots |
| `/tmp/camapp.log` | camapp stdout/stderr (RAM) |
| `/tmp/camapp_fifo` | command pipe |
| `/tmp/update.sh`, `/tmp/camapp.new` | deploy staging (RAM) |
| `/app/bin/sd` | SD card mount point: clips, photos, `LOOKS/`, `LOGS/` |

### 5.3 Updating camapp
`tools/deploy.sh [binary]` (PC) does:
1. Builds with `src/camapp/build.sh` (unless a binary is given).
2. Runs `tools/check_syms.sh` on it.
3. Pushes `update.sh` to `/tmp`, `rollback.sh` to `/customer/camapp/` and the binary to `/tmp/camapp.new`.
4. Runs `/tmp/update.sh /tmp/camapp.new` on the camera and returns its code: 0 OK, 2 rolled back, 3 no answer.

`update.sh` (device) does:
1. Copies the binary to `camapp.new`, then `mv camapp camapp.prev` and `mv camapp.new camapp` (atomic renames: a power cut leaves either the old or the new build).
2. SIGTERM to the old camapp (waits up to 20 s), then starts the new one.
3. Healthy = the process is alive and the log has a line starting with `uvc_start` (the last bring-up step) after 15 s.
4. If unhealthy: `kill -9`, the new build goes to `camapp.bad`, `camapp.prev` is restored and restarted.

A running binary cannot be overwritten ("Text file busy"); always install as a new file and rename.

Deploying takes about 2 minutes, mostly the UART push of the ~180 KB binary (gzip, 900-character chunks).

---

## 6. Development environment on the PC

### 6.1 Machine
- Windows with WSL2 (Ubuntu). The kernel was `6.18.x-microsoft-standard-WSL2` in September 2026. Project at `/home/aftwr/microscopeproject`.
- **WSL `/tmp` is a tmpfs, wiped at every WSL restart.** Never keep anything important there; the session scratchpads under `/tmp/claude-1000/...` were lost that way (§19.3).
- Cross compiler: `arm-linux-gnueabihf-gcc` (Ubuntu 15.2.0). It links against the device's own libraries in `extracted/rootfs/lib` (glibc 2.30 plus the vendor MI libraries).
- Python 3 with `pyserial` (3.5) for all UART tools, and Pillow for `viewgrab.sh`.
- The FFmpeg 4.1.3 headers are in `third_party/ffmpeg-4.1.3`, with the generated `libavutil/avconfig.h` and `ffversion.h`. The device libraries are SigmaStar's own FFmpeg 4.1.3 build. AVCodecContext offsets on ARM: flags 0x3c, width 0x5c, height 0x60, debug 0x240.
- Ghidra 12.1.4 and Temurin JDK 21 are in `~/tools/`, `uv`/`uvx` in `~/.local/bin`. No sudo was used for any of it.
  - Project: `ghidra/microscope.gpr`, with `cardv`, `libmi_isp.so`, `libcus3a.so` and `libavcodec.so.58.35.100` analysed.
  - `.mcp.json` runs `pyghidra-mcp` (stdio) on that project for Claude Code.
  - Only one process may open the Ghidra project at a time.
  - The imx307 driver was decompiled with a headless Ghidra setup in the (lost) scratchpad; its output is `research/decompiles/dec_imx307_MIPI_ko.c`.
- Disassembly: `arm-linux-gnueabihf-objdump -d -M force-thumb <bin>` (the vendor code is Thumb-2). The old `cardv.dis` / `gkuui.dis` listings were in the lost scratchpad; regenerate them with this command from `extracted/rootfs` or `live_system/`.
- Windows side, as accepted by the user:
  - Windows ffmpeg at `/mnt/c/ProgramData/chocolatey/bin/ffmpeg.exe` for grabbing frames through dshow. The camera's webcam is `video="USB Video Device"`. A Logitech BRIO (`video="BRIO 4K Stream Edition"`) points at the HDMI monitor so Claude can see the screen.
  - Files go through `C:\Users\aftwr\AppData\Local\Temp`.
  - The BRIO must not be attached to WSL (isochronous USB hangs there).
  - **`usbipd` (attach the camera's USB to WSL) only when the user allows it, and detach afterwards** (`usbipd.exe detach --busid <id>`). It was used once for `v4l2-ctl` UVC control tests (`usbipd.exe attach --wsl --busid 1-9`).

### 6.2 Building camapp
`src/camapp/build.sh`:
```
arm-linux-gnueabihf-gcc -O2 -Wall -Wno-attributes -std=gnu11 -I$F -U_TIME_BITS -D_TIME_BITS=32 \
  -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32 -fno-stack-protector -o camapp <all .c files> \
  -nodefaultlibs -Wl,--no-as-needed $R/lib/libc.so.6 $R/lib/libm.so.6 $R/lib/libpthread.so.0 $R/lib/libdl.so.2 \
  -L$R/lib -lmi_sys -lmi_common -lcam_os_wrapper -lcam_fs_wrapper -lispalgo -lcus3a \
  -lmi_sensor -lmi_vif -lmi_isp -lmi_scl -lmi_disp -lmi_hdmi -lmi_venc -lmi_rgn \
  -lavformat -lavcodec -lavutil -lswresample -lfdk-aac -lmi_vdec -lmi_jpd -lgcc \
  -Wl,-rpath-link,$R/lib -Wl,-z,max-page-size=4096
arm-linux-gnueabihf-strip camapp
```
- `R=extracted/rootfs`, `F=third_party/ffmpeg-4.1.3`.
- The time and offset flags keep the binary compatible with glibc 2.30: a modern toolchain would otherwise pull in `__*64` symbols the device lacks.
- The vendor libraries have no SONAME, hence `-L/-l`.
- The SigmaStar FFmpeg decoders call MI_VDEC/MI_JPD/MI_SCL/MI_SYS without DT_NEEDED entries, hence `-lmi_vdec -lmi_jpd`.
- `tools/check_syms.sh <bin>` fails when a binary needs a symbol or symbol version that the device libraries lack. deploy.sh runs it every time.
- The stripped binary is ~180 KB.
- There is no Makefile and there are no tests on the PC; verification happens on the device.

### 6.3 Tools (`tools/`)
| Tool | Use |
|---|---|
| `deploy.sh [binary]` | build + check_syms + push + install with rollback (§5.3). Grep its output for `update:` / `UPDATE_RC=` |
| `sh.py "" "cmd" ...` | run shell commands over UART and print output; the first empty command wakes the prompt; output often late, re-read with `sh.py "" "" "true"` |
| `uart_push.py <local> <remote>` | push a file: gzip (when smaller), base64 in 900-char lines, md5 check. A 178 KB camapp takes ~160 round trips |
| `uart_pull.py <remote> <local>` | pull a file: one gzip+base64 stream between markers checked by md5 (seconds); on failure falls back to 1 KB chunks each with its own md5 and retries |
| `check_syms.sh <bin>` | symbol/version check against `extracted/rootfs` libs |
| `push_all.sh` | old helper: re-push camapp, mitrace.so, cam_stop/cam_start to /tmp after a reboot (from the pre-boot-integration days) |
| `uvcgrab.sh NAME` | grab one 1080p frame from the camera's USB webcam with Windows ffmpeg into `scratch/NAME.jpg` (keeps the 5th frame; the first after a reconnect can be empty) |
| `viewgrab.sh NAME` | FIFO `viewgrab` + pull `/tmp/view.ppm`: the processed picture at 160x90 over UART into `scratch/NAME.png`. Works with no HDMI and no USB |
| `trace_parse.py <log> [regex]` | parse mitrace logs into calls with argument dumps |
| `psf2c.py <font.psf> <out.h>` | PSF2 console font to C header (ASCII 32..126 + degree sign at 127) |
| `device/cam_stop.sh`, `device/cam_start.sh` | stop/start the stock stack without reboot (§4.2) |
| `device/cpustat.sh SECS` | on the camera: whole-CPU split + per-thread CPU of camapp (push to /tmp first) |
| `device/flash/*` | the scripts installed on the camera (§5) |

`scratch/` is the local scratch folder (gitignored). `uvcgrab.sh` and `viewgrab.sh` write into it.

### 6.4 Other programs in `src/`
| Dir | What |
|---|---|
| `src/mitrace/` | `mitrace.so`, an LD_PRELOAD shim with 108 MI_* wrappers. It passes 8 argument words through and hex-dumps pointer arguments after a readability probe. Run: `CAM_PRELOAD=/tmp/mitrace.so /tmp/cam_start.sh`, log `/tmp/mi_trace.log`. It produced the cardv recipe |
| `src/getexpo/` | read-only AE state dump (first ISP experiment) |
| `src/fbtest/` | `fbtest` (shows a box through the fb0 colour key), `fbinfo` (dumps fb show/alpha/colour-key state) |
| `src/playtest/` | standalone FFmpeg + SigmaStar hardware decode test (frame format, speed) |
| `src/hello/` | first cross-compile test (prints the glibc version) |

---

## 7. camapp architecture

### 7.1 Source files (`src/camapp/`)
| File | Lines | Role |
|---|---|---|
| `camapp.c` | 1285 | `main`: pipeline bring-up with an undo stack, teardown, restart by re-exec; exposure (auto/manual, P/S/G/M, AE limits, speed, flicker, EV comp), metering weight tables, frame rate (driver scale, HMAX), crop and orientation; JPEG encoder thread (feeds UVC, MJPEG recording, photos + DNG); status bar and corner text (`update_osd`); playback glue; FIFO thread; main 1 s loop |
| `cmd.c` | 1211 | `handle_cmd`: every button action and FIFO line (§10); body gesture translation; UVC control get/set |
| `menu.c` | 1731 | the on-screen menu: pages, rows, values, help texts, button handling, register write/undo, System page functions (format, save logs) |
| `osd.c` / `osd.h` | 778 / 63 | framebuffer OSD: status bar, corner blocks, notices, tally/flash border, menu + help panels, guides and frame lines, histogram, waveform, analysis cells, loupe map, spot box. Shadow buffer + changed-row commit; one mutex |
| `isp.c` | 601 | ISP image controls (brightness … HSV, noise, WDR, NR3D, gamma, CCM), white balance, looks (save/load/export/import) |
| `rec.c` | 672 | recording queue + writer thread, clip names, sidecar, AVI/TS files, split, H.265/H.264 encoder on SCL port 2 (VENC dev 0) and its fetch thread; SD presence, hotplug mount/verify, format |
| `uvc.c` / `uvc.h` | 418 / 16 | UVC gadget protocol on `/dev/video0` (probe/commit, controls, MMAP streaming, pacing, reconnect) |
| `scopes.c` | 265 | loupe (SCL port 0 crop), analysis thread on SCL port 3 (peaking, zebra, false colour, histogram, waveform), `viewgrab` |
| `dng.c` | 280 | FBC off before VIF start, raw frame grab from VIF, DNG writer, `dngview` developer |
| `sensordbg.c` / `.h` | 285 / 35 | IMX307 I2C access (`/dev/i2c-1`, 0x1a), timing debug (`sensor`), register names, descriptions, safe-write list |
| `settings.c` / `.h` | 175 / 85 | settings struct, defaults, key table, load/save (atomic), mode and look subsets, look import filter with clamping |
| `input.c` / `.h` | 230 | input thread: body keys + remote scancodes → action names, repeat/acceleration, OK long press, MENU gestures |
| `player.c` / `.h` | 200 / 20 | FFmpeg + SigmaStar decoder → DISP input port (parked playback) |
| `ts.c` / `.h` | 169 / 17 | MPEG-TS muxer (PAT/PMT before each keyframe, PCR 100 ms ahead, PTS only, stream types 0x24 H.265 / 0x1b H.264) |
| `avi.c` / `.h` | 137 / 16 | AVI 1.0 writer ('00dc' chunks, idx1 and header patch on close) |
| `rgn.c` | 138 | MI_RGN overlay probe (parked, §15.1) |
| `mi.h` | 183 | SigmaStar MI prototypes (no vendor headers exist), module IDs, ChnPort |
| `camapp.h` | 204 | internal interface between the files (globals, constants, struct layouts) |
| `font.h` | 108 | Terminus Bold 12x24 (SIL OFL), generated by `tools/psf2c.py` |
| `build.sh` | | build script (§6.2) |

### 7.2 Threads (named with prctl, visible in `top` / `cpustat.sh`)
| Thread | Job |
|---|---|
| main | bring-up, then a 1 s loop: status line to the log every 10 s, LED (blinks while recording), SD hotplug check, timelapse auto-stop, `update_osd`, debounced settings save (2 s after the last change) |
| `fifo` | reads `/tmp/camapp_fifo` lines into `handle_cmd` |
| `input` | reads event0/event1, fires actions into `handle_cmd` |
| `jpeg` | JPEG encoder (VENC dev 8) stream fetch: UVC frames, MJPEG recording frames, photos |
| `uvc` | UVC events and buffer queueing |
| `scopes` | analysis loop on SCL port 3 every 66 ms (~15 updates/s) |
| `recwrite` | writes queued frames to the SD card |
| `hevc` | H.265/H.264 encoder (VENC dev 0) stream fetch while recording |
| `dng` | raw capture/DNG writing |
| `flash` | photo LED off-blink (short-lived) |

All commands (FIFO, buttons, UVC controls) end in `handle_cmd`. UVC controls are turned into FIFO-style lines (`prio N`, `evcomp N`).

### 7.3 Pipeline as built by `main()`
```
IMX307 --MIPI 4 lanes--> VIF dev 0 --bind (1)--> ISP dev 0 chn 0 --bind realtime (4)--> SCL dev 0 chn 0
    SCL port 0  1920x1080 NV12 --bind (1)--> DISP dev 0 layer 0 port 0 --> HDMI (1080p60)   [depth 3; loupe = crop on this port]
    SCL port 1  1920x1080 NV12 --bind (1)--> VENC dev 8 chn 0 (JPEG) --> UVC + MJPEG clips + photos
    SCL port 2  1920x1080 NV12 --bind------> VENC dev 0 chn 0 (H.265/H.264), only while recording
    SCL port 3  480x270 NV12   --user GetBuf--> analysis thread, only while an aid needs it
```
MI module IDs: VIF 6, ISP 0x21, SCL 0x22, DISP 0xf, VENC 2. `ChnPort = {module, dev, chn, port}`. The bind arguments are `(src fps, dst fps, bind type, 0)`; the fps values are 16-bit.

Bring-up order, with the struct words that matter (all from cardv traces and decompiles):
1. `settings_load` first, because the HDR mode decides the sensor mode. Then `dng_fbc_off()`: `mi_vif` parameter `g_bFBCMode=N` before VIF starts.
2. `MI_SYS_Init(0)`, `MI_SNR_InitDev(NULL)`, `MI_SNR_SetPlaneMode(0, hdr)`, `MI_SNR_SetRes(0, idx)` (idx 0 = 1920x1080@60; HDR: the mode named "HDR"), `MI_SNR_Enable(0)`.
3. Crop (Normal and Slow-mo only): the sensor window registers, then the VIF dev rect.
4. `MI_VIF_CreateDevGroup(0, {4 MIPI, 0, HDR type, 2 work mode, 0,0,0})`, `SetDevAttr(0, {pixfmt 0x20 RG10 (HDR 0x2c RG12), x|y<<16 crop, w|h<<16, 0, 0})`, `EnableDev`, `SetOutputPortAttr(0,0,{0, wh, wh, pixfmt, 0})`, `EnableOutputPort`.
5. `MI_ISP_CreateDevice(0,{1,0})`, `CreateChannel(0,0,{1 = sensor pad mask, ...})`, `SetChnParam(0,0,{HDR type, 3DNR 1, mirror, flip, rot})`, crop: `SetInputPortCrop(0,0,{0,0,W,H})`, `StartChannel`, `EnableOutputPort(0,0,0)`, bind VIF→ISP, then **`load_isp_api_bin()`**: wait for `MI_ISP_IQ_GetParaInitStatus` (≤3 s), then `MI_ISP_API_CmdLoadBinFile(0,0,"/bootconfig/iqfile/isp_api.bin",0x4d2)`.
6. `MI_SCL_CreateDevice(0,{15})`: the hardware port mask 0b1111 enables ports 0-3 (with mask 3, port 2 could not be enabled later). `CreateChannel`, `SetChnParam`, `StartChannel`, port 0 param `{crop xy, crop wh, 0x04380780, 0x101, 0xb NV12, 0}`, enable, depth 3, bind ISP→SCL realtime.
7. `MI_DISP_InitDev(NULL)`, `SetPubAttr(0,{0x800080 bg, 5 = HDMI, 0x10 = 1080p60, zero sync})`, `Enable`, `MI_HDMI_Init({event cb})`, `Open(0)`, `SetAttr(0,{0,1,9 = 1080p60,0,0,0,4,...})`, **`MI_HDMI_Start(0)` directly** (no hotplug wait), `BindVideoLayer(0,0)`, `SetVideoLayerAttr(0,{0,0x04380780,0x780,0x438,0})`, `EnableVideoLayer`, `SetInputPortAttr(0,0,{0,wh,wh})`, bind SCL port 0→DISP, `EnableInputPort`, `SetInputPortSyncMode(0,0,2)`.
8. SCL port 1 `{0,0,0x04380780,0x101,0xb,0}`, enable, `MI_VENC_CreateDev(8,{0x2000,0x2000})`, `CreateChn(8,0,{4 JPEG,1920,1080,0xc00000,1,1920,1080,0,0,0,7,30,1,70})`, bind SCL port 1→VENC, `StartRecvPic`.
9. Threads: fifo, jpeg, `uvc_start("/dev/video0")` (logs the `uvc_start` line that update.sh waits for).
10. Apply the settings, in this order: orientation, fps (+HMAX), weight tables, metering, NR3D, WDR, ISP image, JPEG quality, WB, peaking/analysis, EV comp, then manual exposure if manual. Then the input thread, the OSD, the guides and the LED.

Teardown runs the undo stack in reverse. `restart_requested` (mode switch, crop change, FIFO `restart`) re-execs `/proc/self/exe` after the full teardown: same pid, same log, clean pipeline. If the pipeline fails to start with a crop, camapp clears the crop, saves and restarts at full frame.

### 7.4 Command dispatch order inside `handle_cmd`
1. Body MENU gestures: `menu_long` → `menu`; `menu_short` → `menu` if the menu is open, else `snap`; `menu_double` → `osd_toggle` (ignored with the menu open); `menu_triple` → `rec_toggle` (ignored with the menu open).
2. `osd_toggle`.
3. `playback_input` (while a clip plays).
4. `play [PATH]`.
5. `menu_input` (menu navigation and the remote mirror/loupe button as "back").
6. Everything else (§10). Loupe zoom/pan take the arrows and zoom keys while the loupe is on and the menu is closed. In M/S/G the live buttons change shutter and gain directly (§9.6). With spot metering the arrows move the spot.

### 7.5 Settings persistence
A change calls `settings_changed()`. The main loop saves 2 s after the last change, and camapp saves again at exit. The save writes `settings.tmp`, flushes, fsyncs and renames. Mode switches store the running mode's keys to `modes/<old>`, load `modes/<new>` and restart.

---

## 8. Pipeline, sensor and driver facts (the hard-won knowledge)

Everything here was measured on this camera. When the SigmaStar docs (SSD20X/SSD220 SDK mirror, §19.4) disagree, the measurement wins.

### 8.1 3A and the IQ file
- `MI_ISP_CreateChannel` always starts userspace 3A in the calling process:
  1. `MI_ISP_EnableUserspace3A`, then `CUS3A_Init`.
  2. AE, AWB and AF are enabled.
  3. `MI_ISP_RegisterIspApiAgent` is called.
  
  So auto exposure runs inside camapp, and `MI_ISP_AE_QueryExposureInfo` returns live values only in camapp's own process.
- Without loading `isp_api.bin` (cardv's IQ loader thread at 0x2f128) the picture is very dark. The loader waits for `MI_ISP_IQ_GetParaInitStatus`, then calls `MI_ISP_API_CmdLoadBinFile(..., 0x4d2)`.
- cardv's thread at 0x2f174 (the `Cus3AOpen/WaitIspFrameSync` helper) is not needed.

### 8.2 Sensor timing and the imx307 driver (imx307_MIPI.ko, decompile in `research/decompiles/dec_imx307_MIPI_ko.c`)
- One linear mode: "1920x1080@60fps" (crop 1920x1080, output 1948x1097, fps 3..60). With `SetPlaneMode(0,1)` there is only "1920x1080@30fps_HDR".
- Stock timing: VMAX 1125 lines (0x3018-301A), HMAX 2200 clocks (0x301C-301D), FRSEL 1. At 148.5 MHz that is a 14.81 µs line and exactly 60.00 fps. Formula: **fps = 148.5e6 / (HMAX × VMAX)**. Exposure = (VMAX − SHS1 − 1) × line time (SHS1 = 0x3020-3022).
- **The driver bug (factor 2):** the driver converts times with the 30 fps line (29.6 µs, HMAX 4400) while the sensor runs 14.8 µs lines. So `MI_SNR_SetFps(n)` gives 2n, and requested exposures come out half as long. camapp compensates:
  - It requests fps × 500 milli-fps.
  - It sends manual shutter and AE limit times × `driver_time_scale() = 4400 / HMAX` (2 at 60p and below, 2.5 at 75p).
  - It displays values computed from the registers.
  - The driver's HDR functions use the right time base, so the scale is 1 in HDR.
- The driver's `pCus_SetFPS` accepts only 3-30 fps (in its own units) and never sets VMAX below 1125. `AEStatusNotify` rewrites VMAX and SHS1 on every AE update. HMAX and the window registers are written only at init, so camapp owns them.
- `pCus_SetOrien` only marks the orientation dirty. A frame callback later writes 0x3007 with the orientation bits alone, which clears WINMODE. In crop mode camapp writes 0x3007 itself and never calls SetOrien.
- `pCus_SetAEUSecs` computes µs × 1000 in 32 bits, so exposures wrap above 4,294,967 driver units: 2.147 s real at ≤60p, 1.72 s at 75p. Measured: 1 s and 2 s exact, 3.8 s came out as 1.65 s. This is why the Timelapse shutter stops at 2.1 s.
- A manual shutter longer than the frame makes the driver stretch VMAX and never shrink it back, which silently drops the frame rate. camapp clamps the manual shutter to 97 % of the frame (Timelapse: up to the interval) and re-applies the frame rate after exposure changes.
- The sensor applies exposure 2-3 frames late, so fast AE speeds overshoot and hunt. The "Instant" AE speed is vendor/2, the fastest setting that does not overshoot (settles in ~0.3 s).
- Gain: register 0x3014 in 0.3 dB steps (0 = 1x, 20 = 6 dB, 60 = 18 dB, 80 = 24 dB). The vendor AE ExposureLimit caps sensor gain at 8x, even in manual; the ISP then adds digital gain. camapp raises the cap. Above 16x (register 80) the brightness still doubles at 32x: the driver probably switches HCG (0x3009 bit 4, unverified).
- Shortest shutter: 1 line ≈ 15 µs (~1/67,000). The vendor minimum limit of 40 µs was lowered to ~1 line.

### 8.3 Frame rates above 60p and the MIPI limit
- Above 60 fps the frame stays 1125 lines (the driver floor) and camapp shortens the line: `HMAX = 148.5e6 / (1125 × fps)` (72p = 1833, 75p = 1760), written with REGHOLD (0x3001) so both bytes change in the same frame.
- The line must be longer than its MIPI data (4 lanes × 445.5 Mbps, 10-bit): **HMAX_min ≈ (W + 28) × 5/6 + 160** clocks.
  - Full width: 75p clean. 77.5p and 80p give broken frames.
  - 1280 wide: HMAX 1250 = 105.6p clean. At 1200 the left columns corrupt (a purple line), and 110p is not clean.
  - `crop_max_fps(w)` = 132000 / (((w+28)×5+5)/6 + 160), clamped to 75..120.
- Full-width 120p: the sensor's 891 Mbps settings (REPETITION 0x3405 = 0x00 plus D-PHY timing 0x3446-0x3454) make it output 120 fps, but the picture is garbled. The SoC MIPI receiver would need a different setup that the driver never does (`padinfo` dumps the receiver words). Parked.
- The formula allows ~134p at 960 wide and ~184p at 640 wide. The code caps at 120 (the sensor's rated maximum). A sweep above 120 was skipped by the user: at those rates the SD bus limits recording anyway.

### 8.4 Crop and open gate
- Sensor window, written in standby (0x3000 = 1, then 0):
  - 0x3007 = 0x40 (WINMODE 4 = window) | flip bit 0 | mirror bit 1.
  - WINPH 0x3040-41 = (1920 − W)/2.
  - WINWH 0x3042-43 = W + 28.
  - X_OUT 0x3472-73 = W + 28.

  The height always stays full: the driver's VMAX floor means a vertical crop gives no speed, so the height only changes framing.
- VIF: the crop goes into the **dev attr** rect (`x | y<<16`, `w | h<<16`, y = (1080 − H)/2). A port rect with a y offset is refused (0xa0062008). The port takes the whole dev rect.
- ISP: `MI_ISP_SetInputPortCrop(0,0,{0,0,W,H})`. SCL scales every port to 1920x1080 (so crops are "zoomed").
- Width: multiple of 8, 320..1920. Height: even, 180..1080. Crop works in Normal and Slow-mo only, and is fixed for a run (changing it restarts camapp).
- Open gate: `crop_w = 1944` is a sentinel. No sensor window; VIF (0,0,1944,1096) of the 1948x1097 readout, scaled to 1920x1080 (0.2 % aspect stretch, ~1.3 % more picture), 75p max. It was verified with a clean picture and edges, and 75p recorded with 0 dropped frames.
- Early crop probes (2026-09-25) failed because the driver owns VMAX. The working path came from the driver analysis.

### 8.5 DOL-HDR
- `MI_SNR_SetPlaneMode(0,1)` gives the planes `IMX307_MIPI_HDR_LEF` (long) and `_SEF` (short).
- VIF group word 2 = 2 (DOL). VIF dev and port pixel format 0x2c (RG 12 bpp = 0x14 + precision 2 × 12 + bayer); with the wrong format the picture is solid magenta. ISP param word 0 = 2.
- Registers: VMAX 1158, HMAX 2136 (14.38 µs line), 60.04 sensor frames/s, i.e. 2 exposures per 30 fps output frame. BLKLEVEL 240 (12-bit), ADBIT 1, WDSEL 0x11, CSI_DT_FMT 0x0C0C (RAW12), Y_OUT_SIZE 2426, SHS2 1644, RHS1 101.
- libcus3a runs two shutters by itself (long 71 µs..333 ms, short 28..3036 µs). Manual exposure in HDR was verified (5000 µs / 2x set, 4995 µs / 2.00x read back, 31 frames/s).
- HDR has no crop and no DNG. Its frame rates are 24/25/30. It is very dark in a dim room.
- In HDR, `MI_ISP_AE_GetHisto0HwStats` returns all zeros about every second read. This only mattered for the old ISP histogram.

### 8.6 RAW and DNG
- The VIF output is FBC-compressed to 8 bits per pixel unless the `mi_vif` module parameter `g_bFBCMode=N` is set before VIF starts. camapp writes `/sys/module/mi_vif/parameters/g_bFBCMode` at startup. The picture is unaffected.
- Uncompressed RG_10BPP is a little-endian bit stream, 4 pixels in 5 bytes, stride W × 10 / 8 (2400 for 1920).
- The raw buffer is the readout **rotated 180°** against the displayed picture (mirror/flip off). The DNG therefore carries Orientation 3 with the pixels in readout order, RGGB at (0,0).
- Black level 60 (register 0x300A, 10-bit), white 1023.
- ColorMatrix1 = inverse(sRGB→XYZ × ISP CCM × AWB gains) for D65, plus AsShotNeutral from the gains. Minimal DNG 1.4 with 16-bit samples: a full frame is 4,147,680 bytes.
- `dngview` develops the raw frame on the device to a 160x90 PPM with the DNG's own black/WB/matrix/orientation. It verified that the colours and orientation match the live picture.
- Unverified: mirror/flip on, crop mode, and opening the files in Lightroom/Resolve (left to the user).
- RAW video was dropped: 1080p60 10-bit is ~155 MB/s, far beyond the SD bus.

### 8.7 Encoders
- The JPEG engine is VENC dev 8 (as in cardv). The H.265/H.264 encoder is VENC dev 0, created with `{0x1200, 0xa20}` (max 4608x2592).
- Channel attr, 19 u32 words:

  | Word | Meaning |
  |---|---|
  | [0] | type: 2 H.264, 3 H.265, 4 JPEG |
  | [1], [2] | max W/H |
  | [3] | buffer size |
  | [4] | profile |
  | [5] | by-frame 1 |
  | [6], [7] | picture W/H |
  | [10] | rate control: H.264 CBR 1 / VBR 2, H.265 CBR 8 / VBR 9, MJPEG fixQP 7 |
  | [11] | GOP |
  | [12] | stat time |
  | [13], [14] | fps num/den |
  | [15] | bitrate |
  | [16], [17] | VBR max/min QP (CBR: [16] = fluctuate) |
- JPEG quality is word 0 of `MI_VENC_Get/SetJpegParam` and applies live. At q30 a frame is 43 KB, at q70 69 KB, at q95 193 KB. One encoder serves UVC, photos and MJPEG clips.
- cardv's extra H.265 settings are also applied: `SetRefParam {1,0,0}`, `SetRcPriority 2`, `SetH265Dblk {0,3,3}`.
- VUI Get/Set is only valid after `StartRecvPic`. Slow-mo clips get the playback rate in the VUI timing. H.264 `time_scale` counts fields (2 × fps × 1001).
- The bind frame-rate arguments are 16-bit fps, and `RecvPicEx` does not gate frames. So Timelapse cannot be done inside the H.265 encoder: Timelapse records MJPEG with software frame selection.
- 60→30 fps decimation for recording happens in the bind (src 60 → dst 30), before encoding.
- The SigmaStar H.265 CBR does not pad: a dark static scene gives ~3 Mbps even at 40 Mbps.
- Stream fetch (from cardv @0x494ec):
  1. `MI_VENC_GetFd` + `select`.
  2. `MI_VENC_Query` (stat 0x28 bytes, +0x10 = pack count).
  3. `MI_VENC_GetStream(dev, chn, {packs*, count, ...}[0x40], -1)`. A pack is 0xb8 bytes: +8 data pointer, +0xc length.
  4. `MI_VENC_ReleaseStream`.
  
  H.265 packs may come separately (VPS/SPS/PPS/SEI/slices, up to 8). A keyframe is detected by a VPS/IDR NAL.
- Sharing one SCL port between JPEG and H.265 cost ~6 % fps, so recording gets its own port 2.

### 8.8 OSD framebuffer
- `/dev/fb0` "SStar FB0" is 1600x900 ARGB8888 (6400-byte lines), GOP 1 / GWIN 0, scaled to 1920x1080 on HDMI (×1.2). Alpha blend is per pixel.
- Transparency comes from colour key 0x0a0a0a: fill with 0xff0a0a0a for see-through.
- SStar ioctls:
  - `FBIOGET_SHOW 0x80014662`, `FBIOSET_SHOW 0x40014663` (u8).
  - `FBIOGET_GLOBAL_ALPHA 0x80074664`, `FBIOSET_GLOBAL_ALPHA 0x40074665` (7 bytes).
  - `FBIOGET_COLORKEY 0x80044666`, `FBIOSET_COLORKEY 0x40044667` ({enable, r, g, b}).
  - State: `/proc/mi_modules/mi_fb/mi_fb0`.
- **The owner must keep `/dev/fb0` open**: the driver drops the colour key when the fd closes and the overlay disappears.
- Anything that gets cleared and redrawn is drawn off-screen and only the changed rows are copied; otherwise it flickers. The analysis cells and guides are composited under the status and corner text. Cleared elements invalidate the cells under them.
- `fill()` clips on all sides. A spot box at the frame edge once crashed camapp through a negative coordinate.
- The OSD is only on HDMI. The USB picture and recordings never contain it (§15.1).

### 8.9 UVC
- This is the kernel 4.9 `f_uvc` userspace protocol on `/dev/video0` (opening it turns the USB pull-up on).
  - Private events `V4L2_EVENT_PRIVATE_START + {0 connect, 1 disconnect, 2 streamon, 3 streamoff, 4 setup, 5 data}`.
  - `UVCIOC_SEND_RESPONSE = _IOW('U',1,{s32 length; u8 data[60]})`.
- Probe/commit: the 26-byte UVC 1.0 control. SET_CUR responds with the length only, then the DATA event arrives. Unknown requests are stalled with `length = -EL2HLT`.
- On STREAMON: `S_FMT` MJPEG, then `REQBUFS` 3 × 2 MB MMAP. Each `DQBUF` gets the newest encoder JPEG, paced to the host's committed interval. Unpaced, it pushed 120 frames/s and used 27 % of a core.
- At start camapp toggles `/sys/class/udc/<udc>/soft_connect` (300 ms) so the host re-enumerates after a camapp restart.
- Controls: the 4.9 configfs descriptors are read-only and advertise only AE mode (camera terminal ID 1) and brightness (processing unit ID 3). camapp answers both:
  - AE mode: 1 manual → M, 2 auto → P, 4 shutter priority → S. G reads as auto.
  - Brightness = EV compensation −9..+9 thirds.
  
  Verified with `v4l2-ctl` over usbipd. More controls (exposure time, gain, WB) would need a binary patch of the `usb_f_uvc.ko` defaults (`bmControls` set in `uvc_alloc_inst`).
- The stock `g_webcam` advertised fake sizes and a still pin that made Windows Camera time out. The own gadget fixed that. If `uvc-gadget.sh` falls back to `g_webcam`, every size still gets 1080p frames.
- If Windows stops enumerating the camera (udc state "default", full-speed), a camera reboot fixed it (2026-09-29).
- Android UVC viewer apps work well ("amazing", user). This is the phone-as-viewfinder use.

### 8.10 Analysis port, loupe
- SCL port 3 gives a 480x270 NV12 copy of the picture:
  - Set up with `MI_SYS_SetChnOutputPortDepth(port3, 2, 4)`, then `MI_SYS_ChnOutputPortGetBuf(port, info, &handle)` / `PutBuf(handle)` (from cardv FUN_000510c4).
  - BufInfo: +0 u64 pts, +0x10 buffer type 1, +0x24 pixel format 0xb NV12, +0x38 u16 w + u16 h, +0x3c luma ptr, +0x40 chroma ptr, +0x48/+0x50 phys, +0x60/+0x64 strides, +0x6c size.
- Loupe: SCL port 0 param word 0 = crop x | y<<16, word 1 = crop w | h<<16, set live. Only the display is cropped; ports 1 and 2 keep the full frame. The analysis port follows the loupe crop, so the histogram describes the magnified area.

### 8.11 Playback (parked)
- The device FFmpeg 4.1.3 is SigmaStar's build: decoders `sshevc`, `ssh264` (VDEC) and `ssmjpeg` (JPD); demuxers mpegts, mov, image_jpeg_pipe.
- Decoded frames are NV12 at 1280x720 for 1080p sources (the decoder's own scaler). They are copied into `MI_SYS_ChnInputPortGetBuf` of DISP port 0, with the SCL→DISP bind removed during playback.
- Decode speed: 34 fps with the live pipeline running, 43.6 fps without. 24/25/30p play in real time; 60p plays at ~0.55x.
- **The device decoder smears the bottom of 1080-high H.265.** Recording at 1088 with an SPS conformance window cropping 8 rows fixed it: the files stayed standard 1080 on the PC. That code was reverted with the rest of the post-compact work on 2026-09-25 and never redone.
- PC x265 streams (DPB 3, open-GOP CRA, AUD/SEI) freeze the decoder ("no available frame buffer").
- **A killed decoder breaks VDEC until camapp restarts. Never `kill -9` a decoding process.**

### 8.12 SD card handling
- Presence is `/sys/block/mmcblk0/mmcblk0p1`. `sd_mounted()` needs both presence and a vfat mount at `/app/bin/sd`: a pulled card leaves a stale mount.
- When the card is pulled: `umount2(MNT_DETACH)` and "SD card removed".
- When a card is inserted: "Please wait... Initializing SD card". On the next tick camapp mounts it and runs a write/read-back check, then shows "SD ready: x GB free" or a red error. It does not retry until the card is pulled.
- Recording refuses to start without a mounted card. This is essential: without a card, `/app/bin/sd` is a plain directory in the RAM rootfs, and filling it froze the system once.
- **Format SD** (OK twice, refused while recording) runs in a background thread:
  1. umount.
  2. Zero the first and last MB.
  3. `fdisk` one FAT32 LBA partition.
  4. `blockdev --rereadpt`.
  5. `mkfs.vfat -F 32 -n CAMERA`.
  6. Mount.
  7. Write 256 KB, drop caches, read back.
  
  The result is "Verified" or "FAILED (step n)".
- Cards seen:
  - A 32 GB SanDisk "SK32G" (A1 C10): ~8.5 MB/s in early tests.
  - A 58.4 GB "C10" card, the one in use: 13.9 MB/s dd write.
  - The user's 55 GB "U3" card silently discards all writes (counterfeit or worn). Nothing on it was changed.
- To pull the card safely: `sync; umount /app/bin/sd`, then remove it.

---

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
- Recording › Photo = `JPEG + DNG` also writes `IMG_nnnn.DNG` (raw 10-bit, ~4 MB, §8.6); not in HDR.
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
- SD card: free / size. Format SD: §8.12.
- Build: the compile date and time (also the first log line, `camapp build ...`). Uptime.
- CPU: both cores over the last second. RAM: available of total, plus camapp's own use (video buffers live in the ~170 MB MMA heap). Flash: free space on `/customer`.
- Timecode reset.
- Save logs: copies `/tmp/camapp.log` and `dmesg` to `LOGS/LOG_nnnn_camapp.txt` and `LOGS/LOG_nnnn_dmesg.txt` on the SD card.
- Reset settings (OK twice): `settings_reset()` + re-apply everything.

---

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
| `iq` `gamma` `shp` `ccm` `hsv` `nr` `nr3d` `wdr` `wbinfo` `dpdump` `nradump` | struct dumps (§13) |
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

---

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

---

## 13. ISP / 3A / encoder struct layouts

All were found in Ghidra decompiles of cardv / libmi_isp.so and in FIFO dumps. Sizes come from each getter's ioctl size. The "auto" tables have 16 entries indexed by ISO/gain; entry 17 is the manual slot. The vendor dumps are in `logs/*_dump.txt`.

| Struct (Get/Set) | Size | Layout |
|---|---|---|
| AE QueryExposureInfo | 0x23c | +0 stable, +4 reach boundary, +8 {FNx10, sensor gain, ISP gain, shutter µs}, +0x28 lumY, +0x2c avgY (linear, ~0-255, saturates ≈250), +0x230 LV×10, +0x234 BV, +0x238 AE target (same units as avgY) |
| AE ExpoMode | 4 | 0 auto, 4 manual |
| AE ManualExpo | 16 | {FNx10, sensor gain, ISP gain, shutter µs (driver units)} |
| AE ExposureLimit | 32 | min shutter, max shutter (driver units), min FN×10, max FN×10, min sensor gain, min ISP gain, max sensor gain, max ISP gain. Vendor: 80 (40 µs real), 10000 (1/200 real), 18, 18, 1024, 1024, 8192, 1024 |
| AE Converge | 40 | vendor 20 30 / 100 470 470 2000 / 300 200 200 300; the last four are the speeds |
| AE EVComp | 8 | {s32 EV, u32 grad}; grad 3, so EV counts in 1/3 stops |
| AE State | 4 | 0 run, 1 pause (lock) |
| AE Flicker | 4 | 0 off, 1 60 Hz, 2 50 Hz |
| AE WinWgtType | 4 | 0 average, 1 centre, 2 spot |
| AE WinWgt | 0xc04 | u32 mode + 3 × u8[32][32] (average, centre, spot); only 16x16 used (AE grid) |
| AE Histo0HwStats | 0x100 | 128 × u16 bins, linear pre-gamma; GetHistoWghtY 0x208 (lumY, avgY, 128 × u32) identical |
| IQ Brightness / Contrast / Lightness | 0x4c | u32 enable, u32 op type (0 auto, 1 manual), 16 auto u32, 1 manual u32 |
| IQ Saturation | 0x1a0 | 8 + 17 × 24; byte 0 = strength; manual at 8 + 16×24 |
| IQ OBC (black level) | 0x90 | 17 × 4 u16; manual 3845 = 240 × 16 |
| IQ ColorToGray | 4 | u32 enable |
| IQ RGBGamma | 0x6608 | 8 + 17 × (u16 R[256], G[256], B[256]) = 1536-byte entries, 10-bit output |
| IQ Sharpness | 0x1328 | 8 + 17 × 288; bytes 1 and 2 = edge gains (what `isp_sharpness` scales) |
| IQ RGBMatrix (CCM) | 0x1e8 | u32 enable, u32 op type, u32 0, 16 auto × 28 {u16 CCT, u16 m[3][4] (col 3 = row sum), u16 sat}, manual {m[3][4], sat, pad} at 0x1cc; 13-bit two's complement, 1024 = 1.0; vendor calibrated at 3000/4000/6500 K. QueryCCMInfo 0x1a = live m + CCT |
| IQ HSV | 0x134c | u32 enable, u32 op type, 17 × 290 = s16[145]: 3 × {s16 hue[24], u16 sat[24] (64 = 1.0)} + 1; 24 points = 15°, point 0 = red |
| IQ NRLuma_Adv | 0x9bc | 8 + 17 × 146; two u16[8] threshold tables at entry +2 (strength); +130 per-level u8, +68 falloff, +36 limits |
| IQ NRLuma / NRChroma (basic) | — | no measurable effect in this pipeline; the `_Adv` variant is live |
| IQ NR3D | 0x844 | u32 enable, u32 op type, 17 × 124; entry +2 u16 motion gain (vendor 1000), +4 u8 temporal strength (64), +54 u16[16] strength vs motion; NR3D_EX 0x24, NR3D_P1 4 |
| IQ WDR | 0x668 | 16 auto entries + manual at 0x608; strength byte +0x2B (vendor boots manual, strength 0 = WDR effectively off) |
| IQ DynamicDP | 0x2f4 | 8 + 17 × 44: enable, op, 16 auto + manual; entry = hot enable, hot slope (16), dark enable, dark slope (64), curves |
| AWB Attr | 0x5b8 | +0 u32 state (1 hold), +4 op type (0 auto, 1 manual gains, 2 colour temperature), +8 u16 R, Gr, Gb, B (1024 = 1x); `SetCTMwbAttr(u32 K)` with op type 2 = Kelvin. QueryInfo 0x18: u16 stable, ?, R, Gr, Gb, B, CCT |
| CUS3A GetAFStats | 0x1c00 | 8 windows × 5 u32 (word 0 HF energy, word 4 luma); vendor windows 4x2 over half the frame, 0..1023 coordinates, settable with SetAFWindow |
| VENC chn attr | 19 u32 | §8.7 |
| VENC JpegParam | ≥0x90 | word 0 = quality |
| VENC stream | 0x40 | {Pack *packs, u32 count, ...}; pack 0xb8: +8 data ptr, +0xc length; Query stat 0x28, +0x10 packs |
| SCL port param | 6 u32 | {crop x | y<<16, crop w | h<<16, out w | h<<16, 0x101, pixfmt 0xb NV12, 0} |
| SYS BufInfo | 0x108 | §8.10 |
| DISP input GetBuf conf | 0x74 | u32 type 1 (+0), u16 w (+0x18), u16 h (+0x1a), u32 pixfmt 0xb (+0x20) |
| DISP pub attr | 13 u32 | {bg 0x800080, interface 5 HDMI, timing 0x10 1080p60, sync info zero} |
| HDMI attr | 12 u32 | {0, enable 1, timing 9 = 1080p60, 0, 0, 0, 4, ...} |
| RGN (Mercury6) | — | every call takes a leading u16 SoC id (0). Create {u32 type 0 OSD, u32 pixfmt (0 ARGB1555), w, h}; ChnPort {0x22 SCL, dev, chn, port}; ChnPortParam 40 B: +0 u8 show, +4 x, +8 y, +0xc layer, +0x10 alpha mode, +0x14 bg alpha, +0x15 fg alpha, +0x18 invert; Canvas +0 u64 phys, +8 virt, +0xc w, +0x10 h, +0x14 stride, +0x18 fmt |

---

## 14. Measurements and performance

- Frame rate: VIF 60.0 fps at 60p. The JPEG encoder does 60.1 fps (1202 frames in 20 s). The JPEG and H.265 encoders together hold 59.8-60 fps.
- CPU:
  - Normal mode on the System page, as seen by the user: ~19 %.
  - 100p crop with scopes and UVC on: ~60 % of 2 cores. Waveform + histogram ~30 % of a core; ISP kernel threads ~40 %.
  - Scopes went from 59 % to 28 % of a core (parade + histogram + false colour) after the 15 Hz loop and the capped status recomposite.
  - UVC unpaced used 27 % of a core.
- Memory: Linux ~72 MB. With the stock stack stopped, 43 MB free. MMA heap 172 MB, ~51 MB used by the pipeline, ~110 MB free (enough for a RAM pre-trigger buffer of ~1000 JPEG frames). `/customer` 6.9 MB, ~3.2 MB free.
- JPEG frame sizes: 55-160 KB at q70 1080p, depending on the scene (~70 KB typical).
- H.265 bitrate: a lit scene at 40 Mbps target measured 40.9 Mbps, 60.00 fps, clean decode, 0 dropped.
- SD: 50 MHz 4-bit bus (25 MB/s ceiling). Write 9.5-14 MB/s through FAT, read 18.7 MB/s.
- Shutter check against the registers: 1/48 = 20830 µs real at 24/25/30p; 1/120 at 60p = 8326 µs; the shortest shutters 237/119/59/30/15 µs were verified on SHS1.
- AE: vendor speed overshoots once (~+70 %) and settles in ~350 ms; 50 % speed settles in 200-350 ms without overshoot; 200 % hunts for more than 1.5 s.
- Temporal NR / sharpness / NR effects were measured as JPEG size at fixed scenes (`jpgavg`). Example: temporal NR Off 99.1k > Low 95.2k > Default 89.2k > High 88.5k bytes/frame.
- Decode: 34 fps (sshevc, live pipeline running).
- Boot: faster to picture than stock (never measured in seconds). camapp is healthy about 15 s after start.
- UART: 115200 baud ≈ 11 KB/s raw. A push of the 178 KB camapp takes ~2 minutes with gzip. Pulls stream at several KB/s, or ~5 KB/s in 1 KB chunks.

---

## 15. Known bugs, limits, oddities and parked work

### 15.1 OSD into the USB/recorded picture (MI_RGN): parked after 7 attempts
Goal: burn the OSD into the UVC picture (optionally), via a hardware overlay region on the SCL port that feeds JPEG.

Facts established:
- The layouts are in §13 and in the `rgn.c` header.
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
§8.3. It needs the SoC MIPI receiver configured for 891 Mbps/lane. The driver never does that; `padinfo` dumps the receiver words for a start.

### 15.4 Playback and gallery: parked
- Stage 1 works (FIFO `play`, pause, seek ±5 s, stop back to live). There is no gallery page, and no button starts playback (the gallery button cycles the guides).
- Blockers:
  - The bottom smear on 1080-high H.265 (§8.11); the 1088 + SPS conformance window fix was reverted and never redone.
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
- **UVC** exposes only 2 controls (§8.9).
- **SD UHS** 1.8 V switch fails: a board limit (§2.2).

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
| 19-20 | Playback | play/seek, gallery, photo viewer | parked (§15.4) |
| 21 | Modes | Normal / HDR / Slow-mo / Timelapse | done |
| 22 / 22b | Modes | sensor crop up to 105p at 1280; open gate | done |
| 22c | Speed | above 120 fps at narrow crops | skipped by the user |
| 22d | Speed | full-width 120p | parked (§15.3) |
| 23 | Slow-mo | RAM pre-trigger ring buffer | open (optional) |
| 24 | Timelapse | MJPEG, interval, stop after, shutter to 2.1 s | done |
| 24b | Timelapse | shutter beyond 2.1 s, H.265 timelapse | skipped |
| 25 | Metadata | sidecar, record-run timecode, clip names, crop in sidecar | done |
| 26 | Encoder | JPEG quality, GOP, CBR/VBR + QP, H.264, slow-mo VUI | done |
| 27 | Hardware | System page CPU/RAM/flash/SD, save logs | done |
| 28 | USB | own configfs UVC gadget 1080p 30/60, paced, reconnect | done |
| 28a | USB | OSD injection into UVC (MI_RGN) | parked (§15.1) |
| 28b | USB | record to PC over USB | idea |
| 28c | OSD | OSD hide (body double click, FIFO); no remote button assigned | done |
| 28d | USB | mass storage | blocked |
| 28e | USB | UVC controls: AE mode + brightness | done (2 controls) |
| 28f | USB | own gadget modules (network, MSC, serial) | experimental, not started |
| 28g | USB | host mode + SSD | investigate (physical) |
| 29 | Host control | USB network control, remote preview | planned, needs 28f |
| 30 | HDMI | output modes | parked (§15.2) |
| 31 | Performance | scopes 15 Hz, capped recomposite, named threads | done |
| 32-33 | Firmware | own image / OTA (U-Boot SD upgrade path); custom kernel | planned / blocked |
| 34 | RAW | DNG photo | done (RAW video removed) |
| 35-36 | Tools | flicker meter, slit-scan / photo finish; stabilisation | idea |
| 37 | Hardware (user) | C-mount lens mount + IR-cut, 32 MB NOR, microphone | user's plan |
| 38 | Oddities | hot pixels; 32x gain | investigated, no ISP fix |
| 39-41 | Looks | replicate other cameras, full profiles, PC tuning files | idea / parked |
| — | Ideas | dark-frame subtraction for night timelapse; shade-style frame guides; GPIO hunt page; console overlay; WB flow rework; persistent log in the KMSGLOG flash area; HCG check | ideas |

---

## 17. Debugging playbook

| Symptom | Check | Fix |
|---|---|---|
| No picture on HDMI after a change | `tail /tmp/camapp.log`: which MI step failed (`= -xxxxx`) | deploy rolls back by itself; else `/customer/camapp/rollback.sh` |
| camapp keeps dying at boot | `boot_attempts` count; after 3 → stock cardv runs | fix build, `echo 0 > /customer/camapp/boot_attempts`, reboot |
| Picture frozen | `grep -A1 IsrCnt /proc/mi_modules/mi_vif/mi_vif0` twice: count must grow; encoder count in the status log | never trust a still picture: prove frames with the counter or the user's timer |
| Garbled / purple columns | frame rate too high for the width (HMAX below the MIPI limit) or bad register write | lower fps / `regundo` / restart |
| Crop refused at start | log "crop: pipeline start failed" | camapp resets the crop and restarts by itself |
| Dark picture after changes | IQ bin not loaded? AE limits? | check log for `MI_ISP_API_CmdLoadBinFile`, `aeinfo` |
| Recording drops | status `BUF`/`DROP`, log lines with reason and size | lower bitrate, use H.265, faster card; MJPEG 60p always drops |
| No SD / "SD card error" | `/proc/mounts`, `dmesg` | reinsert, or System › Format SD |
| USB webcam not showing | `cat /sys/class/udc/*/state` (should be `configured`), log `uvc:` lines | reboot the camera; check `uvc-gadget.sh` fell back or not (`lsmod`) |
| deploy.sh md5 mismatch | console flooded (HDMI mode test, kernel spam) | stop the flood (restore `hdmiout 0`), retry; lines ≤ 900 chars |
| UART output garbled / tools fighting | two programs on `/dev/ttyUSB0` | one owner only |
| Decoder wedged (playback) | `playstop` did not return | restart camapp; never `kill -9` a decoding process |
| Stock cardv hangs after camapp | known | reboot |

Useful one-liners on the camera:
- `grep -A1 IsrCnt /proc/mi_modules/mi_vif/mi_vif0`: the frame counter.
- `echo sensor > /tmp/camapp_fifo; sleep 1; tail -20 /tmp/camapp.log`: register-truth fps and exposure.
- `cat /customer/camapp/settings`
- `top -H -p $(pidof camapp)`, or push `tools/device/cpustat.sh`.
- `cat /sys/devices/virtual/mstar/msys/TEMP_R`

---

## 18. Working rules and lessons learned

### 18.1 The user's rules (standing)
- One named step at a time, with a pass condition. Build, then `tools/deploy.sh`, then verify on the device, then commit. Report at milestones; ask only real decisions, with a recommendation. "Focus on one thing at a time."
- **Stop and report after 2 failed hypotheses** on one problem.
- **Verify that frames really advance**: the VIF/encoder count, the JPEG size, or the user's running timer in front of the camera (mirrored digits). Never trust a still picture.
- Never restart camapp from a background script. Never `kill -9` a decoding process. Move only a few KB over UART per pull.
- Tests may change the camera settings freely; no backup/restore ritual.
- Expert controls expose the real parameter as a variable number over its full range (plus Auto/Off), never presets. "Unmatched control."
- Only real sensor/ISP processing for image controls, no artificial software filters.
- The remote has no OK and no long press: every action must work from the remote (play/pause = confirm/reset, mirror = back).
- Docs are plain local Markdown files, not published pages.
- **Host access**: ask before any new Windows-side access (usbipd, drivers, installs). Detach USB devices back to Windows after tests. Accepted: Windows ffmpeg for frame grabs, UART, deploy tooling.
- Binaries go to the camera over UART (`uart_push.py`), not via the SD card. The SD card may hold files and logs.
- After a context compaction: read the state documents fully, say where work resumes, and wait for "go".
- **Commits and anything for GitHub carry no Anthropic/Claude attribution** (user, 2026-09-30). Earlier commits in this repository still contain a `Co-Authored-By: Claude ...` trailer; removing it needs a history rewrite, which the user has not asked for.
- Replies in the chat are terse; persisted text (commits, docs) is normal English.

### 18.2 Lessons (mistakes that cost time)
- **2026-09-25 compaction disaster**: after a context compaction mid-debugging (playback smear), acting on the summary alone stacked guesses, twice mistook a frozen frame for working playback, and wedged the decoder. The user ordered a full revert of all post-compact work (git history starts after that revert, `78a8896`). Rule: read the state documents after every compaction and wait.
- Rejected or interrupted tool calls may have partly run: edits were found applied twice, or applied although "rejected". Always grep and check the device state before redoing.
- Tests once silently overwrote the user's manual exposure; the rule later relaxed to "tests may change settings".
- Claims must be checked on the real output. Examples: "spot centred" (it was not, stored spot_y 7), "110p clean" (it was not), "frame guides done" (they were cut off by the OSD). Look closer before claiming done.
- Committing before the user verified a visual change was premature. Ask the user to look for visual items.
- Large UART transfers look like hangs (500 KB push, 120 KB pull); compute statistics on the device with busybox `od`/`awk` instead.
- Python splice edits duplicated code blocks twice. Prefer exact edits, and build-check after every edit.
- The WSL `/tmp` scratchpad was wiped by a WSL restart; keep anything useful in the project folder.

---

## 19. Repository map, logs index, scratchpad dump

### 19.1 Repository (`/home/aftwr/microscopeproject`)
| Path | In git | What |
|---|---|---|
| `README.md` | yes | project overview and quick start (GitHub front page) |
| `LICENSE` | yes | GPL-2.0 |
| `docs/` | yes | guides (install, build, usage, FIFO commands, settings, hardware, recovery, internals, known issues), `HANDOFF.md` (this document), `imx307_registers.md`, `lenses.md`, `looks.md` |
| `docs/notes/` | yes | `COMPACT.md` (working state document, see §16 corrections) and `handoff-log.md` (the frozen chronological log, formerly `handoff(5).md`) |
| `src/camapp/` | yes | the application (§7) |
| `src/mitrace`, `src/getexpo`, `src/fbtest`, `src/playtest`, `src/hello` | yes | helper programs (§6.4) |
| `tools/` | yes | PC and device tools (§6.3) |
| `research/decompiles/` | yes | Ghidra decompiles of cardv, the SigmaStar libraries and the imx307 driver (formerly `logs/dec_*`) |
| `archive/scratchpad/` | yes (except `*.bin` and `claude_memory/`) | scratch files (§19.3) |
| `logs/` | no | logs, dumps, captures (§19.2) |
| `extracted/` | no | binwalk extraction of the golden dump; `extracted/rootfs` is the link sysroot |
| `live_system/`, `live_system.tar.gz` | no | live filesystem capture (partial) |
| `ghidra/` | no | Ghidra project (+ pyghidra-mcp data) |
| `third_party/` | no | FFmpeg 4.1.3 source/headers |
| `*.bin` (root) | no | flash dumps (golden = `microscope_C22018_0xE41B1D54.bin`) |
| `REC_*`, `TEST_*` (root) | no | early recordings/test clips pulled from the SD card |
| `.mcp.json` | yes | Ghidra MCP server config (local paths) |
| `.claude/settings.local.json` | no (local) | Claude Code permissions |
| `scratch/` | no | live scratch folder of the tools (recreated on use) |

### 19.2 `logs/` index (not in git; decompiles moved to `research/decompiles/`)
- Boot and UART: `boot_2026-09-24.log` (stock boot), `boot_B2/B3/B3b/B4_*.log` (chassis boot tests), `usb_plug_2026-09-24.log`, `hdmi_replug_uart.log`, `ir_capture_raw.log`, `iso_test_uart.log`, `mitrace_*_uart.log`, `cardv_restart.log`, `lsusb_v_2026-09-24.txt`, `v4l2_all.txt`, `xu_probe.txt`.
- camapp step runs: `camapp_3b_run1.log` through `camapp_6b_run2.log`, `camapp_B1.log`, and the teardown tests (`camapp_exit_while_streaming.log`, `camapp_host_teardown.log`, `camapp_teardown_diag.log`).
- MI traces of cardv: `mi_trace_boot1/2/3.log`.
- Ghidra decompiles (now in `research/decompiles/`): `dec_CarDV_*ModuleInit.c`, `dec_CarDV_VifDevInit.c`, `dec_CarDV_IspChannelInit.c`, `dec_CarDV_DispToHdmiTiming.c`, `dec_FUN_000494ec.c` (stream fetch), `dec_MI_ISP_CreateChannel.c`, `dec_VideoEncoder_createNew.c`, `dec_VideoPipeInit.c/.json`, `dec_avcodec_sstar.c`, `dec_cardvInputMessageLoop.c/.txt`, `dec_getbuf_user.c`, `dec_imx307_MIPI_ko.c`, `dec_initVideoEncoder.c`, `dec_isp_process_cmd.c`, `dec_open_video.c`, `dec_player_display.c`, `dec_player_pipe.c`, `rec_5b_attempt.c.txt`.
- ISP dumps: `aeinfo_dump.txt`, `aetrace_*.txt`, `awb_dump.txt`, `ccm_dump.txt`, `gamma_dump.txt`, `hsv_dump.txt`, `iq_dump.txt`, `nr3d_dump.txt`, `nr_dump.txt`, `nrluma_adv_dump.txt`, `shp_dump.txt`, `wdr_dump.txt`, `imx307_regs_hdr.txt`.
- Captures: `uvc_own.mjpeg/.jpg`, `snap_5a.jpg`, `h265test.h265`, `h265pattern.h265/.jpg`, `ts_pattern_test.ts`, `rec_000x_tiles.jpg`, `rec_bottom*.png`, `p120_probe.jpg`, ISO/ESP/band test clips, `isolive.mkv`, `shutterlive.mkv`.
- `flash/`: partition pieces and MISC contents (§3.3).
- Ghidra run logs: `ghidra_analysis.log`, `ghidra_server.log`.
- `handoff5_before_edit.md`: an older 433-line copy of the handoff from before the 2026-09-26 cleanup (commit `fccfd7b`). Superseded; nothing unique in it.

### 19.3 `archive/scratchpad/`
See `archive/scratchpad/README.md`. In short:
- `project_scratch/`: the project scratch folder as it was on 2026-09-30. It holds images and PPMs from the DNG, open-gate, RGN, hot-pixel and HDR tests, `edid.bin`, the helper scripts `dnghook.py`, `focus.py`, `hdmi.py`, `og.py`, `prio.py`, `spot.py`, `dptest.sh`, and backups of the UART tools.
- `recovered_from_transcript/`: 37 scratch files rebuilt from the session transcript (`recover_scratch.py`), including `brio.sh`, `hset.sh`, `hsweep.sh`, `hsweep445.sh`, `cpustat.sh`, `tstest.c`, `hevctest*.c`, `aeexp*.sh`, `sercap.py`, `uartmon.py`, `keylog.py`, `xu.py`, `xref.py`, `export_fns.py` and more. `RECOVERY_LOG.txt` lists every replayed operation.
- `claude_memory/`: the assistant's persistent memory files for this project (the working rules above). On disk only, not published (gitignored).
- `hdmi_output_modes_stash.patch`: `git stash@{0}` as a patch.
- Lost for good: the Ghidra headless setup (`ghko/`), `cardv.dis` / `gkuui.dis` / `avcodec.dis` listings (regenerate with objdump), the `camapp.c.pre-*` source backups (superseded by git), BRIO photos, and `postcompact/` (the reverted 2026-09-25 code, including the 1088 recording fix).
- The full session transcript (all conversations, ~89 MB JSONL) is at `~/.claude/projects/-home-aftwr-microscopeproject/9d550714-17fb-41e2-87ba-ede55841232b.jsonl`.

### 19.4 External references
- SigmaStar MI API docs (SSD20X / SSD220 SDK mirror): https://wx.comake.online/doc/d8clf27cnes2-SSD20X/customer/development/software/Px/en/sys/P2/quick%20start.html and the MI_RGN page https://wx.comake.online/doc/ds82ff82j7jsd9-SSD220/customer/development/mi/en/exclude/mi_rgn.html (API pages at `.../mi/en/mi_<name>.html`). Use them for semantics only; verify struct layouts on Mercury6.
- Chip IDs: OpenIPC ipctool `src/hal/sstar.h`. Community: linux-chenxing.org (has a Mercury6 section). OpenIPC/thingino do not support Mercury6.
- Search terms that work: `Mercury6`, `Tiramisu`, `SSC8826Q`, `SSD268G`, `DGECAM V522`. Avoid "SStar Soc".
- IMX307 register map: the Linux `imx290` driver (IMX290/IMX307/IMX327 share it).

---

## 20. Timeline

### 20.1 Before git (from docs/notes/handoff-log.md)
- **2026-09-16/17**: golden flash dump (two matching reads, CRC32 0xE41B1D54); binwalk extraction; live filesystem tar.
- **2026-09-24**:
  - UART access and the full boot log. SoC identified as Mercury6; sensor IMX307; flash map; auth chip analysis.
  - cardv FIFO decoded (shutter/iso). Ghidra + MCP setup. mitrace shim and the cardv bring-up recipe.
  - camapp steps 3b (sensor→VIF→ISP), 3c (HDMI picture), 4a (AE via the IQ bin), 4b (manual shutter/gain), 5a (JPEG encoder 60 fps), 5b (UVC from our pipeline), 6/6b (MJPEG AVI recording, writer thread).
  - Chassis 1-4: boot integration with fallback, unified input, settings persistence, safe deploy with rollback.
  - OSD framebuffer; EV meter; metering and spot; EV comp; guides; ISP histogram; menu steps 1-3 (shell, exposure, System page with format).
  - **Sensor timing bug found** (driver factor 2) and corrected.
  - Expert › Sensor, temporal NR, WDR (as variable numbers); photo feedback; SD hotplug; DOL-HDR steps 1-3; register viewer and writer; ISP image controls, gamma, sharpness, colour matrix and matrix edit; white balance; hue tweaks.
- **2026-09-25**:
  - Noise & clean-up page; AE limits (8x cap found), AE lock, fast shutters, AE speed, anti-flicker; AE hunting explained.
  - H.265 encoder + MPEG-TS muxer + H.265 recording.
  - Loupe, focus peaking, zebra + false colour, the displayed-luma histogram, waveform.
  - Flash findings (SD upgrade path).
  - Playback research, then playback stage 1; smear root cause (1080-high streams); 1088 recording fix.
  - Then the **revert** of all post-compact work (~07:35). Git history starts at 07:46.

### 20.2 Git history (105 commits, oldest first)
- `78a8896` 2026-09-25 07:46 Stable pre-compact camapp state (reverted 2026-09-25)
- `69f8f56` 2026-09-25 07:51 rec: log reason and size of every dropped frame
- `21e578e` 2026-09-25 07:51 notes: in-progress section
- `ad7fb07` 2026-09-25 07:59 osd: time left on card (measured rate while recording, smoothed estimate in standby)
- `ed26e92` 2026-09-25 08:07 rec: clip counter in status bar; clip numbers unique across .ts and .avi
- `e3a00c5` 2026-09-25 08:17 rec: red tally frame while recording; photo flash returns to it; status bar keeps clear of the frame
- `b24515f` 2026-09-25 08:24 osd: bottom corner blocks (codec, WB, NR, WDR, AE speed, hot SoC), shutter angle in status bar, degree glyph in font
- `e76ae5c` 2026-09-25 08:33 exposure: manual shutter by angle (1..360, kept across frame rates) or by speed with 1/x steps of 1/10/100/1000
- `a20a9ff` 2026-09-25 08:51 controls: camera-scale shutter arrows, fine steps on remote zoom -/+, remote AE toggles exposure mode, live shutter/gain buttons in manual
- `9dc9672` 2026-09-25 08:55 manual gain: step on the half-stop grid values (was stuck at 2.8x)
- `d4c6beb` 2026-09-25 09:03 rec: slow-card warning (BUF % when the queue is over half full, DROP n once frames are lost)
- `ef64fad` 2026-09-25 09:10 menu: function buttons are ignored while the menu is open (record and photo still work)
- `ff85e79` 2026-09-25 09:18 waveform: Luma / RGB overlay / RGB parade modes; waveform as wide as the histogram so the pair is centred
- `cfd55ad` 2026-09-25 09:27 scopes: a lone histogram or waveform sits dead centre, both side by side; layout remembered before the OSD opens
- `3b881bf` 2026-09-25 09:27 notes: progress
- `a0818d8` 2026-09-25 09:47 looks: 5 look slots (save OK twice / load) in Expert > Image; Log gamma marked experimental with -2 EV auto offset
- `f8027a1` 2026-09-25 09:53 looks: export/import all slots as LOOKS/LOOKn.TXT on the SD card (image keys only); docs/looks.md key guide
- `567a00d` 2026-09-25 09:54 notes: roadmap update
- `908de8a` 2026-09-25 09:59 refactor: SigmaStar MI declarations moved to mi.h (shared with player.c), no behaviour change
- `0d62597` 2026-09-25 10:03 looks: clamp imported values to each key's valid range
- `d90258c` 2026-09-25 10:08 refactor: ISP image processing, white balance and looks moved to isp.c; internal interface camapp.h; no behaviour change
- `6808256` 2026-09-25 10:14 refactor: recording queue, clip files, H.265 encoder and SD card handling moved to rec.c; no behaviour change
- `37a5bf9` 2026-09-25 10:18 refactor: on-screen menu moved to menu.c; no behaviour change
- `da9210c` 2026-09-25 10:22 refactor: command dispatcher moved to cmd.c; metering/AE types shared via camapp.h; no behaviour change
- `9973aae` 2026-09-25 10:26 refactor: loupe and analysis thread moved to scopes.c; camapp.h sections tidied; no behaviour change
- `3884dba` 2026-09-25 10:43 osd: frame guides (Frame row); guide lines and peaking stay clear of the status bar, notices, corner blocks and scope panels
- `2bcd487` 2026-09-25 10:45 notes: camera modes plan and decisions
- `32127a5` 2026-09-25 11:05 modes: Mode row (Normal/HDR) with per-mode frame rate, exposure and codec; remote play confirms action rows
- `65288ef` 2026-09-25 11:05 notes: modes step 1 done
- `418d662` 2026-09-25 11:13 notes: crop feasibility findings
- `e8b5fb7` 2026-09-25 11:17 notes: crop probe failed twice, next diagnostic
- `b449dde` 2026-09-25 17:41 notes: crop probe 3 failed, driver owns VMAX; next is driver analysis
- `b1fb2d7` 2026-09-25 17:46 notes: imx307 driver analysis (VMAX owned by driver, HMAX free, 891 Mbps table)
- `2917423` 2026-09-25 17:51 notes: 1080p120 probe - sensor runs 120 fps, picture garbled (photo in logs/, untracked)
- `e12311b` 2026-09-25 17:58 notes: HMAX sweep - clean full-frame 1080p75, MIPI bandwidth limit, 891 Mbps needs RX config
- `e106da2` 2026-09-25 18:02 cmd: padinfo debug dump (MI_SNR_GetPadInfo); notes: 120p parked, 75p plan
- `a103341` 2026-09-25 18:08 fps: 72p and 75p full-frame via shorter sensor lines (HMAX); exposure scale follows the line time
- `f531863` 2026-09-25 18:13 rec: clearer stop log (totals since start); notes: 75p recording verified (H.265 0 drops, MJPEG too slow)
- `40683e0` 2026-09-25 18:38 modes: Slow-mo (48-75p capture, clips stamped to play at 24/25/30)
- `baf4e0e` 2026-09-25 18:50 slow-mo: capture any whole rate 6-75 fps, plays at any rate from 5 up to one below capture
- `33cbe16` 2026-09-25 18:59 cmd: playfps N; notes: slow-mo verified on all paths, timelapse plan
- `69497ec` 2026-09-25 19:08 modes: Timelapse (MJPEG): interval 0.1 s - 60 min, plays at 5-60, frame count and clip length on screen
- `968a375` 2026-09-25 19:31 notes: H.265 timelapse attempts failed (VENC RecvPicEx does not gate; bind frc is 16-bit fps), reverted to MJPEG timelapse
- `179ab06` 2026-09-25 19:36 timelapse: Stop after N frames (auto stop, exact count)
- `a2e686e` 2026-09-25 23:26 timelapse: long manual shutter up to the interval, max 2.1 s (driver 32-bit wrap)
- `a377140` 2026-09-25 23:26 notes: H.265 timelapse skipped
- `d743683` 2026-09-25 23:35 rec: keyframe slots grow to 6 MB and shrink back after writing; log largest frame per clip
- `4b8e034` 2026-09-25 23:50 slow-mo: H.265 stream header (VUI timing) carries the playback rate
- `331b821` 2026-09-26 00:01 rec: clip names per mode (NORM_, HDR_, SLOW_, TLAP_), one counter across all
- `2c800e8` 2026-09-26 00:06 rec: sidecar metadata file per clip with record-run timecode
- `28b1ac7` 2026-09-26 00:11 timecode: on screen (corner block) and System > Timecode reset (OK twice)
- `ec25c6e` 2026-09-26 00:42 encoder page: JPEG quality 1-100 (live)
- `c3ee6b2` 2026-09-26 00:48 encoder page: H.265 GOP length, CBR/VBR, VBR QP range
- `c3aac04` 2026-09-26 00:55 rec: H.264 as a third codec (.ts)
- `e60e24f` 2026-09-26 01:02 notes: resume block for the compact (next: 8a H.264 slow-mo header, 8b crop mode)
- `daaa5bf` 2026-09-26 01:14 Slow-mo H.264: stream header frame rate = playback rate
- `794481c` 2026-09-26 01:47 Crop probe: IMX307 window crop through VIF/ISP/SCL, FIFO restart
- `59ac37e` 2026-09-26 02:04 Crop: 1280-wide speed ceiling is 105.6p (HMAX 1250)
- `286f4c2` 2026-09-26 02:08 COMPACT: menu cleanup decisions (sections + accent colours, after crop)
- `0b3fbc4` 2026-09-26 02:16 Crop from settings: crop_w/crop_h per mode, fps up to the crop's ceiling
- `7fd3b09` 2026-09-26 02:53 Crop menu rows; record queue limited by bytes, 240 frames
- `61be72f` 2026-09-26 03:04 Crop rows set together with running size shown; sidecar crop info
- `6a47dfe` 2026-09-26 03:15 UVC: own configfs gadget 1920x1080 MJPEG 30/60 fps; reconnect after restart
- `6ad7e1a` 2026-09-26 03:15 COMPACT: UVC done, OSD injection plan
- `05c0256` 2026-09-26 03:22 COMPACT: CPU/RAM/storage measurement
- `39f421d` 2026-09-26 03:30 UVC: pace frames to the host's frame interval
- `1f76034` 2026-09-26 03:36 Menu: sections with accent colours, back goes up a level, help side panel
- `6e6dc58` 2026-09-26 03:48 OSD: status bar, corner blocks and menu drawn off-screen (no flicker)
- `b82778b` 2026-09-26 03:57 Menu: MENU closes and remembers, mirror = back; matrix vendor read-only; save logs
- `8d450e9` 2026-09-26 04:23 OSD: analysis layer under status text, no holes or leftovers
- `1245c69` 2026-09-26 04:23 COMPACT: roadmap rebuilt, resume block updated
- `83ac128` 2026-09-26 04:37 Menu: System row shows free SD space
- `6fb8645` 2026-09-26 04:42 System page: live CPU, RAM and flash status rows
- `9d3f7ed` 2026-09-26 04:47 COMPACT: MI_RGN research and OSD-injection step plan
- `9254284` 2026-09-26 04:51 Docs: COMPACT.md rewritten as one current document; handoff frozen
- `fccfd7b` 2026-09-26 04:53 Docs: handoff pruned of stale plans and corrected to the current state
- `b213ba0` 2026-09-26 04:53 COMPACT: point to the handoff log for ISP struct offsets
- `cf1b80e` 2026-09-26 05:14 COMPACT: roadmap item for a global OSD hide button
- `999e9fa` 2026-09-29 12:50 COMPACT: body button gesture decision
- `d1b58f2` 2026-09-29 15:11 Exposure priority modes P / S / G / M; UART and capture helpers in tools/
- `3708f25` 2026-09-29 15:46 RAW photos as DNG; VIF frame buffer compression off
- `9250f89` 2026-09-29 16:20 DNG check image: dngview develops the raw frame to a 160x90 PPM
- `5585191` 2026-09-29 16:26 Body MENU gestures, OSD hide, Photo JPEG/JPEG+DNG menu row
- `1bd27fc` 2026-09-29 16:33 Scopes at ~10-15 Hz, status recomposite capped, named threads
- `08dfe50` 2026-09-29 17:06 viewgrab: processed picture as a 160x90 PPM over UART
- `6fee36b` 2026-09-29 17:14 Open gate: the whole sensor readout as an experimental crop setting
- `fa19107` 2026-09-29 17:32 Resizable spot; OSD fill clipped at negative coordinates (crash fix)
- `f3f3704` 2026-09-29 17:39 COMPACT: roadmap row for exposure priority modes
- `b13a02d` 2026-09-29 17:47 COMPACT: USB options (UVC controls, own gadget modules, host mode); RAW video removed
- `8e5c8ee` 2026-09-29 17:55 RGN overlay probe (R1 layouts, R2 not visible yet)
- `71ad35a` 2026-09-29 18:25 RGN overlay: canvas get/update pairing fixed, still not blended; parked
- `50797ac` 2026-09-29 18:42 UVC camera controls: AE mode and brightness for host apps
- `2f9000d` 2026-09-29 18:50 UART transfers: gzip + larger chunks for push, one-shot stream for pull
- `f5df062` 2026-09-29 18:59 Focus meter from the ISP AF statistics; status log in real microseconds
- `30c7339` 2026-09-29 19:09 Defect-pixel correction probe: dp / dpdump FIFO commands
- `ea14c49` 2026-09-29 19:15 COMPACT: HDR manual exposure and anti-flicker mapping verified
- `9a22eba` 2026-09-29 19:23 Focus meter: AF windows over the whole frame
- `7710ef4` 2026-09-30 19:11 Remove the focus meter
- `0927839` 2026-09-30 19:17 Body MENU: double click waits for a possible third click
- `5dede45` 2026-09-30 19:21 Spot size in even steps so the spot stays centred
- `dcdc03a` 2026-09-30 19:32 COMPACT: hot-pixel dark-frame test - ISP defect correction has no consistent effect
- `67e9b8d` 2026-09-30 19:55 COMPACT: HDMI output modes parked (only the boot timing runs); EDID findings
- `c65e10b` 2026-09-30 20:03 Guides run under the status bar and corner text instead of stopping
- `efdb40c` 2026-09-30 20:03 COMPACT: frame guides row updated
- `0c39ab0` 2026-09-30 20:04 COMPACT: resume state after the polish round
- `c766b67` 2026-09-30 Final handoff document and scratchpad dump
- `7677ceb` 2026-09-30 Repository layout for publishing

### 20.3 Closing state (2026-09-30)
- The camera runs the last committed build (`c65e10b`, guides under the status text) in Normal mode, 1080p60. The user called it "very polished" and declared the project done.
- 2026-09-30, after this list: the repository was organised for publishing (README.md, LICENSE GPL-2.0, `docs/` guides extracted from this document, `docs/notes/`, `research/`, `archive/`). Branch `main` is a single clean commit of that tree for GitHub; `master` keeps the full development history.
- Next, if ever: any of the optional items in §16.
