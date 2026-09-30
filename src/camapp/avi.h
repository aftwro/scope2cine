#ifndef CAMAPP_AVI_H
#define CAMAPP_AVI_H
#include <stdint.h>

/* Minimal MJPEG AVI 1.0 writer. Frames are appended as '00dc' chunks; the
 * idx1 index and header sizes are written by avi_close(). A file cut short by
 * power loss still holds every frame written before the cut. */
typedef struct avi avi_t;

avi_t *avi_open(const char *path, uint32_t width, uint32_t height, uint32_t fps);
int avi_write_frame(avi_t *a, const void *p1, uint32_t n1, const void *p2, uint32_t n2);
uint64_t avi_bytes(const avi_t *a);
uint32_t avi_frames(const avi_t *a);
int avi_close(avi_t *a);

#endif
