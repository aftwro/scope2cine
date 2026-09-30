# Backup, recovery and debugging

How to back up the flash, every escape hatch from mild to strong, and a symptom-by-symptom debugging table.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 3, 17), the complete project reference. Section numbers match it.

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
