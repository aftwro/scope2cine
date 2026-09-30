#ifndef CAMAPP_OSD_H
#define CAMAPP_OSD_H
#include <stdint.h>

#define OSD_WHITE 0xffffffff
#define OSD_RED    0xffff3030
#define OSD_YELLOW 0xffffd020

/* Open /dev/fb0 and keep it open (the SStar fb driver drops the colour key when
 * the last fd closes), enable colour-key transparency and clear. 0 on success. */
int osd_start(void);
void osd_stop(void);
/* Outline box in OSD coordinates (fb pixels); w == 0 removes the current box. */
void osd_box(int x, int y, int w, int h, uint32_t colour);
/* Loupe position map, bottom-left: frame outline with the magnified area (fractions 0..1).
 * on = 0 erases it. */
void osd_loupe_map(int on, double fx, double fy, double fw, double fh);
/* Analysis overlay (peaking / zebra / false colour): idx is mw x mh (max 480x270), each cell a
 * palette index, 0 = transparent; NULL clears all dots. */
void osd_cells(const uint8_t *idx, int mw, int mh, const uint32_t *palette, int n_palette);
/* Composition overlays, bit flags; drawn below the status bar, kept intact when the spot box moves. */
enum { OSD_OVERLAY_THIRDS = 1, OSD_OVERLAY_CROSSHAIR = 2 };
/* Frame (aspect) guide in overlay bits 2-4: index into osd_frame_names, 0 = none. */
#define OSD_FRAME_SHIFT 2
#define OSD_FRAME_MASK (7u << OSD_FRAME_SHIFT)
#define OSD_FRAMES 8
extern const char *const osd_frame_names[OSD_FRAMES];
void osd_overlay(unsigned flags);
/* Luminance histogram panel (bottom right). bins: 128 linear-light bins; NULL hides it. */
/* Luma waveform, bottom centre: counts is WF_W x WF_H (column x level, level 0 at the bottom);
 * NULL hides it. */
#define WF_W 256                       /* same width as the histogram: the pair stays centred */
#define WF_H 100
void osd_waveform(const uint16_t *counts);
/* Bottom-centre scopes: one alone sits dead centre, both sit side by side. Call when either toggles. */
void osd_scopes_layout(int hist_on, int wf_on);
/* RGB waveform: per-channel counts (same layout as osd_waveform), each drawn in its own colour and
 * added where they overlap. For a parade the caller puts each channel in its own third. */
void osd_waveform_rgb(const uint16_t *r, const uint16_t *g, const uint16_t *b, int parade);
/* Luma histogram of the displayed picture: 256 bins (0..255), linear axis; NULL hides it. */
void osd_histogram(const uint32_t *bins);
/* Menu panel: n rows of label/value, row sel highlighted. osd_menu_close() removes it. */
/* help (may be NULL) is word-wrapped under the rows, for the selected row's explanation.
 * more_above / more_below draw scroll arrows when the page has rows outside the visible window. */
void osd_menu(const char *title, const char *const *labels, const char *const *values, int n, int sel,
              const char *help, int more_above, int more_below, uint32_t accent);
void osd_menu_close(void);
void osd_show(int on);                 /* whole OSD layer on/off */
/* Frame border in colour (ARGB), 0 = off: white photo flash, red recording tally.
 * One-line notice under the status bar (NULL clears). */
void osd_border(uint32_t colour);
void osd_notice(const char *msg, uint32_t colour);
int osd_width(void);
int osd_height(void);
/* Redraw the top status bar (left, centre and right text, left and centre in their own colours)
 * and the two bottom corner blocks: bl left-aligned, br right-aligned (br in br_color), each up to
 * CORNER_LINES lines separated by '\n'. */
#define CORNER_LINES 3
#define OSD_DEGREE "\x7f"            /* degree sign in the OSD font (tools/psf2c.py) */
void osd_status(const char *left, uint32_t left_color, const char *centre, uint32_t centre_color,
                const char *right, const char *bl, const char *br, uint32_t br_color);

#endif
