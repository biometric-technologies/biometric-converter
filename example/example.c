#include <stdio.h>
#include <stdlib.h>
#include "converter.h"

// Converts ../sample_image.wsq to an ANSI INCITS 378 record in ../ansi.res;
// run it from a build directory inside example/.
int main(int argc, char *argv[]) {
    FILE *i_fp = fopen("./../sample_image.wsq", "rb");
    unsigned char *idata, *odata;
    int ilen, olen;

    if (i_fp == NULL) {
        perror("./../sample_image.wsq");
        return 1;
    }
    fseek(i_fp, 0, SEEK_END);
    ilen = ftell(i_fp);
    fseek(i_fp, 0, SEEK_SET);
    idata = (unsigned char *) malloc((ilen + 1) * sizeof(unsigned char));
    if (fread(idata, sizeof(unsigned char), ilen, i_fp) != (size_t) ilen) {
        perror("./../sample_image.wsq");
        return 1;
    }
    fclose(i_fp);

    if (img2fmr(idata, ilen, "ANSI", &odata, &olen) != 0) {
        fprintf(stderr, "conversion failed: %s\n", converter_last_error());
        free(idata);
        return 1;
    }
    free(idata);

    FILE *res = fopen("./../ansi.res", "wb");
    if (res == NULL) {
        perror("./../ansi.res");
        return 1;
    }
    fwrite(odata, 1, olen, res);
    fclose(res);
    converter_free(odata);
    printf("Wrote %d bytes to ../ansi.res\n", olen);
    return 0;
}
