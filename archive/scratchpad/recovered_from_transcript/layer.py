p='osd.c'; s=open(p).read()
def rep(a,b):
    global s
    assert a in s, a; s=s.replace(a,b,1)
rep('''void osd_status(const char *left, uint32_t left_color, const char *centre, uint32_t centre_color,
                const char *right, const char *bl, const char *br, uint32_t br_color)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    int shadowed = shadow_begin();
    int y0 = BAR_Y - 2 < BORDER ? BORDER : BAR_Y - 2;  /* never over the tally/flash frame */
    fill(BORDER, y0, width - 2 * BORDER, BAR_Y + BAR_H - y0, KEY);
    text(MARGIN, BAR_Y, left, left_color);
    text((width - strlen(centre) * FONT_W) / 2, BAR_Y, centre, centre_color);
    text(width - MARGIN - strlen(right) * FONT_W, BAR_Y, right, OSD_WHITE);
    /* Corner blocks: clear their whole area, then draw bottom-up so the last line sits lowest. */
    fill(BORDER, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y, KEY);
    fill(width - BORDER - CORNER_W, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y, KEY);
    for (int side = 0; side < 2; side++) {''',
'''/* Status bar and corner blocks sit ON TOP of the analysis layer (false colour, zebra, peaking):
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
    const char *bl = st.bl, *br = st.br;
    uint32_t br_color = st.br_color;
    for (int side = 0; side < 2; side++) {''')
rep('''            text(x, y, lines[i], side ? br_color : OSD_WHITE);
        }
    }
    if (shadowed) {
        shadow_end();
        shadow_commit(BORDER, y0, width - 2 * BORDER, BAR_Y + BAR_H - y0);
        shadow_commit(BORDER, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
        shadow_commit(width - BORDER - CORNER_W, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
    }
    pthread_mutex_unlock(&lock);
}''', '''            text(x, y, lines[i], side ? br_color : OSD_WHITE);
        }
    }
    if (shadowed) {
        shadow_end();
        shadow_commit(BORDER, y0, width - 2 * BORDER, BAR_Y + BAR_H - y0);
        shadow_commit(BORDER, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
        shadow_commit(width - BORDER - CORNER_W, CORNER_Y, CORNER_W, height - BORDER - CORNER_Y);
    }
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
}''')
rep('''static int peak_blocked(int x, int y, int d)
{
    if (y < BAR_Y + BAR_H + 6 + FONT_H + 8)                   /* status bar + notice line */
        return 1;''', '''/* Areas the analysis cells stay out of: notice line, menu, scope panels, loupe map, spot box.
 * The status bar and corner blocks are drawn over the cells instead (draw_status_locked). */
static int cell_blocked(int x, int y, int d)
{
    if (y + d > BAR_Y + BAR_H && y < BAR_Y + BAR_H + 6 + FONT_H + 8)   /* notice line */
        return 1;''')
rep('''    if (y + d > CORNER_Y - 2 && (x < BORDER + CORNER_W || x + d > (int)width - BORDER - CORNER_W))
        return 1;                                            /* bottom corner info blocks */
    if (wf_shown''', '''    if (wf_shown''')
i = s.index('static int cell_blocked(int x, int y, int d)')
j = s.index('\n}\n', i) + 3
s = s[:j] + '''
/* Guide lines also keep out of the status bar and corner blocks (text there must stay readable). */
static int peak_blocked(int x, int y, int d)
{
    if (y < BAR_Y + BAR_H + 6 + FONT_H + 8)
        return 1;
    if (y + d > CORNER_Y - 2 && (x < BORDER + CORNER_W || x + d > (int)width - BORDER - CORNER_W))
        return 1;
    return cell_blocked(x, y, d);
}

static int cells_mw, cells_mh;                                /* grid of the last analysis map */
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
''' + s[j:]
rep('''    int erased = 0, recolour = idx && memcmp(prev_palette, palette, n_palette * sizeof(*palette)) != 0;
    pthread_mutex_lock(&lock);''', '''    int erased = 0, redo_status = 0, recolour = idx && memcmp(prev_palette, palette, n_palette * sizeof(*palette)) != 0;
    pthread_mutex_lock(&lock);
    cells_mw = mw, cells_mh = mh;
    if (idx)
        memcpy(cells_palette, palette, n_palette * sizeof(*palette));''')
rep('''            int x = i * (int)width / mw, dx = (i + 1) * (int)width / mw - x;
            if (!peak_blocked(x, y, dx > dy ? dx : dy)) {''', '''            int x = i * (int)width / mw, dx = (i + 1) * (int)width / mw - x;
            if (in_text_area(x, y, dx, dy)) {          /* under the status text: composited below */
                peak_prev[j][i] = v;
                redo_status = 1;
            } else if (!cell_blocked(x, y, dx > dy ? dx : dy)) {''')
rep('''    if (erased)
        draw_overlay(0);                                    /* erased dots may have cut the guides */
    pthread_mutex_unlock(&lock);''', '''    if (erased)
        draw_overlay(0);                                    /* erased dots may have cut the guides */
    if (redo_status)
        draw_status_locked();
    pthread_mutex_unlock(&lock);''')
open(p,'w').write(s)
