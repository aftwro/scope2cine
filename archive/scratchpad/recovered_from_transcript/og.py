def rep(p, pairs):
    s=open(p).read()
    for a,b in pairs:
        assert a in s, (p,a); s=s.replace(a,b,1)
    open(p,'w').write(s)
rep('camapp.h',[('extern uint32_t crop_w, crop_h;        /* sensor window crop of this run, 0 = full frame */',
'''extern uint32_t crop_w, crop_h;        /* sensor window crop of this run, 0 = full frame */
#define OPEN_GATE_W 1944                /* crop_w value meaning open gate: the whole 1948x1097 readout, */
#define OPEN_GATE_H 1096                /* VIF-aligned (width 8, height 2), scaled to 1920x1080 */
#define OPEN_GATE (crop_w == OPEN_GATE_W)''')])
rep('camapp.c',[
('''    w = !w || w > 1920 ? 1920 : w < 320 ? 320 : w & ~7u;
    h = !h || h > 1080 ? 1080 : h < 180 ? 180 : h & ~1u;''','''    if (w == OPEN_GATE_W) {                          /* open gate (experimental): no sensor window */
        crop_w = OPEN_GATE_W, crop_h = OPEN_GATE_H;
        printf("crop: open gate %ux%u (full readout), max %u fps\\n", crop_w, crop_h, crop_max_fps(crop_w));
        return;
    }
    w = !w || w > 1920 ? 1920 : w < 320 ? 320 : w & ~7u;
    h = !h || h > 1080 ? 1080 : h < 180 ? 180 : h & ~1u;'''),
('''    return crop_w ? crop_sensor() : MI_SNR_SetOrien(0, settings.mirror, settings.flip);''',
'''    return crop_w && !OPEN_GATE ? crop_sensor() : MI_SNR_SetOrien(0, settings.mirror, settings.flip);'''),
('''    if (crop_w) {
        crop_sensor();
        vif_dev[1] = ((1080 - crop_h) / 2) << 16;''','''    if (crop_w) {
        if (!OPEN_GATE)                              /* open gate: the sensor's default full output */
            crop_sensor();
        vif_dev[1] = OPEN_GATE ? 0 : ((1080 - crop_h) / 2) << 16;'''),
])
rep('dng.c',[('(crop_w && ((1080 - crop_h) / 2) & 1 ? 2 : 0)','(crop_w && !OPEN_GATE && ((1080 - crop_h) / 2) & 1 ? 2 : 0)')])
rep('rec.c',[('''    if (crop_w)
        fprintf(f, "sensor_crop=%ux%u centred (scaled to 1920x1080)\\n", crop_w, crop_h);''','''    if (OPEN_GATE)
        fprintf(f, "open_gate=%ux%u (full sensor readout, scaled to 1920x1080)\\n", crop_w, crop_h);
    else if (crop_w)
        fprintf(f, "sensor_crop=%ux%u centred (scaled to 1920x1080)\\n", crop_w, crop_h);''')])
rep('menu.c',[
('''static uint32_t crop_set_w(void) { return !settings.crop_w || settings.crop_w > 1920 ? 1920 : settings.crop_w; }
static uint32_t crop_set_h(void) { return !settings.crop_h || settings.crop_h > 1080 ? 1080 : settings.crop_h; }''',
'''static uint32_t crop_set_w(void)
{
    return settings.crop_w == OPEN_GATE_W ? OPEN_GATE_W : !settings.crop_w || settings.crop_w > 1920 ? 1920 : settings.crop_w;
}
static uint32_t crop_set_h(void)
{
    return settings.crop_w == OPEN_GATE_W ? OPEN_GATE_H : !settings.crop_h || settings.crop_h > 1080 ? 1080 : settings.crop_h;
}'''),
('''            else if (id == MI_CROP_W)
                snprintf(b, 32, "%u%s (max %up)", w, w == 1920 ? " full" : "", crop_max_fps(w));''','''            else if (id == MI_CROP_W && w == OPEN_GATE_W)
                snprintf(b, 32, "Open gate %u (exp.)", w);
            else if (id == MI_CROP_W)
                snprintf(b, 32, "%u%s (max %up)", w, w == 1920 ? " full" : "", crop_max_fps(w));'''),
('''        if (id == MI_CROP_W) {
            int w = (int)crop_set_w() + dir * (menu_fine ? 8 : 32);
            settings.crop_w = w < 320 ? 320 : w > 1920 ? 1920 : w;
            settings.crop_h = crop_set_h();''','''        if (id == MI_CROP_W) {                      /* ... 1888 < 1920 full < Open gate 1944 */
            int cur = (int)crop_set_w(), w = cur == OPEN_GATE_W ? (dir > 0 ? OPEN_GATE_W : 1920)
                                                 : cur == 1920 && dir > 0 ? OPEN_GATE_W : cur + dir * (menu_fine ? 8 : 32);
            settings.crop_h = w == OPEN_GATE_W ? OPEN_GATE_H : cur == OPEN_GATE_W ? 1080 : crop_set_h();
            settings.crop_w = w < 320 ? 320 : w;'''),
('''        } else {
            int h = (int)crop_set_h() + dir * (menu_fine ? 2 : 18);''','''        } else if (crop_set_w() != OPEN_GATE_W) {  /* open gate has a fixed height */
            int h = (int)crop_set_h() + dir * (menu_fine ? 2 : 18);'''),
('''Height changes only the framing, not the speed.''','''Height changes only the framing, not the speed. Above full width: Open gate (experimental) = the whole 1948x1097 sensor readout (1944x1096 used), ~1.3% more picture, scaled to 1920x1080, 75p max.'''),
])
