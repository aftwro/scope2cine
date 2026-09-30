def rep(p, pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s, (p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
rep('camapp.h',[('''/* Spot = 2 x 4 blocks of the 16x16 grid: 240 x 270 sensor pixels on a 16:9 frame, i.e. square on
 * screen, and centred exactly at blocks 7-8 / 6-9 (an odd size cannot sit on the centre line). */
#define SPOT_W 2
#define SPOT_H 4''','''/* Spot = S x 2S blocks of the 16x16 grid (S = settings.spot_size 1..8, default 2 = 240 x 270
 * sensor pixels): square on screen on a 16:9 frame. Size 2 sits exactly on the centre lines. */
int spot_w(void);
#define SPOT_W spot_w()
#define SPOT_H (2 * spot_w())''')])
rep('camapp.c',[('''int weight_box(const struct win_weights *w, int t, struct wgt_box *b)''','''int spot_w(void)
{
    uint32_t s = settings.spot_size ? settings.spot_size : 2;
    return s > 8 ? 8 : (int)s;
}

int weight_box(const struct win_weights *w, int t, struct wgt_box *b)''')])
rep('settings.h',[('    uint32_t spot_x, spot_y;        /* spot top-left block on the 16x16 AE grid */','    uint32_t spot_x, spot_y;        /* spot top-left block on the 16x16 AE grid */\n    uint32_t spot_size;             /* spot = size x 2*size blocks (1..8, 0 = default 2) */')])
rep('settings.c',[('{ "spot_x", &settings.spot_x },','{ "spot_x", &settings.spot_x }, { "spot_size", &settings.spot_size },')])
rep('menu.c',[
('MI_AE_MAXSH, MI_AE_MAXGAIN, MI_METERING, ','MI_AE_MAXSH, MI_AE_MAXGAIN, MI_METERING, MI_SPOT_SIZE, '),
('"Max shutter", "Max gain", "Metering", ','"Max shutter", "Max gain", "Metering", "Spot size", '),
('MI_EVCOMP, MI_METERING, MI_AE_LOCK,','MI_EVCOMP, MI_METERING, MI_SPOT_SIZE, MI_AE_LOCK,'),
('''        if (i == MI_PEAK_COLOUR && !settings.peaking)''','''        if (i == MI_SPOT_SIZE && settings.metering != 2)
            continue;
        if (i == MI_PEAK_COLOUR && !settings.peaking)'''),
('''        case MI_PHOTO_RAW:''','''        case MI_SPOT_SIZE:
            snprintf(b, 32, "%d (%d%% of width)%s", spot_w(), spot_w() * 100 / WGT_GRID, spot_w() == 2 ? " default" : "");
            break;
        case MI_PHOTO_RAW:'''),
('''    case MI_PHOTO_RAW:
        settings.photo_raw ^= 1;
        break;''','''    case MI_PHOTO_RAW:
        settings.photo_raw ^= 1;
        break;
    case MI_SPOT_SIZE:
        spot_resize(spot_w() + dir);
        break;'''),
('''static void menu_change(int dir)
{''','''/* New spot size (1..8), keeping the spot centred where it was. */
static void spot_resize(int size)
{
    size = size < 1 ? 1 : size > 8 ? 8 : size;
    int cx2 = 2 * (int)settings.spot_x + SPOT_W, cy2 = 2 * (int)settings.spot_y + SPOT_H;   /* centre x2 */
    settings.spot_size = size;
    int x = (cx2 - SPOT_W) / 2, y = (cy2 - SPOT_H) / 2;
    settings.spot_x = x < 0 ? 0 : x > WGT_GRID - SPOT_W ? WGT_GRID - SPOT_W : x;
    settings.spot_y = y < 0 ? 0 : y > WGT_GRID - SPOT_H ? WGT_GRID - SPOT_H : y;
    install_weight_tables();
    show_spot_area();
}

static void menu_change(int dir)
{'''),
('''    else if (sel_id == MI_PHOTO_RAW)''','''    else if (sel_id == MI_SPOT_SIZE)
        help = "Spot metering area: 1 (tiny, 6% of the width) to 8 (half the frame), always square on screen and centred where it was. Arrows outside the menu move it. Play = 2 (default).";
    else if (sel_id == MI_PHOTO_RAW)'''),
])
