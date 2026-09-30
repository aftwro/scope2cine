def rep(p, pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s, (p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
rep('settings.h',[('    uint32_t manual;                /* 0 = auto exposure, 1 = manual */',
'''    uint32_t manual;                /* 0 = auto exposure, 1 = manual */
    uint32_t ae_priority;           /* auto only: 0 P (program), 1 S (shutter fixed), 2 G (gain fixed) */''')])
rep('settings.c',[('{ "manual", &settings.manual },','{ "manual", &settings.manual }, { "ae_priority", &settings.ae_priority },'),
 ('"manual", "shutter_us",','"manual", "ae_priority", "shutter_us",')])
rep('camapp.c',[
('''    lim[0] = 14 * DRIVER_TIME_SCALE;                  /* min shutter ~1 line (vendor 40 us) so 1/64000 works */''',
'''    /* ExposureLimit: { min shutter, max shutter (driver us), min FNx10, max FNx10, min sensor gain,
     * min ISP gain, max sensor gain, max ISP gain } (1024 = 1x). */
    lim[0] = 14 * DRIVER_TIME_SCALE;                  /* min shutter ~1 line (vendor 40 us) so 1/64000 works */'''),
('''    lim[6] = cap;
    int r = MI_ISP_AE_SetExposureLimit(0, 0, lim);
    printf("ae limits: max shutter %u (driver), max sensor gain %u, set = %d\\n", lim[1], lim[6], r);''',
'''    lim[6] = cap;
    /* Priority modes = auto exposure with one side pinned by its limits: S pins the shutter (AE moves
     * only gain), G pins sensor gain and holds ISP gain at 1x (AE moves only the shutter). */
    if (!settings.manual && settings.ae_priority == 1) {
        uint32_t frame = frame_us() * 97 / 100, want = manual_shutter_us();
        lim[0] = lim[1] = (want < frame ? want : frame) * DRIVER_TIME_SCALE;
    } else if (!settings.manual && settings.ae_priority == 2) {
        lim[4] = lim[6] = settings.sensor_gain;
        lim[5] = lim[7] = 1024;
    }
    int r = MI_ISP_AE_SetExposureLimit(0, 0, lim);
    printf("ae limits %s: shutter %u-%u (driver), sensor gain %u-%u, isp gain %u-%u (vendor %u %u %u %u %u %u %u %u), set = %d\\n",
           expo_mode_short(), lim[0], lim[1], lim[4], lim[6], lim[5], lim[7], vendor[0], vendor[1], vendor[2], vendor[3],
           vendor[4], vendor[5], vendor[6], vendor[7], r);'''),
('''static int apply_ae_limits(void)
{''', '''/* Exposure mode label: M manual, P program (auto), S shutter priority, G gain priority. */
const char *expo_mode_short(void)
{
    return settings.manual ? "M" : settings.ae_priority == 1 ? "S" : settings.ae_priority == 2 ? "G" : "AUTO";
}

static int apply_ae_limits(void)
{'''),
('''    snprintf(centre, sizeof(centre), "%s%s %s%s  %.1fx%s  %s %s  LV %.1f", settings.manual ? "M" : ae_locked ? "AE-L" : "AUTO", comp,''',
'''    snprintf(centre, sizeof(centre), "%s%s %s%s  %.1fx%s  %s %s  LV %.1f", ae_locked && !settings.manual ? "AE-L" : expo_mode_short(), comp,'''),
])
rep('camapp.h',[('#define EV_THIRDS_MAX 9','#define EV_THIRDS_MAX 9\nconst char *expo_mode_short(void);\nvoid expo_mode_step(int dir);   /* P > S > G > M */')])
rep('menu.c',[
('''        if ((i == MI_SHUTTER_BY || i == MI_SHUTTER || i == MI_GAIN) && !settings.manual)
            continue;''', '''        if ((i == MI_SHUTTER_BY || i == MI_SHUTTER) && !settings.manual && settings.ae_priority != 1)
            continue;
        if (i == MI_GAIN && !settings.manual && settings.ae_priority != 2)
            continue;
        if ((i == MI_AE_MAXSH && settings.ae_priority == 1) || (i == MI_AE_MAXGAIN && settings.ae_priority == 2))
            continue;                               /* pinned by the priority mode */'''),
('''        case MI_EXPOSURE: snprintf(b, 32, "%s", settings.manual ? "Manual" : "Auto"); break;''',
'''        case MI_EXPOSURE:
            snprintf(b, 32, "%s", settings.manual ? "M manual" : settings.ae_priority == 1 ? "S shutter prio"
                                  : settings.ae_priority == 2 ? "G gain prio" : "P auto");
            break;'''),
('''    case MI_EXPOSURE:
        settings.manual ^= 1;
        apply_exposure();
        break;''', '''    case MI_EXPOSURE:
        expo_mode_step(dir);
        break;'''),
('''    else if (!help && sel_id == MI_SEC_EXPO)''', '''    else if (!help && sel_id == MI_EXPOSURE)
        help = "P: auto shutter and gain. S: you set the shutter (angle), auto exposure moves only the gain. G: you set the gain, auto exposure moves only the shutter. M: both manual. EV comp works in P, S and G. No aperture mode: the lens has no iris.";
    else if (!help && sel_id == MI_SEC_EXPO)'''),
])
rep('cmd.c',[
('''    if (settings.manual && !playing) {              /* manual: shutter and gain straight from the buttons */
        int sd = !strcmp(cmd, "right") || !strcmp(cmd, "ok") ? 1 : !strcmp(cmd, "left") || !strcmp(cmd, "ae") ? -1 : 0;
        int gd = !strcmp(cmd, "up") ? 1 : !strcmp(cmd, "down") ? -1 : 0;
        int fd = !strcmp(cmd, "ev_up") ? 1 : !strcmp(cmd, "ev_down") ? -1 : 0;''',
'''    int own_shutter = settings.manual || settings.ae_priority == 1, own_gain = settings.manual || settings.ae_priority == 2;
    if ((own_shutter || own_gain) && !playing) {    /* M / S / G: the user's values straight from the buttons */
        int sd = !own_shutter ? 0 : !strcmp(cmd, "right") || !strcmp(cmd, "ok") ? 1 : !strcmp(cmd, "left") || !strcmp(cmd, "ae") ? -1 : 0;
        int gd = !own_gain ? 0 : !strcmp(cmd, "up") ? 1 : !strcmp(cmd, "down") ? -1 : 0;
        int fd = !settings.manual ? 0 : !strcmp(cmd, "ev_up") ? 1 : !strcmp(cmd, "ev_down") ? -1 : 0;'''),
('''    if (!strcmp(cmd, "remote_ae")) {  /* remote AE button: exposure Auto <-> Manual */
        settings.manual ^= 1;
        apply_exposure();
        settings_changed();
        notice_for(settings.manual ? "Exposure: Manual" : "Exposure: Auto", OSD_WHITE, 2);
        return;
    }''', '''    if (!strcmp(cmd, "remote_ae")) {  /* remote AE button: exposure mode P > S > G > M */
        expo_mode_step(1);
        settings_changed();
        static const char *const names[] = { "Exposure: P auto", "Exposure: S shutter priority", "Exposure: G gain priority" };
        notice_for(settings.manual ? "Exposure: M manual" : names[settings.ae_priority % 3], OSD_WHITE, 2);
        return;
    }
    if (!strcmp(cmd, "prio") && n == 2) {   /* prio 0-3: P S G M */
        settings.manual = val == 3;
        settings.ae_priority = val < 3 ? val : 0;
        apply_exposure();
        settings_changed();
        printf("cmd prio: %s\\n", expo_mode_short());
        return;
    }'''),
])
rep('camapp.c',[('''void apply_exposure(void)
{''', '''void expo_mode_step(int dir)
{
    int m = settings.manual ? 3 : (int)(settings.ae_priority % 3);
    m = (m + 4 + dir) % 4;
    settings.manual = m == 3;
    settings.ae_priority = m == 3 ? 0 : m;
    apply_exposure();
}

void apply_exposure(void)
{''')])
rep('rec.c',[('''    fprintf(f, "exposure=%s\\n", settings.manual ? "manual" : "auto (values at the start)");''',
'''    fprintf(f, "exposure=%s\\n", settings.manual ? "manual" : settings.ae_priority == 1 ? "shutter priority (gain auto, values at the start)"
                               : settings.ae_priority == 2 ? "gain priority (shutter auto, values at the start)" : "auto (values at the start)");''')])
