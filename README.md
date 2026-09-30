# scope2cine

**From a cheap "microscope" to a cinema camera.**

A cheap AliExpress USB/HDMI "microscope" camera is really a SigmaStar Mercury6 dashcam board with a Sony IMX307 starlight sensor. scope2cine turns it into a cinema camera. Its program, **camapp**, replaces the vendor application (`cardv`) with its own: a small C program that runs the whole video pipeline itself and keeps every part of it hackable. The rule of the project was "unmatched control": every value is a real sensor, ISP or encoder parameter that can be set as a free number, never a preset.

The project is finished (September 2026). camapp is the camera's firmware now: HDMI camera, recorder and USB webcam in one.

## What it does

- **Pipeline without the vendor app**: sensor → VIF → ISP → scaler → HDMI display, JPEG encoder (USB webcam, photos, MJPEG), H.265/H.264 encoder, and an analysis port for the scopes. It boots faster and moves more smoothly than the stock firmware.
- **Camera modes**, each with its own settings:
  - **Normal**: 24 to 75p at full width. With a sensor crop it goes faster: up to 105p at 1280 wide. An experimental **open gate** uses the whole 1944x1096 readout.
  - **HDR**: the sensor's DOL-HDR.
  - **Slow-mo**: capture 6 to 105 fps; clips are stamped to play slower in any player.
  - **Timelapse**: 0.1 s to 60 min interval, shutter up to 2.1 s.
- **Recording**:
  - H.265 or H.264 in crash-safe MPEG-TS, or MJPEG in AVI. Bitrate up to 100 Mbps, with GOP, CBR/VBR and QP settings.
  - Per-clip sidecar metadata, record-run timecode, tally, dropped-frame warnings and time left on the card.
- **Photos**: JPEG, optionally with a **10-bit raw DNG** that opens in raw converters.
- **Exposure**:
  - P / S / G / M modes. Shutter by speed or angle, from 1/67,000 s to 2.1 s. Gain 1x to 32x. EV compensation.
  - Average, centre and movable, resizable spot metering. AE lock, AE speed, anti-flicker and AE limits.
- **Image**:
  - White balance: auto, Kelvin, manual gains, set from scene.
  - ISP controls: brightness, contrast, lightness, saturation, gamma (including a Log curve), sharpness, black level, colour matrix strength and a custom 9-number matrix, per-colour hue and saturation.
  - Noise and clean-up: luma and chroma NR, de-spike, false colour, defog, WDR, temporal NR, black and white.
  - Five look slots, exportable as text files.
- **Assists**: loupe (1.2x to 8x, movable), focus peaking, zebra, false colour, histogram, waveform (luma, RGB or parade), guides, frame lines, and hiding the OSD.
- **On-screen menu**:
  - Sections with colour accents and a help panel for every row.
  - Live sensor registers and a register editor with undo.
  - A system page (CPU, RAM, flash, SD, format, logs).
- **Controls**: 5 body buttons (with click, double-click, triple-click and hold gestures) and an 18-key IR remote, plus a text command pipe for scripting.
- **USB webcam**: its own UVC gadget with honest 1920x1080 MJPEG at 30 or 60 fps. It works with Windows Camera, VLC and Android UVC apps (a phone as a viewfinder). The host can set AE mode and brightness.
- **Safety**:
  - Automatic fallback to the stock firmware after 3 bad boots.
  - Updates with a health check and rollback.
  - Atomic settings saves.
  - Nothing of the vendor firmware is modified.

## Hardware

| | |
|---|---|
| SoC | SigmaStar Mercury6 (dual Cortex-A53, 32-bit Linux 4.9.227), 256 MB RAM |
| Sensor | Sony IMX307, 1/2.8", 2.9 µm, 1920x1080 (readout 1948x1097), 4-lane MIPI |
| Flash | 16 MB SPI-NOR (Macronix MX25L12835F) |
| Outputs | HDMI 1080p60, USB 2.0 device (UVC), SD card slot |
| Inputs | 5 buttons, IR remote, UART console |

Board silkscreen `V522_W_XX` / `DGECAM`. Details: [docs/hardware.md](docs/hardware.md).

