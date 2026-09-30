/* PC test: split an Annex-B H.265 stream into access units and mux them with ts.c.
 * New access unit at a VPS, or at a first-slice VCL NAL not preceded by VPS/SPS/PPS in this unit. */
#include "ts.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); rewind(f);
    unsigned char *b = malloc(n); if (fread(b, 1, n, f) != (size_t)n) return 1; fclose(f);
    ts_t *t = ts_open(argv[2], 1);
    long au = -1; int au_key = 0, au_has_vcl = 0; unsigned frame = 0;
    for (long i = 0; i + 6 <= n; i++) {
        int sc = i + 4 < n && b[i] == 0 && b[i+1] == 0 && b[i+2] == 0 && b[i+3] == 1;
        if (!sc && i + 6 < n) continue;
        int type = sc ? (b[i+4] >> 1) & 0x3f : -1;
        int first_slice = sc && type < 32 && (b[i+6] & 0x80);
        int boundary = !sc || type == 32 || (first_slice && au_has_vcl);
        if (boundary && au >= 0) {
            long end = sc ? i : n;
            ts_write_frame(t, b + au, end - au, NULL, 0, (uint64_t)frame * 1500, au_key);
            frame++; au = -1; au_key = 0; au_has_vcl = 0;
        }
        if (!sc) break;
        if (au < 0) au = i;
        if (type == 32 || type == 19 || type == 20) au_key = 1;
        if (type < 32) au_has_vcl = 1;
    }
    printf("frames %u bytes %llu close %d\n", frame, (unsigned long long)ts_bytes(t), ts_close(t));
    return 0;
}
