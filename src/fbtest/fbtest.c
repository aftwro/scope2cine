/* OSD probe: make /dev/fb0 visible over the video with colour key 0x0a0a0a
 * (transparent), then draw a test box. Usage: fbtest [clear]
 * ioctl codes and colour key taken from the stock UI (gkuui). */
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define FBIOSET_SHOW      0x40014663
#define FBIOSET_COLORKEY  0x40044667     /* {enable, r, g, b} */
#define KEY 0xff0a0a0a                   /* ARGB: this colour is transparent */

int main(int argc, char **argv)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) { perror("open /dev/fb0"); return 1; }
    struct fb_var_screeninfo v;
    struct fb_fix_screeninfo fx;
    ioctl(fd, FBIOGET_VSCREENINFO, &v);
    ioctl(fd, FBIOGET_FSCREENINFO, &fx);
    printf("fb0: %ux%u %u bpp, line %u bytes, smem %u\n", v.xres, v.yres, v.bits_per_pixel, fx.line_length, fx.smem_len);

    uint8_t show = 1, key[4] = { 1, 0x0a, 0x0a, 0x0a };
    printf("SET_SHOW = %d, SET_COLORKEY = %d\n", ioctl(fd, FBIOSET_SHOW, &show), ioctl(fd, FBIOSET_COLORKEY, key));

    uint32_t *fb = mmap(NULL, fx.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); return 1; }
    uint32_t stride = fx.line_length / 4;
    for (uint32_t y = 0; y < v.yres; y++)
        for (uint32_t x = 0; x < v.xres; x++)
            fb[y * stride + x] = KEY;
    if (argc < 2 || strcmp(argv[1], "clear")) {
        for (uint32_t y = 40; y < 140; y++)                 /* red box with white frame */
            for (uint32_t x = 40; x < 300; x++)
                fb[y * stride + x] = (y < 44 || y >= 136 || x < 44 || x >= 296) ? 0xffffffff : 0xffd01010;
        printf("drew test box at (40,40) 260x100\n");
    }
    printf("holding /dev/fb0 open for 60 s\n");
    fflush(stdout);
    sleep(60);
    munmap(fb, fx.smem_len);
    close(fd);
    return 0;
}
