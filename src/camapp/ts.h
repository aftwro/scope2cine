#ifndef CAMAPP_TS_H
#define CAMAPP_TS_H
#include <stdint.h>

/* Minimal MPEG transport stream writer for one H.265 or H.264 video track (Annex-B access units
 * straight from the encoder). Every 188-byte packet stands on its own and PAT/PMT are repeated
 * before every keyframe, so a file cut short by power loss plays up to its last frame. */
typedef struct ts ts_t;

ts_t *ts_open(const char *path, int hevc);
/* One access unit, given as up to two parts (ring buffer wrap); pts in 90 kHz units. */
int ts_write_frame(ts_t *t, const void *p1, uint32_t n1, const void *p2, uint32_t n2, uint64_t pts, int key);
uint64_t ts_bytes(const ts_t *t);
uint32_t ts_frames(const ts_t *t);
int ts_close(ts_t *t);

#endif
