/* See sensordbg.h. Read-only: never writes sensor registers. */
#include "sensordbg.h"

#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int MI_SNR_GetFps(uint32_t pad, uint32_t *fps);
int MI_SNR_GetCurRes(uint32_t pad, uint8_t *idx, void *res);
int MI_SNR_QueryResCount(uint32_t pad, uint32_t *count);
int MI_SNR_GetRes(uint32_t pad, uint32_t idx, void *res);

/* MI_SNR resolution entry (0x34 bytes, layout from cardv's CarDV_SensorModuleInit). */
struct snr_res {
    uint16_t crop_x, crop_y, crop_w, crop_h;
    uint16_t out_w, out_h;
    uint32_t max_fps, min_fps;
    char desc[32];
};

#define IMX307_I2C_BUS "/dev/i2c-1"
#define IMX307_ADDR 0x1a               /* 7-bit (0x34 in 8-bit notation) */
#define PIXEL_CLOCK_MHZ 148.5          /* HMAX counts in this clock: 2200 x 1125 = 60 fps */

static int reg_read(int fd, uint16_t reg, uint8_t *val)
{
    uint8_t addr[2] = { reg >> 8, reg & 0xff };
    struct i2c_msg msgs[2] = {
        { .addr = IMX307_ADDR, .flags = 0, .len = 2, .buf = addr },
        { .addr = IMX307_ADDR, .flags = I2C_M_RD, .len = 1, .buf = val },
    };
    struct i2c_rdwr_ioctl_data x = { .msgs = msgs, .nmsgs = 2 };
    return ioctl(fd, I2C_RDWR, &x) == 2 ? 0 : -1;
}

static long reg_read_n(int fd, uint16_t reg, int bytes)        /* little-endian multi-byte */
{
    long v = 0;
    for (int i = 0; i < bytes; i++) {
        uint8_t b;
        if (reg_read(fd, reg + i, &b))
            return -1;
        v |= (long)b << (8 * i);
    }
    return v;
}

static long vif_isr(void)
{
    FILE *f = fopen("/proc/mi_modules/mi_vif/mi_vif0", "r");
    char line[512];
    long cnt = -1;
    int next = 0;
    while (f && fgets(line, sizeof(line), f)) {
        if (next) { sscanf(line, "%*d %ld", &cnt); break; }
        next = strstr(line, "IsrCnt") != NULL;
    }
    if (f)
        fclose(f);
    return cnt;
}

int sensor_timing(double *fps, double *exposure_us)
{
    int fd = open(IMX307_I2C_BUS, O_RDWR);
    if (fd < 0)
        return -1;
    long vmax = reg_read_n(fd, 0x3018, 3), hmax = reg_read_n(fd, 0x301c, 2), shs1 = reg_read_n(fd, 0x3020, 3);
    close(fd);
    if (vmax <= 0 || hmax <= 0 || shs1 < 0)
        return -1;
    vmax &= 0x3ffff;
    shs1 &= 0x3ffff;
    double line_us = hmax / PIXEL_CLOCK_MHZ;
    *fps = 1e6 / (vmax * line_us);
    *exposure_us = (vmax - shs1 - 1) * line_us;
    return 0;
}

int sensor_read_regs(struct sensor_regs *r)
{
    *r = (struct sensor_regs){ -1, -1, -1, -1, 0, 0, 0 };
    int fd = open(IMX307_I2C_BUS, O_RDWR);
    if (fd < 0)
        return -1;
    r->vmax = reg_read_n(fd, 0x3018, 3);
    r->hmax = reg_read_n(fd, 0x301c, 2);
    r->shs1 = reg_read_n(fd, 0x3020, 3);
    r->gain = reg_read_n(fd, 0x3014, 1);           /* 0.3 dB per step */
    close(fd);
    if (r->vmax <= 0 || r->hmax <= 0 || r->shs1 < 0)
        return -1;
    r->vmax &= 0x3ffff;
    r->shs1 &= 0x3ffff;
    r->line_us = r->hmax / PIXEL_CLOCK_MHZ;
    r->fps = 1e6 / (r->vmax * r->line_us);
    r->exposure_us = (r->vmax - r->shs1 - 1) * r->line_us;
    return 0;
}

