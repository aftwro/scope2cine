def rep(p, pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s, (p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
# osd: focus panel top right under the status bar
rep('osd.c',[('''void osd_border(uint32_t colour)
{''','''/* Focus meter panel, top right under the status bar: "FOCUS" + bar (level of the peak) + number.
 * value/peak < 0 hides it. Drawn off-screen and copied (no flicker); cells keep out of it. */
#define FOCUS_W 300
#define FOCUS_H (FONT_H + 10)
#define FOCUS_X ((int)width - MARGIN - FOCUS_W)
#define FOCUS_Y NOTICE_Y
static int focus_shown;
void osd_focus(int value, int peak)
{
    if (fd < 0)
        return;
    pthread_mutex_lock(&lock);
    int shadowed = shadow_begin();
    clear_area(FOCUS_X, FOCUS_Y, FOCUS_W, FOCUS_H);
    focus_shown = value >= 0 && peak > 0;
    if (focus_shown) {
        char num[16];
        snprintf(num, sizeof(num), "%3d%%", value * 100 / peak);
        fill(FOCUS_X, FOCUS_Y, FOCUS_W, FOCUS_H, HIST_BG);
        text_plain(FOCUS_X + 6, FOCUS_Y + 5, "FOCUS", OSD_WHITE);
        int bx = FOCUS_X + 6 + 6 * FONT_W, bw = FOCUS_W - 12 - 11 * FONT_W, by = FOCUS_Y + 8, bh = FOCUS_H - 16;
        int lv = value >= peak ? bw : value * bw / peak;
        fill(bx, by, bw, bh, HIST_TICK);
        fill(bx, by, lv, bh, value * 100 >= peak * 97 ? 0xff40e060 : 0xfff0f0f0);   /* green at the peak */
        fill(bx + bw - 2, by - 3, 2, bh + 6, 0xff40e060);                            /* peak mark */
        text_plain(bx + bw + FONT_W / 2, FOCUS_Y + 5, num, OSD_WHITE);
    }
    if (shadowed) {
        shadow_end();
        shadow_commit(FOCUS_X, FOCUS_Y, FOCUS_W, FOCUS_H);
    }
    pthread_mutex_unlock(&lock);
}

void osd_border(uint32_t colour)
{''')])
s=open('osd.c').read()
# forward decls needed? text_plain defined earlier? check order later
open('osd.c','w').write(s)
rep('osd.c',[('''    if (notice_len && x < MARGIN + notice_len * FONT_W + 6 && y + d > NOTICE_Y - 4 && y < NOTICE_Y + FONT_H + 4)''',
'''    if (focus_shown && x + d > FOCUS_X - 4 && y + d > FOCUS_Y - 4 && y < FOCUS_Y + FOCUS_H + 4)
        return 1;                                            /* focus meter panel */
    if (notice_len && x < MARGIN + notice_len * FONT_W + 6 && y + d > NOTICE_Y - 4 && y < NOTICE_Y + FONT_H + 4)''')])
rep('osd.h',[('void osd_show(int on);','void osd_show(int on);\nvoid osd_focus(int value, int peak);   /* focus meter panel; value < 0 hides it */')])
rep('settings.h',[('    uint32_t histogram;             /* OSD histogram panel on/off */','    uint32_t histogram;             /* OSD histogram panel on/off */\n    uint32_t focus_meter;           /* focus meter from the ISP AF statistics on/off */')])
rep('settings.c',[('{ "histogram", &settings.histogram },','{ "histogram", &settings.histogram }, { "focus_meter", &settings.focus_meter },')])
# scopes: compute in analysis thread
rep('scopes.c',[('''        cleared = 0;
        const uint8_t *y = *(uint8_t **)(info + 0x3c);''','''        cleared = 0;
        focus_update();
        const uint8_t *y = *(uint8_t **)(info + 0x3c);'''),
('''static volatile int peak_run;''','''static volatile int peak_run;

/* Focus meter: MI_ISP_CUS3A_GetAFStats (0x1c00 bytes) holds 8 AF windows of 5 u32 each; word 0 is
 * the window's high-frequency energy (4.5k..110k on the test scene, steady between reads, highest
 * over text), word 4 its brightness (measured 2026-09-29). The sum of word 0 rises as focus gets
 * sharper. Shown as a share of the peak of the last few seconds: focus until the bar hits the peak. */
static void focus_update(void)
{
    static uint32_t st[0x1c00 / 4];
    static int tick, shown;
    static double avg, peak;
    static time_t peak_at;
    if (!settings.focus_meter || playing) {
        if (shown)
            osd_focus(-1, 0);
        shown = 0;
        return;
    }
    if (++tick % 3)                                     /* ~3 updates a second */
        return;
    if (MI_ISP_CUS3A_GetAFStats(0, 0, st) != 0)
        return;
    double sum = 0;
    for (int w = 0; w < 8; w++)
        sum += st[w * 5];
    avg = avg ? avg * 0.5 + sum * 0.5 : sum;            /* a little smoothing against noise */
    time_t now = time(NULL);
    if (avg > peak || now - peak_at > 4)                /* peak holds 4 s, then follows the value */
        peak = avg, peak_at = avg > peak ? now : peak_at;
    if (avg >= peak)
        peak_at = now;
    osd_focus((int)(avg / 100), (int)(peak / 100) > 0 ? (int)(peak / 100) : 1);
    shown = 1;
}'''),
('''    int want = settings.peaking || settings.zebra || settings.false_colour || settings.histogram || settings.waveform ||
               view_request;''','''    int want = settings.peaking || settings.zebra || settings.false_colour || settings.histogram || settings.waveform ||
               settings.focus_meter || view_request;'''),
])
rep('menu.c',[
('MI_GUIDES, MI_FRAME, MI_HISTOGRAM,','MI_GUIDES, MI_FRAME, MI_HISTOGRAM, MI_FOCUS,'),
('"Guides", "Frame", "Histogram", ','"Guides", "Frame", "Histogram", "Focus meter", '),
('MI_WAVEFORM, MI_HISTOGRAM,\n','MI_WAVEFORM, MI_HISTOGRAM, MI_FOCUS,\n'),
('''        case MI_HISTOGRAM: snprintf(b, 32, "%s", settings.histogram ? "On" : "Off"); break;''','''        case MI_HISTOGRAM: snprintf(b, 32, "%s", settings.histogram ? "On" : "Off"); break;
        case MI_FOCUS: snprintf(b, 32, "%s", settings.focus_meter ? "On" : "Off"); break;'''),
('''    case MI_HISTOGRAM:
        settings.histogram ^= 1;''','''    case MI_FOCUS:
        settings.focus_meter ^= 1;
        apply_peaking();                            /* read in the analysis thread */
        break;
    case MI_HISTOGRAM:
        settings.histogram ^= 1;'''),
('''    else if (sel_id == MI_SPOT_SIZE)''','''    else if (sel_id == MI_FOCUS)
        help = "Focus meter: sharpness from the ISP's autofocus statistics (8 windows, full resolution). Turn the focus ring until the bar reaches the green peak mark and turns green; the peak is held for 4 s.";
    else if (sel_id == MI_SPOT_SIZE)'''),
])
