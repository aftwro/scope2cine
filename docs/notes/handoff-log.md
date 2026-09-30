> **Layout note (2026-09-30):** this log was `handoff(5).md` in the project root. Paths in it refer to the old layout: `logs/dec_*.c` is now `research/decompiles/`, `COMPACT.md` is `docs/notes/COMPACT.md`. The complete current reference is [`docs/HANDOFF.md`](../HANDOFF.md).

> **HISTORICAL LOG (frozen 2026-09-26).** This file records how the project got here, including dead ends and superseded states. It is NOT the current state: `COMPACT.md` is authoritative (resume block, roadmap, findings). Do not act on instructions or "next steps" found in this file.

# Project Handoff — "AliExpress Microscope" → Experimental Camera

Cheap AliExpress USB/HDMI microscope built on a repurposed dashcam board.
Goal: turn it into a hackable experimental camera (C-mount glass, custom
resolutions/crops, custom firmware, custom UI). It **already does UVC**.

Status: **Golden firmware backup secured.** SoC confirmed as **SigmaStar Mercury6** (2026-09-24). Sensor confirmed IMX307.
UART reachable from WSL as `/dev/ttyUSB0` @ 115200. Full boot log: `logs/boot_2026-09-24.log`.

---

## TL;DR

- **SoC:** SigmaStar (SStar / ex-MStar) **Mercury6** (SDK codename **Tiramisu**, chip ID **0xF4**), **dual-core ARM Cortex-A53** running a 32-bit (ARMv7/AArch32) kernel. Not Infinity6/6E. Top marking **ground off** by vendor. See "SoC IDENTIFICATION" below.
- **Flash:** Macronix **MX25L12835F**, SPI-NOR, **16 MB**, 3.3V. JEDEC ID **C2 20 18**.
- **Backup:** full 16,777,216-byte dump, **CRC32 = 0xE41B1D54**, verified by two matching reads. This is the undo button — nothing is unrecoverable now.
- **Firmware origin:** repurposed **dashcam** ("cardvini" = car DVR ini).
- **Anti-clone auth:** GPIO 5/4 challenge–response, but the challenge is **unseeded glibc `rand()`** → constant every boot. The response bytes change per boot, so a plain byte replay is **not proven** (see auth section).
- **Now (2026-09-26):** our own app `camapp` replaces cardv entirely (pipeline, recording, UVC, OSD, menus). Current state and roadmap: `COMPACT.md`.

---

## POSITIVE FINDINGS (confirmed, working, in-hand)

### Access
- **Root shell over UART** — Ctrl+C **interrupts the boot** and drops to the shell. The **entire environment is BusyBox** (not a separate rescue console — BusyBox *is* the userland).
- Serial console confirmed working @ 115200 (SmarTTY / COM18).
- **Permanent UART port being added** to the chassis (in progress) — no more clip/reseat cycles.

### Hardware identity (from the running kernel — the grind-off is defeated)
- `/proc/cpuinfo`: **2× ARMv7 Processor rev 4 (v7l)**, CPU part **0xd03**, BogoMIPS 12.00, implementer 0x41 (ARM).
  - **Correction:** part 0xd03 is **Cortex-A53**, not Cortex-A7 (A7 = 0xc07). The kernel confirms it: `hw perfevents: enabled with armv7_cortex_a53 PMU driver`. The A53 runs in 32-bit mode here.
- `Hardware : SStar Soc (Flattened Device Tree)` — **SStar = SigmaStar** confirmed by device tree.
- Board silkscreen: **`V522_W_XX`**, **`DGECAM`** — better search terms than "SStar Soc".

### Flash / storage (dual-confirmed)
- Kernel FSP line: `[FSP] Flash is detected (0x0509, 0xC2, 0x20, 0x18) ver1.1`
- `mtd .name = NOR_FLASH, .size = 0x01000000 (16MiB)`
- Serial-flash controller window: `phys=0x23850000, virt=0xc3850000, bus=0x03850000`
- **JEDEC ID C2 20 18** = Macronix (0xC2) + MX25L128 class (0x20 0x18 = 128 Mbit / 16 MiB).
- External read via **CH341A + NeoProgrammer** returned **SPI ID: C22018** — matches the kernel. Two independent sources agree.

### The backup (golden image)
- Two full reads, **identical CRC32 = 0xE41B1D54**, ~3:07 each.
- Size: **16,777,216 bytes** exactly.
- ⚠️ ACTION: confirm saved to file and **copied off the working machine** (cloud/USB/second drive).

### The live system dump (decompressed running FS) ✅
- Grabbed the **live running filesystem** (not the compressed flash images) via tar to the onboard SD, so no SquashFS/JFFS2 digging needed.
- Root `/` is **rootfs (in-RAM, uncompressed, rw)**; `/customer` + `/config` are live **JFFS2**. All captured as plain files.
- File: `live_system.tar.gz`, **7,469,846 bytes**, **`gzip -t` = GZIP OK** (verified intact).
- Command used (excludes virtual/special mounts + the SD itself):
  ```
  cd /
  tar -czf /app/bin/sd/live_system.tar.gz \
    --exclude='./proc' --exclude='./sys' --exclude='./dev' \
    --exclude='./tmp' --exclude='./var' --exclude='./app/bin/sd' --exclude='./misc' .
  ```
- `tar: short read` during run = harmless skip of a live/special file; `gzip -t` passing confirms the archive is complete.
- **Extract on Linux/WSL/Mac** (not Windows Explorer) to preserve symlinks/perms:
  `tar -xzf live_system.tar.gz -C extracted/`

### Already-present capabilities
- **UVC** — device already enumerates/works as a USB webcam. (Biggest wishlist item already done.)
- **HDMI OSD** — full on-screen UI is drawn over the video in the HDMI path (vendor app rendering to framebuffer/OSD layer).

---

## SoC IDENTIFICATION — **Mercury6**, not Infinity6E (confirmed 2026-09-24)

Live evidence (UART):

| Source | Value |
|---|---|
| `/sys/devices/virtual/mstar/msys/CHIP_ID` | `Chip_ID: 0xF4` |
| `/sys/devices/soc0/soc_id` / `family` | `244` (= 0xF4) / `M6` |
| `/proc/device-tree/compatible` / `model` | `sstar,mercury6` / `MERCURY6 SZDEMO BGA2` |
| `/proc/cpuinfo` CPU part | `0xd03` = **Cortex-A53** |
| Kernel PMU | `armv7_cortex_a53 PMU driver` |
| Build strings | kernel `MVX4##M6##ge27f37c...`, U-Boot `M6##ge27f37c` |
| DRAM | `256MB` (IPL) |

Reference chip-ID table (OpenIPC `ipctool` `src/hal/sstar.h`):
`INFINITY5 0xED`, `INFINITY6 0xEF`, `INFINITY6E 0xF1` (Pudding), `INFINITY6B 0xF2`,
**`MERCURY6 0xF4` (Tiramisu)**, `INFINITY6C 0xF9`, `INFINITY6F 0xFB`.
An Infinity6E would read `0xF1` and report Cortex-A7 (part `0xc07`).

Why Infinity6E looked plausible: several dashcam SoCs (SSC8629D/G/Q) **are** Infinity6E
(dual Cortex-A7). This board is a dashcam board too, but its core is Cortex-A53, and
linux-chenxing lists Mercury6 as the dual-A53 family. Chip ID, core type and build tags
all agree.

Candidate part numbers (top marking ground off, so not final):
- **SSC8826Q** — dashcam part, dual Cortex-A53 @ 1.2 GHz, **2 Gb (256 MB) DDR3 built in**. Matches our 256 MB and the dashcam firmware. Best fit.
- **SSD268G** — dual Cortex-A53 (linux-chenxing Mercury6 list).
- **SSC359G** — dual Cortex-A53, BGA, external RAM, 4K IPC part. Less likely.

Consequences:
- **OpenIPC does not list any Mercury6 part** (its SigmaStar support = SSC30Kx, SSC32x, SSC33x, SSC37x). No drop-in open firmware.
- Mainline Linux support for Mercury6 is minimal. Stay on the vendor 4.9 kernel + MI SDK.
- A53 cores can run AArch64, but this firmware is 32-bit end to end. Toolchain target stays **armv7 / glibc** (`cardv app version: MVX2 glibc m6 ...`).

