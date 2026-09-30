import re
p='osd.c'; s=open(p).read()
def rep(a,b,cnt=1):
    global s
    assert s.count(a) >= cnt, a; s=s.replace(a,b,cnt)
# forward decl + clear_area near shadow helpers
rep('''static void shadow_end(void) { fb = screen; }
''', '''static void shadow_end(void) { fb = screen; }

/* Clear an OSD element's area to transparent and mark the analysis cells under it for redraw
 * (otherwise cells the element covered stay missing: black holes after hiding a scope panel). */
static void cells_invalidate(int x0, int y0, int w, int h);
static void clear_area(int x0, int y0, int w, int h)
{
    fill(x0, y0, w, h, KEY);
    cells_invalidate(x0, y0, w, h);
}
''')
for a in ['fill(x0, y0, HIST_W, HIST_H, KEY);', 'fill(WF_X - 2, WF_Y - 2, WF_W + 4, WF_H + 4, KEY);',
          'fill(HIST_X, HIST_Y, HIST_W, HIST_H, KEY);', 'fill(MENU_X, MENU_Y, MENU_W, menu_h, KEY);',
          'fill(HELP_X, MENU_Y, HELP_W, help_h, KEY);', 'fill(x0 - 2, y0 - 2, MAP_W + 4, MAP_H + 4, KEY);',
          'fill(bx - 1, by - 1, bw + 2, 4, KEY);', 'fill(bx - 1, by + bh - 3, bw + 2, 4, KEY);',
          'fill(bx - 1, by - 1, 4, bh + 2, KEY);', 'fill(bx + bw - 3, by - 1, 4, bh + 2, KEY);',
          'fill(MARGIN - 2, NOTICE_Y - 2, NOTICE_W, FONT_H + 4, KEY);']:
    n = s.count(a); assert n >= 1, a
    s = s.replace(a, 'clear_area' + a[len('fill'):].replace(', KEY)', ')'))
# notice: remember text length; only the text blocks cells
rep('''    if (msg)
        text(MARGIN, NOTICE_Y, msg, colour);
    draw_overlay(0);''', '''    notice_len = msg ? (int)strlen(msg) : 0;
    if (msg)
        text(MARGIN, NOTICE_Y, msg, colour);
    draw_overlay(0);''')
rep('''    if (y + d > BAR_Y + BAR_H && y < BAR_Y + BAR_H + 6 + FONT_H + 8)   /* notice line */
        return 1;''', '''    if (notice_len && x < MARGIN + notice_len * FONT_W + 6 && y + d > NOTICE_Y - 4 && y < NOTICE_Y + FONT_H + 4)
        return 1;                                            /* notice text, only while one is shown */''')
rep('''static int cells_mw, cells_mh;                                /* grid of the last analysis map */''',
'''static int cells_mw, cells_mh;                                /* grid of the last analysis map */

static void cells_invalidate(int x0, int y0, int w, int h)
{
    if (!cells_mw || !cells_mh)
        return;
    for (int j = y0 * cells_mh / (int)height; j < cells_mh && j * (int)height / cells_mh < y0 + h; j++)
        for (int i = x0 * cells_mw / (int)width; i < cells_mw && i * (int)width / cells_mw < x0 + w; i++)
            if (peak_prev[j][i])
                peak_prev[j][i] = 0xff;                      /* was drawn, now unknown */
}''')
open(p,'w').write(s)
