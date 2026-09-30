# Building and tools

The PC setup, how camapp is compiled against the camera's own libraries, and every helper tool.

> Extract of [HANDOFF.md](HANDOFF.md) (sections 6), the complete project reference. Section numbers match it.

## 6. Development environment on the PC

### 6.1 Machine
- Windows with WSL2 (Ubuntu). The kernel was `6.18.x-microsoft-standard-WSL2` in September 2026. Project at `/home/aftwr/microscopeproject`.
- **WSL `/tmp` is a tmpfs, wiped at every WSL restart.** Never keep anything important there; the session scratchpads under `/tmp/claude-1000/...` were lost that way ([§19.3](HANDOFF.md)).
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
| `deploy.sh [binary]` | build + check_syms + push + install with rollback ([§5.3](install.md)). Grep its output for `update:` / `UPDATE_RC=` |
| `sh.py "" "cmd" ...` | run shell commands over UART and print output; the first empty command wakes the prompt; output often late, re-read with `sh.py "" "" "true"` |
| `uart_push.py <local> <remote>` | push a file: gzip (when smaller), base64 in 900-char lines, md5 check. A 178 KB camapp takes ~160 round trips |
| `uart_pull.py <remote> <local>` | pull a file: one gzip+base64 stream between markers checked by md5 (seconds); on failure falls back to 1 KB chunks each with its own md5 and retries |
| `check_syms.sh <bin>` | symbol/version check against `extracted/rootfs` libs |
| `push_all.sh` | old helper: re-push camapp, mitrace.so, cam_stop/cam_start to /tmp after a reboot (from the pre-boot-integration days) |
| `uvcgrab.sh NAME` | grab one 1080p frame from the camera's USB webcam with Windows ffmpeg into `scratch/NAME.jpg` (keeps the 5th frame; the first after a reconnect can be empty) |
| `viewgrab.sh NAME` | FIFO `viewgrab` + pull `/tmp/view.ppm`: the processed picture at 160x90 over UART into `scratch/NAME.png`. Works with no HDMI and no USB |
| `trace_parse.py <log> [regex]` | parse mitrace logs into calls with argument dumps |
| `psf2c.py <font.psf> <out.h>` | PSF2 console font to C header (ASCII 32..126 + degree sign at 127) |
| `device/cam_stop.sh`, `device/cam_start.sh` | stop/start the stock stack without reboot ([§4.2](hardware.md)) |
| `device/cpustat.sh SECS` | on the camera: whole-CPU split + per-thread CPU of camapp (push to /tmp first) |
| `device/flash/*` | the scripts installed on the camera ([§5](install.md)) |

`scratch/` is the local scratch folder (gitignored). `uvcgrab.sh` and `viewgrab.sh` write into it.

### 6.4 Other programs in `src/`
| Dir | What |
|---|---|
| `src/mitrace/` | `mitrace.so`, an LD_PRELOAD shim with 108 MI_* wrappers. It passes 8 argument words through and hex-dumps pointer arguments after a readability probe. Run: `CAM_PRELOAD=/tmp/mitrace.so /tmp/cam_start.sh`, log `/tmp/mi_trace.log`. It produced the cardv recipe |
| `src/getexpo/` | read-only AE state dump (first ISP experiment) |
| `src/fbtest/` | `fbtest` (shows a box through the fb0 colour key), `fbinfo` (dumps fb show/alpha/colour-key state) |
| `src/playtest/` | standalone FFmpeg + SigmaStar hardware decode test (frame format, speed) |
| `src/hello/` | first cross-compile test (prints the glibc version) |
