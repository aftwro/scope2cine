/* On-screen display on the SStar framebuffer (see osd.h).
 * fb0 is 1600x900 ARGB8888, scaled to the HDMI timing by the display engine.
 * Pixels of colour KEY are transparent (colour key, as in the stock UI). */
#include "osd.h"
#include "font.h"

#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define FBIOSET_SHOW     0x40014663
#define FBIOSET_COLORKEY 0x40044667      /* {enable, r, g, b} */
#define KEY    0xff0a0a0a
#define SHADOW 0xff000000
#define BAR_Y  10
#define BAR_H  (FONT_H + 4)
#define CORNER_W 480                   /* bottom corner blocks: clear of the centre waveform/histogram */
#define CORNER_BOTTOM 16
#define CORNER_Y ((int)height - CORNER_BOTTOM - CORNER_LINES * FONT_H - 2)
#define MARGIN 16
#define BORDER 6                   /* photo flash / recording tally frame width */

static int fd = -1;
static unsigned overlay;                 /* OSD_OVERLAY_* flags currently shown */
#define GUIDE     0xb0ffffff             /* semi-transparent white guide line */
#define GUIDE_EDGE 0x60000000            /* faint dark edge so guides read on bright scenes */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;   /* drawing comes from several threads */
static uint32_t *fb, stride, width, height, fb_len;
static uint32_t *screen;                 /* the mapped framebuffer; fb points here or at the shadow */
static int notice_len;                   /* characters of the notice shown, 0 = none */
static uint32_t *shadow;                 /* off-screen copy for areas that are cleared and redrawn */

/* Flicker-free redraw of an area that is cleared and drawn again (status bar, corner blocks, menu):
 * draw into the shadow buffer, then copy only the rows that changed to the screen. Clearing the
 * visible framebuffer first let the display scan out the empty state (OSD flicker every second). */
static int shadow_begin(void)
{
    if (!shadow)
        shadow = malloc(fb_len);             /* pages are only touched where we draw */
    if (shadow)
        fb = shadow;
    return shadow != NULL;
}

static void shadow_commit(int x0, int y0, int w, int h)
{
    if (x0 < 0) w += x0, x0 = 0;
    if (y0 < 0) h += y0, y0 = 0;
    if (x0 + w > (int)width) w = width - x0;
    if (y0 + h > (int)height) h = height - y0;
    for (int y = y0; shadow && w > 0 && y < y0 + h; y++) {
        uint32_t *src = shadow + y * stride + x0, *dst = screen + y * stride + x0;
        if (memcmp(src, dst, w * 4))
            memcpy(dst, src, w * 4);
    }
}

static void shadow_end(void) { fb = screen; }

/* Clear an OSD element's area to transparent and mark the analysis cells under it for redraw
 * (otherwise cells the element covered stay missing: black holes after hiding a scope panel). */
static void cells_invalidate(int x0, int y0, int w, int h);
static void fill(int x0, int y0, int w, int h, uint32_t c);
static void clear_area(int x0, int y0, int w, int h)
{
    fill(x0, y0, w, h, KEY);
    cells_invalidate(x0, y0, w, h);
}

/* Clipped to the framebuffer on all four sides: outlines of boxes at the frame edge start at -1
 * (a spot at the top edge wrote before the buffer and crashed camapp, 2026-09-29). */
static void fill(int x0, int y0, int w, int h, uint32_t c)
{
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < (int)height; y++)
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < (int)width; x++)
            fb[y * stride + x] = c;
}

static int glyph_bit(int ch, int x, int y)
{
    if (ch < FONT_FIRST || ch > FONT_LAST)
        ch = '?';
    const uint8_t *g = font_glyphs[ch - FONT_FIRST];
    return g[y * FONT_ROW_BYTES + x / 8] & (0x80 >> (x % 8));
}

/* Draw text with a 1-pixel black outline so it stays readable on any image. */
static void text(int x0, int y0, const char *s, uint32_t colour)
{
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; s[i]; i++)
            for (int y = 0; y < FONT_H; y++)
                for (int x = 0; x < FONT_W; x++) {
                    if (!glyph_bit((unsigned char)s[i], x, y))
                        continue;
                    int px = x0 + i * FONT_W + x, py = y0 + y;
                    if (pass == 0)
                        fill(px - 1, py - 1, 3, 3, SHADOW);
                    else if (px < (int)width && py < (int)height)
                        fb[py * stride + px] = colour;
                }
}

