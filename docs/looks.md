# Looks: editing on a PC

A look is a snapshot of every image-processing setting: the Colour › Image & looks page, the colour matrix, hue tweaks, the noise page, WDR and temporal NR. Exposure, white balance, frame rate and recording settings are never part of a look.

## Moving looks between the camera and a PC

1. On the camera: Colour › Image & looks › **Export looks** (OK). Every saved slot is copied to the SD card as `LOOKS/LOOK1.TXT` … `LOOKS/LOOK5.TXT`.
2. Edit the files on a PC with any text editor. Windows line endings are fine.
3. Put the card back and choose Colour › Image & looks › **Import looks** (OK twice). Each `LOOKn.TXT` found on the card replaces slot n.
4. Pick the slot with **Look slot**, then **Load look** (OK).

Only the keys below are accepted on import; any other line is ignored, so a look file cannot change exposure or recording. A key missing from a file keeps its current value when the look is loaded.

## File format

One `key=value` per line, whole numbers only, no comments. Example:

```
isp_contrast=60
isp_saturation=45
isp_gamma=-1
```

## Keys

**-1 means "Auto / vendor"** wherever it is allowed: the camera's own tuning, which usually adapts to light and gain.

| Key | Range | Meaning |
|---|---|---|
| `isp_brightness` | -1, 0–100 | ISP brightness |
| `isp_contrast` | -1, 0–100 | ISP contrast |
| `isp_lightness` | -1, 0–100 | Midtone lightness (black and white stay put) |
| `isp_saturation` | -1, 0–100 | Colour saturation. Vendor fades colour as gain rises. |
| `isp_gamma` | -1, 1, 10–30 | Tone curve: -1 vendor, 1 = Log (experimental; auto exposure runs 2 stops under), 10–30 = gamma × 10 (22 = 2.2 video) |
| `isp_sharpness` | -1, 0, 1–1000 | Edge sharpening in % of the vendor strength; 0 = off |
| `isp_ccm` | -1, 0–200 | Colour matrix strength in %: 0 = sensor-native colour, 100 = vendor calibration |
| `isp_black` | about -1024…1024 | Black level offset in ISP units (16 = one 12-bit sensor code). Plus crushes shadows, minus lifts them. 0 = default. |
| `isp_bw` | 0, 1 | 1 = black and white |
| `ccm_custom` | 0, 1 | 1 = use the custom matrix below instead of the vendor matrices |
| `ccm_rr` `ccm_rg` `ccm_rb` | about -4096…4096 | Custom matrix row R: output red from input R, G, B. 1024 = 1.0 |
| `ccm_gr` `ccm_gg` `ccm_gb` | same | Row G |
| `ccm_br` `ccm_bg` `ccm_bb` | same | Row B. The ISP forces each row to sum to 1.0 (1024), so the diagonal follows the other two. |
| `hsv_r_hue` … `hsv_m_hue` | about -128…128 | Hue shift of the red, yellow, green, cyan, blue and magenta bands (`r y g c b m`); 0 = none |
| `hsv_r_sat` … `hsv_m_sat` | 0–300 | Saturation of each band in %; 100 = vendor |
| `nr_luma` | -1, 0, 1–400 | Spatial luma noise reduction in % of vendor strength; 0 = off |
| `nr_chroma` | -1, 0, 1 | Colour noise reduction: vendor / off / on |
| `nr_despike` | -1, 0, 1 | Hot/stray pixel removal: vendor / off / on |
| `nr_falsecolor` | -1, 0, 1 | False-colour (moiré) suppression: vendor / off / on |
| `defog` | 0–100 | Haze removal strength; 0 = off |
| `wdr` | -1, 0, 1–255 | Wide dynamic range (local tone mapping): -1 vendor auto, 0 off, else strength. Ignored in HDR mode. |
| `nr3d_gain` | 0, 125–16000 | Temporal noise reduction motion gain; 0 = off, 1000 = vendor. Higher = stronger, more ghosting on motion. |

Values outside a range are clamped or behave unpredictably; if a look looks wrong, use the Reset page rows on the camera.
