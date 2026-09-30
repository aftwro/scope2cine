/* OSD in the USB picture: a hardware overlay region (MI_RGN) attached to SCL port 1, the JPEG path
 * that feeds UVC, photos and MJPEG clips. HDMI (port 0) is not touched.
 *
 * Mercury6 MI_RGN calls take a leading u16 SoC id (cardv passes g_s32SocId = 0); layouts read from
 * cardv's mid_osd_full_init / mid_OSD_CleanWidget_Rgn (2026-09-29), matching SigmaStar's MI_RGN doc:
 *   Create attr     { u32 type (0 = OSD), u32 pixel format (0 = ARGB1555), u32 width, u32 height }
 *   ChnPort         { u32 module (0x22 = SCL), u32 dev, u32 chn, u32 port }
 *   ChnPortParam    40 bytes: +0 u8 show, +4 x, +8 y, +0xc layer, +0x10 alpha mode (0 = per pixel),
 *                   +0x14 u8 background alpha, +0x15 u8 foreground alpha, +0x18 colour invert (off)
 *   CanvasInfo      +0 u64 phys, +8 virt, +0xc width, +0x10 height, +0x14 stride, +0x18 pixel format
 * Drawing: MI_RGN_GetCanvasInfo and MI_RGN_UpdateCanvas must be called as a pair around each draw,
 * with nothing in between: a canvas taken before AttachToChn made every later update fail with
 * "Front buf state error" in the kernel log while the call still returned 0 (2026-09-29). */
#include "camapp.h"
#include "mi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RGN_SOC 0
#define RGN_MOD_SCL 0x22

static int rgn_inited;
static uint8_t rgn_palette[256 * 4];         /* only indexed formats use it */

struct canvas {
    uint64_t phys;
    uint8_t *virt;
    uint32_t w, h, stride, fmt;
    uint32_t pad[2];
};

static int rgn_init(void)
{
    if (rgn_inited)
        return 0;
    void *init = rgn_palette;
    int r = MI_RGN_InitDev(RGN_SOC, &init);
    printf("rgn: InitDev %d\n", r);
    rgn_inited = r == 0;
    return r;
}

/* Region `handle` of w x h ARGB1555 on SCL port 1 at x,y. Returns the canvas or NULL. */
static int rgn_fg_alpha = 0xff;              /* test: cardv attaches with 0 / 0 */
static uint32_t rgn_port = 1;                /* SCL output port the region goes on (test: cardv uses 0) */

int rgn_create(uint32_t handle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, struct canvas *c)
{
    uint32_t attr[4] = { 0, 0, w, h };
    uint32_t port[4] = { RGN_MOD_SCL, 0, 0, rgn_port };
    uint8_t param[40] = { 0 };
    int r = rgn_init();
    r = r ? r : MI_RGN_Create(RGN_SOC, handle, attr);
    printf("rgn: create %u %ux%u: rc %d\n", handle, w, h, r);
    if (r)
        return r;
    (void)c;
    param[0] = 1;                                 /* show */
    memcpy(param + 4, &x, 4);
    memcpy(param + 8, &y, 4);
    param[0x14] = 0;                              /* alpha of pixels with A = 0: transparent */
    param[0x15] = rgn_fg_alpha;                   /* alpha of pixels with A = 1: opaque */
    r = MI_RGN_AttachToChn(RGN_SOC, handle, port, param);
    printf("rgn: attach %u to SCL port %u at %u,%u: rc %d\n", handle, rgn_port, x, y, r);
    return r;
}

/* Draw: rgn_begin gives the canvas, rgn_end hands it to the overlay hardware (always as a pair). */
int rgn_begin(uint32_t handle, struct canvas *c) { return MI_RGN_GetCanvasInfo(RGN_SOC, handle, c); }
int rgn_end(uint32_t handle) { return MI_RGN_UpdateCanvas(RGN_SOC, handle); }

int rgn_destroy(uint32_t handle)
{
    uint32_t port[4] = { RGN_MOD_SCL, 0, 0, rgn_port };
    int r1 = MI_RGN_DetachFromChn(RGN_SOC, handle, port), r2 = MI_RGN_Destroy(RGN_SOC, handle);
    printf("rgn: destroy %u: detach %d destroy %d\n", handle, r1, r2);
    return r1 | r2;
}

/* Step R2 test: a 256x64 red box at 100,100 in the USB picture (FIFO rgn_test 1 / 0). */
int rgn_test(int on)
{
    static int shown;
    static struct canvas c;
    if (!on) {
        if (shown)
            rgn_destroy(0);
        shown = 0;
        return 0;
    }
    if (shown)
        return 0;
    rgn_fg_alpha = on == 3 ? 0 : 0xff;
    rgn_port = on == 4 ? 0 : 1;
    if (on == 5) {                                /* cardv's exact order: draw + update, then attach + realtime flip */
        uint32_t attr[4] = { 0, 0, 256, 64 }, port[4] = { RGN_MOD_SCL, 0, 0, 1 }, x = 100, y = 100;
        uint8_t param[40] = { 0 };
        int r = rgn_init();
        r = r ? r : MI_RGN_Create(RGN_SOC, 0, attr);
        r = r ? r : rgn_begin(0, &c);
        for (uint32_t j = 0; r == 0 && c.virt && j < c.h; j++)
            for (uint32_t i = 0; i < c.w; i++)
                ((uint16_t *)(c.virt + j * c.stride))[i] = (j < 4 || j >= c.h - 4 || i < 4 || i >= c.w - 4) ? 0xffff : 0xfc00;
        r = r ? r : rgn_end(0);
        param[0] = 1;
        memcpy(param + 4, &x, 4);
        memcpy(param + 8, &y, 4);
        param[0x20] = 0x60;                       /* colour-invert luma threshold as cardv (invert off) */
        int a = r ? r : MI_RGN_AttachToChn(RGN_SOC, 0, port, param);
        FILE *f = fopen("/proc/mi_modules/mi_rgn/mi_rgn0", "w");
        if (f) {
            fprintf(f, "setRealtimeFlip %d %d %d %d %d", RGN_MOD_SCL, 0, 0, 1, 1);
            fclose(f);
        }
        printf("rgn: cardv order: draw/update %d, attach %d, realtime flip %s\n", r, a, f ? "set" : "failed");
        rgn_port = 1;
        shown = r == 0 && a == 0;
        return r | a;
    }
    int r = rgn_create(0, 100, 100, 256, 64, &c);
    r = r ? r : rgn_begin(0, &c);
    printf("rgn: canvas %ux%u stride %u fmt %u virt %p (rc %d)\n", c.w, c.h, c.stride, c.fmt, (void *)c.virt, r);
    if (r == 0 && c.virt) {
        for (uint32_t y = 0; y < c.h; y++)
            for (uint32_t x = 0; x < c.w; x++)
                ((uint16_t *)(c.virt + y * c.stride))[x] = (y < 4 || y >= c.h - 4 || x < 4 || x >= c.w - 4) ? 0xffff : 0xfc00;
        r = rgn_end(0);
        printf("rgn: update %d\n", r);
        if (on == 2) {                            /* re-enable the SCL port so its overlay hardware is set up */
            int d = MI_SCL_DisableOutputPort(0, 0, 1), e = MI_SCL_EnableOutputPort(0, 0, 1);
            printf("rgn: SCL port 1 disable %d enable %d\n", d, e);
        }
    }
    shown = r == 0;
    return r;
}