int osd_start(void)
{
    struct fb_var_screeninfo v;
    struct fb_fix_screeninfo fx;
    fd = open("/dev/fb0", O_RDWR);
    if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &v) || ioctl(fd, FBIOGET_FSCREENINFO, &fx)) {
        printf("osd: cannot open /dev/fb0\n");
        return -1;
    }
    width = v.xres;
    height = v.yres;
    stride = fx.line_length / 4;
    fb_len = fx.smem_len;
    fb = screen = mmap(NULL, fb_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) {
        close(fd);
        fd = -1;
        return -1;
    }
    uint8_t show = 1, key[4] = { 1, 0x0a, 0x0a, 0x0a };
    ioctl(fd, FBIOSET_SHOW, &show);
    ioctl(fd, FBIOSET_COLORKEY, key);
    fill(0, 0, width, height, KEY);
    printf("osd: %ux%u ready\n", width, height);
    return 0;
}

/* Status bar and corner blocks sit ON TOP of the analysis layer (false colour, zebra, peaking):
 * their areas are rebuilt as cell colours + text in the shadow buffer, both every second and
 * whenever a cell under them changes, so the layer shows under the text without holes. */
static struct {
    char left[160], centre[160], right[160], bl[160], br[160];
    uint32_t left_color, centre_color, br_color;
} st;
#define BAR_Y0 (BAR_Y - 2 < BORDER ? BORDER : BAR_Y - 2)  /* never over the tally/flash frame */
static void cells_under(int x0, int y0, int w, int h);

static int in_text_area(int x, int y, int dx, int dy)
{
    if (y < BAR_Y + BAR_H && y + dy > BAR_Y0)
        return 1;
    return y + dy > CORNER_Y && y < (int)height - BORDER &&
           (x < BORDER + CORNER_W || x + dx > (int)width - BORDER - CORNER_W);
}

static void draw_overlay(int erase);

