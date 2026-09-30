#ifndef CAMAPP_UVC_H
#define CAMAPP_UVC_H
#include <stdint.h>

/* Open the UVC gadget node and start answering the host. 0 on success. */
int uvc_start(const char *dev);
void uvc_stop(void);
/* Hand the newest JPEG (up to two parts) to the UVC sender. */
void uvc_put_frame(const void *p1, uint32_t n1, const void *p2, uint32_t n2);
extern long uvc_frames_sent;
/* Camera controls the host can read and set (implemented by the app, see cmd.c). */
enum { UVC_CTRL_AE_MODE, UVC_CTRL_BRIGHTNESS };
int uvc_control_get(int ctrl);
void uvc_control_set(int ctrl, int value);

#endif
