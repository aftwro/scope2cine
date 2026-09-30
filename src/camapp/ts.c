/* MPEG-TS writer (see ts.h). One program, PMT on PID 0x1000, video on PID 0x100 which also
 * carries the PCR. PES packets are unbounded (length 0), PTS only (the encoder uses no B-frames). */
#include "ts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TS_PACKET 188
#define PID_PAT 0x0000
#define PID_PMT 0x1000
#define PID_VIDEO 0x0100
#define PCR_LEAD 9000               /* PCR runs 100 ms ahead of the PTS it belongs to */

struct ts {
    FILE *f;
    uint8_t stream_type;            /* 0x24 H.265, 0x1b H.264 */
    uint8_t cc_pat, cc_pmt, cc_video;
    uint64_t bytes;
    uint32_t frames;
    int error;
};

static uint32_t crc32_mpeg(const uint8_t *p, int n)
{
    uint32_t c = 0xffffffff;
    for (int i = 0; i < n; i++) {
        c ^= (uint32_t)p[i] << 24;
        for (int b = 0; b < 8; b++)
            c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : c << 1;
    }
    return c;
}

static void put_packet(ts_t *t, const uint8_t *pkt)
{
    if (fwrite(pkt, 1, TS_PACKET, t->f) != TS_PACKET)
        t->error = 1;
    t->bytes += TS_PACKET;
}

/* PSI section (without CRC) in one packet: pointer field 0, section, CRC, 0xff fill. */
static void put_psi(ts_t *t, uint16_t pid, uint8_t *cc, uint8_t *sec, int len)
{
    uint8_t pkt[TS_PACKET];
    memset(pkt, 0xff, sizeof(pkt));
    uint32_t crc = crc32_mpeg(sec, len);
    sec[len] = crc >> 24;
    sec[len + 1] = crc >> 16;
    sec[len + 2] = crc >> 8;
    sec[len + 3] = crc;
    pkt[0] = 0x47;
    pkt[1] = 0x40 | pid >> 8;
    pkt[2] = pid & 0xff;
    pkt[3] = 0x10 | (*cc)++ % 16;
    pkt[4] = 0;
    memcpy(pkt + 5, sec, len + 4);
    put_packet(t, pkt);
}

static void put_tables(ts_t *t)
{
    uint8_t pat[16] = { 0x00, 0xb0, 13, 0x00, 0x01, 0xc1, 0, 0, 0x00, 0x01, 0xe0 | PID_PMT >> 8, PID_PMT & 0xff };
    put_psi(t, PID_PAT, &t->cc_pat, pat, 12);
    uint8_t pmt[21] = { 0x02, 0xb0, 18, 0x00, 0x01, 0xc1, 0, 0, 0xe0 | PID_VIDEO >> 8, PID_VIDEO & 0xff, 0xf0, 0,
                        t->stream_type, 0xe0 | PID_VIDEO >> 8, PID_VIDEO & 0xff, 0xf0, 0 };
    put_psi(t, PID_PMT, &t->cc_pmt, pmt, 17);
}

ts_t *ts_open(const char *path, int hevc)
{
    ts_t *t = calloc(1, sizeof(*t));
    if (!t)
        return NULL;
    t->f = fopen(path, "wb");
    if (!t->f) {
        free(t);
        return NULL;
    }
    setvbuf(t->f, NULL, _IOFBF, 256 * 1024);
    t->stream_type = hevc ? 0x24 : 0x1b;
    return t;
}

/* Copy n bytes of the PES (header, then part 1, then part 2) starting at offset pos. */
static void pes_copy(uint8_t *dst, uint32_t pos, uint32_t n, const uint8_t *hdr, uint32_t nh,
                     const uint8_t *p1, uint32_t n1, const uint8_t *p2)
{
    while (n) {
        const uint8_t *src;
        uint32_t avail;
        if (pos < nh) {
            src = hdr + pos;
            avail = nh - pos;
        } else if (pos < nh + n1) {
            src = p1 + (pos - nh);
            avail = nh + n1 - pos;
        } else {
            src = p2 + (pos - nh - n1);
            avail = n;
        }
        uint32_t k = avail < n ? avail : n;
        memcpy(dst, src, k);
        dst += k;
        pos += k;
        n -= k;
    }
}

int ts_write_frame(ts_t *t, const void *p1, uint32_t n1, const void *p2, uint32_t n2, uint64_t pts, int key)
{
    if (key)
        put_tables(t);
    pts += PCR_LEAD;                                    /* keep the PCR (pts - lead) positive */
    uint8_t hdr[14] = { 0, 0, 1, 0xe0, 0, 0, 0x80, 0x80, 5,
                        0x21 | (uint8_t)(pts >> 29 & 0x0e), (uint8_t)(pts >> 22), 0x01 | (uint8_t)(pts >> 14 & 0xfe),
                        (uint8_t)(pts >> 7), 0x01 | (uint8_t)(pts << 1 & 0xfe) };
    uint32_t total = sizeof(hdr) + n1 + n2, pos = 0;
    int first = 1;
    while (pos < total) {
        uint8_t pkt[TS_PACKET];
        int pcr = first && key;
        int af = pcr ? 8 : 0;                           /* adaptation field bytes incl. its length byte */
        uint32_t room = 184 - af, left = total - pos;
        if (left < room)
            af = 184 - left;                            /* pad the last packet via the adaptation field */
        uint32_t n = 184 - af;
        pkt[0] = 0x47;
        pkt[1] = (first ? 0x40 : 0) | PID_VIDEO >> 8;
        pkt[2] = PID_VIDEO & 0xff;
        pkt[3] = (af ? 0x30 : 0x10) | t->cc_video++ % 16;
        if (af) {
            pkt[4] = af - 1;
            if (af > 1) {
                memset(pkt + 5, 0xff, af - 1);
                pkt[5] = pcr ? 0x50 : 0x00;             /* random access + PCR on keyframes */
                if (pcr) {
                    uint64_t base = pts - PCR_LEAD;
                    pkt[6] = base >> 25;
                    pkt[7] = base >> 17;
                    pkt[8] = base >> 9;
                    pkt[9] = base >> 1;
                    pkt[10] = (base & 1) << 7 | 0x7e;
                    pkt[11] = 0;
                }
            }
        }
        pes_copy(pkt + 4 + af, pos, n, hdr, sizeof(hdr), p1, n1, p2);
        put_packet(t, pkt);
        pos += n;
        first = 0;
    }
    t->frames++;
    return t->error ? -1 : 0;
}

uint64_t ts_bytes(const ts_t *t) { return t->bytes; }
uint32_t ts_frames(const ts_t *t) { return t->frames; }

int ts_close(ts_t *t)
{
    int r = t->error;
    if (fflush(t->f) != 0)
        r = 1;
    if (fclose(t->f) != 0)
        r = 1;
    free(t);
    return r ? -1 : 0;
}
