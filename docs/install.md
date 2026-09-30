# Installing camapp

How camapp is installed on the camera, how it boots, how to update it and how to go back to the stock firmware.

## Before anything: back up the flash

Read the whole 16 MB SPI-NOR flash with a CH341A programmer and a SOP8 clip before changing anything, and keep the image safe (details in [recovery.md](recovery.md)). With that image nothing is unrecoverable.

## What you need

- The camera (SigmaStar Mercury6 board with a Sony IMX307; see [hardware.md](hardware.md)).
- A 3.3 V USB-UART adapter on the board's console pads (115200 8N1). On Linux/WSL it shows up as `/dev/ttyUSB0`.
- A Linux or WSL PC with the cross compiler and the camera's own libraries ([building.md](building.md)), Python 3 and `pyserial`.
- A FAT32 SD card (the camera can format it: System › Format SD).

## First install (camera still on the stock firmware)

All camapp files live in `/customer/camapp/` plus one wrapper at `/config/cardv`; no vendor file is changed.

1. Build camapp: `src/camapp/build.sh`, then `tools/check_syms.sh src/camapp/camapp`.
2. Open the UART console once and press Enter to get the root prompt (Ctrl+C during boot also works). Close the terminal again: only one program may use the port.
3. Create the directory and push the files (each push checks an md5 sum):
   ```
   python3 tools/sh.py "" "mkdir -p /customer/camapp"
   tools/uart_push.py tools/device/flash/cardv        /config/cardv
   tools/uart_push.py tools/device/flash/boot.sh      /customer/camapp/boot.sh
   tools/uart_push.py tools/device/flash/uvc-gadget.sh /customer/camapp/uvc-gadget.sh
   tools/uart_push.py tools/device/flash/update.sh    /customer/camapp/update.sh
   tools/uart_push.py tools/device/flash/rollback.sh  /customer/camapp/rollback.sh
   tools/uart_push.py src/camapp/camapp               /customer/camapp/camapp
   ```
4. Enable it and reboot:
   ```
   python3 tools/sh.py "" "chmod +x /config/cardv /customer/camapp/*.sh /customer/camapp/camapp; echo 0 > /customer/camapp/boot_attempts; touch /customer/camapp/enable; sync; reboot"
   ```
5. Check: the HDMI monitor shows the live picture with the camapp status bar. `python3 tools/sh.py "" "head -1 /tmp/camapp.log"` prints `camapp build <date> <time>`. After 30 s the console prints `camapp: healthy after 30 s, boot counter reset`.

## Updating

`tools/deploy.sh` builds, checks, pushes and installs a new build with an automatic rollback if it does not come up (details below).

## Going back to stock

- `rm /customer/camapp/enable` and reboot: the wrapper starts the vendor `cardv`.
- `rm /config/cardv` and reboot: the exact stock boot.
- Optionally `rm -r /customer/camapp` to remove everything.

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
| `/customer/camapp/settings` | all settings, key=value ([§11](settings.md)) |
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