static void draw_status_locked(void)
{
    int shadowed = shadow_begin(), y0 = BAR_Y0;
    cells_under(BORDER, y0, width - 2 * BORDER, BAR_Y + BAR_H - y0);
    text(MARGIN, BAR_Y, st.left, st.left_color);
    text((width - strlen(st.centre) * FONT_W) / 2, BAR_Y, st.centre, st.centre_color);
    text(width - MARGIN - strlen(st.right) * FONT_W, BAR_Y, st.right, OSD_WHITE);
    /* Corner blocks: rebuild their whole area, then draw bottom-up so the last line sits lowest. */
    cells_under(BORDER, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
    cells_under(width - BORDER - CORNER_W, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
    if (shadowed)
        draw_overlay(0);                             /* guide lines under the text (only these areas are copied) */
    const char *bl = st.bl, *br = st.br;
    uint32_t br_color = st.br_color;
    for (int side = 0; side < 2; side++) {
        char buf[160], *lines[CORNER_LINES], *save;
        int n = 0;
        snprintf(buf, sizeof(buf), "%s", side ? br : bl);
        for (char *p = strtok_r(buf, "\n", &save); p && n < CORNER_LINES; p = strtok_r(NULL, "\n", &save))
            lines[n++] = p;
        for (int i = 0; i < n; i++) {
            int y = height - CORNER_BOTTOM - (n - i) * FONT_H;
            int x = side ? width - MARGIN - (int)strlen(lines[i]) * FONT_W : MARGIN;
            text(x, y, lines[i], side ? br_color : OSD_WHITE);
        }
    }
    if (shadowed) {
        shadow_end();
        shadow_commit(BORDER, y0, width - 2 * BORDER, BAR_Y + BAR_H - y0);
        shadow_commit(BORDER, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
        shadow_commit(width - BORDER - CORNER_W, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
    }
}

/* Whole OSD layer on/off (the display keeps showing the video). */
void osd_show(int on)
{
    uint8_t show = on ? 1 : 0;
    if (fd >= 0)
        ioctl(fd, FBIOSET_SHOW, &show);
}

void osd_status(const char *left, uint32_t left_color, const char *centre, uint32_t centre_color,
                const char *right, const char *bl, const char *br, uint32_t br_color)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    snprintf(st.left, sizeof(st.left), "%s", left);
    snprintf(st.centre, sizeof(st.centre), "%s", centre);
    snprintf(st.right, sizeof(st.right), "%s", right);
    snprintf(st.bl, sizeof(st.bl), "%s", bl);
    snprintf(st.br, sizeof(st.br), "%s", br);
    st.left_color = left_color, st.centre_color = centre_color, st.br_color = br_color;
    draw_status_locked();
    pthread_mutex_unlock(&lock);
}

/* Menu panel geometry (fb pixels) and colours. */
#define MENU_X 40
#define MENU_Y 70
#define MENU_W 470
#define MENU_ROW 40
#define MENU_PAD 16
#define MENU_BG     0xc8101418     /* dark, lets the picture show through */
#define MENU_SEL    0xffffb020     /* amber selection bar */
#define MENU_INK    0xffe8ecee
#define MENU_DIM    0xff9aa6ab
#define MENU_SEL_INK 0xff15181a
static int menu_h;                 /* current panel height, 0 = closed */

static int cell_blocked(int x, int y, int d);

/* Fill for guide lines. Guides run under the whole OSD: opaque panels (menu, scopes, loupe map,
 * notice text, spot box outline) cover them, and in the status bar and corner blocks they are
 * composited under the text by draw_status_locked (drawing into the shadow buffer) - drawn straight
 * to the screen they skip those text areas, else they would cut the letters. (Before 2026-09-30 they
 * skipped the whole status bar and corner areas, which cut frame lines, user.) */
static void guide_fill(int x0, int y0, int w, int h, uint32_t c)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
            if (!cell_blocked(x, y, 1) && (fb != screen || !in_text_area(x, y, 1, 1)))
                fill(x, y, 1, 1, c);
}

static void hline(int x0, int x1, int y, uint32_t c, uint32_t edge)
{
    guide_fill(x0, y - 1, x1 - x0, 1, edge);
    guide_fill(x0, y, x1 - x0, 2, c);
    guide_fill(x0, y + 2, x1 - x0, 1, edge);
}

static void vline(int x, int y0, int y1, uint32_t c, uint32_t edge)
{
    guide_fill(x - 1, y0, 1, y1 - y0, edge);
    guide_fill(x, y0, 2, y1 - y0, c);
    guide_fill(x + 2, y0, 1, y1 - y0, edge);
}

/* Draw (or erase, with KEY) the current overlay. Caller holds the lock. */
/* Frame guides: aspect ratio x 100 (the picture itself is 16:9). */
const char *const osd_frame_names[OSD_FRAMES] = { "Off", "2.39", "2.00", "1.85", "1.66", "4:3", "1:1", "9:16" };
static const int frame_x100[OSD_FRAMES] = { 0, 239, 200, 185, 166, 133, 100, 56 };

static void draw_overlay(int erase)
{
    uint32_t c = erase ? KEY : GUIDE, e = erase ? KEY : GUIDE_EDGE;
    int W = width, H = height;
    int f = frame_x100[(overlay & OSD_FRAME_MASK) >> OSD_FRAME_SHIFT];
    if (f && f * 9 > 1600) {                        /* wider than 16:9: lines top and bottom */
        int fh = W * 100 / f, y0 = (H - fh) / 2;
        hline(0, W, y0, c, e);
        hline(0, W, y0 + fh - 1, c, e);
    } else if (f && f * 9 < 1600) {                 /* narrower: lines left and right */
        int fw = H * f / 100, x0 = (W - fw) / 2;
        vline(x0, 0, H, c, e);
        vline(x0 + fw - 1, 0, H, c, e);
    }
    if (overlay & OSD_OVERLAY_THIRDS) {
        vline(W / 3, 0, H, c, e);
        vline(2 * W / 3, 0, H, c, e);
        hline(0, W, H / 3, c, e);
        hline(0, W, 2 * H / 3, c, e);
    }
    if (overlay & OSD_OVERLAY_CROSSHAIR) {
        int cx = W / 2, cy = H / 2, arm = 28, gap = 8;
        hline(cx - gap - arm, cx - gap, cy, c, e);
        hline(cx + gap, cx + gap + arm, cy, c, e);
        vline(cx, cy - gap - arm, cy - gap, c, e);
        vline(cx, cy + gap, cy + gap + arm, c, e);
    }
}

void osd_overlay(unsigned flags)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    draw_overlay(1);
    overlay = flags;
    draw_overlay(0);
    pthread_mutex_unlock(&lock);
}

/* Histogram panel: x axis in stops (8 stops, linear bins mapped by log2), height by sqrt(count)
 * so small populations stay visible. A red edge marks clipped highlights. */
#define HIST_W 256
#define HIST_H 100                   /* same height as the waveform, side by side at the bottom centre */
#define PANEL_GAP 8
static int scopes_both;              /* histogram and waveform both on: side by side, else one centred */
#define HIST_X (scopes_both ? (int)width / 2 + PANEL_GAP / 2 : (int)width / 2 - HIST_W / 2)
#define HIST_Y ((int)height - HIST_H - 24)
#define HIST_BG   0x70000000
#define HIST_BAR  0xd0f0f0f0
#define HIST_TICK 0x28ffffff

static int hist_shown, map_shown, wf_shown;   /* peaking keeps clear of these panels */
static int box_x, box_y, box_w, box_h;   /* current spot box */

void osd_histogram(const uint32_t *bins)
{
    if (fd < 0)
        return;
    int x0 = HIST_X, y0 = HIST_Y;
    pthread_mutex_lock(&lock);
    hist_shown = bins != NULL;
    if (!bins) {
        clear_area(x0, y0, HIST_W, HIST_H);
        draw_overlay(0);                    /* guides run through where the panel was */
        pthread_mutex_unlock(&lock);
        return;
    }
    /* One column per luma level (HIST_W = 256); square-root height so small counts stay visible.
     * Render off-screen, then copy row by row (drawing into the visible framebuffer flickered). */
    uint32_t max = 1, total = 0;
    for (int x = 0; x < HIST_W; x++) {
        max = bins[x] > max ? bins[x] : max;
        total += bins[x];
    }
    static uint32_t img[HIST_H][HIST_W];
    for (int y = 0; y < HIST_H; y++)
        for (int x = 0; x < HIST_W; x++)
            img[y][x] = HIST_BG;
    for (int q = 1; q < 4; q++)                                  /* ticks at 25 / 50 / 75 % */
        for (int y = 0; y < HIST_H; y++)
            img[y][q * HIST_W / 4] = HIST_TICK;
    for (int x = 0; x < HIST_W; x++) {
        int h = (int)(sqrt((double)bins[x] / max) * (HIST_H - 4));
        for (int y = HIST_H - 2 - h; y < HIST_H - 2; y++)
            img[y][x] = HIST_BAR;
    }
    if (bins[255] > total / 1000)                               /* > 0.1 % clipped: red marker */
        for (int y = 0; y < HIST_H; y++)
            for (int x = HIST_W - 4; x < HIST_W; x++)
                img[y][x] = OSD_RED;
    for (int y = 0; y < HIST_H && y0 + y < (int)height; y++)
        memcpy(&fb[(y0 + y) * stride + x0], img[y], sizeof(img[y]));
    pthread_mutex_unlock(&lock);
}

#define WF_X (scopes_both ? (int)width / 2 - PANEL_GAP / 2 - WF_W : (int)width / 2 - WF_W / 2)
#define WF_Y ((int)height - WF_H - 24)
/* Waveform panel: 25/50/75 % lines (and parade separators), then copy to the framebuffer. */
static void wf_finish(uint32_t img[WF_H][WF_W], int parade)
{
    for (int q = 1; q < 4; q++)
        for (int x = 0; x < WF_W; x += 2)
            if (img[WF_H - 1 - q * (WF_H - 1) / 4][x] == HIST_BG)
                img[WF_H - 1 - q * (WF_H - 1) / 4][x] = HIST_TICK;
    for (int t = 1; parade && t < 3; t++)
        for (int y = 0; y < WF_H; y++)
            img[y][t * WF_W / 3] = HIST_TICK;
    for (int y = 0; y < WF_H; y++)
        memcpy(&fb[(WF_Y + y) * stride + WF_X], img[y], sizeof(img[0]));
}

void osd_waveform(const uint16_t *counts)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    wf_shown = counts != NULL;
    if (!counts) {
        clear_area(WF_X - 2, WF_Y - 2, WF_W + 4, WF_H + 4);
        draw_overlay(0);
        pthread_mutex_unlock(&lock);
        return;
    }
    /* Phosphor look: brightness grows with the number of pixels at that column/level. */
    static uint32_t img[WF_H][WF_W];
    for (int y = 0; y < WF_H; y++)
        for (int x = 0; x < WF_W; x++) {
            uint32_t c = counts[(WF_H - 1 - y) * WF_W + x];
            if (c) {
                uint32_t g = c > 12 ? 255 : 80 + c * 14;
                img[y][x] = 0xf0000000 | (g * 7 / 10) << 16 | g << 8 | (g * 7 / 10);
            } else {
                img[y][x] = HIST_BG;
            }
        }
    wf_finish(img, 0);
    pthread_mutex_unlock(&lock);
}

