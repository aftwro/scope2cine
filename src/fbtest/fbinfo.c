/* Dump SStar fb0 show / global alpha / colour key state. */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define FBIOGET_SHOW          0x80014662
#define FBIOGET_GLOBAL_ALPHA  0x80074664
#define FBIOGET_COLORKEY      0x80044666
int main(void)
{
    int fd = open("/dev/fb0", O_RDWR);
    uint8_t show = 0xff, ga[7] = { 0 }, ck[4] = { 0 };
    int r1 = ioctl(fd, FBIOGET_SHOW, &show), r2 = ioctl(fd, FBIOGET_GLOBAL_ALPHA, ga), r3 = ioctl(fd, FBIOGET_COLORKEY, ck);
    printf("show=%u (rc %d)\nglobal_alpha=%u-%u-%x-%x-%x-%x-%x (rc %d)\ncolorkey=%u %02x%02x%02x (rc %d)\n",
           show, r1, ga[0], ga[1], ga[2], ga[3], ga[4], ga[5], ga[6], r2, ck[0], ck[1], ck[2], ck[3], r3);
    close(fd);
    return 0;
}
