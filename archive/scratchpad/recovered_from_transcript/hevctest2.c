#include <stdio.h>
#include <stdlib.h>
#include "hevc.h"
int main(int c, char **v) {  /* split the whole ES as one "pack" around its first SPS */
    FILE *f = fopen(v[1], "rb"); fseek(f, 0, 2); long n = ftell(f); rewind(f);
    unsigned char *in = malloc(n), sps[1024]; if (fread(in, 1, n, f) != (size_t)n) return 1;
    unsigned s0, s1;
    if (hevc_find_sps(in, n, 4096, &s0, &s1)) { puts("no sps"); return 1; }
    int m = hevc_crop_sps(in + s0, s1 - s0, sps, sizeof sps, 8);
    printf("sps at %u..%u -> %d B\n", s0, s1, m);
    FILE *o = fopen(v[2], "wb"); fwrite(in, 1, s0, o); fwrite(sps, 1, m, o); fwrite(in + s1, 1, n - s1, o); fclose(o);
}