Sources: [ipctool sstar.h](https://github.com/OpenIPC/ipctool/blob/master/src/hal/sstar.h),
[linux-chenxing index](https://github.com/linux-chenxing/linux-chenxing.org/blob/master/index.md),
[OpenIPC SigmaStar list](https://openipc.org/cameras/vendors/sigmastar),
[SSC359G brief](https://www.comake.online/uploadfile/file/20220418/20220418070418_52213.pdf).

---

## BOOT LOG FINDINGS (2026-09-24, `logs/boot_2026-09-24.log`)

- **Boot chain:** IPL `3648cd2` → IPL_CUST → U-Boot 2015.01 → kernel (MZ-compressed legacy uImage, load `0x20008000`) → rootfs loaded as initrd into **ramfs** (`initrd=0x21000000,0x5A0000`). `Initramfs unpacking failed: junk in compressed archive` appears but boot continues.
- **Kernel cmdline:** `console=ttyS0,115200 rootfstype=ramfs initrd=0x21000000,0x5A0000 LX_MEM=0xffe0000 mma_heap=mma_heap_name0,miu=0,sz=0xAC00000 ... cma=2M@0x23800000 mmap_reserved=fb,...`
- **Inputs are standard Linux input devices** — no GPIO hunting needed:
  - `input: soc:adc-keys as .../input/input0` (hardware buttons, ADC ladder)
  - `Registered IR keymap rc-mstar-dtv` / `input: mstar ir as /devices/virtual/rc/rc0/input1`
  - NEC/RC5/RC6/JVC/Sony/SANYO/Sharp/XMP IR decoders all loaded.
- **USB gadget = legacy `g_webcam` module** in the stock boot (replaced 2026-09-26 by our configfs gadget, see COMPACT.md): `g_webcam gadget: Webcam Video Gadget`. Also `g_webcam: unknown parameter 'uac_function_enable' ignored` (no USB audio). Mode switching therefore means `rmmod`/`insmod` of gadget modules (or reboot fallback); check `lsmod` and `/sys/kernel/config` for `libcomposite` before planning configfs.
- **UVC warning:** `[APP ERR][dev:/dev/video0]: _UVC_Fill_Streaming_Control[2499]: No Support Format` (seen with no host attached).
- **Fake stills resolution:** `[vs20_setCaptureResolution] : w = 7488,h = 4212` and `set venc chn1 res 7488x4212` → ~31 MP stills upscaled from 1080p.
- **ISP:** IQ file `/bootconfig/iqfile/iqfile0.bin` minor-version mismatch (`ispversion(6) in sdk, ispversion(5) in binfile`); API bin `/bootconfig/iqfile/isp_api.bin`. AE starts at 60 fps.
- **Harmless errors:** AF motor params invalid (no focus motor); HDMI `Check Edid Data Fail` (no monitor attached); missing `/customer/modules/4.9.227/{nfsv3,media,videodev,v4l2-common}.ko`; `fail direction[out] gpio[11]`.
- **Shutdown:** `cardv` segfaults while tearing down on `reboot` (`Caught "Segmentation fault"` + maps dump). Only on the way down.
- **Framebuffer:** `/dev/fb0` "SStar FB0", OSD drawn at **1600x900 32bpp** with colour key `0x0a0a0a`, scaled to 1920x1080 HDMI.

---

## FLASH PARTITION MAP (from `/proc/mtd`)

Partitions are contiguous; offsets are the running sum of sizes.

| # | Name       | Size (hex) | Start (hex) | End (hex) | Notes |
|---|------------|-----------|-------------|-----------|-------|
| mtd0 | BOOT      | 0x070000 | 0x000000 | 0x070000 | bootloader (U-Boot) |
| mtd1 | KERNEL    | 0x220000 | 0x070000 | 0x290000 | Linux kernel (likely uImage/LZMA) |
| mtd2 | rootfs    | 0x5A0000 | 0x290000 | 0x830000 | main FS (likely SquashFS) — init scripts, main app, sensor driver, UI app |
| mtd3 | miservice | 0x050000 | 0x830000 | 0x880000 | SigmaStar MI middleware |
| mtd4 | customer  | 0x6F0000 | 0x880000 | 0xF70000 | **`/customer/cardvini/` + `one_bayer_1080.ini`** |
| mtd5 | MISC      | 0x060000 | 0xF70000 | 0xFD0000 | misc/config |
| mtd6 | KMSGLOG   | 0x020000 | 0xFD0000 | 0xFF0000 | kernel message log |

- Partitions sum to **0xFF0000**; the top **~0x10000 (64 KB)** at `0xFF0000–0x1000000` is **not exposed by MTD** (purpose unknown). **The full clip dump captured it; a software dump would not have.**
- **U-Boot env is NOT in the top 64 KB.** U-Boot prints `env_offset=0x6F000 env_size=0x1000` → env is the last 4 KB of the BOOT partition (`0x6F000–0x70000`).
- Partition table source: **MXP** table at flash offset `0x20000` (`MXP found at mxp_offset[3]=0x00020000, size=0x1000`). Kernel and U-Boot both read partitions from it.

### Split commands (run on host against the full dump)
```
dd if=dump.bin of=0_boot.bin      bs=1 skip=$((0x000000)) count=$((0x070000))
dd if=dump.bin of=1_kernel.bin    bs=1 skip=$((0x070000)) count=$((0x220000))
dd if=dump.bin of=2_rootfs.bin    bs=1 skip=$((0x290000)) count=$((0x5A0000))
dd if=dump.bin of=3_miservice.bin bs=1 skip=$((0x830000)) count=$((0x050000))
dd if=dump.bin of=4_customer.bin  bs=1 skip=$((0x880000)) count=$((0x6F0000))
dd if=dump.bin of=5_misc.bin      bs=1 skip=$((0xF70000)) count=$((0x060000))
dd if=dump.bin of=6_kmsglog.bin   bs=1 skip=$((0xFD0000)) count=$((0x020000))
```

---

## STORAGE UPGRADE (16 MB → 32 MB NOR?)

**Units:** SPI-NOR is sized in **megabits**. "256" = **W25Q256 = 256 Mbit = 32 MB** (double current). There is effectively no 256-**megabyte** SPI-NOR; NOR gets impractical past ~64 MB. Realistic bump: **16 MB → 32 MB (W25Q256)**, or 64 MB (W25Q512).

### Physically: drop-in
- MX25L128 and W25Q256 are both standard 8-pin SPI-NOR, **same pinout** (CS, DO, WP, GND, DI, CLK, HOLD, VCC), same SOIC-8 footprint. Desolder old, solder new.
- ✅ **Buy the 3.3V part: `W25Q256JV`.**
- ❌ **Do NOT buy `W25Q256JW` / `FW`** — those are **1.8V** and will not work on this 3.3V board (same trap as the 1.8V adapter in the kit).
- Match the package (SOIC-8 209-mil vs WSON-8) to the board.

### The real wall: **4-byte addressing**
- ≤16 MB (128 Mbit) uses **3-byte (24-bit)** addressing — why the current chip "just works."
- **>16 MB requires 4-byte (32-bit)** addressing. 32 MB is unreachable with 24 bits.
- A 32 MB chip only fully works if **every boot stage** supports 4-byte mode:
  1. SoC mask ROM / IPL — usually fine (first 16 MB reachable in 3-byte mode regardless).
  2. SigmaStar FSP flash controller + driver — must support the chip + 4-byte.
  3. U-Boot SPI-NOR driver — must know W25Q256 and enable `EN4B`.
  4. Kernel MTD driver — same.
  5. Partition table — must be rewritten to use the extra 16 MB.
- ⚠️ **Budget SigmaStar dashcam SDKs are often capped at 16 MB with 4-byte NOT enabled.** If so, a 32 MB chip will **only show the first 16 MB** or **fail to boot**. Confirm in the U-Boot/kernel SPI-NOR chip table + 4-byte config *before* buying — or plan to enable it yourself. OpenIPC does **not** support Mercury6 (see SoC section), so plan to enable it yourself.

### Do NOT swap NOR → big SPI-NAND
- NAND (the cheap route to 128 MB / 1 GB) is a **different protocol, driver, and boot strapping** (NOR-boot vs NAND-boot is set by strap pins). **Not a solder swap.**

### Better paths for *bulk* storage (footage/assets)
- This is an ex-**dashcam** board → the SoC/SDK almost certainly supports an **SD card**, even if the slot is unpopulated. Check: `dmesg | grep -i -e mmc -e sd` and look for an unpopulated SD footprint.
- Already does **UVC/USB** → **USB mass storage or SD** adds gigabytes with **zero boot-flash risk**.

### Verdict
- Want room for **custom firmware** → 16→32 MB is worth it **only after confirming 4-byte support**.
- Want **bulk storage** → NOR is the wrong tool; use **SD or USB**.
- If proceeding with W25Q256JV: write the existing 16 MB dump into the new chip's first 16 MB, then extend the partition map + fix bootargs for the added space — only after confirming >16 MB NOR support.

---

## FIRMWARE PROVENANCE

- Boot log path: `+++++hdmi_ini:[/customer/cardvini/one_bayer_1080.ini]-----Success !..`
- **`cardvini` = "car DVR ini"** → firmware adapted from a **dashcam**. Common for budget microscopes: reuse a car-DVR board + SDK, swap lens/output.
- **`one_bayer_1080.ini`** = config for **one raw Bayer image sensor at 1080p**, output over HDMI.
- Implies the standard **SigmaStar Linux SDK** partition layout (`/customer/` etc.).

---

## ANTI-CLONE / AUTH CHIP (defeated in principle)

- Boot log: `AUTH_GPIO_INIT!:5-4` and `[0,5,4]` → auth device on **GPIO 5 and 4**, probably **bit-banged I²C**.
- **tx_data (8-byte challenge):** `67 c6 69 73 51 ff 4a ec`
- **rx_data (10-byte response):** `d0 3a 35 d3 03 8f 01 4a 79 0b` (8 bytes payload + header/checksum)
- **2026-09-24 boot:** rx_data = `4a 77 53 40 5a 23 d3 23 4c 52` — **different bytes**, same challenge, same decoded ex_data. The chip adds its own randomness (nonce/IV) per boot.
- **ex_data (decoded, matches tx):** `67 c6 69 73 51 ff 4a ec` → `Success` → device boots.
- **Weakness:** the "random" challenge `67 c6 69 73 51 ff 4a ec` is exactly the **low bytes of unseeded glibc `rand()` (seed = 1)** — developer forgot `srand()`. So:
  - Every boot sends the **same** challenge and expects the **same** response.
  - A replay of one recorded 10-byte response probably passes (decode gives the same ex_data every time), but this is **untested**.
  - On a cloned board without the chip, firmware would normally refuse/cripple; here it's a non-issue for our purposes.

---

## TARGETS / WISHLIST (the project)

Original wishlist; status on 2026-09-26: custom resolution/crop modes done (sensor window crop, up to 105p at 1280 wide), custom UI done (camapp OSD + sections menu), UVC done (own gadget), custom firmware = camapp on the vendor MI SDK (OpenIPC/thingino do not cover Mercury6), open gate and C/CS-mount lens still open. Live list: `COMPACT.md` roadmap.

---

## HOST CONTROL SOFTWARE — USB MODE SWITCHING + FULL REMOTE CONTROL (plan from 2026-09-24, not built; button/IR codes as used now are in COMPACT.md) (planned)

New target: on-device host software, controllable from a few presses on the
existing **hardware buttons**, that switches the board's USB personality
between **UVC** (plain webcam) and **software control** (custom
protocol/management channel), plus other modes as they get defined. The
permanent UART hole gives a live debug/control path independent of USB, so
button GPIOs can be mapped and the switching logic iterated on without
re-flashing each time.

### Button → GPIO mapping (done: buttons are input0 adc-keys, codes in COMPACT.md)
- Board already has physical buttons (they drive the existing HDMI OSD menu),
  so a key-input driver almost certainly already exists — check for it before
  writing a new GPIO poller:
  ```
  cat /proc/interrupts | grep -i gpio
  ls /sys/class/gpio/
  dmesg | grep -i -e key -e button -e gpio
  find / -iname "*key*" -o -iname "*gpio*" 2>/dev/null
  ```
- If a vendor key driver exists, reuse its event device (`/dev/input/eventX`,
  read with `evtest`/`hexdump`) instead of bit-banging GPIO — same approach
  already used for GPIO 5/4 auth, so the pattern is proven on this board.
- If nothing exists, map raw GPIO numbers with the UART shell live (toggle
  candidates, watch `/sys/class/gpio` or `/proc/interrupts` react), same
  method as the anti-clone GPIO 5/4 discovery.

### IR remote (alternate/parallel input path)
- Camera ships with an **IR remote**, already used to drive the OSD menu.
- Confirmed: pressing remote buttons produces **visible output on UART** —
  means the IR receiver driver is live and logging (or the OSD app prints on
  keypress), independent confirmation the input path is already wired into
  something observable from the shell.
- On next live session, capture the exact UART output per button:
  ```
  dmesg | grep -i -e ir -e remote -e nec -e rc5
  cat /proc/bus/input/devices
  ls /sys/class/rc/ 2>/dev/null
  ir-keytable -t 2>/dev/null   # if ir-keytable is available; else read raw driver output
  ```
- If it's a standard Linux `rc-core`/LIRC input device (`/dev/input/eventX` or
  `/dev/lirc0`), it's a **second control surface for free** — same event
  interface as physical buttons, no extra GPIO work, can drive UVC/software-
  control mode switch, flip/rotate, or any control-daemon command from the
  existing remote instead of (or alongside) hardware buttons.
- If it's a raw/proprietary bit-bang decode instead (no standard rc-core
  device), the UART output is still useful — reverse the printed
  codes-per-button from the log, decode NEC/RC5/etc timing by hand.
- Do this investigation **in parallel** with the hardware-button GPIO mapping
  (same session, same UART access) — don't treat as separate follow-up.

### Switching mechanism (in order of preference)
1. **configfs USB gadget** — SigmaStar SDK's UVC gadget is very likely built
   on Linux `configfs`/`libusbgx`. If so, a runtime script can `rmdir`/rebuild
   gadget functions (UVC function, then add a CDC-ACM or CDC-ECM/RNDIS
   function) and rebind, switching personality **without reboot**. Check for
   `/sys/kernel/config/usb_gadget/` on the live shell.
2. **Fallback — reboot into alternate gadget config**: if the SDK's gadget
   setup is a fixed compiled-in composite (not configfs-driven), button press
   writes a flag (e.g. to `/config` or `/misc` mtd) and triggers a reboot into
   a different startup script / `.ini` that brings up the other USB mode.
   Works, but loses the "instant" feel — treat as fallback only.

### Preferred outcome: supersede switching entirely
If the gadget can present **UVC + a control interface (CDC-ACM, or
CDC-ECM/RNDIS network-over-USB) simultaneously** as one composite device,
button-triggered mode switching becomes unnecessary — video always streams
over UVC, and the control channel is always present alongside it. Confirm
`configfs` support first; if present, build the composite gadget and treat
"switch modes" as a manual override / status LED thing rather than the
primary UX.

### Host software feature set (unmatched customizability target)
- **Settings/customizability**: full exposed control surface for every
  tunable already identified — linear vs DOL-HDR, exposure ratio, crop/
  windowing, resolution, framerate — read/write against the sensor driver's
  `.ini` (`one_bayer_1080.ini`, `one_bayer_4k.ini`, `default.ini` under
  `bootconfig/bin/`) and the ISP tuning (`isp_api.xml`, `iqfile*.bin` under
  `bootconfig/iqfile/`).
- **Remote window**: a live remote view of the HDMI/OSD output for
  headless configuration — cheapest path is an MJPEG/snapshot pull off the
  ISP/venc pipeline (`mi_venc.ko`, `mi_isp.ko`, `mi_disp.ko` are already
  loaded) rather than a true VNC framebuffer mirror; revisit VNC-style
  mirroring only if MJPEG proves insufficient.
- **BusyBox shell access**: `busybox telnetd` is already started unauthenticated
  in `rcS` — usable today over network-over-USB (RNDIS/ECM) once that gadget
  function exists, or formalize with `dropbear`/SSH if the rootfs has room.
  This is the lowest-effort item on the list — mostly already done.
- **ISP / sensor control (recording + sensor settings)**: wrap the SigmaStar
  MI (Media Interface) API — the same API the vendor app already calls into
  via `mi_isp.ko`, `mi_venc.ko`, `mi_sensor.ko`, `imx307_MIPI.ko` — in a small
  on-device control daemon exposing start/stop recording, sensor mode swap
  (linear/HDR), and exposure/gain over the control channel. Vendor app
  binary/source for this glue lives in the **`/customer` JFFS2 partition**,
  which is *not* in the static flash dump extracted so far (`/customer` was
  empty in `microscope_C22018_0xE41B1D54_extracted-claude.tar.gz` — it's a
  live-mounted partition, not part of the SquashFS/rootfs image). Pull it via
  `live_system.tar.gz` (already captured per above) or re-tar `/customer`
  directly to find the actual app binary and its MI API call sites.

### Immediate next steps (added to existing list)
1. Finish UART port; on first live session, look for an existing button/key
   input driver (`/dev/input/*`) before building a new GPIO handler.
2. Check `/sys/kernel/config/usb_gadget/` for configfs support — this decides
   whether mode switching (or the simultaneous-mode supersede path) is a
   config change or a driver-level job.
3. Extract `/customer` (from `live_system.tar.gz` or a fresh live tar) to find
   the vendor app binary that owns the MI API calls, recording control, and
   `.ini` loading — this is the code the control daemon needs to either call
   alongside, or replace.

### Image flip / rotate (90, 180, horizontal mirror, vertical mirror)
- Add to the same settings surface as above, not a separate feature.
- Check ISP/VIF/SCL layer first — `mi_scl.ko` (scaler) and the VIF sensor pad
  config are the likely home for hardware mirror/flip (most ISPs expose
  H-flip/V-flip as a register bit read straight off the sensor readout
  direction — near-zero cost, no CPU frame touch). 90°/270° rotate is usually
  **not** a sensor-level op — check `mi_disp.ko` / venc rotate capability;
  if absent, 90°/270° falls back to a scaler or software rotate (real CPU/DMA
  cost, so test for dropped frames once implemented).
- `.ini` files (`one_bayer_1080.ini`, `default.ini`) or MI API calls
  (mirror/flip flags on the sensor or VIF bind) are the two places to check
  once `/customer` app is pulled — same investigation as the ISP control item
  above, do both together.

## DEVELOPMENT PROCESS — add, test, bake (not one-shot)

This is going to be used as an actual **film camera** — reliability matters
more than feature velocity. Every feature (mode switching, control daemon,
flip/rotate, ISP settings, remote window, everything above) goes through:

1. **Add** — smallest working version of the one feature, isolated.
2. **Test** — exercise it live over UART/network before calling it done:
   normal case, edge cases (mode switch mid-recording, flip while streaming,
   button mashed rapidly, USB re-enumeration under a host OS, power-cycle
   mid-write), and a regression pass on whatever shipped before it (does
   flip still work after the mode-switch change? does UVC still enumerate
   after the control daemon change?).
3. **Bake** — run it for real, unattended, across power cycles / host
   reconnects / at least one actual shoot, before treating it as stable
   enough to build the next feature on top of.

Do not batch multiple features into one untested change. Do not treat "it
compiled" or "it worked once" as done. Every change to boot scripts, gadget
config, or MTD-backed config gets validated against the **golden 16 MB dump**
as the rollback point (already secured — CRC32 0xE41B1D54) before being
treated as a checkpoint of its own.

---

## NEGATIVES / HARD LIMITS (set expectations)

- This is a **surveillance/dashcam ISP pipeline**, not a cinema ISP.
- Encoder targets **8-bit, low-bitrate H.264/H.265**. Hard bitrate ceiling.
- **No 12-bit RAW / open-gate ProRes** — not in this silicon. Expect *characterful*, not *Alexa*.
- **No "open gate" possible** — IMX307 is a **native 16:9 sensor** (1945×1097 active). There is no taller/4:3 readout to unlock; the pixel array is ~1080p-shaped. You can crop *within* 1080p; you cannot read out *more*. Hard **sensor** limit.
- **Sensor ceiling is 1080p (2.13 MP).** Stock mode 1080p60; we reach 75p full width (shorter HMAX) and 105p with a 1280-wide sensor crop (2026-09-26). This is a *video* sensor, not a stills/high-res sensor.
- **"4K Resolution" on the OSD is marketing/interpolation** — the sensor physically cannot produce 4K. See note below.
- **Small sensor (1/2.8"):** C-mount glass will **crop heavily** (long effective focal length) — use the center of the image circle, favor wider lenses.
- SoC **top marking ground off** (vendor obfuscation) — irrelevant now that the kernel IDs it, but noted.

---

## SENSOR — CONFIRMED: **Sony IMX307**

From `dmesg`:
```
Connect IMX307_HDR_init_driver linear to sensor pad 0
Connect IMX307_HDR_init_driver SEF to vif sensor pad 0
Connect IMX307_HDR_init_driver LEF to sensor pad 0
```

### What it is
- **Sony IMX307**, **1/2.8" back-illuminated CMOS**, **~2.13 MP (1945×1097 active, 1920×1080 output)**.
- **~2.9 µm pixels** — large for this class → **strong low-light** ("starlight"-class sensor), good DR.
- Native **1080p up to 60fps**. Matches `one_bayer_1080.ini`.
- **Native 16:9** — no taller/4:3 readout exists.

### The HDR detail (the interesting bit)
- Driver **supports DOL-HDR** (Digital Overlap): `SEF` = **Short Exposure Frame**, `LEF` = **Long Exposure Frame**, combined per-frame for extended dynamic range.
- **Current mode is linear, not HDR.** 2026-09-24 boot: `select index 0, Crop(0,0,1920,1080), outputsize(1948,1097), maxfps 60 ... ResDesc 1920x1080@60fps` and `MhalCameraOpen ... HdrType: 0`.
- Also supports **linear** (non-HDR) mode (`...init_driver linear...`).
- Exposure ratio / linear-vs-HDR are **tunable in the sensor driver `.ini`** — closest thing to a "cinema" latitude knob on this board.

### Implications for the build
- ✅ **Low-light + HDR video** — genuine strengths. Build toward a moody, clean-in-the-dark, characterful **1080p60** cine-experiment cam.
- ✅ **Custom crops within 1080p** (1:1, 2.4:1, windowing) — doable.
- ✅ **C-mount** — works, but **1/2.8" is small** → heavy crop / long effective focal length. Favor wider lenses; you're using the center of the image circle.
- ❌ **No open gate** — nothing taller than ~1080p to read out.
- ❌ **No 4K** — sensor physically can't. See below.

### The "4K Resolution" OSD claim = marketing/interpolation
- The IMX307 tops out at **1920×1080**. Any "4K" shown in the UI is one of:
  - **Digital upscale/interpolation** (1080p → 3840×2160 by the ISP/encoder), which adds pixels but **no real detail**, or
  - a plain **spec-sheet lie** on the menu string.
- Real 4K needs an ~8 MP sensor (e.g. IMX415/IMX334 class). Not this chip. Treat any 4K mode as upscaled 1080p.

---

## TOOLING NOTES / GOTCHAS

- **Programmer:** CH341A "black" (V1.6-era). This model has a **known 5V-on-VCC defect** on some units — but **this unit tested fine** (user's prior 3.3V flashing history + clean C22018 read). Meter VCC if ever in doubt.
- **1.8V-Adapter** included in the kit is **NOT for this 3.3V chip** — would under-volt and give garbage reads. Set aside.
- **NeoProgrammer:** must select **SPI / 25-series** (`MX25L12835F`), *not* I2C / 24-series (24Cxx). Initial failure (`IC not responding`, `i2C CHIP Size: 128`) was from being in I2C mode / wrong header block.
- **CH341A headers:** use the **`BIOS 25 SPI`** block, not `EEPROM 24 I2C`. Set the 1-2/2-3 jumper to **1-2 (programming)**.
- **Clip orientation rule (foolproof):** ribbon **colored stripe → pin 1 → chip's dotted corner**, at **both** ends. Pin 8 (VCC) is diagonally opposite pin 1.
- **In-circuit reads:** watch for back-feed (inconsistent IDs / mismatched re-reads). Two matching CRCs = trustworthy. Here they matched.

### Live filesystem / mount layout (from `mount`, `df -h`)
- `/` = **rootfs** (in-RAM, rw, uncompressed) — no SquashFS to unpack for the live view.
- `/customer` = **mtd:customer JFFS2** (6.9M, ~49% used) — `one_bayer_1080.ini`, IMX307 config, `cardvini` tree.
- `/config` = **mtd:miservice JFFS2** (320K); `/config/config` = configfs.
- `/misc` = **fwfs (FUSE)** (384K); `/tmp /var /dev` = tmpfs (35.5M each).
- `/app/bin/sd` = **/dev/mmcblk0, VFAT, 58.4 GB SD** (removable, PC-readable).
- `init -> bin/busybox` symlink at `/` (why FAT can't hold the raw tree — tar preserves it).

### SD card gotchas (this board)
- SD controller **fails 1.8V UHS negotiation** (`cannot verify signal voltage switch` / `Over Time during 1.8v setting`) then falls back to `Skipping voltage switch` and mounts high-speed anyway. Ugly but works; may cap write speed for recording.
- If the card is removed/re-detected, it may come back **unmounted** — remount with `mount /dev/mmcblk0 /app/bin/sd` (or `-t vfat`, or `/dev/mmcblk0p1` if partitioned).
- `Volume was not properly unmounted... run fsck` after a re-detect is a **FAT dirty-flag**, not necessarily real damage. Integrity check that matters for the tarball is **`gzip -t`**; only run `fsck.vfat` if you need to keep using the FS in place.
- Clean-exit ritual: `sync` → `umount /app/bin/sd` → wait for prompt → power down → pull card. Don't touch the slot mid-remount.

---

## SEARCH TERMS THAT ACTUALLY WORK

- ❌ "SStar Soc" → returns the Society for Sex Therapy and Research. Avoid.
- ✅ `Mercury6` / `Tiramisu` (SDK codename)
- ✅ `SSC8826Q`, `SSD268G`, `SSC359G` (Mercury6 parts)
- ❌ `SSD21x` / `Infinity6` / `Infinity6E` — wrong family (Cortex-A7 parts)
- ✅ `DGECAM V522`
- ✅ `purple board camera SigmaStar`
- ✅ **`linux-chenxing.org`** — community MStar/SigmaStar research, has a Mercury6 section
- ⚠️ **`OpenIPC`** / **`thingino`** — useful SigmaStar references, but no Mercury6 support

---

## QUICK REFERENCE

| Item | Value |
|------|-------|
| SoC | SigmaStar **Mercury6** (Tiramisu), chip ID 0xF4, dual Cortex-A53 in 32-bit mode; exact part unknown (SSC8826Q best fit) |
| RAM | 256 MB DRAM |
| Kernel / U-Boot | Linux 4.9.227 / U-Boot 2015.01 (both built Apr 30 2024) |
| Board | V522_W_XX / DGECAM |
| Flash | Macronix MX25L12835F, 16 MB SPI-NOR, 3.3V |
| JEDEC ID | C2 20 18 |
| Dump CRC32 | 0xE41B1D54 |
| Dump size | 16,777,216 bytes |
| Firmware base | dashcam ("cardvini"), 1080p single-Bayer over HDMI |
| Auth | GPIO 5/4, unseeded rand() challenge = constant; response varies per boot |
| Inputs | `adc-keys` = input0, IR `rc0` (`rc-mstar-dtv`) = input1 |
| USB gadget | stock: legacy `g_webcam`; now our own configfs UVC gadget (`uvc-gadget.sh`, 1080p MJPEG 30/60) |
| Confirmed working | root serial shell (Ctrl+C interrupts boot; whole userland is BusyBox); camapp replaces cardv |
| Sensor | **Sony IMX307** — 1/2.8", 2.13 MP, 1080p60 stock (75p / 105p crop with camapp), DOL-HDR, strong low-light. No 4K; "open gate" = only 1948x1097 readout (~1.5% more). |

---

## CONTROL SURFACES FOUND (2026-09-24)

### 1. `cardv` text command FIFO — the main control path
- `echo "<cmd> <args>" > /tmp/cardv_fifo` (named pipe owned by `cardv`; `/tmp/cardv_internal` is a second, internal one). Output prints on the UART console.
- `echo help > /tmp/cardv_fifo` prints usage (tested, works). Usage text wrongly says `/tmp/cardv_info`.
- Full command table (from `cardv` strings): `ctmwb playback frequency mirror seamless capture capraw lock res vidres capres loop timelapse slowmotion sdrename thumb bri con sat sha scene flicker shutter iso gamma effect winweight night audioplay aplaystop audiorec pbvolume park gsensor zoom video hdmiset voice flip bitrate codec hdmitiming disp muxdebug dcf srcchg timezone quality micsen prerec vmdrectime autorec hdr wnr snr burstshot dumpimpl quit help datelogoStamp timeformat str reset lcdbri lcdcon lcdhue lcdsat recvol sclrotate aiattr player ispget ispset decrypt yuvdump scl hdmi dispscal`
- Of interest for the film camera: `shutter`, `iso`, `ctmwb` (colour temp WB), `hdr`, `capraw` (RAW capture?), `yuvdump`, `mirror`/`flip`/`sclrotate`, `zoom`, `res`/`vidres`/`bitrate`/`codec`, `ispget`/`ispset`, `gamma`, `night`, `rec`.
- Not yet tested beyond `help`. Test each with the add/test/bake process.

### 2. UVC Extension Unit (GUID `{63610682-5070-49ab-b8cc-b3855e8d221d}`)
- Handler `usb_vc_eu_cs_out` @ `0x472ad` in `cardv`.
  - selector 12 (0x0c), 16 bytes: only stored in a global buffer, no action.
  - selector 13 (0x0d), bytes [0..3] → `appCameraSet(op, a, b, c)` @ `0x4718d`:
    - op 0: a=1 → auto WB (`MI_ISP_AWB_SetAttr`); a=0 → manual WB via ISP cmd 0x421 (CTM WB / colour temp)
    - op 1: ISP cmd 0x41a with -1 (reset AWB attr)
    - op 2: ISP cmd 0x41a with {1, a, b, c} (manual WB gains?)
    - op 4: ISP cmd 0x41b (stub, printf only)
    - op 3/5/6: no-op
  - → the XU is **white balance only**. Not a general control channel.
- Descriptor topology is broken: XU unit 2 lists itself as source, PU (unit 3) is not linked to the output terminal. Linux `uvcvideo` cannot reach XU unit 2 (ENOENT); XU unit 6 answers selectors 12/13 only.

### 3. UVC standard controls — caveats
- (stock g_webcam) MJPEG only, max 30 fps at every size (sensor does 60). 2560x1440 and 3840x2160 are upscaled. Our own gadget now offers only honest 1920x1080 at 30/60.
- `exposure_time_absolute` is **not** a shutter control: `cardv` takes the low byte of the value and maps it to EV compensation (`AE:232` → `EXPOSURE: -8`). Default 13000 (`AE:200`) = EV +0.
- `auto_exposure` manual mode only logs `exp mode:1`; AE keeps running. Host cannot switch the menu back (only Manual reported valid).
- Every UVC stream stop re-inits HDMI (shared pipeline).

### 4. `cardv` internal ISP command IDs (`isp_process_cmd` @ `0x31571`, base 0x403)
| ID | MI calls |
|---|---|
| 0x403 | IQ Saturation |
| 0x404 | IQ Sharpness |
| 0x405 | IQ Lightness |
| 0x407 | AE EVComp |
| 0x40a | AE ExposureLimit (shutter/gain range — can pin shutter) |
| 0x412 | IQ Contrast |
| 0x413 | IQ Brightness |
| 0x415 | AE Flicker |
| 0x41a | AWB Attr |
| 0x41c | RGBGamma + AE ExpoMode/ManualExpo + ExposureLimit (night/scene mode?) |
| 0x41f | AE WinWgtType (metering) |
| 0x420 | IQ WDR |
| 0x421 | AWB CTMwb (colour temp) |

### 5. GPIO map
| GPIO | State | Role |
|---|---|---|
| 4, 5 | out hi | auth chip — do not touch |
| 10, 11, 12 | out lo/lo/hi, owned by `cardv` | no visible effect when toggled; likely IR-cut H-bridge (10/11) + IR LED/enable (12), unpopulated |
| **38** | out hi | **status LED** (confirmed by blink test) |
| 124 | out hi | set by U-Boot; likely power hold — do not touch |
| 131 | in lo | input, unknown (USB/card detect?) |

### 6. Other useful on-device tools
- `/bootconfig/bin/riu_r`, `riu_w` — raw SoC register read/write (RIU bus). Powerful, dangerous.
- `libmi_iqserver.so` linked into `cardv` — SigmaStar IQ tuning server (PC IQ tool over network).
- `libmi_ipu.so` present — NPU/IPU is available on this SoC.
- Device libc is **glibc 2.30** (`/lib/libc-2.30.so`) — cross-compile against ≤2.30 or link static.
- `busybox telnetd` already runs from `rcS`.

### Tooling notes
- Only one program may open `/dev/ttyUSB0` at a time. A monitor plus a command sender on the same port crash each other — use one owner (serial broker) when both are needed.
- `extracted/` = unpacked static flash rootfs (`microscope_C22018_0xE41B1D54_extracted-claude.tar.gz`). Disassembly: `arm-linux-gnueabihf-objdump -d -M force-thumb` (code is Thumb-2).

### Exposure via FIFO — decoded + tested (2026-09-24)
Source: Ghidra decompile of `cardvInputMessageLoop` (command table, `CarDVAddCmdHandler(name, argc, fn)`) and `isp_process_cmd`.
- Syntax: `echo "<cmd> <value>" > /tmp/cardv_fifo`. Wrong argument count = silently ignored.
- **`shutter N`** (ISP cmd 0x40b): AE → manual, keeps current gain. 1 = 16666 µs (1/60), 2 = 8333 (1/120), 3 = 4166 (1/240), 4 = 2000 (1/500), 5 = 1000 (1/1000). Other = back to auto.
- **`iso N`** (ISP cmd 0x412): 0 = auto. 1–6 = sensor gain 1×…32× (`0x400 << (N-1)`). −1 = AE state 1. **−2 = AE lock** (wait for AE to settle, then freeze that exposure as manual). −3 = restore saved exposure. **Any other value = raw sensor gain** (1024 = 1×).
- Live test (one continuous UVC stream, command mid-stream), mean luma: auto 31 → 1/60 99 → 1/120 75 → 1/240 55 → 1/500 39 → 1/1000 26 → +`iso 6` 58 → `shutter 0` back to auto 32. ISO presets at 1/100: 45/58/72/92/117 for 1×…16×. Changes are instant and hold.
- **Any new UVC stream start resets exposure** (`cardv` re-applies its default manual expo on stream start). Settings must be re-sent after the host opens the camera.
- Arbitrary shutter values are not exposed by `cardv`; plan = own small program calling `MI_ISP_AE_SetExpoMode`/`MI_ISP_AE_SetManualExpo` from `libmi_isp.so`.
- Commands registered in this build: ctmwb playback frequency mirror seamless rec capture capraw lock res vidres capres loop timelapse rm format sdrename thumb tt bri con sat sha wb scene ev flicker shutter iso gamma effect winweight night audioplay aplaystop audiorec pbvolume park gsensor zoom video hdmiset voice bitrate hdmitiming muxdebug dcf srcchg timezone hdr snr quit help datelogoStamp timeformat str reset lcdbri lcdcon lcdhue lcdsat recvol sclrotate ispget decrypt hdmi dispscal. (`ispset`, `yuvdump`, `codec`, `flip` etc. exist as strings only — **not** registered.) ⚠️ `format`, `rm`, `reset`, `quit` are destructive — never send blind.

### Ghidra setup
- Ghidra 12.1.4 + Temurin JDK 21 in `~/tools/`, `uv` in `~/.local/bin`. No sudo used.
- Project: `ghidra/microscope.gpr` — analyzed `cardv`, `libmi_isp.so`, `libcus3a.so`.
- MCP: `.mcp.json` in project root runs `pyghidra-mcp` (stdio). Only one process may open the Ghidra project at a time.

### Boot chain + manual cardv stop/restart (tested 2026-09-24)
- Launcher: `/bootconfig/demo.sh` → loads `/customer/modules/*.ko`, runs `/customer/gadget/udc-uvc.sh` (configfs + `libcomposite` loaded, but it finally uses `g_webcam.ko streaming_maxpacket=3072`), exports GPIO 4/5, starts `cardv /customer/cardvini/one_bayer_4k.ini &` (4k ini because `/misc/config.ini` lacks `FB_WIDTH = 1920`), `wait_rec.sh`, `riu_w 0x1154 16 0x0613`, `/customer/gadget/vif.sh &`.
- `gkuui` (`/customer/UI/bin/gkuui 1920 1080`) = separate UI/OSD process, started by `cardv`.
- `vif.sh` = sensor watchdog: every ~20 ms compares VIF ISR count; if frozen, toggles IMX307 standby reg 0x3000 via `/customer/i2c_read_write -b 1 -a 0x34 -f A16D8`. Burns CPU.
- `/customer/i2c_read_write` = direct sensor register access (IMX307 on I2C bus 1, addr 0x34).
- `/customer/gadget/udc-msdc.sh` = USB mass-storage gadget script (mode-switch precedent).
- **Stop:** `kill <vif.sh>`, `kill <gkuui>`, `kill <cardv>`. `cardv`'s exit handler hangs on `system("cat /proc/kmsg | tee /mnt/mmc/kmsg_info_*.txt")` — kill that `cat` to let teardown finish, then delete `/mnt/mmc/kmsg_info_*.txt` and `/mnt/mmc/pid*_maps.txt` (RAM). Free RAM 27 MB → 43 MB with everything stopped.
- Stopping `cardv` drops the USB gadget (`PULL_UP(OFF)`) → usbipd detaches; re-attach from Windows.
- **Restart:** `cd / && cardv /customer/cardvini/one_bayer_4k.ini &`, then `riu_w 0x1154 16 0x0613`, then `/customer/gadget/vif.sh &`. UVC + FIFO commands verified working after restart.

### SD card
- 32 GB SanDisk ("SK32G"), formatted on-device 2026-09-24: `mkfs.vfat -F 32 -n CAMERA /dev/mmcblk0p1`, mounted `/app/bin/sd`, 1 MB write/readback md5 OK.
- No exFAT in this kernel (only vfat) → 4 GB max file size for recordings.
- User rule: SD may hold files, but **push binaries via UART base64** (`tools/uart_push.py`), not SD.

## MI CALL TRACE — cardv pipeline recipe (2026-09-24)
Tool: `src/mitrace/mitrace.so` (LD_PRELOAD shim, 108 wrappers, 8-word passthrough, pointer args hex-dumped after a pipe readability probe). Run: `CAM_PRELOAD=/tmp/mitrace.so /tmp/cam_start.sh`. Log: `/tmp/mi_trace.log` (RAM — cardv remounts the SD at startup, stranding open files there). Captures: `logs/mi_trace_boot1.log` (no HDMI), `logs/mi_trace_boot2.log` (HDMI + replug). All calls return 0; UVC works with the shim loaded.

Bring-up order (pid = cardv):
1. `MI_SYS_Init`
2. Sensor: `MI_SNR_InitDev` → `SetPlaneMode(0,0)` (linear) → `QueryResCount` → `GetRes` → `SetRes(0,0)` (idx 0 = 1920x1080@60) → `Enable(0)` → `GetPadInfo`
3. VIF: `CreateDevGroup(0)` → `MI_SNR_GetPlaneInfo` → `SetDevAttr(0)` → `EnableDev(0)` → `SetOutputPortAttr(0,0)` → `EnableOutputPort(0,0)`
4. ISP: `CreateDevice(0)` → `CreateChannel(0,0)` → `SetChnParam` → `StartChannel` → `EnableOutputPort(0,0,0)`
5. SCL dev 0 (ports 0 and 1) and dev 3 (port 0): `CreateDevice` → `CreateChannel` → `SetChnParam` → `StartChannel` → `SetOutputPortParam` → `EnableOutputPort`; `SetChnOutputPortDepth` 3
6. `MI_SYS_BindChnPort2` ×4 @ 30/30 fps (bind types 1 and 4)
7. `MI_ISP_API_CmdLoadBinFile` (IQ), SCL port 1 reconfigured for UVC/venc
8. Display: `MI_DISP_SetPubAttr` → `Enable` → `SetVideoLayerAttr` (1920x1080) → `BindVideoLayer` → `EnableVideoLayer` → `SetInputPortAttr` → `EnableInputPort` → `SetInputPortSyncMode`
9. HDMI: `MI_HDMI_Init` (registers event callback in cardv @ `0x30f35`) → `Open(0)` → `SetAttr` (timing 9) → polls `GetAttr` … `MI_HDMI_Start` **only from the hotplug callback**.
10. VENC: `CreateDev(8)` → `CreateChn(8,1)` → `SetChnAttr`.

### HDMI no-picture bug (stock firmware)
- Symptom: monitor shows "HDMI" then "Check Device Power" (hotplug seen, no TMDS signal). UART: `Check Edid Data Fail`.
- Cause: cardv starts HDMI only in its hotplug callback. With the cable already plugged at cardv start, the first EDID read fails and `MI_HDMI_Start` is never called.
- Workaround: unplug/replug HDMI while cardv runs → `Hdmi_callback [HDMI In]`, `EDID OK`, `MI_HDMI_Start`, picture.
- Own app must: check HPD at startup, retry EDID, call `MI_HDMI_Start` directly.
- Monitor EDID: 1080p OK, YUV444, 36-bit deep colour; driver forces RGB444.

### Transfer tools
- `tools/uart_push.py <local> <remote>` — base64 over UART, md5 check.
- `tools/uart_pull.py <remote> <local>` — 1 KB chunks with per-chunk md5 + retry (console output from cardv/kernel interleaves with transfers). ~5 KB/s. Pull a frozen copy (`cp` first) of files still being written.
- `tools/check_syms.sh <bin>` — fails if a binary needs symbols/versions missing from the device libs. Build flags needed for glibc 2.30: `-std=gnu11 -U_TIME_BITS -D_TIME_BITS=32 -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=32 -fno-stack-protector`, link `-nodefaultlibs` against `extracted/rootfs/lib/*.so` with `-L/-l` (vendor libs lack SONAME).
- `tools/device/cam_stop.sh`, `cam_start.sh` (push to `/tmp`; `CAM_PRELOAD=` optional; refuses to start `vif.sh` if cardv died).

## OWN APP — `src/camapp` (replacing cardv, step by step)
### Step 3b ✅ (2026-09-24): sensor → VIF → ISP without cardv
- `camapp [secs]`: `MI_SYS_Init(0)` → `MI_SNR_InitDev(NULL)` → `SetPlaneMode(0,0)` → `SetRes(0,0)` → `Enable(0)` → `MI_VIF_CreateDevGroup(0,{4,0,0,2,0,0,0})` → `SetDevAttr(0,{0x20,0,0x04380780,0,0})` → `EnableDev(0)` → `SetOutputPortAttr(0,0,{0,0x04380780,0x04380780,0x20,0})` → `EnableOutputPort(0,0)` → `MI_ISP_CreateDevice(0,{1,0})` → `CreateChannel(0,0,{1,0…}[0x50])` → `SetChnParam(0,0,{hdr 0, 3dnr 1, mirror 0, flip 0, rot 0})` → `StartChannel(0,0)` → `EnableOutputPort(0,0,0)` → `MI_SYS_BindChnPort2(0,{VIF 6,0,0,0},{ISP 0x21,0,0,0},30,30,1,0)`; reverse teardown via undo stack.
- Result: all calls 0, **VIF IsrCnt ≈ 59/s (sensor runs 1080p60)**, clean teardown; baked 3 consecutive runs (identical, 27/27 calls OK), stock `cam_start.sh` recovers after.
- Struct sources: Ghidra `CarDV_VifModuleInit`, `CarDV_VifDevInit`, `CarDV_IspModuleInit`, `CarDV_IspChannelInit` + trace dumps. MI module IDs: VIF 6, ISP 0x21, SCL 0x22, DISP 0xf, VENC 2. `ChnPort = {mod, dev, chn, port}`.
- Frame check: `/proc/mi_modules/mi_vif/mi_vif0`, line after `IsrCnt` header.
- **Mirror/flip/rotate = `MI_ISP_SetChnParam` fields 3/4/5** (future flip feature).
- 3A (AE/AWB) not started yet → exposure is whatever the ISP defaults to.
- Next: 3c = SCL dev0 port0 → DISP → HDMI (picture on monitor).

### Step 3c ✅ (2026-09-24): sensor → VIF → ISP → SCL → DISP → HDMI without cardv
- Added: `MI_SCL_CreateDevice(0,{3})` → `CreateChannel(0,0,{0})` → `SetChnParam(0,0,{0})` → `StartChannel(0,0)` → `SetOutputPortParam(0,0,0,{0,0x04380780,0x04380780,0x101,0xb NV12,0})` → `EnableOutputPort(0,0,0)` → `MI_SYS_SetChnOutputPortDepth(0,{SCL,0,0,0},0,3)` → bind ISP→SCL (type 4 realtime) → `MI_DISP_InitDev(NULL)` → `SetPubAttr(0,{0x800080 bg, 5 HDMI, 0x10 1080p60, +40 zero bytes})` → `Enable(0)` → `MI_HDMI_Init({our event cb, NULL})` → `Open(0)` → `SetAttr(0,{0,1,9,0,0,0,4,…}[12])` → **`MI_HDMI_Start(0)` directly** (no hotplug wait) → `MI_DISP_BindVideoLayer(0,0)` → `SetVideoLayerAttr(0,{0,0x04380780,0x780,0x438,0})` → `EnableVideoLayer(0)` → `SetInputPortAttr(0,0,{0,0x04380780,0x04380780})` → bind SCL port0→DISP (type 1) → `EnableInputPort(0,0)` → `SetInputPortSyncMode(0,0,2)`.
- Result: 63/63 calls OK, VIF 60.0 fps over 20 s, **live picture on HDMI confirmed by user; motion visibly smoother than stock cardv**. Clean teardown, stock restart OK.
- Struct sources: Ghidra `CarDV_SclModuleInit`, `CarDV_DispModuleInit`, `CarDV_HdmiModuleInit` + trace. HDMI with cable already plugged at start worked (no EDID-at-boot issue with direct Start).
- No 3A, no OSD, no UVC, no encoder yet.

### Step 4a ✅ (2026-09-24): auto-exposure / AWB running inside camapp
- 3A start is automatic: `MI_ISP_CreateChannel` always calls `MI_ISP_EnableUserspace3A` → `CUS3A_Init` + `CUS3A_EnableUserspaceAE/AWB/AF` + `MI_ISP_RegisterIspApiAgent(Cus3A_Set/GetIspApiData)`. So AE runs in whichever process creates the ISP channel.
- Missing piece was cardv's IQ loader thread (cardv `0x2f128`): wait for `MI_ISP_IQ_GetParaInitStatus(0,chn,&done)`, then `MI_ISP_API_CmdLoadBinFile(0, chn, "/bootconfig/iqfile/isp_api.bin", 0x4d2)`. Without it the picture stays very dark.
- cardv `0x2f174` thread = frame-sync helper (`Cus3AOpen/WaitIspFrameSync`) that applies temporary manual exposures; not needed for AE.
- Result (30 s, lens covered / flash by user): AE live and adapting — shutter 3645–9985 µs, sensor gain 1×–8×, `stable` flag toggles. Auto mode caps at **9985 µs (≈1/100, flicker banding step) and 8× gain**. VIF 60.0 fps. All MI calls OK. In-process `MI_ISP_AE_QueryExposureInfo` now returns live values (unlike a separate process).

### Step 4b ✅ (2026-09-24): arbitrary manual shutter / gain in camapp
- `/tmp/camapp_fifo` commands: `shutter <us>`, `gain <sensor gain, 1024 = 1x>`, `auto`. Implementation: `MI_ISP_AE_GetManualExpo` → change one field → `MI_ISP_AE_SetExpoMode(4 = manual)` → `MI_ISP_AE_SetManualExpo`; `auto` = `SetExpoMode(0)`. Expo struct `{FNx10, sensor gain, ISP gain, shutter us}`.
- Test (40 s, steady scene): `shutter 20833` → read back 20830 µs; `gain 2048` → 2.00x; `shutter 5000` → 4978 µs; `gain 8192` → 8.00x; `auto` → AE resumes and reconverges within ~7 s. All calls 0; user saw the brightness cycle on HDMI.
- ⚠️ VIF stayed at **60 fps** the whole run, so a 20.8 ms exposure cannot physically fit a 16.7 ms frame. The 20830 µs readback is the ISP's value; the real sensor exposure is probably clamped near 1/60. True 1/48 (180° at 24p) needs the sensor at ≤ 48 fps (`MI_SNR_SetFps` or a 30/24 fps sensor mode). Measure before trusting long shutters.
- Stock lens is ~F12 (user) → very dark images; lens was removed for tests.

### Step 4c — frame rate (2026-09-24; SOLVED later: driver time scale, see COMPACT.md findings)
- `MI_SNR_SetFps(0, n)` works but the driver **doubles** the result in the 1080p60 mode: `fps 24`/`24000` → 48, `25` → 50, `30000` → 60 (VIF IsrCnt), `60000` → error `0xA01A…`.
- Stock sensor timing (IMX307 regs): VMAX 0x3018-301A = 1125, HMAX 0x301C-301D = 0x898 (2200), FRSEL 0x3009 = 1 → true 60 fps; line time 2200/148.5 MHz ≈ 14.8 µs. So VIF IsrCnt = real frames; driver computes VMAX as if HMAX were 4400 (30 fps mode).
- Workaround (untested): request half (`fps 12000` → 24). Verify with VMAX; exposure lines = VMAX − SHS1 (0x3020-3022) − 1.
- camapp teardown does not restore fps → stock cardv inherits the changed timing (dark/odd picture) until reboot. Fix: set fps explicitly at start, restore at teardown.
- Exposure longer than the frame time is clamped by the sensor; the ISP readback is not proof. Long shutters (1/48, 180° @ 24p) need a real ≤ 48 fps frame rate.
- `tools/push_all.sh` re-pushes camapp, mitrace.so, cam_stop/cam_start.sh to /tmp after a reboot.

### Step 5a ✅ (2026-09-24): JPEG encoder in camapp
- `MI_SCL_SetOutputPortParam(0,0,1,{0,0,0x04380780,0x101,0xb,0})` + `EnableOutputPort(0,0,1)` → `MI_VENC_CreateDev(8,{0x2000,0x2000})` → `MI_VENC_CreateChn(8,0,{4 JPEG,1920,1080,0xc00000,1,1920,1080,0,0,0,7,30,1,70}[19])` → bind SCL port1 → VENC {2,8,0,0} type 1 → `MI_VENC_StartRecvPic(8,0)`.
- Fetch loop (from cardv @0x494ec): `MI_VENC_GetFd` + select → `MI_VENC_Query(stat[0x28])` (stat+0x10 = packs) → `MI_VENC_GetStream(8,chn,{packs*, count,…}[0x40],-1)`, pack = 0xb8 bytes (+8 data, +0xc len) → `MI_VENC_ReleaseStream`.
- Result: **1080p MJPEG at 60.1 fps** (1202 frames/20 s), ~55–160 KB/frame at q70, HDMI still live, `snap` fifo command saved a valid 1920x1080 JPEG (`logs/snap_5a.jpg`).
- cardv source was config: dev 8 = JPEG engine; cardv used chn 1 (resized to 7488x4212 for fake stills).
- Stock cardv does not survive a restart after camapp (hangs, no HDMI) → after tests use `reboot` over UART instead of `cam_start.sh`.

### Step 5b ✅ (2026-09-24): UVC webcam from our own pipeline
- `src/camapp/uvc.c`: kernel 4.9 f_uvc userspace protocol on the device's `/dev/video0` (opening it = USB pull-up on). Private events `V4L2_EVENT_PRIVATE_START + {0 connect, 1 disconnect, 2 streamon, 3 streamoff, 4 setup, 5 data}`, `UVCIOC_SEND_RESPONSE = _IOW('U',1,{s32 length; u8 data[60]})`.
- Handles VS probe/commit (26-byte UVC 1.0 control; SET_CUR responds with length only, then DATA event), stalls every other request (`length = -EL2HLT`). On STREAMON: `S_FMT` MJPEG + `REQBUFS` 3 × 2 MB MMAP, queue latest encoder JPEG on each `DQBUF`.
- Result: host commit frame 4 (1920x1080) @ 333333 (30 fps); WSL `ffmpeg` captured 90/90 1080p MJPEG frames from our app (`logs/uvc_own.mjpeg`, `uvc_own.jpg`). Windows enumeration + PU/XU queries handled without crash.
- Bug fixed on the way: `respond(NULL, 26)` for SET_CUR did `memcpy` from NULL → segfault.
- Descriptors were vendor g_webcam (MJPEG only, max 30 fps); DONE 2026-09-26: own configfs gadget, 1080p 30/60.

### Step 6 ✅ (2026-09-24): recording MJPEG AVI to SD
- `src/camapp/avi.c`: minimal AVI 1.0 writer ('00dc' chunks, idx1 + header patch on close, 1 GB split). FIFO `rec 1` / `rec 0` → `/app/bin/sd/REC_NNNN.avi`.
- Test (A1 C10 32 GB card): `REC_0001.avi` 248 MB, 1688 frames 1920x1080, ffprobe OK, **0 decode errors**, frames look right. ~56 fps average: encoder dips to 48–60 fps while recording because writes happen in the encoder thread (~8.5 MB/s at q70 60 fps). Header says 60 fps → playback ~7% fast.
- **Hang root cause (fixed):** after `cam_stop.sh`, cardv had unmounted the SD; `/app/bin/sd` is then a plain dir on the RAM rootfs, so recording filled RAM → system freeze. Fix: `rec` only if `/proc/mounts` has `/app/bin/sd vfat`; stop on first write error; `cam_stop.sh` remounts the card.
- Teardown with a USB host attached / mid-stream verified clean (not the hang cause).
- Next (6b): writer thread + frame queue, `rec` fps option (30 fps halves data rate), try V30 card; later H.264/H.265.
- SD pull procedure: `sync; umount /app/bin/sd`, then remove.

### SD card notes (2026-09-24)
- Bus: `/sys/kernel/debug/mmc0/ios` (mount debugfs) → 4-bit, 50 MHz, "sd high-speed" timing, 3.3 V (UHS negotiation fails) → ~25 MB/s theoretical.
- SK32G 32 GB (A1 C10): writes OK, sustained ~8.5 MB/s through FAT during 60 fps MJPEG recording (borderline).
- User's 55 GB SDXC "U3" card (from a handheld, imaged by user): **silently discards all writes** (pattern written to sector 2048, caches dropped, old data read back; `ro` = 0). Card-side write lock / worn or counterfeit. Nothing on it changed.

### Step 6b (2026-09-24): recording writer thread + 30 fps option
- Encoder thread copies each JPEG into a queue (`REC_QUEUE` slots × 512 KB); a writer thread does the SD writes. `rec 1` = 60 fps, `rec 1 30` = every 2nd frame (true 30 fps, header matches). Drops counted.
- Card now in use: 58.4 GB "C10" (was the live_system.tar.gz card; tarball copied to project root, gzip OK). Wiped first/last 1 MB, 1 × FAT32 `CAMERA`, write-verified (md5 after drop_caches), `dd` 300 MB fsync = **13.9 MB/s**.
- Results: encoder stays at a steady **60.1 fps** while recording (stall fixed).
  - `rec 1 30`, 30 s: **901 frames, 0 dropped** ✅
  - `rec 1` (60 fps), queue 8: 1597 written / 205 dropped (11 %); queue 24, 60 s: 3268 / 337 (9 %).
  - 60 fps MJPEG q70 needs ~8.9 MB/s; our write path sustains ~8.1 MB/s while the pipeline runs → a bigger queue does not help (throughput, not bursts).
- Conclusion: 30 fps MJPEG recording is clean now. 60 fps needs a smaller stream — H.264/H.265 (future) or lower JPEG quality.

## USER NOTES / IDEAS (collected 2026-09-24/25)
Status 2026-09-26: camera modes, crop, Log gamma, register Safe/All switch, real ISP image controls, phone viewfinder via UVC are DONE; 720p/HDMI modes, Phantom RAM buffer, GPIO hunt, flicker tools, RAW, custom kernel, console on HDMI remain ideas/roadmap items (see `COMPACT.md`). The details below are kept as design notes.
- **720p HDMI output option** (user, 2026-09-25: buying a 720p monitor to cut cost): add HDMI timing 720p60 (DISP pub attr + HDMI attr), OSD layout already scales from the 1600x900 fb; playback is 720p native anyway.
- **GPIO hunt, preferably live** (user, 2026-09-25): fish around the board for usable GPIOs (test pads, unpopulated IR-cut driver GPIO 10/11/12, buttons, LED GPIO38) — toggle/read them live from a tool page (Expert › Hardware) while probing with a meter. Ties into roadmap "hardware page".
- **Flicker meter / rolling-shutter tools** (user, 2026-09-25): with a 1-line shutter (15 µs) each row is a time sample 14.8 µs apart (~67.5 kHz). Measure stripe period on screen → show light-flicker Hz; guitar-string / propeller / LED-data effects; slit-scan / photo-finish mode (stack one row over time).
- **Video modes as separate camera modes** (user, 2026-09-25): Normal / HDR / Slow-motion / Crop / Timelapse … each its own mode — pick the mode, confirm, pipeline restarts into it (like Sensor HDR now: OK twice + re-exec). Each mode keeps its own settings.
- **Slow motion must be ultra customizable, Phantom-style, with a trigger** (user, 2026-09-25). Target feature set:
  - RAM ring buffer, always recording while armed; **trigger** (remote / body button / FIFO, later motion- or brightness-change auto trigger) with adjustable **pre/post-trigger split** (e.g. 80/20).
  - Free choice of **fps vs. crop** (resolution/height trade-off shown live with the resulting max fps and buffer seconds), shutter set independently (angle or 1/x).
  - After trigger: **review/scrub on HDMI**, set in/out points, **save only the trimmed range** to SD (as a normal clip with the playback fps chosen, e.g. 24p).
  - Buffer maths: JPEG ~100–150 KB/frame → ~1000 frames in ~150 MB of the MMA heap (≈ 8 s at 120 fps); raw NV12 3 MB/frame would only give ~50 frames.
  - Depends on: crop modes (VIF/ISP/SCL reconfig), ~120 fps experiments, playback/gallery, crash-safe clip writing.
- **Slow motion** (user, 2026-09-25): today 60p recorded and played in a 24p timeline = 2.5x. More: (a) vertical crop (e.g. 1920x540, VMAX ~562 at the same line time) → ~120 fps without changing the MIPI rate, needs VIF/ISP/SCL reconfigured for the crop; (b) IMX290-style 120 fps full frame (IMX307 is the 60 fps sibling of the same family; needs double MIPI rate: REPETITION/INCKSEL + SoC receiver) — experiment, "just a hang" risk. Pairs with the crop/resolution modes item.
- **WB menu logic feels weird** (user, 2026-09-24: "a bit weird, it's fine, AWB works all the time anyway"). Revisit the UX: Auto / Kelvin / Manual + "Set from scene" + remote AWB lock is three concepts; consider a camera-style flow (AWB, Kelvin, "Custom WB" = white-card measure, lock as a toggle on the status bar).
- **Custom kernel** (user asked 2026-09-24): blocker is ABI, not size — all MI/sensor/HDMI modules are prebuilt for 4.9.227 (vermagic "4.9.227 SMP preempt mod_unload ARMv7 thumb2 p2v8"), no /proc/config.gz. Public: SigmaStar vendor kernels 4.9.84 (linux-chenxing/linux-ssc325, OpenIPC sigmastar-infinity6e, Sigmastar-Opensource); no Mercury6 4.9.227 source found in a quick search. Path: find GPL/SDK source (dashcam vendors) → same kernel + fbcon. HDMI still only comes up when modules load unless U-Boot drives it. Parked under custom firmware.
- **Console on HDMI** (user, 2026-09-24: busybox/boot output on the monitor at all times). Kernel has no VT/fbcon (no /dev/tty0, cmdline `console=ttyS0,115200`, no /proc/config.gz) and HDMI only comes up when camapp starts, so a real kernel console is out without a new kernel. Plan: camapp "Console" overlay — replay dmesg (ring buffer ~577 lines, earliest boot already lost) + boot.sh output, then follow /dev/kmsg + camapp log + command output; toggle via menu/remote (AWB free), modes off/on/errors-only. Later with custom firmware: `log_buf_len=` bootarg for the full boot log.
- **Image adjustment menu = real sensor/ISP controls only** (user, 2026-09-24: "all from sensor/ISP, I don't want artificial"): black level, gamma (ISP RGB gamma LUT), saturation, contrast, brightness, sharpening, colour matrix, AWB gains, NR — no software filters/overlays. Roadmap item 4 (Expert › ISP).
- User wants to play with register "corruptions" (sensor writes are volatile, worst case a hang) → register writes get a Safe/All access switch.
- **Histogram accuracy** (user, 2026-09-24): "a bit inaccurate, will revisit later". Candidates: Histo0 is linear pre-gamma stats while the picture is gamma/WDR/HDR-processed; HDR stats alternate empty; maybe use the post-ISP luma (`GetHistoWghtY` or a Y histogram from the SCL/encoder frame).
- **10-bit Bayer RAW recording — wanted.** Sensor delivers 1920x1080 @60 RG 10-bit ≈ 155 MB/s (≈ 78 MB/s @30, ≈ 62 MB/s @24). SD path tops out ~8–14 MB/s → needs a faster sink. Raw capture hooks exist in cardv (`capraw` cmd, `GetRaw%u`, "Get Raw Only Support Frame Mode", `CarDV_GetRawEnable` on VIF output port) → VIF frame-mode raw dump is the likely source.
- **Phone as monitor/viewfinder via UVC** (USB OTG UVC viewer app) — works with current camapp UVC (1080p MJPEG).
- **Faster storage idea: USB host mode + SSD** ("frankenstein" SSD / USB-C adapter on the USB port, switch the port to host mode).
  - Facts from boot log: two **EHCI USB 2.0 host controllers** already registered (`Sstar-ehci-1`, `Sstar-ehci-2`, 1 port each) besides the device controller (`Sstar-udc`, used for UVC); `usb-storage.ko` is loaded by `demo.sh`; and **`SStar USB3 PHY probe, base:fd2a4600`** → the SoC has a USB 3 PHY.
  - USB 2.0 host realistic ≈ 30–40 MB/s → enough for compressed/lightly packed RAW at low fps, not 1080p60 RAW. USB 3 (if routed on this board) ≈ 300+ MB/s → full RAW possible.
  - To check: which physical pins/connector the EHCI host ports and the USB3 PHY go to (the USB-B is the device port on the UDC); whether the UDC port can do OTG role switch; board test pads.
- No FPC/LCD connector on the board → viewfinder = HDMI field monitor or phone (UVC).
- **Log / flat picture profile** (idea only, user doesn't need it now): custom gamma LUT via `MI_ISP_IQ_SetRGBGamma` (0x6608-byte struct) + lower contrast/saturation/sharpness; 8-bit output → keep the curve gentle; pairs with DOL-HDR.

## CHASSIS 1 ✅ — boot integration (2026-09-24)
- Boot chain: inittab `::respawn:-/bin/sh` → `/etc/profile` → `source /bootconfig/demo.sh` → `cardv <ini> &` → `wait_rec.sh` (waits ≤ 2 s for `/tmp/cardv_rec`) → `riu_w` → `/customer/gadget/vif.sh &`.
- `/etc/profile` PATH puts **`/config` before `/bootconfig/bin`** → our wrapper `/config/cardv` shadows the vendor binary (no vendor file edited).
- Files on flash (JFFS2):
  - `/config/cardv` (`tools/device/flash/cardv`): no `/customer/camapp/enable` → `exec /bootconfig/bin/cardv "$@"` (stock). With flag: increments `/customer/camapp/boot_attempts`, `exec /customer/camapp/boot.sh`; at 3 attempts → falls back to stock.
  - `/customer/camapp/boot.sh` (`tools/device/flash/boot.sh`): touches `/tmp/cardv_rec`, kills `vif.sh`, mounts SD (retry 20 s), starts `camapp` (daemon mode, log `/tmp/camapp.log`), resets counter after 30 s healthy.
  - `/customer/camapp/camapp` (+ `camapp.good` backup), `/customer/camapp/enable` (flag).
- Tests: flag off → stock via wrapper (`/bootconfig/bin/cardv` full path in cmdline) ✅; flag on → camapp boots, no cardv/gkuui/vif.sh, HDMI picture ✅ (user), SD mounted ✅; counter=3 → automatic stock fallback ✅.
- Escape hatches: UART Ctrl+C during boot; `rm /customer/camapp/enable`; `rm /config/cardv` = exact stock; SOP8 clip + golden dump.
- **Update rule:** a running binary cannot be overwritten ("Text file busy") → install as new file + `mv` (atomic rename).
- `camapp` with no argument = daemon mode (until SIGTERM/SIGINT, clean teardown, status every 10 s).

### Input devices (camapp running, cardv gone)
- `event0` = `soc:adc-keys` (hardware buttons), `event1` = `mstar ir` (remote, keymap rc-mstar-dtv), `event2` = MCE IR keyboard/mouse.
- Remote (18 keys, user photo): zoom+ ▲ zoom− / ◀ MENU ▶ / mirror ▼ AE / flip + AWB / contrast freeze play-pause / photo record gallery.
- **IR remote scancodes** (`/dev/input/event1`, EV_MSC/MSC_SCAN; only 5 of 18 produce EV_KEY via rc-mstar-dtv → use scancodes): zoom+ 0x45, ▲ 0x46, zoom− 0x47, ◀ 0x44, MENU 0x40, ▶ 0x43, mirror 0x07, ▼ 0x15, AE 0x09, flip 0x16, + 0x19, AWB 0x0d, contrast 0x0c, freeze 0x18, play/pause 0x5e, photo 0x08, record 0x1c, gallery 0x5a. (Captured in one sequence in reading order; `logs/ir_capture_raw.log`.) Event record = 16 bytes: timeval(8) type(u16) code(u16) value(s32).
- **Body buttons** (`/dev/input/event0`, adc-keys, EV_KEY; captured one at a time 2026-09-24): UP 103 (KEY_UP), AE 105 (KEY_LEFT), MENU 139 (KEY_MENU), OK 28 (KEY_ENTER), DOWN 108 (KEY_DOWN). Body also has IR receiver window + red LED (GPIO38).

## CHASSIS 2 ✅ (first increment, 2026-09-24) — unified control layer
- `src/camapp/input.c`: one thread reads `/dev/input/event0` (body EV_KEY) + `/dev/input/event1` (remote MSC_SCAN), maps to action names, calls the same `handle_cmd()` as the FIFO. Remote repeats debounced (same code within 300 ms ignored). Unmapped/unused actions logged as `cmd ignored: <action>`.
- Working actions (tested one at a time with the user):
  - remote **mirror** / **flip** → `MI_SNR_SetOrien(0, mirror, flip)` — sensor readout direction, instant, colours correct. (`MI_ISP_SetChnParam` is rejected while the ISP channel runs: `0xA0072008`.)
  - remote **photo** → `snap` → `/app/bin/sd/IMG_NNNN.jpg` (1080p JPEG, ~230 KB).
  - remote **record** → `rec_toggle` → 30 fps AVI (`REC_0005`: 391 frames, 0 dropped).
  - **LED GPIO38**: solid when idle, blinks (0.5 Hz) while recording.
  - body **OK** reaches the layer (`button OK -> ok`, no effect yet).
- Remote table verified by use: mirror, flip, record, photo, gallery.
- Installed to flash via atomic update: `camapp.new` → `mv camapp camapp.prev` → `mv camapp.new camapp`. `camapp.good` = first boot-integrated build. Reboot OK, counter reset, input live.

## CHASSIS 3 ✅ (2026-09-24) — settings persistence
- `src/camapp/settings.c`: `/customer/camapp/settings`, key=value (`mirror flip manual shutter_us sensor_gain rec_fps`). Saved 2 s after the last change (and at exit) via `settings.tmp` + `fsync` + `rename`. Loaded right after pipeline bring-up: `MI_SNR_SetOrien` + manual exposure if `manual=1`.
- New FIFO command `recfps 30|60` (used by `rec_toggle` / `rec 1`). `auto` / `shutter N` / `gain N` now persist.
- Test: mirror on + shutter 16000 via FIFO → file written → reboot → restored (user saw mirrored image; log `AE mode=4 shutter=16000`). Shutter 1000 µs visibly ~4 stops darker (user). Boot to picture noticeably faster than stock cardv (user).
- Current saved state on the device: mirror=1, manual=1, shutter 16000 µs, gain 1x, rec 30 fps.

## CHASSIS 4 ✅ (2026-09-24) — safe update path
- **`tools/deploy.sh [binary]`** (PC): builds `src/camapp` (unless a binary is given) → `check_syms.sh` → pushes `update.sh` + `rollback.sh` + the binary → runs `/tmp/update.sh /tmp/camapp.new` on the camera, prints its result, exits with its code (0 OK, 2 rolled back).
- **`update.sh`** (device, `tools/device/flash/update.sh`): copy to `camapp.new` → `mv camapp camapp.prev` → `mv camapp.new camapp` → SIGTERM old, start new (`/tmp/camapp.log`) → after 15 s healthy = alive + `uvc_start` line in log. Unhealthy → new build moved to `camapp.bad`, `camapp.prev` restored and restarted.
- **`/customer/camapp/rollback.sh`**: manual swap camapp ↔ camapp.prev + restart.
- camapp prints `camapp build <date> <time>` as its first log line.
- Tests: live deploy without reboot (uptime unchanged) ✅; deliberately broken build → `NOT healthy, rolling back` → `ROLLED BACK, previous build running` ✅.
- Layers of safety now: deploy health check → boot counter fallback to stock cardv (3 bad boots) → `rm /customer/camapp/enable` → `rm /config/cardv` → SOP8 clip + golden dump.

**Chassis complete (1–4).** Next: interior (OSD first).

## OSD (interior) — step a ✅ (2026-09-24): framebuffer overlay proven
- `/dev/fb0` "SStar FB0": 1600x900 ARGB8888, line 6400 B, GOP 1 / GWIN 0, stretched to 1920x1080 on HDMI; alpha blend on (per-pixel alpha). State: `/proc/mi_modules/mi_fb/mi_fb0`.
- SStar fb ioctls (from stock UI `gkuui`): `FBIOGET_SHOW 0x80014662`, `FBIOSET_SHOW 0x40014663` (u8), `FBIOGET_GLOBAL_ALPHA 0x80074664`, `FBIOSET_GLOBAL_ALPHA 0x40074665` (7 bytes), `FBIOGET_COLORKEY 0x80044666`, `FBIOSET_COLORKEY 0x40044667` ({enable, r, g, b}).
- Transparency: colour key `0x0a0a0a` (fill with `0xff0a0a0a` = see-through, anything else = opaque OSD) — same as stock.
- **The OSD owner must keep `/dev/fb0` open**: the driver drops the colour key (and the overlay stops showing) when the fd is closed. Test program `src/fbtest/fbtest.c` (box shows while held open), `fbinfo.c` dumps state.
- `riu_w 0x1154 16 0x0613` (demo.sh) is already set; not needed for OSD.
- Stock UI `gkuui` is built on **LVGL** — option for menus later.
- `live_system.tar.gz` is truncated inside the tar stream (`Unexpected EOF`; the original capture had `tar: short read`), gzip layer OK; partial extraction in `live_system/` includes `customer/UI/bin/gkuui`.

### OSD step b ✅ + EV meter (2026-09-24)
- `src/camapp/osd.c` (+ `font.h` generated by `tools/psf2c.py` from Terminus Bold 12x24, SIL OFL): camapp opens `/dev/fb0` for its whole life, colour key 0x0a0a0a, text with 1-px black outline. Top status bar redrawn every second: left `STBY` / red `* REC mm:ss`; centre `M|AUTO 1/<shutter> <gain>x EV ±x.x LV y.y` (yellow when |EV| > 1 or meter saturated); right `<rec fps>p [MIR] [FLIP] <SD free GiB>|NO SD`. User confirmed on HDMI.
- **EV meter** from `MI_ISP_AE_QueryExposureInfo` (0x23c bytes): `+0x2c` avgY (linear, ~0–255, saturates ≈ 250 → shown `EV >+x.x`), `+0x238` AE target (same units), `+0x230` LV×10 (scene light value), `+0x234` BV. EV = log2(avgY / target). Calibrated: auto stable avgY 128 / target ~110 (≈ +0.2 EV); manual 16000 µs avgY 239, 4000 µs avgY 56 → avgY proportional to shutter. Manual-mode target reads 100.
- FIFO `meter` prints the raw fields.

### Metering modes ✅ (2026-09-24)
- `MI_ISP_AE_SetWinWgtType(0,0,&u32)`: 0 average, 1 centre-weighted, 2 spot (as cardv `winweight` / ISP cmd 0x41f). FIFO `metering 0|1|2`, remote **+** (crosshair) = `metering_next`; persisted (`metering=` in settings); OSD shows AVG/CTR/SPOT next to EV. Affects auto exposure and the EV meter. User confirmed switching works.
- Spot area = weight table from the IQ tuning file (centre blocks); exact size unknown yet.

### Real metering tables + spot box ✅ (2026-09-24)
- Vendor AE weight tables (`MI_ISP_AE_GetWinWgt`, 16x16 used of 32x32): "AVG" = dashcam road pattern (top rows ~0), "CTR" = proper centre-weighted bump (peak 237), "SPOT" = flat 8 (actually average).
- camapp installs its own at startup (`install_weight_tables`, `MI_ISP_AE_SetWinWgt`): AVG = uniform 8, CTR = vendor, SPOT = 3x3 blocks (6..8, 255) else 0. FIFO `wgt` dumps them.
- OSD draws a yellow box around the spot area while metering = SPOT. User confirmed in auto: picture darkens on the shield, brightens on the PCB. Exposure left in AUTO (settings persisted).

### Movable spot + EV compensation ✅ (2026-09-24)
- Spot: remote ◀ ▶ ▲ ▼ (and body UP/DOWN) move the 3x3 spot one AE block while metering = SPOT (`install_weight_tables()` rewrites the table, OSD box follows); persisted as `spot_x`/`spot_y`. Arrows outside SPOT are reserved for menus. OSD drawing is mutex-protected (called from input + main threads).
- EV compensation: `MI_ISP_AE_Get/SetEVComp` struct `{s32 EV, u32 grad}`; grad reads **3** → 1 unit = 1/3 EV. Remote zoom+ / zoom− = `ev_up` / `ev_down` (±1/3 EV, range ±3 EV), FIFO `ev0` resets; persisted as signed `ev_thirds` (settings file now signed). OSD shows e.g. `AUTO -1.7`. User confirmed working.
- Spot-metering semantics explained to user: meter renders the metered area as mid-grey (spot on bright → faster shutter).

### Lens guide
- Local doc `docs/lenses.md` (also published once as an artifact by mistake: https://claude.ai/artifact/8XoDuKLZmB2G1mU1sy9H8w). Key facts: recorded area 5.57 × 3.13 mm, diag 6.39 mm, crop ×6.8, normal ≈ 6.5 mm; ≈ Super 8 frame; recommends a short-flange (≤ 12.29 mm, D-mount) custom mount + spacers for CS/C, a 650 nm IR-cut filter in the mount, shim-calibrate infinity.

### OSD overlays ✅ (2026-09-24)
- `osd_overlay(flags)`: bit 0 rule-of-thirds grid, bit 1 centre crosshair. Lines 2 px semi-transparent white (`0xb0ffffff`, per-pixel alpha) with a faint dark edge; kept below the status bar; redrawn after the spot box moves/erases (all drawing under one mutex).
- Remote **GALLERY** = `overlay_next` (off → thirds → crosshair → both); FIFO `overlay 0..3`; persisted as `overlay=`. User confirmed.

### OSD histogram ✅ (2026-09-24)
- Source `MI_ISP_AE_GetHisto0HwStats` (0x100 bytes = 128 × u16 bins, linear light, ~8640 samples); `GetHistoWghtY` (0x208: lumY, avgY, 128 × u32) returned identical data. Both linear → plotted on a **stops** x-axis (8 stops, one faint tick per stop): per column v = 2^(x·8/W − 8), density = interpolated bin(v·128) × v, height ∝ sqrt(count). Red right edge when the top bin is populated (highlight clipping).
- Panel bottom-right 256×96 fb px, background `0x70000000`. Remote **❄ FREEZE** = `histogram_toggle`; persisted `histogram=`. Updated once per second (main loop).
- Bug fixed: command parser buffer was 15 chars → `histogram_toggle` truncated; now `char cmd[32]` / `%31s`.
- User confirmed: smooth curve, moves with scene brightness, clipping edge works.

### Menu step 1 ✅ (2026-09-24) — menu shell
- `osd_menu()` / `osd_menu_close()` in `osd.c`: left panel at (40,70), 470 px wide, 40 px rows, background `0xc8101418` (picture shows through), amber title + 1 px rule, selected row = solid amber bar `0xffffb020` with dark text and `< value >`, other rows label white / value grey. Plain text (no outline) on the panel.
- camapp menu (`menu_input()` runs before every other command): MENU opens/closes; up/down select; left/right/AE/OK change. Items: Record fps (30/60, locked while recording), Exposure (Auto/Manual), Metering, Guides, Histogram, Mirror, Flip. Every change applies immediately + persists. Non-navigation buttons (record, photo, …) keep working with the menu open.
- Guide lines clip around the open menu panel (`guide_fill`), redrawn in full on close.
- User: "on par with cinema UIs… reminds me of Sony FX / Venice".

### Menu step 2 ✅ (2026-09-24) — exposure entries
- Rows depend on mode: Manual → **Shutter** (1/24 1/25 1/30 1/48 1/50 1/60 1/100 1/120 1/250 1/500 1/1000 1/2000; values > one frame at 60 fps tagged `(>frame)`) + **Gain** (half stops, 0–24 dB = 1×–16×, shown `+12 dB 4.0x`); Auto → **EV comp** (±3 EV in 1/3 steps).
- Tested by user: Manual 1/250 + 4× and 1/24 + 16×; reboot restores `manual=1 shutter 41667 gain 16384` (log `exposure: manual 41667 us, sensor gain 16384`).
- Reminder: at 60 fps the sensor caps exposure near 1/60; the visible "ghosting" at high gain is ISP 3DNR (temporal NR) → strength belongs in the Expert menu, with a real 24/30 fps sensor mode for true 1/24–1/48.

### Menu step 3 ✅ (2026-09-24) — System page
- Main menu last row `System` → page "MENU > SYSTEM": SD card (free / size GB), **Format SD** (double OK: `OK again = ERASE`; refused while recording; background thread: umount → zero first + last 1 MB → fdisk 1 × FAT32 LBA → `blockdev --rereadpt` → `mkfs.vfat -F 32 -n CAMERA` → mount → write 256 KB / drop caches / read back → `Verified` or `FAILED (step n)`), Build (compile date/time), Uptime, **Reset settings** (double OK, `settings_reset()` + `apply_all_settings()`), Back. Moving off an armed row cancels it. Menu redraws every second while open (format progress, uptime).
- User tested all of it: format → `sd: format Verified`, reset → defaults.

## SENSOR TIMING BUG — root cause found (2026-09-24)
- Debug command `sensor` (`src/camapp/sensordbg.c`): driver view (`MI_SNR_GetFps`, `MI_SNR_GetCurRes`, mode list), IMX307 registers over `/dev/i2c-1` addr 0x1a (VMAX 0x3018-301A, HMAX 0x301C-301D, SHS1 0x3020-3022, FRSEL 0x3009) with computed fps/exposure (line = HMAX / 148.5 MHz), and VIF-measured fps. `fps N` logs before/after.
- One sensor mode only: #0 "1920x1080@60fps", crop 1920x1080, out 1948x1097, fps 3..60. Timing: VMAX 1125, HMAX 2200, FRSEL 1 → 60.00 fps, line 14.81 µs.
- Results (all three views agree): `fps 30` → still 60; `fps 20` → VMAX 1688, **40.0 fps**; `fps 12` → VMAX 2813, **24.00 fps**. Driver computes VMAX = 1125·30/req → assumes the 30 fps line time.
- **Same factor on exposure:** manual 20833 µs requested → SHS1 gives **10415 µs real**. The driver converts µs → lines with 29.6 µs/line while the sensor runs 14.8 µs lines ⇒ **every shutter time so far was half of what was shown** (AE unaffected in brightness, but its reported times are 2× too long).
- Likely cause: imx307_MIPI.ko line-time constant vs its register table (maybe tied to `lane_num=4`); not user error.
- Fix plan: correction layer in camapp — request fps/2 and shutter×2, display register-truth values; verify each with `sensor`.

### Sensor timing correction ✅ (2026-09-24, done while user napped)
- `DRIVER_TIME_SCALE 2` in camapp: `apply_fps()` sends `fps*1000/2` milli-fps to `MI_SNR_SetFps`; manual shutter is sent ×2 to `MI_ISP_AE_SetManualExpo`. `sensor_timing()` (sensordbg.c) reads VMAX/HMAX/SHS1 each second → status bar shows **register-truth** shutter and fps (fallback: ISP value / 2).
- New setting `sensor_fps` (24/25/30/48/50/60, default 60): menu row **Frame rate** (main menu, first row; locked while recording), FIFO `framerate N`. Record fps (30/60 decimation) only applies at 60 fps; other rates record every frame with the AVI header at the sensor rate.
- `apply_exposure()`: manual shutter clamped to 97 % of the frame time (otherwise the driver stretches VMAX and never shrinks it → silent fps drop), then waits 150 ms (ISP applies exposure 1–2 frames later) and re-applies the frame rate. Auto mode also re-applies fps.
- Verified with `sensor` (registers + VIF count): auto 24/25/30/48.01/50/60 exact; manual 1/48 = 20830 µs real at 24/25/30p, clamped 20207/19393/16163 µs at 48/50/60p with fps exact; 1/120 at 60p = 8326 µs; 1/50 at 25p = 20000 µs; 1/60 at 30p = 16667 µs. Recording at 24p: 144 frames / 6 s, 0 dropped (test clip deleted).
- Bug during the work (caught by build, never deployed): a Python splice duplicated a code block; repaired from the file itself (backup `camapp.c.broken` in scratchpad).
- State restored after testing: auto, 60 fps, manual slot 10000 µs.
- User confirmed from the menu: frame rate + shutter changes show correctly on the status bar.

### Expert › Sensor step 1 ✅ (2026-09-24) — live sensor info
- Main menu row `Expert` (above System) → page "MENU > EXPERT > SENSOR", read-only, refreshed 1 Hz: Real fps, Frame length (VMAX), Line length (HMAX + µs), Exposure (lines + µs from SHS1), Sensor gain (reg 0x3014, 0.3 dB/step), SoC temp (`/sys/devices/virtual/mstar/msys/TEMP_R`). `sensor_read_regs()` in sensordbg.c. User confirmed values climb when the lens is covered.

### Expert › Temporal NR ✅ (2026-09-24) — user confirmed levels look right
- `MI_ISP_IQ_Get/SetNR3D` (0x844 bytes: u32 enable, u32 op type, 16 auto entries by ISO + 1 manual, 124 bytes each; +4 u8 temporal strength (64), +54 u16[16] temporal strength vs motion 0x0fff→0x008b). `NR3D_EX` 0x24, `NR3D_P1` 4 bytes. Dump: FIFO `nr3d`, `logs/nr3d_dump.txt`.
- First mapping (Low = byte+4 /2 + curve /2, High = curve ×1.5) was WRONG: user saw Low noisier than Off. Measured with JPEG size on a static 24 dB scene (FIFO `jpgavg`, variants `nr3d_exp N`): byte+4 halved 199k, curve halved 138k (both far noisier than Off 99k), curve/byte raised 104k (worse than vendor 90k) → tuning curves, not strength. Motion-detector gain (u16 at +2, vendor 1000) is monotonic: /2 95k, ×2 88.5k.
- Final levels (from a snapshot of vendor tables after isp_api.bin load): Off = enable 0; Low = motion gain /2; Default = vendor; High = motion gain ×2. Verified 2 runs each: Off 99.1k > Low 95.2k > Default 89.2k > High 88.5k bytes/frame. Temporal NR changes static-scene noise only ~10 % overall; its main effect is on motion (smoothing vs trails) → user's eye is the final judge. Live `SetNR3D` accepted (rc 0). Expert page first row "Temporal NR"; FIFO `nr3d_level 0..3`; persisted `nr3d=` (default 2).
- Photo capture feedback (OSD flash + "IMG_nnnn saved" + LED blink; error when no SD): done later the same day.

### Photo feedback + SD hotplug ✅ (2026-09-24)
- Photo: white 6 px frame border for 150 ms + LED off-blink (short detached thread, encoder never blocks) + notice under the status bar `IMG_nnnn saved` (2 s); `PHOTO FAILED: no SD card` / `PHOTO FAILED: write error` in red. `osd_border()`, `osd_notice()`, `notice_for()`.
- SD hotplug (status loop, 1 Hz): presence = `/sys/block/mmcblk0/mmcblk0p1`; `sd_mounted()` now also requires presence (a pulled card leaves a stale mount). Pulled → `umount2(MNT_DETACH)` + `SD card removed`. Inserted → `Please wait... Initializing SD card`, next tick `mount()` + `sd_verify()` (write/read-back) → `SD ready: x GB free` or red `SD card error: mount failed` / `not writable` (no retry until the card is pulled). Skipped during format. User confirmed removal/insert/photo flow (init message added after).

### Expert › WDR step 1 ✅ (2026-09-24) — read-only dump
- FIFO `wdr` (camapp build md5 a3962fad…) → `logs/wdr_dump.txt`. `MI_ISP_IQ_GetWDR` rc 0.
- Live state after isp_api.bin load: **enable 1, op type 1 (manual)**. Manual entry: byte +0 = 3, byte +8 = 0x80, **strength +0x2B = 0**.
- Auto table (16 entries by ISO): byte +0 = 2, +8 = 0x40, strength +0x2B = 64, 64, 64, 64, 32, 16, then 0 from entry 6 up (fades out at high gain). Entries 11–15 also have +0x2D = 0xff.
- Rest identical in all entries: +0x2C..+0x31 ff0f ff0f ff0f-ish limits, +0x34 100e9808 4006b004 f4010280, then 0x80 × 32 (a flat curve).
- Reading: the vendor boots in manual mode with strength 0, so WDR is probably not doing anything now. Next: experiment command to set the manual strength (0/64/128/255) and measure.
- Sweep with FIFO `wdr_exp N` (manual strength, everything else vendor): JPEG bytes/frame 0 258k, 64 352k, 128 390k, 255 483k, back to 0 258k; AE avgY unchanged (AE stats are pre-WDR). User at 255: everything lifted, shadows open, extra noise.

### Expert › WDR row ✅ (2026-09-24, build md5 629afe53…)
- User wants a **variable number, not preset levels** ("unmatched control"). Setting `wdr` (int): -1 = vendor auto table (Auto), 0 = Off (vendor boot state, manual strength 0), 1..255 = manual strength.
- `apply_wdr()`: vendor snapshot on first call, enable 1, op type 0 (Auto) or 1 + manual byte +0x608+0x2B. Applied at startup and after "Reset settings" (reset now also re-applies Temporal NR, which it did not before).
- Expert page row 2 "WDR": right/OK +8, left/AE −8, sequence Auto < Off < 8 … 248 < 255 (clamped, no wrap). FIFO `wdr_level N` (-1..255, exact value).
- Verified: `wdr_level -1` 309k, `128` 384k, `0` 234k bytes/frame (scene lighting differed from the sweep); SetWDR rc 0; `wdr=0` saved to settings.
- User confirmed 2026-09-24: menu stepping live + value persists across power cycle. ✅

### Expert › Temporal NR as a variable number ✅ (2026-09-24, build md5 76d5411e…)
- Per the user's "unmatched control" rule the Off/Low/Default/High presets are gone. Setting `nr3d_gain` (replaces `nr3d`): the NR3D motion-detector gain (u16 at entry +2) written to all 17 entries; vendor value is 1000 in every auto entry (op type stays auto). 0 = temporal NR disabled.
- Menu: Off < 125 … 2000 (step 125) … 16000 (step 500); shows "1000 (default)". FIFO `nr3d_gain N` (0..65535 exact). Old FIFO `nr3d_level` removed.
- Verified 500 / 2000 / 0 → SetNR3D rc 0, saved. User's previous choice (Off) restored as `nr3d_gain=0` (old `nr3d=` key is ignored now).

### Input: hold-to-repeat (2026-09-24, build md5 837d78d2…)
- User: stepping numbers took forever. Old logic dropped any scan within 300 ms of the previous one and reset the window on every NEC repeat, so a held key fired once and quick taps were lost.
- Now: remote scan after a gap > 200 ms (or a different code) = new press. Holding repeats `up/down/left/right/ev_up/ev_down` only (never photo/record/etc.): after 400 ms, one step per NEC repeat frame (~108 ms), 4 steps per frame after 2 s held.
- Body buttons (adc-keys) have no kernel autorepeat (EVIOCSREP fails) → software repeat in the input thread every 110 ms until the release event, same delay/acceleration.
- User: the remote sends no repeat codes while held; wants faster tapping. New-press gap lowered 200 → 120 ms (build after 837d78d2).

### DOL-HDR step 1 ✅ (2026-09-24) — read-only check: the driver supports it
- `imx307_MIPI.ko` (description "Sensor_IMX307_HDR") contains three mode strings: `1920x1080@30fps`, `1920x1080@30fps_HDR`, `1920x1080@60fps`, and full DOL code: `pCus_init_mipi4lane_HDR_DOL`, `pCus_SetVideoRes_HDR_DOL`, LEF/SEF handles (`cus_camsensor_init_handle_hdr_dol_lef` / `_sef1`), separate shutter/gain/fps functions for the long (LEF) and short (SEF1) exposure.
- Module params: `lane_num` (default 2; demo.sh passes 4), `hdr_lane_num` (default **4**), `chmap`, `mclk`, `i2c_slave_id`. `init_module` registers, for each pad in chmap, the sensor driver (linear/LEF handle) AND plane driver 1 (SEF) unconditionally → no module reload needed.
- Stock cardv: `MI_SNR_SetPlaneMode(pad, ini HDR)` then `MI_SNR_GetPlaneInfo` → VIF dev attr; the ini (`one_bayer_1080.ini`) has `HDR = 0`. So stock never used DOL.
- camapp currently: `MI_SNR_SetPlaneMode(0, 0)`, `SetRes(0, 0)`, VIF group attr `{4 MIPI, 0, 0, 2, 0, 0, 0}` (third word = HDR type, 0 off), ISP channel attr word 1 = HDR type (0).
- Expected HDR constraints: max 30 fps (only DOL mode is @30fps), two exposures merged in the ISP; our sensor timing correction (DRIVER_TIME_SCALE 2) may or may not apply to the HDR functions → verify with `sensor` register dump.
- Next (step 2): one-shot test run `camapp hdr` (command-line only, not persisted): SetPlaneMode(0,1), list modes, pick the HDR mode, VIF group + ISP channel HDR type DOL, log everything; normal restart afterwards.

### DOL-HDR step 2 ✅ (2026-09-24) — test run, user confirmed clean, no ghosting
- `camapp hdr` test run: after `MI_SNR_SetPlaneMode(0,1)` the driver lists only mode 0 `1920x1080@30fps_HDR` (out 1948x1097, fps 3..30). `MI_SNR_GetPlaneInfo` plane 0 = `IMX307_MIPI_HDR_LEF`, plane 1 = `IMX307_MIPI_HDR_SEF`, cap 1920x1080, bayer 0, precision 2.
- VIF group attr word 2 = 2 (HDR type DOL), ISP `SetChnParam` word 0 = 2 (HDR type DOL). Only VIF dev 0 is needed.
- **First try was solid magenta:** DOL planes are **RG 12bpp** (pixel format 0x2c = 0x14 + precision 2 × 12 + bayer, the stock app's formula) while linear is 0x20 (10bpp). With vif_dev/vif_port pixel format 0x2c the image is correct.
- libcus3a AE switches to two shutters itself: long 71 µs..333 ms, short 28..3036 µs.
- Timing: registers VMAX 1158, HMAX 2136 (line 14.38 µs) = 60.04 sensor frames/s = 2 exposures per 30 fps output frame. **The driver's HDR functions use the correct time base:** SetFps 12000 mfps → 12.0 output frames/s (VMAX 2895). So DRIVER_TIME_SCALE is 1 in HDR.

### DOL-HDR step 3 ✅ (2026-09-24, build md5 1c756b78…) — persisted setting, user confirmed
- Setting `hdr` (0/1), loaded before the pipeline starts (settings_load moved to the top of main). Command-line `hdr` still forces it for tests.
- Expert page row 3 "Sensor HDR": "Off/On (30p max)", OK arms ("OK again = ON/OFF"), second OK saves and restarts; locked while recording. FIFO `hdr 0/1`.
- Restart = normal teardown (settings saved at exit, undo stack) then `execl("/proc/self/exe")`: same pid, same log, clean new pipeline. Verified via FIFO and by the user from the menu both ways.
- In HDR: frame rates limited to 24/25/30 (a stored 48/50/60 shows as 30 without changing the setting), no fps/shutter ×2 correction, status bar shows output fps (register fps / 2), the ISP long-exposure time, and "HDR".
- Not yet verified in HDR: manual shutter/gain (AE is dual-exposure; manual expo probably sets the long exposure only).

### Fixes (2026-09-24, build md5 0bd9792d…)
- Expert "Real fps" in HDR showed the sensor register rate (60); now shows output fps with the sensor rate in brackets, e.g. "30.02 (sensor 60.04)".
- Histogram flickered (vanished briefly each refresh: background was drawn straight into the visible framebuffer, then the bars). Now rendered off-screen and copied row by row, and refreshed by its own thread at ~10 Hz (`histo_loop`, HISTO_PERIOD_US 100000) instead of the 1 Hz status loop. camapp CPU ~3.5 %.
- Histogram still pulsed: in HDR `MI_ISP_AE_GetHisto0HwStats` returns all zeros on about every second read. histo_loop now skips empty reads and averages each good read 50/50 with the shown curve (rounding down so the clip marker decays). Build md5 36c47561…

### Register peek step 1 ✅ (2026-09-24, build md5 a52729b9…) — read-only viewer
- Expert page row "Registers" → page "MENU > EXPERT > REGISTERS": row Address (left/right steps 8 registers, wraps within 0x3000–0x33F8; hold repeats) + 8 rows `0xADDR  0xVV dec bits`, live at 1 Hz, Back. Nothing is written.
- FIFO `reg ADDR [COUNT]` (hex ok, 1–64) → `reg 0x3018: 86 04 00`. `sensor_read_block()` in sensordbg.c.
- Verified in HDR: 0x3018 = 0x000486 = 1158 (VMAX), 0x301C = 0x0858 = 2136 (HMAX), both match the `sensor` dump. 0x3005 = 0x01 (ADBIT 12-bit) and 0x300C = 0x11 (WDSEL DOL 2-frame) confirm the HDR findings.
- User confirmed viewer (0x3014 climbs when covering the lens). Asked for a guide → `docs/imx307_registers.md` (IMX290-family register map from the Linux imx290 driver + live HDR dump `logs/imx307_regs_hdr.txt`; unverified items marked). On screen each row label now shows the short name (`3018 VMAX L`, `3014 GAIN`…) from `sensor_reg_name()`; viewer range extended to 0x3000–0x34F8 (MIPI block). Build md5 5f39f095…
- Dump cross-checks (HDR): BLKLEVEL 240 (12-bit), INCKSEL1-7 = 37.125 MHz set, PHY lanes 3 (=4), CSI_DT_FMT 0x0C0C RAW12, Y_OUT_SIZE 2426 (both exposures), EXTCK 0x2520. SHS2 1644, RHS1 101. 0x317C reads 0xFE (not the documented 0x00/0x12) → unverified.
- User: on-screen text too vague → each known register has a one-sentence description (`sensor_reg_desc()`), shown word-wrapped (up to 4 lines) under the rows for the selected register; `osd_menu()` gained a `help` parameter. Build md5 ebe8f39d…

### Register poke step 2 ✅ (2026-09-24, build md5 3a6a31c0…) — writes, access switch, undo
- `sensor_write_reg()` (I2C_RDWR 3-byte write), `sensor_reg_safe()` list: 0x3001 REGHOLD, 0x300A/0x300B BLKLEVEL, 0x3014 GAIN, 0x308C PGCTRL.
- Registers page rows added: "Write access" Safe / All (!) (Safe→All needs OK twice), "Undo writes" (count; OK restores all). Register rows: left/right = value ∓/± 1 when writable; help line prefixed `[left/right edits]` / `[read-only]`.
- FIFO `regw ADDR VAL` (any register 0x3000–0x34FF, logged with old value, "(UNSAFE register)" tag), `regundo`. First write per address stores the original (128 slots); undo restores newest first. Not persisted: a restart re-inits the sensor anyway.
- Verified: `regw 0x308c 0x11` (REGEN + pattern 1) → full-frame fine vertical stripes on HDMI (user photo). User is playing with it before the next step.
- Source-only follow-up (ships with the next build): brace style fix in menu_draw help code.

### Expert › ISP step 1 ✅ (2026-09-24, build md5 9083f4c0…) — API survey + read-only dump
- libmi_isp exports ~50 MI_ISP_IQ_Get/Set pairs: Brightness, Contrast, Lightness, Saturation, Sharpness (+SHP_EX), RGBGamma, YUVGamma, AdaptiveGamma, RGBMatrix (CCM), HSV, OBC (black level), ColorToGray, Defog, FalseColor, DEMOSAIC, NRLuma/NRChroma/NRDeSpike, LSC/ALSC (lens shading), CrossTalk, DynamicDP (defect pixels), WDR/WDRCurve, HDR/HDR_EX, R2Y, COLORTRANS, Effect, IQMode, TEMP…
- Struct sizes (from the ioctl size in each getter): Brightness/Contrast/Lightness 0x4c (u32 enable, u32 op type 0 auto/1 manual, 16 auto u32 by ISO, 1 manual u32), Saturation 0x1a0 (17 × 24 bytes, byte 0 = strength), Sharpness 0x1328, OBC 0x90 (17 × 4 u16), RGBMatrix 0x1e8, RGBGamma 0x6608, HSV 0x134c, ColorToGray 4 (u32 enable).
- FIFO `iq` dump (`logs/iq_dump.txt`, in HDR):
  - Brightness: on, manual 52 (auto table all 50).
  - Contrast: on, auto: 42, 42, then 50 at higher ISO; manual slot 40.
  - Lightness: off, 50.
  - Saturation: on, auto: strength 42 → 20 as ISO rises (colour fades in low light); manual 50.
  - OBC (ISP black level): on, manual 3845 per channel (= 240 × 16, 12-bit black in 16-bit scale); auto 3145–3845 by ISO.
  - ColorToGray: off.

### Expert › Image (ISP) step 2 (2026-09-24, build md5 ae2dc1c7…) — six live controls, awaiting user test
- HDR switched off for ISP development (user asked; normal mode is the main use). Note: ISP black level (OBC) manual is 3845 in normal 10-bit mode too — the ISP normalises, so the value is not mode-dependent; the setting is stored as an offset from the vendor value anyway.
- Expert row "Image (ISP)" → page "MENU > EXPERT > IMAGE": Brightness, Contrast, Lightness, Saturation (Auto = vendor tuning, else 0–100 in the ISP manual slot; leaving Auto starts from the vendor manual value), Black level (offset in steps of 16 = 1 sensor code, shown as ±codes, "Default" = vendor), Black & white (ColorToGray). One-line help per row.
- Settings: isp_brightness/contrast/lightness/saturation (-1 = Auto), isp_black (offset), isp_bw. Applied at start (`apply_isp_image()`) and on Reset settings.
- FIFO `isp <brightness|contrast|lightness|saturation> -1..100`, `isp black OFFSET`, `isp bw 0/1`.
- Verified every Set returns rc 0 (contrast 0/100/auto, saturation 0/auto, black +160/0, bw 1/0, brightness 80/auto). Visual check pending.
- User: reaching Auto meant stepping down through every number. Now OK on a number row resets it (Image rows → Auto/Default, WDR → Off, Temporal NR → 1000); left/right still step. Help text says so. `menu_ok` flag set by menu_input for the "ok" action.
- Correction (user): OK is a body button (right/+ on the device), the remote has no OK. OK is +1 again. Reset to Auto/Default = body OK held 0.8 s (input.c: OK decided on release, "reset" fired after LONG_PRESS_MS) or remote play/pause (now action "reset"). `menu_reset()` handles it; ignored outside the menu.
- User confirmed 2026-09-24: all six Image controls + long-press reset work ("this image is really beautiful", B&W photo).

### Expert › Image: Gamma (2026-09-24, build md5 8991c3b3…)
- RGB gamma LUT decoded (FIFO `gamma`, `logs/gamma_dump.txt`): 0x6608 = 8 + 17 entries × (u16 R[256], G[256], B[256]), output 10-bit 0..1023. Vendor: enable 1, op type 1 (manual); manual curve ≈ gamma 2.4 with a soft shoulder (0.5 in → 791/1023); auto entries differ by ISO.
- Setting `isp_gamma` (-1 = vendor curve, else gamma × 10, 10..30). Curve = x^(1/g) with a linear toe of slope ≤ 4.5 (Rec.709 style), written to all 17 entries × 3 channels. Menu row "Gamma" after Lightness (Auto < 1.0 … 3.0 in 0.1 steps; leaving Auto starts at 2.2; hold OK / play = Auto). FIFO `isp gamma 22`.
- Verified: set rc 0; JPEG bytes/frame gamma 1.0 108k, 2.2 209k, 3.0 228k, vendor 183k.
- Bug caught right after deploy: Gamma first sat between Contrast and Lightness, breaking the `iq_simple[id - II_BRIGHT]` index for Lightness; moved after Lightness before any test.
- User confirmed gamma ("that's beautiful").

### Expert › Image: Sharpness (2026-09-24, build md5 6d6d2620…)
- Sharpness struct 0x1328 = 8 + 17 × 288, layout undocumented (FIFO `shp`, `logs/shp_dump.txt`; vendor op type auto). Field experiments via FIFO `shp_exp N` + `jpgavg` (vendor ~270k): disabled 204k; byte 1 ×2 302k; bytes 1+2 = 0 235k; byte 0 ×2 239k and = 0 226k (tuning, not strength); byte 2 ×2 283k, table +0x98 off 265k / ×2 285k (~no effect).
- Setting `isp_sharpness`: -1 Auto (vendor), 0 Off (enable 0), else % of the vendor bytes 1 and 2 in every entry. Menu row "Sharpness" (Auto < Off < 10% … 400% in 10% steps; leaving Auto = 100%; hold OK / play = Auto). FIFO `isp sharpness N`.
- Sweep (bytes/frame): Auto 263k, Off 188k, 50% 252k, 100% 263k, 200% 288k, 400% 258k → monotonic up to ~200%, beyond that detail drops again (probably clipping/halo saturation). Range kept to 400% (full control), help text explains.
- On-screen help for the "do not change" registers rewritten to say why (PLL/clock, MIPI link, bit-depth consistency, geometry/mode); new doc section "Why some registers say don't touch" in docs/imx307_registers.md. Colour matrix row (isp_ccm) verified set rc 0 at 0/100/200/Auto. Build md5 ca733060…

### Expert › Image › Matrix edit (2026-09-24)
- Colour matrix decoded: 0x1e8 = u32 enable, u32 op type, u32 0, 16 auto entries × 28 {u16 CCT, u16 m[3][4] (col 3 = row sum), u16 sat %}, manual entry {m[3][4], sat, pad} at 0x1cc. 13-bit two's complement, 1024 = 1.0. Vendor: auto, calibrated 3000/4000/6500 K (other slots CCT 0), blended by AWB CCT. `QueryCCMInfo` (0x1a) = live m[3][4] + CCT.
- "Colour matrix" row = strength: every calibrated matrix scaled m' = I + k(m − I) (0% = sensor native, 200% max in menu, 300% FIFO). Verified live matrix changes (red row 1168/−344/200 → 1312/−688/400 at 200%). Neutrals don't change (rows sum to 1.0), so a grey scene shows little difference; taught the user.
- "Matrix edit" page (MENU > IMAGE > MATRIX): Matrix Vendor (auto) / Custom (ISP manual CCM, fixed for all light), Copy current (QueryCCMInfo → custom), 9 coefficients "R from R" … "B from B" in steps of 8/1024; hold OK / play on a coefficient = copy current. Settings `ccm_custom`, `ccm_rr` … `ccm_bb`. FIFO `isp ccm_custom 0/1`, `isp ccm_copy 0`, `isp ccm_m<N> <x1024>`.
- Finding: the ISP forces every CCM row to sum to 1.0 (an unbalanced row came back with its third coefficient changed: R row 1432/−1024/−6 → 1432/−1024/616). So editing an off-diagonal is balanced by the diagonal, the diagonal is shown "(auto)" and not editable; no "keep greys" option (it would only be ignored).
- Housekeeping: settings.h comments had been shifted onto the wrong lines by earlier inserts; fixed.

### White balance ✅ code (2026-09-24, build md5 64754357…) — awaiting user test
- AWB API decoded (`wbinfo` dump, `logs/awb_dump.txt`, experiments `awb_exp`): Attr 0x5b8: +0 u32 state (1 = pause/hold), +4 u32 op type (0 auto, 1 manual gains, 2 colour temperature), +8 u16 gains R, Gr, Gb, B (1024 = 1x). `SetCTMwbAttr(u32 K)` + op type 2 = Kelvin WB (3000 K → R 1356 B 3303, 6500 K → R 2694 B 1706 under a ~2400 K lamp). QueryInfo 0x18: u16 stable, ?, R, Gr, Gb, B, CCT.
- Main menu (shooting control, after Metering): "White bal." Auto / Kelvin / Manual; "Kelvin" 2000–10000 K step 100 (Kelvin mode); "Red/Green/Blue gain" 0.25–8x step 16/1024 (Manual mode); "Set from scene" = grab current gains into Manual (persistent WB lock). Hold OK / play on a WB row = Auto.
- Remote AWB button (action `awb`): Auto → grab/lock ("WB locked" notice), otherwise back to Auto. Debug dump renamed `wbinfo`.
- Status bar right shows " 3200K" (Kelvin) or " WB M" (manual/locked), nothing in Auto.
- Settings wb_mode, wb_kelvin (5600), wb_r/g/b. FIFO `wb auto | wb kelvin K | wb manual R G B | wb grab`.
- Verified: Kelvin 3200 → ISP gains R 1474 B 3283 at 3200 K; `awb` grabbed 1220/1024/3613 at 2383 K and held; second `awb` back to Auto; settings saved.
- User test: cursor jumped when WB rows appeared/disappeared (menu_sel was positional) → menu_build_rows now keeps the selected row by id (menu_want_id for explicit targets); "Set from scene" hidden in Manual and lands on White bal. after grabbing. Explained: grabbing in Kelvin mode takes the Kelvin gains into Manual, so manual edits start from them (expected).

### Expert › Image › Hue tweaks (2026-09-24, build md5 90840213…) — awaiting user eye test
- HSV decoded (FIFO `hsv`, `logs/hsv_dump.txt`): 0x134c = u32 enable, u32 op type (vendor 1 manual), 17 entries × 290 bytes = s16[145]: three pairs {s16 hue shift[24], u16 sat[24] (64 = 1.0x)} at [0], [48], [96] + 1 u16. 24 points = 15° steps, point 0 = red. Vendor: all flat except red hue −2.
- Setting per band R/Y/G/C/B/M (points 0/4/8/12/16/20): `hsv_<b>_hue` (±128 ISP units, step 1), `hsv_<b>_sat` (0–300 %, step 5, 100 = vendor). Triangle weights half-width 4 points (neighbours sum to 1), added to vendor hue / multiplied into vendor sat, all 17 entries and all 3 pairs.
- Page "MENU > IMAGE > HUE" (Image › Hue tweaks): 12 rows + Back, help text, hold OK / play = 0 / 100 %. FIFO `isp hsv_r_h 40`, `isp hsv_g_s 0`.
- Verified readback: red hue 40 → table 38 30 20 10 … 0 10 20 30 (wraps around the circle, vendor −2 kept); green sat 0 → 64 48 32 16 0 16 32 48 64 around point 8. SetHSV rc 0. Hue unit (degrees per ISP unit) not yet known — user eye test.
- User confirmed Hue tweaks ("lovely"). Added "Reset page" rows (OK twice) to Image (all Image rows → Auto/Default; Matrix edit + Hue keep their own), Matrix edit (vendor + identity), Hue tweaks (0 / 100 %). Build md5 af6ea93a…

### Status bar gain = total (2026-09-24, build after 3e028cd1)
- User noticed "M 1/4219 8.0x" while 1/4000 × 16x was requested (during an NR test). Log: sensor 8.00x × ISP digital 2.11x = 16.9x; the bar showed only the sensor part. 1/4219 = 16 lines × 14.8 µs (shutter moves in whole lines; the ISP adds ~5 % digital gain to compensate).
- Now: gain = sensor × ISP (AE info[3] × info[4]), " d" appended when the ISP part is > 5 %. Centre buffer 48 → 64 chars.
- Open: why the sensor stopped at 8x at 1/4000 (user had real 16x at 1/24 before) — check the driver/AE gain split later.
- NR experiments (1/4000, 16.9x, avgY ~93): NRLuma / NRChroma changes incl. enable = 0 gave no measurable JPEG-size change (165–175k, ±3 % drift). These blocks may not be active in this pipeline, or the *_Adv variants are the ones used. Next: try NRLuma_Adv / NRChroma_Adv.
- Mistake + fix: my first NR sweep used exposure values from old notes and ended with `auto`, overwriting the user's manual 1/100 1.0x; restored via FIFO (`shutter 10000`, `gain 1024`) and verified in the settings file. Rule for tests: read the current settings first and restore exactly those.

### Expert › Image › Noise & clean-up (2026-09-25 early, build md5 ed32576f…) — unattended work
- Found which NR blocks are live (16.9x gain, 1/4000, JPEG size proxy, user settings read first and restored each time): basic NRLuma/NRChroma no effect; **NRLuma_Adv active** (off +5 %); NRChroma_Adv, FalseColor ~no size change; DeSpike off −13 %; Defog off by vendor.
- NRLuma_Adv (0x9bc = 8 + 17 × 146; `logs/nrluma_adv_dump.txt`): strength = two u16[8] threshold tables at entry +2 (×2 → −5 %, ÷2 → +12 %); per-level u8 table +130, falloff curve +68, limit table +36, u16 +34 = minor.
- Page "MENU > IMAGE > NOISE" (Image › Noise & clean-up): Luma NR (Auto / Off / 10–400 % of the thresholds), Chroma NR / De-spike / False colour (Auto / Off / On), Defog (Off / 5–100), Reset page. Settings nr_luma, nr_chroma, nr_despike, nr_falsecolor, defog. FIFO `isp luma_nr N`, `isp chroma_nr|despike|falsecolor -1/0/1`, `isp defog N`.
- Verified sweep (bytes/frame): Auto 135k, Off 142k, 50 % 153k, 200 % 128k, 400 % 123k; Defog 100 → 260k (strong local-contrast boost, clearly active), Defog 0 → 135k. Note 50 % noisier than Off (low thresholds add structure), as with Temporal NR. All Set rc 0.
- Also: status bar gain now = sensor × ISP gain with " d" marker (see above).

### AE limits: max auto shutter + max gain (2026-09-25, build md5 acff34bb…)
- Found why manual 16x showed 8.0x: AE ExposureLimit (8 u32: min shutter 80, max shutter 10000, FN 18/18, min sensor gain 1024, min ISP gain 1024, max sensor gain 8192, max ISP gain 1024; `aeinfo`, `logs/aeinfo_dump.txt`) caps sensor gain at 8x, also in manual; the ISP then made up the rest digitally. GAIN register: 4x → 40 (12 dB), 8x → 60 (18 dB), 16x → 60 (capped). With the cap raised: 16x → 80 (24 dB, real); 32x → still 80 but twice as bright (driver likely switches HCG for the last 6 dB; to check 0x3009 bit 4).
- Max shutter limit is in driver units (×2 in normal mode): limit 1000 → real 489 µs. Vendor 10000 = auto never slower than 1/200 real — why auto went dark in dim light.
- Other AE info: converge (speed) 20 30 100 470 470 2000 300 200 200 300; state 0; flicker 2 (probably 50 Hz); target table.
- `apply_ae_limits()` (called from apply_exposure, so frame-rate changes re-clamp): max shutter = setting clamped to 97 % of the frame × DRIVER_TIME_SCALE; max sensor gain = setting (or vendor 8x), raised to the manual gain in manual mode.
- Main menu (Auto only, after EV comp): "Max shutter" Vendor 1/200, 1/1000 … 1/100, 1/60, 1/50, 1/48, 1/30, 1/25, 1/24, 1 frame; "Max gain" Vendor 8x, 1x … 32x. Hold OK / play = Vendor. Settings ae_max_shutter_us, ae_max_gain. FIFO `aemax US GAIN`, debug `aelim IDX VAL`, `aeinfo`.
- Verified: aemax 20833 32768 at 60p → driver 32332 (16.2 ms clamp), gain cap 32768; manual 16x → register 80. User settings restored (auto, 500 µs / 1x slots, limits vendor).

### AE lock (2026-09-25, build md5 d6d27c09…)
- `MI_ISP_AE_SetState(1)` pauses AE on its current values (verified twice: +1 EV comp while locked → no change; unlock → immediate +0.66 EV).
- `ae_locked` (not persisted): remote ◐ contrast button (action `ae_lock`, notice "AE locked/unlocked"; yellow note if manual), main-menu row "AE lock" (Auto only), status bar "AE-L" instead of "AUTO". apply_exposure re-applies the state; switching to manual clears it. Debug `aestate 0/1`.

### Fixes + faster shutter (2026-09-25, build md5 c91f7cf8…)
- Noise › Reset page did nothing: NI_RESET was missing from the menu_change case list (user saw "changed" on every OK). Audited every menu row id against menu_change / menu_draw cases with a script: that was the only gap.
- Manual shutter stopped at 1/2000 (shown 1/2044 = 33 lines). Added 1/4000, 1/8000, 1/16000, 1/32000, 1/64000; AE min shutter limit lowered from vendor 40 µs to ~1 line. Verified on SHS1: 237 / 119 / 59 / 30 / 15 µs (1 line = sensor minimum, ~1/67,000). User settings restored (manual 1/250, 1x).

### AE speed (2026-09-25, build md5 f47cdfbd…) — needs the user's eye test
- AE Converge (10 u32): vendor 20 30 | 100 470 470 2000 | 300 200 200 300; last four = speeds (assumed). Debug `aeconv IDX VAL`, `aestep [THIRDS]` (EV-comp step, samples shutter × gain every 50 ms, restores the user's EV comp).
- Measurements are inconclusive: once speeds 50/50/50/50 settled in ~900 ms vs 50–100 ms vendor, but another run at 10 % (30 20 20 30) settled in 100 ms. The EV-comp step probably moves the target at once and the 1/200 max-shutter cap limits the swing, so it's a poor proxy for a real light change.
- Setting `ae_speed` (% of vendor speeds, 0 = vendor), main-menu row "AE speed" (Auto only): 5 %, 10 %, 20 %, 30 %, 50 %, 75 %, Vendor, 150 %, 200 %, 300 %; hold OK / play = Vendor. FIFO `aespeed PCT`. Applied from apply_exposure.
- Bug found and fixed: an interrupted tool call had already written the aeconv/aestep block, and it was added again → the block existed twice (second copy dead code). Removed; scanned all FIFO handlers for duplicates (only "left", which is legitimately in two places: menu + spot).

### Menu scrolling, AE Instant, Anti-flicker (2026-09-25, build md5 d5a30d3f…)
- Menus show at most 14 rows (MENU_VISIBLE); the window follows the cursor; amber ▲ (title line) / ▼ (under the last row) arrows mark hidden rows (`osd_menu(..., more_above, more_below)`, triangles drawn with fill()).
- AE speed gains "Instant" (ae_speed 1000 → all four speeds 2000). FIFO `aespeed 1000`.
- Anti-flicker row (main menu, Auto only): Vendor 50Hz / Off / 50 Hz / 60 Hz → MI_ISP_AE_SetFlicker 2/0/2/1 (enum assumed 0 off, 1 60 Hz, 2 50 Hz; readback matches each). Setting `ae_flicker` (0 vendor, 1 off, 2 50, 3 60). FIFO `flicker N`. The 60 Hz mapping is unverified until tested under mains light with a longer max shutter.

### AE hunting explained + "Instant" fixed (2026-09-25, build md5 4ad137f2…)
- User: under intense light AE flickers (hunts) then stabilises after seconds. Added `aetrace [N]` (avgY / shutter / sensor gain / ISP gain every 50 ms). Traces (logs/aetrace_steps.txt, logs/aetrace_variants.txt, EV-comp −1 EV steps driven by /tmp/aeexp.sh):
  - 200 % speeds (600 400 400 600): 12 swings, not settled in 1.5 s — the user's hunting.
  - Vendor (300 200 200 300): one overshoot (~+70 %), settled ~350 ms.
  - 50 % (150 100 100 150): no overshoot, settled 200–350 ms = fastest stable.
  - Vendor + big-error speed 2000: one large overshoot, 350 ms.
  Cause: the sensor applies exposure 2–3 frames late, so faster speeds overcorrect.
- "Instant" now = vendor speeds / 2 (was 2000 each, which would hunt). Help text on AE speed and Anti-flicker rows. User's own setting (200 %) left as they set it; recommended Instant.
- Test mistake: the first trace run ended with `aespeed 1000`, overwriting the user's newer 200 % — restored at once. (Rule already in memory: read live settings right before a test, not earlier in the session.)

### H.265 encoder ✅ verified (2026-09-25, build md5 f27309dc…)
- cardv's encoder setup decoded (`logs/dec_initVideoEncoder.c`; stock ini: Venc0 H.264 10 Mbps, codec 0 h264 / 1 h265 / 2 jpeg). Chn attr = same 19 words as JPEG: [0] type 2 H.264 / 3 H.265 / 4 JPEG, [1,2] max W/H, [3] buffer (W*H/2), [4] profile (0), [5] by-frame 1, [6,7] W/H, [8,9] 0, [10] rc mode (H.264 CBR 1 / VBR 2, H.265 CBR 8 / VBR 9, MJPEG fixQP 7), [11] GOP, [12] stat time, [13,14] fps num/den, [15] bitrate, [16,17] VBR max/min QP (CBR: [16] fluctuate). VENC dev 0 created with {0x1200, 0xa20} (max 4608x2592); JPEG is dev 8. cardv then sets RefParam {1,0,0}, SuperFrameCfg, RcPriority 2, H264/H265 VUI.
- FIFO `h265test [Mbps]`: H.265 CBR 1080p on dev 0 chn 0, bound to SCL port 1 next to JPEG, 10 s, writes the first 400 KB to /tmp/h265test.h265, full teardown. Results: 566 frames / 10 s = 56.6 fps (JPEG dropped to the same 56.6 → sharing one SCL port costs ~6 %; real recording gets its own port), 10 VPS = 1 s GOP; 3.6 Mbps in a dark static scene (SigmaStar CBR does not pad). All rc 0.
- Decode check on the PC (80 KB pulled; `logs/h265test.h265`): HEVC Main 1920x1080 yuv420p, 12/12 frames decode (black: dark room). With the sensor test pattern (regw 0x308c 0x21, undone after): 181 frames decode, colour bars (`logs/h265pattern.jpg`).
- uart_pull is ~10+ min for 400 KB; pull ≤ 80 KB cuts (dd) for checks.
- Container decision: MPEG-TS (crash-safe by design, Annex-B H.265 as is, stream_type 0x24; remux to MOV/MP4 losslessly with `ffmpeg -c copy`). Next: own SCL port for the encoder, ts.c muxer, recording integration (codec MJPEG/H.265, bitrate, GOP settings).
- `src/camapp/ts.c/.h` MPEG-TS muxer (PAT/PMT before every keyframe, PCR on keyframes 100 ms ahead, PES unbounded, PTS only, stream_type 0x24/0x1b, two-part writes for the ring buffer). PC-verified with the real camera sample (scratchpad tstest.c splits the Annex-B stream into access units): 181/181 frames, 60 fps, decodes clean; truncated mid-packet at 50 KB → 121 frames decode clean (crash-safe); `ffmpeg -c copy -tag:v hvc1` → MOV 181 frames. Sample: logs/ts_pattern_test.ts. Not yet in the camapp build.

### H.265 recording ✅ first clip (2026-09-25, build md5 13b289ba…)
- SCL device created with hw port mask 7 (ports 0 display, 1 JPEG, 2 video encoder). Runtime enable of port 2 fails with mask 3.
- h265test timing fixed (whole-second clock under-reported fps): H.265 59.8 fps alone and next to JPEG (59.9) — no loss.
- Recording integration: settings rec_codec (1 H.265 .ts default, 0 MJPEG .avi) and rec_mbps (40). Main menu rows "Codec" and "Bitrate" (10–100, H.265 only; locked while recording; help text with the MOV remux command). FIFO `codec 0/1`, `mbps N`.
  - At rec start: SCL port 2 → VENC dev 0 chn 0 (H.265 CBR, GOP = 1 s, bind src sensor fps → dst rec fps so 60→30 happens before encoding) + hevc_thread (packs → rec_queue_frame, key = VPS/IDR NAL). Writer thread writes ts.c; split at 1.9 GiB (32-bit offsets) only at keyframes. Queue slots grow up to 2 MB for big keyframes. MJPEG path unchanged (rec_frame → rec_queue_frame).
- First test: REC_0001.ts, 60 fps, 1462 frames, 0 dropped, 1.2 MB (dark static scene, CBR does not pad), closed rc 0. Not yet verified on the PC (uart pull too slow — user is bringing the SD card contents).
- Still to test: bright/detailed scene at 40–100 Mbps (sustained SD throughput), 30p decimation, 24/25 fps, file split, power-cut playback.
- PC check of the user's SD copy: REC_0001.ts (dark) 1462 frames 24.37 s 60 fps decode clean; **REC_0002.ts (user, lit scene) 888 frames 14.80 s = 60.00 fps, 40.9 Mbps (on target), decode clean, 0 dropped** (logs/rec_0002_tiles.jpg). Power-cut test: truncated at 12.3 MB / 40.0 MB / 75.7 MB → 142 / 472 / 888 frames, no decode errors.
- User asked "why TS": crash safety (MP4/MOV index written at the end), simplicity, no hvcC. Resolve free doesn't like .ts → user will remux in post (`ffmpeg -i X.ts -c copy -tag:v hvc1 X.mov`). "Finalize to MOV" declined for now.
- Device tests: 30p at 60p sensor → 299 frames / 10 s, 0 dropped (bind converts before encoding). 100 Mbps: 0 dropped but the dark room only gave 3 Mbps even at 32x gain → sustained high-bitrate write untested; card measured 13.9 MB/s FAT write, 100 Mbps = 12.5 MB/s is borderline. User settings restored (auto, 40 Mbps, luma NR auto); test clips REC_0003/0004 deleted.

### Focus loupe (2026-09-25, build md5 92d751d0…)
- Display-only magnify: `MI_SCL_SetOutputPortParam(0, 0, 0, {x | y<<16, w | h<<16, 0x04380780, 0x101, 0xb, 0})` live — port-param word 0 = crop x/y, word 1 = crop w/h (0 = none). Recording (port 2) and JPEG/UVC (port 1) keep the full frame (JPEG stayed 60 fps). User: "that did magnify" + wants variable zoom and movable.
- `apply_loupe()`: loupe_on, loupe_z10 (1.2x–8x), centre loupe_cx/cy clamped inside the frame, crop w multiple of 16, x/y/h even. Bottom-left position map (`osd_loupe_map`), status bar " LOUPE 2.5x".
- Controls: remote mirror button → action `loupe` (on/off); while on and menu closed: zoom ± (ev_up/ev_down) = ±0.5x, arrows = pan 1/8 of the view (hold repeats). Main-menu row "Magnify" (Off, 1.2/1.4/1.6/1.8/2.0, then 0.5x steps to 8x; hold OK / play = Off). FIFO `loupe`, `magnify Z10` (10 = off). Not persisted (view aid).
- Bug during testing: loupe handlers first sat after the EV/spot handlers in handle_cmd, so zoom presses changed EV comp (+3); restored to 0 at once and moved the loupe block right after menu_input. Verified 2.5→8x, pan and clamp, EV untouched.

### Focus peaking (2026-09-25, build md5 f8d8f9b5…) — awaiting user eye test
- SCL device mask 15 (port 3 = analysis). Frame grab: `MI_SYS_SetChnOutputPortDepth(port3, 2, 4)` + `MI_SYS_ChnOutputPortGetBuf(port, info, &handle)` / `PutBuf(handle)` (from cardv FUN_000510c4, logs/dec_getbuf_user.c). BufInfo measured with `pk_probe`: +0 u64 pts, +0x10 buf type 1, +0x24 pixel format 0xb NV12, +0x38 u16 w, u16 h, +0x3c luma ptr, +0x40 chroma ptr, +0x48/+0x50 phys, +0x60/+0x64 strides, +0x6c size.
- Port 3 = 480x270 NV12 with the same crop as the loupe (peak_set_crop from apply_loupe). Thread every ~80 ms drains to the newest buffer, edge = |dx|+|dy| on luma > 110 − 10 × sensitivity, `osd_peaking(mask, 480, 270, colour)` draws 3x3 dots, only changed cells, skipping status bar/notice, menu, histogram, loupe map, spot-box outline; erased dots trigger a guide redraw.
- Settings `peaking` (0 off, 1–10), `peak_colour` (red/yellow/white/blue). Main-menu rows "Peaking", "Peak colour" (only when on), help text. Remote flip button → action `peaking` (toggle, remembers level; default 5). FIFO `peaking [N]`. Stopped before osd_stop at exit.
- Verified: rc 0; camapp 9.5 % CPU with peaking on; encoder 60 fps; loupe crop sync. User setting restored to off.
- User: loupe pan felt flipped on both axes → arrows now move the picture (left = view moves right). Peaking "slow, doesn't follow": loop was 80 ms (~11 Hz). Timing added (log every 5 s): 47 updates/s at a 15 ms sleep with edge calc 2.8 ms + draw 2.8 ms; settled on 25 ms sleep ≈ 33 updates/s (~18 % of one core). Build md5 503712a7…; peaking back off (user's state).
- Loupe map mirrored (box moves with the arrow press, matching the picture-moving arrows). Build after 503712a7.

### Zebra + false colour (2026-09-25, build md5 db3c5092…)
- Analysis thread generalised: palette-index map per 480x270 cell (1 peaking, 2 zebra, 3–8 false colour), priority peaking > zebra > false colour; `osd_cells(idx, w, h, palette, n)` replaces osd_peaking. Port 3 runs while any of peaking / zebra / false colour is on.
- Zebra: luma ≥ threshold % of 255, 45° stripes ((i+j) % 8 < 4). False colour (ARRI-like, % of 255): <3 purple, <10 blue, 38–42 green (mid grey), 52–56 pink (skin), 97–98 yellow, ≥99 red, rest clear.
- Settings `zebra` (0 off, 50–100 %, leaving Off = 95), `false_colour` (0/1). Main-menu rows "Zebra", "False colour" (help text), hold OK / play = off. FIFO `zebra N`, `falsecolour 0/1`.
- Load with peaking 5 + zebra 80 + false colour: ~29.5 updates/s, calc 4.3–4.8 ms, draw 3.5–4.3 ms.
- User: "THAT was ugly" (zebra 80 + false colour during my test). Fixes: cells now tile seamlessly (the 3x3 dots on a 3.33 px pitch left a 1 px mesh), zebra and false colour 0xA0 alpha (~60 % opaque, picture shows through), zebra stripes 8-cell period (~13 px). Test settings restored (zebra 0, false colour 0, user's peaking 5 kept).
- Zebra range 0–100 % (user). Stored as threshold + 1 so 0 stays Off (existing files keep working); menu Off < 0 % … 100 % (leaving Off = 95); FIFO `zebra N` / bare `zebra` = off. (This edit had landed although its tool call showed as rejected — memory note feedback-check-after-reject.)

### Histogram from the displayed picture (2026-09-25, build md5 0114a700…)
- User: "histo is not correct" (dark scene piled up at the left). Cause: the old histogram used the ISP's Histo0 HW stats = linear sensor data before gamma, so it never matched the picture.
- Now the analysis thread (port 3, 480x270) counts the displayed luma (every 2nd pixel/row), smooths 50/50, draws every 3rd update (~10 Hz): `osd_histogram(uint32_t bins[256])`, one column per level, ticks at 25/50/75 %, red marker when > 0.1 % of samples are at 255. histo_loop thread and the Histo0 path removed (debug `histo` command kept). The analysis port now runs while histogram, peaking, zebra or false colour is on; histogram toggles call apply_peaking(). Note: with the loupe on, the histogram describes the magnified area.

### Waveform + spot fix (2026-09-25)
- Waveform monitor (build 85b62d47…): `osd_waveform(uint16_t counts[WF_H*WF_W])`, 240x100 panel bottom centre, phosphor-green intensity by count, 25/50/75 % dotted lines; computed in the analysis thread every 3rd update (every 2nd row). Setting `waveform`, main-menu row "Waveform" with help, FIFO `waveform 0/1`. Peaking/zebra cells keep clear of the panel.
- Spot box off-centre / odd size (user): the 3x3-block spot on the 16x16 AE grid can't be centred (blocks 6–8 → centre 0.469) and 16 blocks over 16:9 make flat blocks. Now SPOT_W 2 × SPOT_H 4 blocks = 240x270 sensor px (square on screen), default spot_x 7 / spot_y 6 = exact centre. FIFO `spot_centre`; used once on the user's camera (spot_x 7, spot_y 6, metering spot). Build 8bed6c2a….
- User mentioned "MTB partitions on the SPI flash": that's the MTD map already documented under FLASH PARTITION MAP (7 partitions from the MXP table at 0x20000, incl. KMSGLOG 0xFD0000–0xFF0000 = kernel message log in flash; top 64 KB not in MTD). Live /proc/mtd re-checked 2026-09-25, unchanged.

### Flash findings for our flasher + panel layout (2026-09-25)
- Golden dump analysis (logs/flash/): KMSGLOG (0xFD0000, 128 KB) and the top 64 KB are completely erased — the kernel never writes a log there (free for our own persistent log later). MISC (0xF70000, 384 KB) = "firmwarefs" (fuse fwfs at /misc): `config.ini` (display config: HDMI timings 1080p30/60, 4K30, VGA; `[LOGO]` HDMI 1080p60 bootlogo.jpg + upgrade.jpg — bootlogo.jpg absent) and `upgrade.jpg` (480x640 "SYSTEM UPGRADE" screen, logs/flash/upgrade.jpg).
- U-Boot env (0x6F000): `bootcmd=sf probe 0;sf read 0x22000000 ${sf_kernel_start} ${sf_kernel_size}; mxp r.info rootfs; sf read 0x21000000 ${sf_part_start} ${sf_part_size}; bootm 0x22000000`, bootdelay 0, `wmSdUpgradeImage=ImageUpgradeSD.bin`, sysVer SMC_20240430111451. U-Boot does no display init and no logo command; the stock "loading screen" was drawn by cardv after HDMI init.
- **SD-card upgrade path exists in U-Boot**: every boot it probes for an upgrade image (boot log: "Unknown command 'usb'", "Bad device usb 0", "VER_HEAD len =7 / Version head error. It is VerVS20") → basis for our own flasher (build an ImageUpgradeSD.bin with the right version header). User: "the bonus stuff is awesome, for our flasher". Boot logo idea dropped (user: doesn't matter).
- Histogram + waveform side by side at the bottom centre (user drawing): waveform left (x = w/2 − 8 − 240), histogram right (x = w/2 + 8), both 100 px high; peaking exclusion updated. User: waveform "very smooth".

### Playback research ✅ (2026-09-25)
- Device FFmpeg 4.1.3 is SigmaStar's build (configure line in libavcodec): hardware decoders `sshevc`, `ssh264` (VDEC), `ssmjpeg` (JPD); demuxers mpegts, mov, image_jpeg_pipe; muxers mpegts/mp4; libfdk-aac. The sstar decoders call MI_VDEC/MI_JPD/MI_SCL/MI_SYS but the libs have no DT_NEEDED on them → link the app with -lmi_vdec -lmi_jpd -lmi_scl -lmi_sys … plus -lswresample -lfdk-aac.
- cardv's player (logs/dec_open_video.c, dec_player_display.c): standard API (avformat_open_input → av_read_frame → avcodec_send_packet/receive_frame, decoder by name), after avcodec_open2 sets ctx->debug (0x240) = 1 and flags (0x3c) = 0 (no visible effect in tests). Frames go to the display with `MI_SYS_ChnInputPortGetBuf(&disp_port, &conf{u32 type 1 (+0x0), u16 w (+0x18), u16 h (+0x1a), u32 pixfmt 0xb NV12 (+0x20); 0x74 bytes}, &info(0x108), &handle, -1)`, `MI_SYS_GetCurPts(0, &info)`, copy data[0]/data[1] row by row into info +0x3c/+0x40 with strides +0x60/+0x64, `MI_SYS_ChnInputPortPutBuf(handle, &info, 0)`. Preview bind SCL→DISP is removed during playback (CarDV_DispModuleBindPreOrPlayer).
- FFmpeg 4.1.3 headers: third_party/ffmpeg-4.1.3 (generated libavutil/avconfig.h + ffversion.h). AVCodecContext offsets on ARM: flags 0x3c, width 0x5c, height 0x60, debug 0x240.
- `src/playtest/playtest.c` (standalone): REC_0002.ts decodes with sshevc: frames NV12 (fmt 23) **1280x720** (decoder's own scaler output; debug=1 doesn't change it), linesize 1280, pts in 90 kHz. Speed **34 fps with the live pipeline running, 43.6 fps with camapp stopped** → 24/25/30p real time, 60p plays at ~0.55x unless we find a faster path. One "CompleteDecodeHeader failed 0x101d" at start, recovers.
- Next: link FFmpeg into camapp; gallery page (file list), player (unbind SCL→DISP, set DISP input 1280x720, push frames at the clip rate, pause/stop/seek), JPEG photo view (image_jpeg_pipe + ssmjpeg).

### Playback stage 1 ✅ (2026-09-25, build md5 86a86853…)
- `src/camapp/player.c/.h`: FFmpeg (device libs, headers third_party/ffmpeg-4.1.3) → sshevc/ssh264/ssmjpeg → NV12 frames (1280x720 for 1080p) → `MI_SYS_ChnInputPortGetBuf` on DISP layer 0 port 0 (conf type 1, w/h, NV12) → PutBuf; display port source set to the frame size (attr word 2) and back to 1920x1080 after; pacing by pts; pause, seek ±5 s (keyframe back), hold last frame at end (play again = from start).
- camapp: `play [PATH]` (default newest REC_nnnn), `playstop`, `playpause`; while playing: MENU = back to live, play button / OK = pause, arrows/AE = ±5 s; status bar "PLAY" + "PLAY REC_0002.ts 00:05 / 00:14 PAUSE"; analysis overlays, histogram, waveform, spot box and guides hidden; SCL→DISP unbound during playback, rebound on stop (also automatic if the player thread ends). Build links -lavformat -lavcodec -lavutil -lswresample -lfdk-aac -lmi_vdec -lmi_jpd.
- Verified with the BRIO: REC_0002 plays full screen, pause shows PAUSE, stop returns to live.
- Max shutter: "Vendor 1/200" replaced by **Auto = one frame** (default 0), user bug report. Build 2a16f4b5.
- User reports "artifacting at the bottom of the frames" during playback: recording is clean (PC decode of every 74th frame, bottom strips); device decoder output bottom rows match the PC reference row means exactly (frame 30, od+awk on the device). Not yet reproduced/understood — asked the user for details. Suspects: the display copy/scaling path, or decode errors ("CompleteDecodeHeader failed 0x101d" at stream start).
- Lessons: never push/pull more than a few KB over UART (a 500 KB static binary and a 120 KB pull looked like hangs); compute stats on the device with busybox od/awk instead. BRIO works only from Windows (dshow), not attached to WSL.

## Playback smear: root cause found (2026-09-25 ~06:05)
- PC x265 reference clip TEST_0001.ts froze the device decoder on frame 1 ("no available frame buffer", then endless "start one frame failed 0x19"). Not comparable: x265 used DPB 3 (ours 2), open-GOP CRA frames, AUD + SEI NALs. The SigmaStar decoder is very picky.
- Device decode of REC_0002 with camapp stopped gives byte-identical output to the run with the live pipeline (regions md5 2d00a6f1…): not bandwidth contention, deterministic.
- Added cardv's extra encoder settings to hevc_start (MI_VENC_SetRefParam {1,0,0}, SetRcPriority 2, SetH265Dblk {0,3,3}; all rc 0, kept). Smear unchanged (REC_0004.ts).
- Recorded at 1920x1088 via the new debug FIFO `hevch 1088` (SCL port 2 + VENC at 1088): **playback clean** (REC_0003.ts, 5 s in, moving hand, bottom crisp). Cause: the device decoder mishandles 1080-high streams (partial 64-px CTU row / bottom padding); PC decoders are fine, so the 1080 recordings are valid files.
- Fix options: (a) record 1088 coded with an SPS conformance window cropping 8 rows, so files are standard 1080 everywhere and the device decoder sees 1088; needs an SPS rewrite in ts.c, and the extra 8 rows either scaled (0.7 % squeeze) or real sensor rows (IMX307 has ~1097 effective lines; needs a VIF/driver window change). (b) Plain 1920x1088 files.
- Bugs seen: recording can start while playback is shown (should be blocked); clip numbering is per extension (REC_0003.ts created next to REC_0003.avi), must be unique across formats.

## 1088 recording + drop fix (2026-09-25 ~06:50)
See COMPACT.md "CURRENT STATE" for the full list. Summary: VIF 1920x1088 in normal mode, SCL ports 0/1/3 crop to the top 1080, port 2 (H.265) 1088, SPS conformance window via hevc.c (hevc_find_sps + hevc_crop_sps, keyframe pack split in hevc_thread), decoder opened with aligned height, REC_SLOT_MAX 6 MB with shrink after write and logged drops. Camera playback of REC_0008 clean; PC sees 1920x1080. Open: 24p playback timer stuck at 00:00.

## Revert (2026-09-25 ~07:35)
At the user's request all post-compact changes were reverted; the camera runs the pre-compact source again (build dated 07:33). Post-compact code is archived in scratchpad/postcompact/. See COMPACT.md "REVERTED".
