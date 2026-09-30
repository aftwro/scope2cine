# Hardware and stock firmware

The board, sensor, inputs and outputs, and the vendor firmware that camapp replaces.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 2, 4), the complete project reference. Section numbers match it.

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
- **HDMI**: SigmaStar HDMI TX. It runs only the boot timing 1080p60 ([§15.2](known-issues.md)). The user's monitor is an LG FHD, 1920x1080 at up to 100 Hz; its EDID is in `archive/scratchpad/project_scratch/edid.bin`.
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

camapp copies this recipe ([§8.1](internals.md)). Decompiles of the relevant cardv functions are in `research/decompiles/dec_*.c`.
