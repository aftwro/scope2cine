def rep(p,pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s,(p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
rep('settings.h',[('    uint32_t photo_raw;','    uint32_t hdmi_out;              /* HDMI output mode, index into hdmi_modes (camapp.c), 0 = 1080p60 */\n    uint32_t photo_raw;')])
rep('settings.c',[('{ "photo_raw", &settings.photo_raw },','{ "photo_raw", &settings.photo_raw }, { "hdmi_out", &settings.hdmi_out },')])
rep('camapp.h',[('extern int osd_hidden;','''extern int osd_hidden;
/* HDMI output modes: display timing enum and HDMI timing id from cardv's astTimingArray (the HDMI
 * transmitter has a fixed list; no custom timings, so no 1080p100). */
struct hdmi_mode { const char *name; uint32_t disp, hdmi, w, h; };
#define N_HDMI_MODES 7
extern const struct hdmi_mode hdmi_modes[N_HDMI_MODES];
extern uint32_t out_w, out_h;          /* HDMI output size of this run (display, loupe) */
extern int hdmi_mode_run;               /* hdmi_modes index this run started with */''')])
rep('camapp.c',[
('''int ae_locked;
int osd_hidden;''','''int ae_locked;
int osd_hidden;
const struct hdmi_mode hdmi_modes[N_HDMI_MODES] = {
    { "1080p60", 16, 9, 1920, 1080 }, { "1080p50", 15, 7, 1920, 1080 }, { "1080p30", 12, 10, 1920, 1080 },
    { "1080p25", 11, 11, 1920, 1080 }, { "1080p24", 10, 12, 1920, 1080 }, { "720p60", 9, 5, 1280, 720 },
    { "720p50", 8, 4, 1280, 720 },
};
uint32_t out_w = 1920, out_h = 1080;
int hdmi_mode_run;'''),
('''    int rc = 1, pipeline_ok = 0;''','''    int rc = 1, pipeline_ok = 0;
    /* HDMI output mode (settings.hdmi_out): the display port gets the picture already scaled to the
     * output size by SCL port 0, the layer and input port match it; the OSD framebuffer's stretch
     * follows the display timing. */
    settings_load(SETTINGS_PATH);
    hdmi_mode_run = settings.hdmi_out < N_HDMI_MODES ? (int)settings.hdmi_out : 0;
    const struct hdmi_mode *hm = &hdmi_modes[hdmi_mode_run];
    out_w = hm->w, out_h = hm->h;
    disp_pub[2] = hm->disp;
    hdmi_attr[2] = hm->hdmi;
    layer_attr[1] = out_w | out_h << 16, layer_attr[2] = out_w, layer_attr[3] = out_h;
    port_attr[1] = port_attr[2] = out_w | out_h << 16;
    scl_port0[2] = out_w | out_h << 16;
    printf("hdmi: output %s (%ux%u, disp timing %u, hdmi timing %u)\\n", hm->name, out_w, out_h, hm->disp, hm->hdmi);'''),
])
rep('scopes.c',[('''    uint32_t p0[6] = { loupe_on ? x | y << 16 : 0, w | h << 16, 0x04380780, 0x101, 0xb, 0 };''','''    uint32_t p0[6] = { loupe_on ? x | y << 16 : 0, w | h << 16, out_w | out_h << 16, 0x101, 0xb, 0 };''')])
rep('player.c',[('''    uint32_t attr[3] = { 0, 0x04380780, (uint32_t)w | (uint32_t)h << 16 };''','''    uint32_t attr[3] = { 0, out_w | out_h << 16, (uint32_t)w | (uint32_t)h << 16 };''')])
rep('menu.c',[
('SI_SD, SI_FORMAT, SI_BUILD,','SI_HDMI, SI_SD, SI_FORMAT, SI_BUILD,'),
('"SD card", "Format SD", "Build",','"HDMI output", "SD card", "Format SD", "Build",'),
('rows_system[] = { MI_MIRROR, MI_FLIP, SI_SD,','rows_system[] = { MI_MIRROR, MI_FLIP, SI_HDMI, SI_SD,'),
('''        case SI_UPTIME: {''','''        case SI_HDMI:
            if ((int)settings.hdmi_out != hdmi_mode_run)
                snprintf(b, 32, "%s: %s", hdmi_modes[settings.hdmi_out % N_HDMI_MODES].name, rec_on ? "stop rec" : "Play = go");
            else
                snprintf(b, 32, "%s", hdmi_modes[hdmi_mode_run].name);
            break;
        case SI_UPTIME: {'''),
('''    case SI_LOGS: {                                 /* camapp log + kernel log to the SD card */''','''    case SI_HDMI:                                   /* arrows pick, OK / play restarts into it */
        if (menu_ok) {
            if ((int)settings.hdmi_out != hdmi_mode_run && !rec_on) {
                printf("menu: HDMI output %s, restarting\\n", hdmi_modes[settings.hdmi_out].name);
                settings_changed();
                restart_requested = stop_requested = 1;
            }
            return;
        }
        settings.hdmi_out = (settings.hdmi_out + N_HDMI_MODES + dir) % N_HDMI_MODES;
        return;
    case SI_LOGS: {                                 /* camapp log + kernel log to the SD card */'''),
('case SI_FORMAT: case SI_RESET: case SI_TC: case SI_LOGS:','case SI_FORMAT: case SI_RESET: case SI_TC: case SI_LOGS: case SI_HDMI:'),
('''    else if (sel_id == SI_LOGS)''','''    else if (sel_id == SI_HDMI)
        help = "HDMI output mode: 1080p 60/50/30/25/24 or 720p 60/50. The picture is scaled straight to the output size, the OSD follows. Left/right pick, OK/Play applies (restart). Your monitor must support the mode; 100 Hz is not possible (the HDMI chip has a fixed mode list).";
    else if (sel_id == SI_LOGS)'''),
])