void osd_scopes_layout(int hist_on, int wf_on)
{
    if ((hist_on && wf_on) == scopes_both)
        return;
    if (fd < 0) {                                       /* before the OSD opens: just remember it */
        scopes_both = hist_on && wf_on;
        return;
    }
    pthread_mutex_lock(&lock);
    clear_area(HIST_X, HIST_Y, HIST_W, HIST_H);          /* clear both at the old positions */
    clear_area(WF_X - 2, WF_Y - 2, WF_W + 4, WF_H + 4);
    scopes_both = hist_on && wf_on;
    draw_overlay(0);
    pthread_mutex_unlock(&lock);
}

void osd_waveform_rgb(const uint16_t *r, const uint16_t *g, const uint16_t *b, int parade)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    wf_shown = 1;
    static uint32_t img[WF_H][WF_W];
    for (int y = 0; y < WF_H; y++)
        for (int x = 0; x < WF_W; x++) {
            int k = (WF_H - 1 - y) * WF_W + x;
            uint32_t cr = r[k], cg = g[k], cb = b[k];
            if (cr | cg | cb) {
                uint32_t vr = !cr ? 0 : cr > 12 ? 255 : 80 + cr * 14;
                uint32_t vg = !cg ? 0 : cg > 12 ? 255 : 80 + cg * 14;
                uint32_t vb = !cb ? 0 : cb > 12 ? 255 : 80 + cb * 14;
                img[y][x] = 0xf0000000 | vr << 16 | vg << 8 | vb;
            } else {
                img[y][x] = HIST_BG;
            }
        }
    wf_finish(img, parade);
    pthread_mutex_unlock(&lock);
}

