# IMX307 register guide

What the sensor registers shown in **Expert › Registers** mean. Written 2026-09-24.

Sources:
- The Linux `imx290` driver. The IMX290, IMX307 and IMX327 share one register map.
- A live dump from this camera in DOL-HDR mode (`logs/imx307_regs_hdr.txt`).
- Our own tests.

Anything marked *(unverified)* comes from the sister sensors and has not been checked on this camera yet.

## Basics

- Addresses run from 0x3000 to 0x34FF, one byte each. Values wider than 8 bits are **little-endian**: the lowest byte sits at the lowest address. For example, VMAX = 0x3018 + 0x3019·256 + 0x301A·65536.
- Everything happens on I²C bus 1, address 0x1a (0x34 in 8-bit form). The viewer only reads.
- Many values depend on the mode (normal 10-bit 60p vs DOL-HDR 12-bit), so both are listed where they differ.
- The sensor driver and the ISP's auto exposure keep rewriting the exposure, gain and frame-length registers. A manual write to those will be overwritten within a frame.

## Operation

| Addr | Name | Meaning | Here |
|---|---|---|---|
| 0x3000 | STANDBY | bit 0: 1 = standby, 0 = running | 0x00 |
| 0x3001 | REGHOLD | bit 0: 1 = hold register updates, so several registers change on the same frame | 0x00 |
| 0x3002 | XMSTA | bit 0: 0 = master mode running (sensor makes its own sync) | 0x00 |
| 0x3003 | SW_RESET | write 1 = software reset (don't) | 0x00 |

## Readout and format

| Addr | Name | Meaning | Here (HDR) |
|---|---|---|---|
| 0x3005 | ADBIT | bit 0: A/D converter, 0 = 10-bit, 1 = 12-bit | 0x01 (normal mode: 0x00) |
| 0x3007 | CTRL_07 | bit 0 VREVERSE (flip), bit 1 HREVERSE (mirror), bits 7:4 WINMODE (0 = 1080p, 1 = 720p, 4 = cropped window) | 0x00 |
| 0x3009 | FR_FDG_SEL | bits 1:0 FRSEL (frame-rate base: 1 = 60/50 fps timing, 2 = 30/25). Bit 4 FDG_SEL: conversion gain, 0 = low (LCG), 1 = high (HCG, lower noise at high gain) *(bit 4 unverified)* | 0x01 |
| 0x300A–0x300B | BLKLEVEL | black level added to the data: 60 (0x3C) in 10-bit, 240 (0xF0) in 12-bit | 0x00F0 |
| 0x300C | WDMODE / WDSEL | bit 0 WDMODE: 1 = DOL-HDR. Bits 5:4 WDSEL: 1 = 2 exposures per frame (DOL 2-frame) | 0x11 (normal mode: 0x00) |
| 0x3046 | OUT_CTRL | bits 1:0 ODBIT: output width, 0 = 10-bit, 1 = 12-bit | 0x01 |
| 0x3129 | ADBIT1 | A/D tuning that must match ADBIT: 0x1D in 10-bit, 0x00 in 12-bit | 0x00 |
| 0x317C | ADBIT2 | 0x12 in 10-bit, 0x00 in 12-bit *(this camera reads a different value; unverified)* | — |
| 0x31EC | ADBIT3 | 0x37 in 10-bit, 0x0E in 12-bit | — |

## Frame timing

| Addr | Name | Meaning | Here (HDR) |
|---|---|---|---|
| 0x3018–0x301A | VMAX | **frame length** in lines (18 bits). fps = 148.5 MHz / (HMAX × VMAX). Longer frame = lower fps | 1158 (normal 60p: 1125) |
| 0x301C–0x301D | HMAX | **line length** in 148.5 MHz clocks. Line time = HMAX / 148.5 µs | 2136 = 14.38 µs (normal: 2200 = 14.81 µs) |

In HDR the sensor runs at 60 frames/s internally. Each output frame is one long plus one short exposure, so the output is 30 fps.

## Exposure and gain

| Addr | Name | Meaning | Here (HDR) |
|---|---|---|---|
| 0x3014 | GAIN | **sensor gain**, 0.3 dB per step: 0 = 0 dB (1×), 20 = 6 dB (2×), 80 = 24 dB (16×). Up to 30 dB is analog, above that digital | 0x00 |
| 0x3020–0x3022 | SHS1 | **shutter start line** of exposure 1. Normal mode: exposure = (VMAX − SHS1 − 1) × line time, so a *smaller* SHS1 means a *longer* exposure. In HDR, SHS1 belongs to the short exposure | 59 |
| 0x3024–0x3026 | SHS2 | HDR: shutter start line of the long exposure *(unverified)* | 1644 |
| 0x3030–0x3032 | RHS1 | HDR: readout offset of the short exposure. It also limits how long the short exposure can be *(unverified)* | 101 |
| 0x30F0–0x30F2 | GAIN1 | HDR: separate gain for the second exposure *(unverified)* | — |

## Window (crop)

| Addr | Name | Meaning | Here |
|---|---|---|---|
| 0x303A | WINWV_OB | number of optical-black lines output | 8 |
| 0x303C–0x303D | WINPV | vertical start of the window (used when WINMODE = 4) | 4 |
| 0x303E–0x303F | WINWV | window height | 1089 |
| 0x3040–0x3041 | WINPH | horizontal start of the window | 0 |
| 0x3042–0x3043 | WINWH | window width | 1948 |

These are the crop/resolution controls for the "crop modes" roadmap item.

## Clock (don't touch)

| Addr | Name | Meaning | Here |
|---|---|---|---|
| 0x305C–0x305F | INCKSEL1–4 | input clock setup for 37.125 MHz | 18 03 20 01 |
| 0x315E | INCKSEL5 | | 0x1A |
| 0x3164 | INCKSEL6 | | 0x1A |
| 0x3480 | INCKSEL7 | | 0x49 |

Wrong values here stop the sensor. Recovery is a camapp restart (the driver re-initialises the sensor).

## MIPI output (don't touch)

| Addr | Name | Meaning | Here (HDR) |
|---|---|---|---|
| 0x3405 | REPETITION | MIPI data-rate setting | 0x10 |
| 0x3407 | PHY_LANE_NUM | lanes − 1: 3 = 4 lanes | 0x03 |
| 0x3414 | OPB_SIZE_V | optical-black lines in the output | |
| 0x3418–0x3419 | Y_OUT_SIZE | output lines per frame. In HDR both exposures plus the gap between them | 2426 |
| 0x3441–0x3442 | CSI_DT_FMT | MIPI data type: 0x0A0A = RAW10, 0x0C0C = RAW12 | 0x0C0C |
| 0x3443 | CSI_LANE_MODE | 3 = 4 lanes | 0x03 |
| 0x3444–0x3445 | EXTCK_FREQ | input clock × 256 / 1 MHz: 0x2520 = 37.125 MHz | 0x2520 |
| 0x3446–0x3455 | TCLKPOST … TLPX | MIPI D-PHY timing (8 × 16-bit) | |
| 0x3472–0x3473 | X_OUT_SIZE | output width | 1952 |

## Why some registers say "don't touch"

"Don't touch" never means "this can damage something". Every register lives in the sensor's RAM, and a camapp restart (or replug) loads the full setup again. It means: **the picture stops or turns to garbage until you restart**. The reasons fall into four groups.

### 1. Clocks (INCKSEL1–7: 0x305C–0x305F, 0x315E, 0x3164, 0x3480)

The sensor gets one 37.125 MHz clock from the board. Inside, a PLL (frequency multiplier) turns that into three clocks: the **pixel clock** (148.5 MHz, which sets line and frame timing), the **ADC clock** (how fast pixels are digitised) and the **MIPI bit clock** (how fast data leaves the chip). The INCKSEL registers are the PLL's multipliers and dividers, and Sony only specifies a few valid sets.

What goes wrong:
- The MIPI output runs at a different speed than the SoC's receiver expects. The receiver can't lock, so no frames arrive (black or frozen picture).
- Frame timing stretches or shrinks, so the "fps" maths (VMAX × HMAX) no longer holds.
- Far off-spec values overclock the ADC. Sony doesn't document the margin, so it's the one place where "don't" is also about staying inside the datasheet.

### 2. The MIPI link to the SoC (0x3405, 0x3407, 0x3441–0x3455, 0x3443)

Pixels travel to the SoC over MIPI CSI-2: 4 differential pairs ("lanes") on the board, with packets tagged by data type. Both ends must agree on everything:
- **Lane count** (0x3407, 0x3443): the board wires 4 lanes. Telling the sensor to use 2 puts the data on lanes the SoC isn't reading, and the result is garbage.
- **Bit rate** (0x3405, REPETITION): the SoC's receiver is set up for one rate by the driver.
- **Data type** (0x3441): each packet carries a tag, RAW10 (0x0A) or RAW12 (0x0C). The SoC keeps only packets with the expected tag, so a wrong tag means every frame is thrown away.
- **D-PHY timing** (0x3446–0x3455: TCLK/THS prepare, zero, trail, TLPX): how long the lines hold each state when switching between low-power and high-speed mode. Slightly wrong: occasional sparkles or dropped frames. Very wrong: no link.
- **Input clock value** (0x3444–0x3445): the MIPI block times those phases from it.

### 3. Bit depth consistency (0x3005, 0x3046, 0x3129, 0x317C, 0x31EC)

The A/D converter, its tuning registers (ADBIT1–3), the output width (ODBIT) and the SoC's expected pixel format must all say 10-bit (normal mode) or all say 12-bit (HDR).
- Change only the output width: the SoC reads 12-bit pixels as 10-bit (or the reverse). Every pixel's bits land in the wrong place, which gives the magenta or garbage image we saw when the HDR format was wrong.
- Change only the tuning registers: the ADC works but with the wrong calibration, which shows as banding, noise or odd black levels rather than a stop.

### 4. Frame geometry and operating mode (0x3000–0x3003, 0x3418, 0x3472)

- **STANDBY (0x3000) = 1:** the sensor sleeps and the picture freezes. Writing 0 wakes it.
- **XMSTA (0x3002) = 1:** slave mode, where the sensor waits for external sync pulses. This board never sends them, so it stops.
- **SW_RESET (0x3003):** wipes every register back to Sony defaults, which means no clock setup and standby. Only a camapp restart (the driver's full init) brings it back.
- **Output size (Y_OUT 0x3418, X_OUT 0x3472):** the SoC expects an exact frame size. More or fewer lines give torn, shifted or dropped frames.

### What's fun to change instead

Gain (0x3014), black level (0x300A), the test pattern (0x308C), flip/mirror bits (0x3007 bits 0–1), frame length (VMAX, which slows the frame rate) and the window registers (crop). The worst these do is a broken picture, and "Undo writes" fixes it.

## Test pattern

| Addr | Name | Meaning |
|---|---|---|
| 0x308C | PGCTRL | bit 0 REGEN: 1 = test pattern on. Bits 7:4: pattern number 0–7 (bars, gradients…). The live value is 0x02, meaning off |

## Registers not listed

Anything else in 0x3000–0x34FF is either a fixed tuning value from Sony's init table or undocumented. Only read those. We will build a list of "safe to write" registers before the write step.