/* Names from docs/imx307_registers.md; multi-byte values name each byte (L/M/H = low/mid/high). */
static const struct { uint16_t reg; const char *name; const char *desc; } reg_names[] = {
    { 0x3000, "STANDBY",
      "1 = sensor asleep (standby), 0 = running. Writing 1 freezes the picture." }, { 0x3001, "REGHOLD",
      "1 = hold: register changes wait, then all apply on the same frame when set back to 0." }, { 0x3002, "XMSTA",
      "0 = master: the sensor makes its own frame timing. 1 = slave: it waits for sync pulses the board never sends, so the picture stops." }, { 0x3003, "SW_RESET",
      "Software reset: every register goes back to Sony defaults (no clock setup, standby). Picture stops until camapp restarts." },
    { 0x3005, "ADBIT",
      "A/D depth: 0 = 10-bit, 1 = 12-bit. Must match 3129/317C/31EC, 3046 and the SoC format, else garbage colours." }, { 0x3007, "FLIP/WIN",
      "Bit 0 = vertical flip, bit 1 = mirror, bits 7-4 = window mode (0 1080p, 1 720p, 4 crop)." }, { 0x3009, "FRSEL/FDG",
      "Bits 1-0 = frame-rate base (1 = 60p timing, 2 = 30p). Bit 4 = conversion gain: 1 = HCG, lower noise at high gain (unverified)." }, { 0x300a, "BLKLEV L",
      "Black level, low byte. Offset added to every pixel so noise below black is kept. 60 in 10-bit, 240 in 12-bit." },
    { 0x300b, "BLKLEV H",
      "Black level, high bit (bit 0). Together with 300A: the black offset." }, { 0x300c, "WDMODE",
      "HDR switch: bit 0 = DOL-HDR on, bits 5-4 = exposures per frame (1 = two). 0x11 in HDR, 0 normal." }, { 0x3014, "GAIN",
      "Sensor gain in 0.3 dB steps: 0 = 1x, 20 = 2x, 40 = 4x, 80 = 16x. Up to 100 (30 dB) is analog, above is digital. AE rewrites it." }, { 0x3018, "VMAX L",
      "Frame length in lines, low byte. Frame length x line time = frame time. The driver sets it from the frame rate." },
    { 0x3019, "VMAX M",
      "Frame length (VMAX), middle byte. 1125 lines = 60p, 2250 = 30p, 2813 = 24p." }, { 0x301a, "VMAX H",
      "Frame length (VMAX), top 2 bits." }, { 0x301c, "HMAX L",
      "Line length in 148.5 MHz clocks, low byte. 2200 = 14.81 us per line (normal mode)." }, { 0x301d, "HMAX H",
      "Line length (HMAX), high byte." },
    { 0x3020, "SHS1 L",
      "Shutter start line, low byte. Exposure = (frame length - SHS1 - 1) lines: smaller = longer. In HDR: the short exposure." }, { 0x3021, "SHS1 M",
      "Shutter start line (SHS1), middle byte." }, { 0x3022, "SHS1 H",
      "Shutter start line (SHS1), top 2 bits." }, { 0x3024, "SHS2 L",
      "HDR: shutter start line of the long exposure, low byte (unverified)." },
    { 0x3025, "SHS2 M",
      "HDR: long exposure start line (SHS2), middle byte." }, { 0x3026, "SHS2 H",
      "HDR: long exposure start line (SHS2), top bits." }, { 0x3030, "RHS1 L",
      "HDR: readout offset of the short exposure, low byte. Also caps how long the short exposure can be (unverified)." }, { 0x3031, "RHS1 M",
      "HDR: short exposure readout offset (RHS1), middle byte." },
    { 0x3032, "RHS1 H",
      "HDR: short exposure readout offset (RHS1), top bits." }, { 0x303a, "WINWV_OB",
      "Number of optical-black (masked, always dark) lines sent before the image. Used for black clamping." }, { 0x303c, "WINPV L",
      "Crop window: first line (vertical start), low byte. Only used in window mode 4." }, { 0x303d, "WINPV H",
      "Crop window vertical start, high byte." },
    { 0x303e, "WINWV L",
      "Crop window height in lines, low byte." }, { 0x303f, "WINWV H",
      "Crop window height, high byte." }, { 0x3040, "WINPH L",
      "Crop window: first column (horizontal start), low byte." }, { 0x3041, "WINPH H",
      "Crop window horizontal start, high byte." },
    { 0x3042, "WINWH L",
      "Crop window width in pixels, low byte." }, { 0x3043, "WINWH H",
      "Crop window width, high byte." }, { 0x3046, "ODBIT",
      "Output bit depth: 0 = 10-bit, 1 = 12-bit. The SoC reads a fixed width: wrong = every pixel misread (garbage colours)." }, { 0x305c, "INCKSEL1",
      "PLL setup 1 for the 37.125 MHz crystal. Sets pixel, ADC and MIPI clocks; wrong = link to the SoC loses sync, picture stops." },
    { 0x305d, "INCKSEL2",
      "PLL setup 2 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." }, { 0x305e, "INCKSEL3",
      "PLL setup 3 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." }, { 0x305f, "INCKSEL4",
      "PLL setup 4 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." }, { 0x308c, "PGCTRL",
      "Test pattern: bit 0 = pattern on, bits 7-4 = pattern number 0-7 (colour bars, gradients)." },
    { 0x30f0, "GAIN1",
      "HDR: gain of the second exposure (unverified)." }, { 0x3129, "ADBIT1",
      "A/D tuning; must match the bit depth (0x1D 10-bit, 0x00 12-bit). Mismatch = banding or noise, not a stop." }, { 0x315e, "INCKSEL5",
      "PLL setup 5 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." }, { 0x3164, "INCKSEL6",
      "PLL setup 6 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." },
    { 0x317c, "ADBIT2",
      "A/D tuning 2: documented as 0x12 (10-bit) / 0x00 (12-bit), but this camera reads other values. Unverified." }, { 0x31ec, "ADBIT3",
      "A/D tuning 3: 0x37 in 10-bit, 0x0E in 12-bit." }, { 0x3405, "REPETIT",
      "MIPI bit rate per lane. The SoC receiver is set for one rate; a different rate means it can't decode anything." }, { 0x3407, "LANES",
      "MIPI lanes minus one (3 = 4 lanes). The board wires 4; fewer = data on the wrong lanes, garbage or no frames." },
    { 0x3414, "OPB_V",
      "Optical-black lines included in the output." }, { 0x3418, "Y_OUT L",
      "Output lines per frame, low byte. The SoC expects exactly this many (HDR: both exposures, 2426); wrong = torn or dropped frames." }, { 0x3419, "Y_OUT H",
      "Output lines per frame, high byte." }, { 0x3441, "CSI_DT",
      "MIPI data type tag (0x0A RAW10, 0x0C RAW12). The SoC keeps only packets with the expected tag: wrong = frames dropped." },
    { 0x3443, "CSI_LANE",
      "MIPI lane mode (3 = 4 lanes). Must match 3407 and the board wiring, else no frames." }, { 0x3444, "EXTCK L",
      "Input clock x 256 in MHz, low byte. The MIPI block times its signals from it; wrong = timing errors, sparkles or no frames." }, { 0x3445, "EXTCK H",
      "Input clock frequency, high byte (see 3444)." }, { 0x3472, "X_OUT L",
      "Output width, low byte (1952). Must match what the SoC receiver expects, else torn or dropped frames." },
    { 0x3473, "X_OUT H",
      "Output width, high byte." }, { 0x3480, "INCKSEL7",
      "PLL setup 7 (see 305C). Wrong = clocks off-spec, MIPI receiver can't lock, no frames. Restart fixes." },
};