/* Plain text without outline, for text on solid panels. Caller holds the lock. */
static void text_plain(int x0, int y0, const char *s, uint32_t colour)
{
    for (int i = 0; s[i]; i++)
        for (int y = 0; y < FONT_H; y++)
            for (int x = 0; x < FONT_W; x++)
                if (glyph_bit((unsigned char)s[i], x, y))
                    fill(x0 + i * FONT_W + x, y0 + y, 1, 1, colour);
}



#define HELP_X (MENU_X + MENU_W + 12)            /* help panel right of the menu */
#define HELP_W 500
#define HELP_COLS ((HELP_W - 2 * MENU_PAD) / FONT_W)
#define HELP_LINES 16
static int help_h;                                 /* height of the help panel on screen, 0 = none */

/* Word-wrap text into lines of at most HELP_COLS characters. Returns the line count. */
static int wrap(const char *text, char lines[HELP_LINES][HELP_COLS + 1])
{
    int n = 0;
    while (*text && n < HELP_LINES) {
        int len = strlen(text), cut = len;
        if (len > HELP_COLS) {
            cut = HELP_COLS;
            while (cut > 0 && text[cut] != ' ')
                cut--;
            if (cut == 0)
                cut = HELP_COLS;
        }
        memcpy(lines[n], text, cut);
        lines[n++][cut] = 0;
        text += cut;
        while (*text == ' ')
            text++;
    }
    return n;
}

/* Small filled triangle, 13 px wide, 7 px tall, pointing up or down, centred on cx. */
static void arrow(int cx, int y, int up, uint32_t colour)
{
    for (int r = 0; r < 7; r++) {
        int half = up ? r : 6 - r;
        fill(cx - half, y + r, 2 * half + 1, 1, colour);
    }
}

