#ifndef CAMAPP_SENSORDBG_H
#define CAMAPP_SENSORDBG_H
#include <stdint.h>

/* Frame-rate debugging: prints the driver's view (MI_SNR), the IMX307 timing registers
 * read over I2C and the frame rate measured from VIF interrupts. */
void sensor_debug(const char *tag);

/* Real frame rate and exposure from the IMX307 registers (VMAX, HMAX, SHS1). 0 on success. */
int sensor_timing(double *fps, double *exposure_us);

/* Raw IMX307 timing/gain registers; values -1 when unreadable. */
/* Read n consecutive IMX307 registers from reg. 0 on success. */
int sensor_read_block(uint16_t reg, uint8_t *buf, int n);

/* Write one IMX307 register. 0 on success. The value lasts until the sensor is re-initialised
 * (camapp restart / power cycle); nothing is stored in flash. */
int sensor_write_reg(uint16_t reg, uint8_t val);

/* 1 if the register is on the known-safe write list (gain, black level, test pattern, hold). */
int sensor_reg_safe(uint16_t reg);

/* Short name of a known IMX307 register (see docs/imx307_registers.md), or "". */
const char *sensor_reg_name(uint16_t reg);

/* One-sentence description of a register for the on-screen help. Never NULL. */
const char *sensor_reg_desc(uint16_t reg);

/* Index of the first sensor mode whose description contains text, or -1. */
int sensor_find_mode(const char *text);

struct sensor_regs { long vmax, hmax, shs1, gain; double line_us, fps, exposure_us; };
int sensor_read_regs(struct sensor_regs *r);

#endif
