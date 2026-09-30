def rep(p, pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s, (p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
rep('build.sh',[('settings.c osd.c sensordbg.c \\','settings.c osd.c sensordbg.c dng.c \\')])
rep('camapp.h',[('void notice_for(const char *text, uint32_t colour, int secs);   /* one-line OSD notice for secs */',
'''void notice_for(const char *text, uint32_t colour, int secs);   /* one-line OSD notice for secs */
void dng_fbc_off(void);                /* dng.c: VIF output uncompressed (before pipeline start) */
int dng_capture(const char *path, uint32_t x0, uint32_t y0, uint32_t w, uint32_t h);''')])
rep('settings.h',[('    uint32_t ae_priority;','    uint32_t photo_raw;             /* photos also as DNG (raw 10-bit Bayer) */\n    uint32_t ae_priority;')])
rep('settings.c',[('{ "ae_priority", &settings.ae_priority },','{ "ae_priority", &settings.ae_priority }, { "photo_raw", &settings.photo_raw },')])
rep('camapp.c',[
('''    DO(MI_SYS_Init(0));                              UNDO(u_sys);''','''    dng_fbc_off();                                   /* before VIF: real raw for DNG photos */
    DO(MI_SYS_Init(0));                              UNDO(u_sys);'''),
('''                char msg[48];
                snprintf(msg, sizeof(msg), "%s saved", strrchr(path, '/') + 1);
                photo_feedback(msg, OSD_WHITE, 1);''','''                char msg[64], raw[72];
                snprintf(raw, sizeof(raw), "%.*s.DNG", (int)(strrchr(path, '.') - path), path);
                int dng = settings.photo_raw && dng_capture(raw, 0, 0, 0, 0) == 0;
                snprintf(msg, sizeof(msg), "%s saved%s", strrchr(path, '/') + 1, dng ? " + DNG" : settings.photo_raw ? " (DNG failed)" : "");
                photo_feedback(msg, OSD_WHITE, 1);'''),
])
rep('cmd.c',[('''    if (!strcmp(cmd, "raw_probe")) {''','''    if (!strcmp(cmd, "dng")) {        /* dng [x y w h]: raw frame (or region) to /tmp/test.dng (FIFO test) */
        unsigned x = 0, y = 0, w = 0, h = 0;
        sscanf(line, "%*s %u %u %u %u", &x, &y, &w, &h);
        printf("dng: capture %d\\n", dng_capture(sd_mounted() && !w ? REC_DIR "/TEST.DNG" : "/tmp/test.dng", x, y, w, h));
        return;
    }
    if (!strcmp(cmd, "photoraw") && n == 2) {
        settings.photo_raw = val ? 1 : 0;
        settings_changed();
        return;
    }
    if (!strcmp(cmd, "raw_probe")) {''')])