void osd_menu(const char *title, const char *const *labels, const char *const *values, int n, int sel,
              const char *help, int more_above, int more_below, uint32_t accent)
{
    if (fd < 0)
        return;
    char lines[HELP_LINES][HELP_COLS + 1];
    int nl = help ? wrap(help, lines) : 0;
    pthread_mutex_lock(&lock);
    int old_h = menu_h, old_help = help_h, shadowed = shadow_begin();
    int h = MENU_PAD + FONT_H + 10 + n * MENU_ROW + MENU_PAD + (more_below ? 6 : 0);
    if (menu_h > h)
        clear_area(MENU_X, MENU_Y, MENU_W, menu_h);
    menu_h = h;
    fill(MENU_X, MENU_Y, MENU_W, h, MENU_BG);
    fill(MENU_X, MENU_Y, 6, h, accent);              /* section colour stripe */
    text_plain(MENU_X + MENU_PAD, MENU_Y + MENU_PAD, title, accent);
    fill(MENU_X + MENU_PAD, MENU_Y + MENU_PAD + FONT_H + 4, MENU_W - 2 * MENU_PAD, 1, MENU_DIM);
    for (int i = 0; i < n; i++) {
        int y = MENU_Y + MENU_PAD + FONT_H + 10 + i * MENU_ROW;
        int ty = y + (MENU_ROW - FONT_H) / 2;
        char val[48];
        snprintf(val, sizeof(val), "< %s >", values[i]);
        int vx = MENU_X + MENU_W - MENU_PAD - (int)strlen(val) * FONT_W;
        if (i == sel) {
            fill(MENU_X + 6, y + 2, MENU_W - 12, MENU_ROW - 4, MENU_SEL);
            text_plain(MENU_X + MENU_PAD, ty, labels[i], MENU_SEL_INK);
            text_plain(vx, ty, val, MENU_SEL_INK);
        } else {
            text_plain(MENU_X + MENU_PAD, ty, labels[i], MENU_INK);
            text_plain(vx + FONT_W * 2, ty, values[i], MENU_DIM);      /* arrows only on the selected row */
        }
    }
    if (more_above)                                  /* right end of the title line */
        arrow(MENU_X + MENU_W - MENU_PAD - 7, MENU_Y + MENU_PAD + 8, 1, MENU_SEL);
    if (more_below)                                  /* just under the last row */
        arrow(MENU_X + MENU_W - MENU_PAD - 7, MENU_Y + MENU_PAD + FONT_H + 10 + n * MENU_ROW + 1, 0, MENU_SEL);
    /* Help: its own panel right of the menu, as tall as the text needs. */
    int hh = nl ? 2 * MENU_PAD + nl * (FONT_H + 4) : 0;
    if (help_h > hh)
        clear_area(HELP_X, MENU_Y, HELP_W, help_h);
    help_h = hh;
    if (nl) {
        fill(HELP_X, MENU_Y, HELP_W, hh, MENU_BG);
        fill(HELP_X, MENU_Y, 3, hh, accent);
        for (int i = 0; i < nl; i++)
            text_plain(HELP_X + MENU_PAD, MENU_Y + MENU_PAD + i * (FONT_H + 4), lines[i], MENU_INK);
    }
    if (shadowed) {
        shadow_end();
        shadow_commit(MENU_X, MENU_Y, MENU_W, h > old_h ? h : old_h);
        shadow_commit(HELP_X, MENU_Y, HELP_W, hh > old_help ? hh : old_help);
    }
    pthread_mutex_unlock(&lock);
}

void osd_menu_close(void)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    if (menu_h)
        clear_area(MENU_X, MENU_Y, MENU_W, menu_h);
    if (help_h)
        clear_area(HELP_X, MENU_Y, HELP_W, help_h);
    help_h = 0;
    menu_h = 0;
    draw_overlay(0);               /* the panel covered part of the guides */
    pthread_mutex_unlock(&lock);
}

#define NOTICE_Y (BAR_Y + BAR_H + 6)
#define NOTICE_W 640


void osd_border(uint32_t colour)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    uint32_t c = colour ? colour : KEY;
    fill(0, 0, width, BORDER, c);
    fill(0, height - BORDER, width, BORDER, c);
    fill(0, 0, BORDER, height, c);
    fill(width - BORDER, 0, BORDER, height, c);
    if (!colour)
        draw_overlay(0);                 /* the border may have covered guide ends */
    pthread_mutex_unlock(&lock);
}

void osd_notice(const char *msg, uint32_t colour)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    clear_area(MARGIN - 2, NOTICE_Y - 2, NOTICE_W, FONT_H + 4);
    notice_len = msg ? (int)strlen(msg) : 0;
    if (msg)
        text(MARGIN, NOTICE_Y, msg, colour);
    draw_overlay(0);
    pthread_mutex_unlock(&lock);
}

int osd_width(void) { return width; }
int osd_height(void) { return height; }