## Quick start

**Using the camera.**
- Body MENU: hold = menu, 1 click = photo, 2 clicks = hide the OSD, 3 clicks = record.
- On the remote: MENU opens the menu, play/pause confirms or resets a row, the mirror button goes back, record and photo do what they say.

Everything else: [docs/usage.md](docs/usage.md).

**Scripting it.** Over the UART console, write commands to the pipe:
```
echo "framerate 50" > /tmp/camapp_fifo
echo "rec 1" > /tmp/camapp_fifo
tail /tmp/camapp.log
```
All commands: [docs/fifo-commands.md](docs/fifo-commands.md).

**Building and installing.**
- [docs/building.md](docs/building.md): cross compiler, the camera's own libraries as sysroot, `src/camapp/build.sh`.
- [docs/install.md](docs/install.md): first install over UART, updates with `tools/deploy.sh`, going back to stock.

## Documentation

| Document | Contents |
|---|---|
| [docs/usage.md](docs/usage.md) | modes, recording, photos, exposure, colour, assists, OSD, menu tree, every button |
| [docs/fifo-commands.md](docs/fifo-commands.md) | every command of `/tmp/camapp_fifo` |
| [docs/settings.md](docs/settings.md) | settings file keys and defaults, paths on the camera |
| [docs/install.md](docs/install.md) | first install, boot integration, updates, uninstall |
| [docs/building.md](docs/building.md) | PC setup, build flags, tools |
| [docs/hardware.md](docs/hardware.md) | board, sensor, GPIOs, remote codes, stock firmware |
| [docs/recovery.md](docs/recovery.md) | flash backup and restore, partition map, escape hatches, debugging table |
| [docs/internals.md](docs/internals.md) | architecture, threads, pipeline, sensor/driver findings, struct layouts, measurements |
| [docs/known-issues.md](docs/known-issues.md) | open bugs, parked work, roadmap |
| [docs/imx307_registers.md](docs/imx307_registers.md) | IMX307 register guide |
| [docs/lenses.md](docs/lenses.md) | lenses for the 1/2.8" sensor, mounts, IR-cut |
| [docs/looks.md](docs/looks.md) | look file format for editing looks on a PC |
| [docs/HANDOFF.md](docs/HANDOFF.md) | the complete project reference in one file (all of the above and more) |
| [docs/notes/](docs/notes/) | the development-time notes: working state document and the chronological log |

## Repository layout

```
README.md, LICENSE
docs/                 guides, HANDOFF.md, register/lens/looks guides
  notes/              COMPACT.md (working state), handoff-log.md (chronological log)
src/camapp/           the application (C, ~9,400 lines) and build.sh
src/mitrace/          LD_PRELOAD tracer used to capture the vendor app's pipeline calls
src/getexpo, fbtest, playtest, hello   small probes from the reverse-engineering phase
tools/                deploy, UART transfer, frame grabs, symbol check; tools/device/ = scripts on the camera
research/decompiles/  Ghidra decompiles of the vendor app, SigmaStar libraries and the sensor driver
archive/scratchpad/   scratch scripts and test images from development
```

## Not in this repository

- **Vendor binaries**: the flash dumps, the extracted root filesystem (glibc and the SigmaStar libraries camapp links against), the Ghidra project, and the FFmpeg source. Get the libraries from your own camera: dump the flash and unpack its root filesystem into `extracted/rootfs` ([docs/building.md](docs/building.md)).
- **Logs and captures** from development (`logs/`).

## Safety and disclaimer

- Back up the flash (16 MB, CH341A + SOP8 clip) before installing anything. camapp itself never writes the flash outside its own files, and the stock firmware stays one command away.
- This project is not affiliated with SigmaStar, Sony or the camera's seller. The vendor libraries remain their owners' property and are not distributed here.

## License

- camapp and the tools are licensed under the **GNU General Public License v2.0** (see [LICENSE](LICENSE)).
- The OSD font in `src/camapp/font.h` is converted from Terminus Bold 12x24, licensed under the SIL Open Font License 1.1.
