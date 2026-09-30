# Scratchpad dump

Everything left over from the development sessions' scratch areas, collected on 2026-09-30 when the
project was closed. Nothing here is needed to build or run camapp; it is kept for reference and for
re-running old experiments. See `../../docs/HANDOFF.md` §19.3.

## Contents

| Path | What |
|---|---|
| `project_scratch/` | The project's `scratch/` folder as it was on 2026-09-30 (moved here; `tools/uvcgrab.sh` and `tools/viewgrab.sh` recreate an empty `scratch/` when used). Test images and PPMs: DNG checks (`col.dng`, `red.dng`, `strip.dng`, `test.dng`, `dngview.*`, `fbc_off*.jpg`, `raw4.bin`), open gate (`og_*`), RGN overlay attempts (`rgn1..6*`), hot-pixel dark frames (`dp_*`), HDR (`hdr_*`), UVC/boot frames (`boot.jpg`, `ctl0.jpg`, `now.jpg`, `view1.*`, `cmp1.png`). `edid.bin` = the user's LG monitor EDID. Scripts: `dnghook.py`, `focus.py`, `hdmi.py`, `og.py`, `prio.py`, `spot.py` (source-splice helpers used to add features), `dptest.sh` (hot-pixel test). `uart_push.py.orig`, `uart_pull.py.orig` = the UART tools before the gzip/stream rewrite. `pulltest.bin` = UART pull test data. |
| `recovered_from_transcript/` | The session scratchpad lived in WSL's `/tmp` (tmpfs) and was wiped by a WSL restart on 2026-09-30. These 37 files were rebuilt by replaying every Write/Edit/heredoc the assistant made there, in order, from the session transcript. `RECOVERY_LOG.txt` lists each operation. `hsweep445.sh` was produced by a `sed` in the original session and was replayed by hand. Paths inside the scripts still point at the old `/tmp/claude-1000/.../scratchpad/` location. |
| `recover_scratch.py` | The rebuild tool: `recover_scratch.py TRANSCRIPT.jsonl OUT_DIR`. |
| `claude_memory/` | Copies of the assistant's persistent memory files for this project (working rules and preferences). On disk only: gitignored, not published. The live copies are in `~/.claude/projects/-home-aftwr-microscopeproject/memory/`. |
| `hdmi_output_modes_stash.patch` | `git stash@{0}` (parked HDMI output-mode work) as a patch; apply on top of commit `c65e10b` with `git apply`. |

## Recovered scripts worth knowing

| File | Use |
|---|---|
| `brio.sh [name]` | grab one frame of the HDMI monitor from the BRIO webcam through Windows ffmpeg (dshow) |
| `hset.sh HMAX` | write HMAX in sensor standby and read back the measured fps (line-length experiments) |
| `hsweep.sh HMAX NAME` / `hsweep445.sh` | HMAX sweep with (891 Mbps) or without (445.5 Mbps) the faster MIPI settings, BRIO photo + CSI error counters |
| `cpustat.sh` | early version of `tools/device/cpustat.sh` |
| `sh.py`, `sercap.py`, `uartmon.py` | early UART helpers (the current ones are in `tools/`) |
| `key1.py`, `keylog.py` | capture body-button codes one at a time |
| `xu.py` | UVC extension-unit probe (stock firmware) |
| `tstest.c`, `hevctest.c`, `hevctest2.c` | PC-side tests of the MPEG-TS muxer and H.265 stream handling |
| `aeexp.sh`, `aeexp_dev.sh` | AE speed/overshoot experiments |
| `band.sh`, `lad.sh`, `fs.sh`, `rectest_dev.sh`, `blk.awk` | banding, ladder, file-system and recording tests; block-edge energy metric |
| `export_fns.py`, `extract.py`, `gh.py`, `xref.py` | Ghidra/export helpers |
| `isolive.py`, `isotest.py` | stock-cardv ISO tests over the FIFO |
| `layer.py`, `layer2.py`, `off.c` | OSD layering edits; struct offset checks |
| `lens-guide.html` | the lens guide that was published by mistake as an HTML page (the real doc is `docs/lenses.md`) |

## Lost for good

The Ghidra headless setup (`ghko/`), disassembly listings (`cardv.dis`, `gkuui.dis`, `avcodec.dis`;
regenerate with `arm-linux-gnueabihf-objdump -d -M force-thumb`), the `*.pre-*` source backups
(git history covers the committed states), BRIO photos, and `postcompact/` (the code reverted on
2026-09-25, including the 1088-line recording fix for the device decoder).