void osd_box(int x, int y, int w, int h, uint32_t colour)
{
    static int bx, by, bw, bh;          /* previous box, erased before drawing the new one */
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    if (bw) {
        clear_area(bx - 1, by - 1, bw + 2, 4);
        clear_area(bx - 1, by + bh - 3, bw + 2, 4);
        clear_area(bx - 1, by - 1, 4, bh + 2);
        clear_area(bx + bw - 3, by - 1, 4, bh + 2);
    }
    bx = x; by = y; bw = w; bh = h;
    box_x = x; box_y = y; box_w = w; box_h = h;
    if (!w) {
        draw_overlay(0);                 /* erasing the box may have cut the guides */
        pthread_mutex_unlock(&lock);
        return;
    }
    fill(x - 1, y - 1, w + 2, 4, SHADOW);            /* black outline under a 2 px coloured line */
    fill(x - 1, y + h - 3, w + 2, 4, SHADOW);
    fill(x - 1, y - 1, 4, h + 2, SHADOW);
    fill(x + w - 3, y - 1, 4, h + 2, SHADOW);
    fill(x, y, w, 2, colour);
    fill(x, y + h - 2, w, 2, colour);
    fill(x, y, 2, h, colour);
    fill(x + w - 2, y, 2, h, colour);
    draw_overlay(0);
    pthread_mutex_unlock(&lock);
}

#define MAP_W 192
#define MAP_H 108
void osd_loupe_map(int on, double fx, double fy, double fw, double fh)
{
    if (fd < 0)
        return;
    int x0 = 24, y0 = CORNER_Y - MAP_H - 8;         /* above the bottom-left settings block */
    pthread_mutex_lock(&lock);
    map_shown = on;
    clear_area(x0 - 2, y0 - 2, MAP_W + 4, MAP_H + 4);
    if (on) {
        fill(x0 - 2, y0 - 2, MAP_W + 4, MAP_H + 4, SHADOW);
        fill(x0, y0, MAP_W, MAP_H, HIST_BG);
        int x = x0 + (int)(fx * MAP_W), y = y0 + (int)(fy * MAP_H);
        int w = (int)(fw * MAP_W) < 4 ? 4 : (int)(fw * MAP_W), h = (int)(fh * MAP_H) < 3 ? 3 : (int)(fh * MAP_H);
        fill(x, y, w, h, OSD_YELLOW);
    }
    draw_overlay(0);
    pthread_mutex_unlock(&lock);
}

/* Analysis overlay layer (peaking, zebra, false colour): one dot per analysis cell, coloured from
 * a small palette (index 0 = transparent). Only cells that change are redrawn; dots keep clear of
 * the status bar, notices, menu, histogram, loupe map and spot box. */
#define PEAK_MAX_W 480
#define PEAK_MAX_H 270
static uint8_t peak_prev[PEAK_MAX_H][PEAK_MAX_W];

/* Areas the analysis cells stay out of: notice line, menu, scope panels, loupe map, spot box.
 * The status bar and corner blocks are drawn over the cells instead (draw_status_locked). */
static int cell_blocked(int x, int y, int d)
{
    if (notice_len && x < MARGIN + notice_len * FONT_W + 6 && y + d > NOTICE_Y - 4 && y < NOTICE_Y + FONT_H + 4)
        return 1;                                            /* notice text, only while one is shown */
    if (menu_h && x + d > MENU_X - 4 && x < MENU_X + MENU_W + 4 && y + d > MENU_Y - 4 && y < MENU_Y + menu_h + 4)
        return 1;
    if (hist_shown && x + d > HIST_X - 4 && x < HIST_X + HIST_W + 4 && y + d > HIST_Y - 4)
        return 1;
    if (map_shown && x < 24 + MAP_W + 4 && y + d > CORNER_Y - MAP_H - 12)
        return 1;
    if (wf_shown && x + d > WF_X - 4 && x < WF_X + WF_W + 4 && y + d > WF_Y - 4)
        return 1;
    if (box_w && x + d > box_x - 4 && x < box_x + box_w + 4 && y + d > box_y - 4 && y < box_y + box_h + 4 &&
        !(x > box_x + 4 && x + d < box_x + box_w - 4 && y > box_y + 4 && y + d < box_y + box_h - 4))
        return 1;                                            /* only the box outline, not its inside */
    return 0;
}


static int cells_mw, cells_mh;                                /* grid of the last analysis map */

static void cells_invalidate(int x0, int y0, int w, int h)
{
    if (!cells_mw || !cells_mh)
        return;
    for (int j = y0 < 0 ? 0 : y0 * cells_mh / (int)height; j < cells_mh && j * (int)height / cells_mh < y0 + h; j++)
        for (int i = x0 < 0 ? 0 : x0 * cells_mw / (int)width; i < cells_mw && i * (int)width / cells_mw < x0 + w; i++)
            if (peak_prev[j][i])
                peak_prev[j][i] = 0xff;                      /* was drawn, now unknown */
}
static uint32_t cells_palette[16];