const char *sensor_reg_name(uint16_t reg)
{
    for (unsigned i = 0; i < sizeof(reg_names) / sizeof(reg_names[0]); i++)
        if (reg_names[i].reg == reg)
            return reg_names[i].name;
    return "";
}

const char *sensor_reg_desc(uint16_t reg)
{
    for (unsigned i = 0; i < sizeof(reg_names) / sizeof(reg_names[0]); i++)
        if (reg_names[i].reg == reg)
            return reg_names[i].desc;
    return "No description: fixed tuning value or undocumented. Read only.";
}

int sensor_write_reg(uint16_t reg, uint8_t val)
{
    int fd = open(IMX307_I2C_BUS, O_RDWR);
    if (fd < 0)
        return -1;
    uint8_t data[3] = { reg >> 8, reg & 0xff, val };
    struct i2c_msg msg = { .addr = IMX307_ADDR, .flags = 0, .len = 3, .buf = data };
    struct i2c_rdwr_ioctl_data x = { .msgs = &msg, .nmsgs = 1 };
    int r = ioctl(fd, I2C_RDWR, &x) == 1 ? 0 : -1;
    close(fd);
    return r;
}

int sensor_reg_safe(uint16_t reg)
{
    return reg == 0x3001 || reg == 0x300a || reg == 0x300b || reg == 0x3014 || reg == 0x308c;
}

