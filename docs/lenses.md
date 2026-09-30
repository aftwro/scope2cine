# Glass for the IMX307

Lens guide for the camera build: what fits the 1/2.8-inch IMX307 sensor, how it looks, and how to mount it.
Written 2026-09-24.

## Sensor size

| | |
|---|---|
| Recorded area (1920 × 1080 at 2.9 µm) | 5.57 × 3.13 mm |
| Diagonal | 6.39 mm |
| Crop factor vs full frame | ×6.8 |
| "Normal" focal length (≈ diagonal) | ~6.5 mm |

The sensor is almost exactly the size of a **Super 8** frame, so lenses made for 8 mm and 16 mm cine cameras are its natural partners.

| Format | Size (mm) | Diagonal (mm) | Covers the sensor? |
|---|---|---|---|
| 1-inch video | 12.8 × 9.6 | 16.0 | yes |
| Super 16 | 12.52 × 7.41 | 14.5 | yes |
| 16 mm | 10.26 × 7.49 | 12.7 | yes |
| 2/3-inch video | 8.8 × 6.6 | 11.0 | yes |
| Super 8 | 5.79 × 4.01 | 7.0 | yes |
| **IMX307 1080p** | **5.57 × 3.13** | **6.4** | — |
| Standard 8 | 4.8 × 3.5 | 5.9 | slightly smaller: corners may darken |

**Where the character lives:** vintage signature (swirl, soft edges, vignetting, glow) sits near the edge of a lens's image circle. A 16 mm lens only uses its central half on this sensor, so it looks cleaner than on film. For the most period look, pick lenses made for the smallest formats (8 mm / Super 8, small CCTV formats).

## Focal lengths

Multiply by ~6.8 for the full-frame equivalent. The classic 16 mm cine focal lengths all act long here.

| Focal length | Horizontal view | Full-frame equiv. | Feels like |
|---|---|---|---|
| 3.5 mm | 77° | 24 mm | wide establishing |
| 4 mm | 70° | 27 mm | wide |
| **6 mm** | **50°** | **41 mm** | **normal** |
| 8 mm | 38° | 54 mm | slightly long normal |
| 10 mm | 31° | 68 mm | portrait-ish |
| 12.5 mm | 25° | 85 mm | classic portrait |
| 16 mm | 20° | 108 mm | short tele |
| 25 mm | 13° | 169 mm | tele |
| 50 mm | 6.4° | 339 mm | long tele, needs support |

Depth of field: multiply the aperture by ~6.8 for full-frame-equivalent depth of field (f/1.4 ≈ f/9.5 FF, f/0.95 ≈ f/6.4 FF). Fast lenses still matter for low light and close subjects.

## Mounts

| Mount | Thread | Flange | Notes |
|---|---|---|---|
| C-mount | 1"-32 UN | 17.526 mm | 16 mm cine, CCTV, machine vision, microscope ports |
| CS-mount | 1"-32 UN | 12.526 mm | C lens fits via 5 mm ring; CS lens can't go on C |
| D-mount | 5/8"-32 | 12.29 mm | Standard 8 cine lenses; shortest flange |
| M12 (S-mount) | M12 × 0.5 | not fixed | board lenses; best for ultra-wide / fisheye |

The camera currently has an M12 holder. We build the replacement mount, so we pick the flange:

- **Build it short: ≤ 12.29 mm (D-mount).** Then D lenses fit directly, CS lenses need a ~0.24 mm shim, and C lenses use a D→C ring (~5.24 mm) or CS→C 5 mm ring.
- **Calibrate infinity:** glass in front of the sensor (cover glass, IR-cut filter) shifts focus back by about a third of its thickness. Use a shim set (0.05–0.5 mm) and set the flange by focusing at infinity.

## IR-cut filter

The IMX307 is a "starlight" sensor that sees into near-infrared. Cine lenses have no IR-cut filter, so without one, colours drift magenta, foliage glows and blacks lift. Put a **650 nm IR-cut filter** in the new mount (removable if you want IR effects on purpose).

To check: whether the board already has IR-cut glass over the sensor (a tinted window visible with the M12 lens removed). The unpopulated IR-cut driver (GPIO 10/11) hints the filter lived in the lens holder.

## Lens families

- **Kern-Paillard Switar** (C, 16 mm / Bolex): warm, gentle contrast. Prefer non-"RX" versions: RX lenses are computed for the Bolex reflex prism and are softer wide open elsewhere. 10 mm f/1.6, 16 mm f/1.8, 25 mm f/1.4, 75 mm f/1.9.
- **Angénieux** (C, 16 mm): classic French look, highlight bloom wide open. 10 mm f/1.8, 15 mm f/1.3, 25 mm f/0.95 (legendary, expensive), 12–120 zoom.
- **Kodak Cine Ektar** (C, 16 mm): crisp centre, characterful falloff, often cheap. 15 mm f/2.5, 25 mm f/1.4.
- **Schneider / Som Berthiot / Cooke** (C, 16 mm): Cinegon/Xenoplan clinical; Cinor soft and dreamy; Cooke Kinetal premium.
- **D-mount primes** (Standard 8): sensor uses nearly the whole image circle, so the most period character per euro, plus the wide end. Watch corners. E.g. Switar 5.5 mm f/1.8, 12.5 mm f/1.3; Wollensak, Elgeet, Ektar 8 mm lines.
- **CCTV / machine vision** (C/CS, 1/2"–1"): Fujinon, Computar, Tamron, Kowa, Pentax/Cosmicar. Sharp, cheap, f/0.95–1.4, real wide/normal at 4–8 mm. Vintage Cosmicar/Pentax TV lenses give an 80s video look. Budget "CCTV 25 mm f/1.4 / 35 mm f/1.7" mirrorless lenses lose their swirl here (edge effect).
- **Microscope ports** (C): the board started as a microscope camera; C-mount fits trinocular heads via 0.35×/0.5× relays.
- **M12 board lenses:** ultra-wide and fisheye (1.8–3 mm); pick ≥ 1/2.7" rated, with IR-cut option.

## Starting kit

1. Fast 6–8 mm C/CS CCTV lens (Computar/Fujinon, f/1.2–1.4): normal view, proves the mount.
2. D-mount 12.5 mm f/1.3 or 5.5 mm f/1.8: the period look.
3. 10 mm Switar or Angénieux: 68 mm-equivalent portraits, 16 mm rendering.
4. 650 nm IR-cut filter + C-mount shim set.

## Shooting notes for this camera

- **Manual iris lenses:** AUTO + SPOT metering and set exposure with the iris ring while watching the OSD EV meter; or manual shutter 1/48–1/50 and expose with the iris.
- **Focus:** deep depth of field, but fast lenses wide open are critical. Focus peaking is on the roadmap.
- **Heavy glass** (12–120 zoom etc.) needs a lens support.
- **Dust:** the sensor sits right behind the mount; cap it whenever the lens is off.
