/* getexpo: read-only dump of the ISP auto-exposure state.
 * Calls only MI_ISP_AE_GetExpoMode and MI_ISP_AE_QueryExposureInfo (no setters).
 * Struct layouts come from Ghidra decompiles of cardv / libmi_isp.so:
 *   ExpoMode: one u32 (0 = auto, 4 = manual).
 *   ExposureInfo: 0x23c bytes; +0 stable, +4 reach boundary,
 *   +8 long-exposure value {FNx10, sensor gain, ISP gain, shutter us}.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int MI_ISP_AE_GetExpoMode(uint32_t dev, uint32_t chn, void *mode);
int MI_ISP_AE_QueryExposureInfo(uint32_t dev, uint32_t chn, void *info);

int main(int argc, char **argv)
{
    uint32_t dev = argc > 1 ? strtoul(argv[1], NULL, 0) : 0;
    uint32_t chn = argc > 2 ? strtoul(argv[2], NULL, 0) : 0;
    uint32_t mode = 0xffffffff;
    uint32_t info[0x23c / 4];

    memset(info, 0, sizeof(info));
    int r1 = MI_ISP_AE_GetExpoMode(dev, chn, &mode);
    int r2 = MI_ISP_AE_QueryExposureInfo(dev, chn, info);

    printf("dev %u chn %u: getmode rc=%d query rc=%d\n", dev, chn, r1, r2);
    printf("mode=%u (%s) stable=%u boundary=%u\n", mode,
           mode == 0 ? "auto" : mode == 4 ? "manual" : "?", info[0], info[1]);
    printf("FNx10=%u sensor_gain=%u (%.2fx) isp_gain=%u (%.2fx) shutter_us=%u\n",
           info[2], info[3], info[3] / 1024.0, info[4], info[4] / 1024.0, info[5]);
    printf("raw:");
    for (int i = 0; i < 12; i++)
        printf(" %u", info[i]);
    printf("\n");
    return 0;
}