int sensor_read_block(uint16_t reg, uint8_t *buf, int n)
{
    int fd = open(IMX307_I2C_BUS, O_RDWR), r = fd < 0 ? -1 : 0;
    for (int i = 0; r == 0 && i < n; i++)
        r = reg_read(fd, reg + i, &buf[i]);
    if (fd >= 0)
        close(fd);
    return r;
}

int sensor_find_mode(const char *text)
{
    uint32_t count = 0;
    if (MI_SNR_QueryResCount(0, &count) != 0)
        return -1;
    for (uint32_t i = 0; i < count; i++) {
        struct snr_res r = { 0 };
        char desc[33] = { 0 };
        if (MI_SNR_GetRes(0, i, &r) == 0) {
            memcpy(desc, r.desc, 32);
            if (strstr(desc, text))
                return i;
        }
    }
    return -1;
}

void sensor_debug(const char *tag)
{
    /* 1. What the driver says */
    uint32_t fps = 0, count = 0;
    uint8_t idx = 0xff;
    struct snr_res cur = { 0 };
    int r1 = MI_SNR_GetFps(0, &fps), r2 = MI_SNR_GetCurRes(0, &idx, &cur), r3 = MI_SNR_QueryResCount(0, &count);
    printf("sensor[%s]: driver GetFps=%u (rc %d) | cur res #%u %ux%u fps %u..%u \"%.32s\" (rc %d) | %u modes (rc %d)\n",
           tag, fps, r1, idx, cur.out_w, cur.out_h, cur.min_fps, cur.max_fps, cur.desc, r2, count, r3);
    for (uint32_t i = 0; i < count && i < 8; i++) {
        struct snr_res r = { 0 };
        if (MI_SNR_GetRes(0, i, &r) == 0)
            printf("sensor[%s]:   mode %u: crop %u,%u %ux%u out %ux%u fps %u..%u \"%.32s\"\n", tag, i, r.crop_x,
                   r.crop_y, r.crop_w, r.crop_h, r.out_w, r.out_h, r.min_fps, r.max_fps, r.desc);
    }

    /* 2. What the sensor registers say */
    int fd = open(IMX307_I2C_BUS, O_RDWR);
    long vmax = -1, hmax = -1, shs1 = -1, frsel = -1;
    if (fd >= 0) {
        vmax = reg_read_n(fd, 0x3018, 3) & 0x3ffff;
        hmax = reg_read_n(fd, 0x301c, 2);
        shs1 = reg_read_n(fd, 0x3020, 3) & 0x3ffff;
        frsel = reg_read_n(fd, 0x3009, 1) & 0x3;
        close(fd);
    }
    if (vmax > 0 && hmax > 0) {
        double line_us = hmax / PIXEL_CLOCK_MHZ;
        double reg_fps = 1e6 / (vmax * line_us);
        double exp_us = (vmax - shs1 - 1) * line_us;
        printf("sensor[%s]: regs VMAX %ld HMAX %ld SHS1 %ld FRSEL %ld -> line %.2f us, %.2f fps, exposure %.0f us\n",
               tag, vmax, hmax, shs1, frsel, line_us, reg_fps, exp_us);
    } else {
        printf("sensor[%s]: regs unreadable (bus %s addr 0x%02x)\n", tag, IMX307_I2C_BUS, IMX307_ADDR);
    }

    /* 3. What actually happens */
    long a = vif_isr();
    usleep(2000000);
    long b = vif_isr();
    printf("sensor[%s]: measured VIF %.1f frames/s over 2 s\n", tag, (b - a) / 2.0);
}
