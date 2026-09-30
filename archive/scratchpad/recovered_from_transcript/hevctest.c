#include <stdio.h>
#include <stdlib.h>
#include "hevc.h"
int main(int c, char **v) {
    FILE *f = fopen(v[1], "rb"); fseek(f, 0, 2); long n = ftell(f); rewind(f);
    unsigned char *in = malloc(n), *out = malloc(n + 4096); fread(in, 1, n, f);
    int m = hevc_crop_sps(in, n, out, n + 4096, 8);
    printf("in %ld out %d\n", n, m);
    if (m > 0) fwrite(out, 1, m, fopen(v[2], "wb"));
}