/* Fill a rectangle with the analysis layer as it is now (transparent where no cell is set). */
static void cells_under(int x0, int y0, int w, int h)
{
    fill(x0, y0, w, h, KEY);
    if (!cells_mw || !cells_mh)
        return;
    int j0 = y0 * cells_mh / (int)height, j1 = ((y0 + h) * cells_mh + (int)height - 1) / (int)height;
    int i0 = x0 * cells_mw / (int)width, i1 = ((x0 + w) * cells_mw + (int)width - 1) / (int)width;
    for (int j = j0; j < j1 && j < cells_mh; j++)
        for (int i = i0; i < i1 && i < cells_mw; i++) {
            int v = peak_prev[j][i];
            if (v == 0 || v >= 16)
                continue;
            int cy = j * (int)height / cells_mh, cdy = (j + 1) * (int)height / cells_mh - cy;
            int cx = i * (int)width / cells_mw, cdx = (i + 1) * (int)width / cells_mw - cx;
            int ax = cx > x0 ? cx : x0, ay = cy > y0 ? cy : y0;
            int bx = cx + cdx < x0 + w ? cx + cdx : x0 + w, by = cy + cdy < y0 + h ? cy + cdy : y0 + h;
            if (bx > ax && by > ay)
                fill(ax, ay, bx - ax, by - ay, cells_palette[v]);
        }
}

void osd_cells(const uint8_t *idx, int mw, int mh, const uint32_t *palette, int n_palette)
{
    static uint32_t prev_palette[16];
    if (fd < 0 || mw > PEAK_MAX_W || mh > PEAK_MAX_H || n_palette > 16)
        return;
    int erased = 0, redo_status = 0, recolour = idx && memcmp(prev_palette, palette, n_palette * sizeof(*palette)) != 0;
    pthread_mutex_lock(&lock);
    cells_mw = mw, cells_mh = mh;
    if (idx)
        memcpy(cells_palette, palette, n_palette * sizeof(*palette));
    for (int j = 0; j < mh; j++) {
        int y = j * (int)height / mh, dy = (j + 1) * (int)height / mh - y;   /* cells tile without gaps */
        for (int i = 0; i < mw; i++) {
            int v = idx ? idx[j * mw + i] : 0;
            if (v >= n_palette)
                v = 0;
            if (v == peak_prev[j][i] && !(v && recolour))
                continue;
            int x = i * (int)width / mw, dx = (i + 1) * (int)width / mw - x;
            if (in_text_area(x, y, dx, dy)) {          /* under the status text: composited below */
                for (int py = y; py < y + dy; py++)     /* ... the part outside the text areas directly */
                    for (int px = x; px < x + dx; px++)
                        if (!in_text_area(px, py, 1, 1) && !cell_blocked(px, py, 1))
                            fill(px, py, 1, 1, v ? palette[v] : KEY);
                peak_prev[j][i] = v;
                redo_status = 1;
            } else if (!cell_blocked(x, y, dx > dy ? dx : dy)) {
                fill(x, y, dx, dy, v ? palette[v] : KEY);
                erased |= !v;
                peak_prev[j][i] = v;
            } else if (!v) {                           /* erase near a panel: every pixel not under it */
                for (int py = y; py < y + dy; py++)     /* (a panel's 4 px margin is not covered by it) */
                    for (int px = x; px < x + dx; px++)
                        if (!cell_blocked(px + 4, py + 4, 0) || !cell_blocked(px - 4, py - 4, 0))
                            if (!in_text_area(px, py, 1, 1))
                                fill(px, py, 1, 1, KEY);
                erased = 1;
                peak_prev[j][i] = 0;                   /* under the panel it is gone once the panel clears */
            } else {
                peak_prev[j][i] = 0xff;             /* not drawn: unknown, redraw once it is free */
            }
        }
    }
    if (idx)
        memcpy(prev_palette, palette, n_palette * sizeof(*palette));
    if (erased)
        draw_overlay(0);                                    /* erased dots may have cut the guides */
    static struct timespec last_redo;                   /* cells under the status text: at most 4x a second */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (redo_status && (now.tv_sec - last_redo.tv_sec) * 1000 + (now.tv_nsec - last_redo.tv_nsec) / 1000000 >= 250) {
        draw_status_locked();
        last_redo = now;
    }
    pthread_mutex_unlock(&lock);
}

void osd_stop(void)
{
    if (fd < 0)
        return;
    fill(0, 0, width, height, KEY);
    munmap(fb, fb_len);
    close(fd);
    fd = -1;
}
