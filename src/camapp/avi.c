/* Minimal MJPEG AVI 1.0 writer (see avi.h). Little-endian host assumed (ARM). */
#include "avi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct avi {
    FILE *f;
    uint32_t fps, frames;
    long movi_list_pos;          /* offset of the 'LIST' size field of movi */
    long movi_fourcc_pos;        /* offset of the 'movi' fourcc (idx1 offsets are relative to it) */
    uint32_t *index;             /* per frame: offset, size */
    uint32_t index_cap;
    uint32_t max_frame;
};

/* Header field offsets, patched on close. */
enum { OFF_RIFF_SIZE = 4, OFF_AVIH_TOTAL_FRAMES = 48, OFF_AVIH_SUGGESTED_BUF = 60,
       OFF_STRH_LENGTH = 140, OFF_STRH_SUGGESTED_BUF = 144 };

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }
static void fourcc(FILE *f, const char *s) { fwrite(s, 4, 1, f); }

static void patch32(FILE *f, long pos, uint32_t v)
{
    long here = ftell(f);
    fseek(f, pos, SEEK_SET);
    put32(f, v);
    fseek(f, here, SEEK_SET);
}

avi_t *avi_open(const char *path, uint32_t w, uint32_t h, uint32_t fps)
{
    avi_t *a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;
    a->f = fopen(path, "wb");
    if (!a->f) {
        free(a);
        return NULL;
    }
    setvbuf(a->f, NULL, _IOFBF, 1 << 20);
    a->fps = fps;
    FILE *f = a->f;

    fourcc(f, "RIFF"); put32(f, 0); fourcc(f, "AVI ");
    fourcc(f, "LIST"); put32(f, 4 + 64 + 12 + 64 + 48); fourcc(f, "hdrl");
    fourcc(f, "avih"); put32(f, 56);
    put32(f, 1000000 / fps);     /* us per frame */
    put32(f, 0);                 /* max bytes/s */
    put32(f, 0);                 /* padding granularity */
    put32(f, 0x10);              /* AVIF_HASINDEX */
    put32(f, 0);                 /* total frames (patched) */
    put32(f, 0);                 /* initial frames */
    put32(f, 1);                 /* streams */
    put32(f, 0);                 /* suggested buffer (patched) */
    put32(f, w); put32(f, h);
    put32(f, 0); put32(f, 0); put32(f, 0); put32(f, 0);
    fourcc(f, "LIST"); put32(f, 4 + 64 + 48); fourcc(f, "strl");
    fourcc(f, "strh"); put32(f, 56);
    fourcc(f, "vids"); fourcc(f, "MJPG");
    put32(f, 0);                 /* flags */
    put16(f, 0); put16(f, 0);    /* priority, language */
    put32(f, 0);                 /* initial frames */
    put32(f, 1); put32(f, fps);  /* scale, rate */
    put32(f, 0);                 /* start */
    put32(f, 0);                 /* length (patched) */
    put32(f, 0);                 /* suggested buffer (patched) */
    put32(f, 0xffffffff);        /* quality */
    put32(f, 0);                 /* sample size */
    put16(f, 0); put16(f, 0); put16(f, w); put16(f, h);
    fourcc(f, "strf"); put32(f, 40);
    put32(f, 40); put32(f, w); put32(f, h);
    put16(f, 1); put16(f, 24);
    fourcc(f, "MJPG"); put32(f, w * h * 3);
    put32(f, 0); put32(f, 0); put32(f, 0); put32(f, 0);
    a->movi_list_pos = ftell(f) + 4;
    fourcc(f, "LIST"); put32(f, 0);
    a->movi_fourcc_pos = ftell(f);
    fourcc(f, "movi");
    return a;
}

int avi_write_frame(avi_t *a, const void *p1, uint32_t n1, const void *p2, uint32_t n2)
{
    uint32_t n = n1 + n2;
    if (a->frames == a->index_cap) {
        uint32_t cap = a->index_cap ? a->index_cap * 2 : 4096;
        uint32_t *idx = realloc(a->index, cap * 2 * sizeof(uint32_t));
        if (!idx)
            return -1;
        a->index = idx;
        a->index_cap = cap;
    }
    a->index[a->frames * 2] = ftell(a->f) - a->movi_fourcc_pos;
    a->index[a->frames * 2 + 1] = n;
    fourcc(a->f, "00dc");
    put32(a->f, n);
    if (fwrite(p1, 1, n1, a->f) != n1 || (n2 && fwrite(p2, 1, n2, a->f) != n2))
        return -1;
    if (n & 1)
        fputc(0, a->f);
    if (n > a->max_frame)
        a->max_frame = n;
    a->frames++;
    return 0;
}

uint64_t avi_bytes(const avi_t *a) { return ftell(a->f); }
uint32_t avi_frames(const avi_t *a) { return a->frames; }

int avi_close(avi_t *a)
{
    FILE *f = a->f;
    long movi_end = ftell(f);
    fourcc(f, "idx1");
    put32(f, a->frames * 16);
    for (uint32_t i = 0; i < a->frames; i++) {
        fourcc(f, "00dc");
        put32(f, 0x10);          /* AVIIF_KEYFRAME */
        put32(f, a->index[i * 2]);
        put32(f, a->index[i * 2 + 1]);
    }
    long end = ftell(f);
    patch32(f, a->movi_list_pos, movi_end - a->movi_list_pos - 4);
    patch32(f, OFF_RIFF_SIZE, end - 8);
    patch32(f, OFF_AVIH_TOTAL_FRAMES, a->frames);
    patch32(f, OFF_AVIH_SUGGESTED_BUF, a->max_frame + 8);
    patch32(f, OFF_STRH_LENGTH, a->frames);
    patch32(f, OFF_STRH_SUGGESTED_BUF, a->max_frame + 8);
    int rc = fclose(f);
    free(a->index);
    free(a);
    return rc;
}
