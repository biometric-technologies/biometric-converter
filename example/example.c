#include <sys/queue.h>
#include <stdio.h>
#include <malloc.h>
#include "converter.h"

int main(int argc, char *argv[]) {
    FILE *i_fp = fopen("./../sample_image.wsq", "rb");
    unsigned char *idata, *odata;
    int ilen, olen;
    fseek(i_fp, 0, SEEK_END);
    ilen = ftell(i_fp);
    fseek(i_fp, 0, SEEK_SET);
    idata = (unsigned char *) malloc((ilen + 1) * sizeof(unsigned char));
    fread(idata, sizeof(unsigned char), ilen, i_fp);
    fclose(i_fp);
    img2fmr(idata, ilen, "ANSI", &odata, &olen);
    FILE *res = fopen("./../ansi.res", "wb");
    fwrite(odata, 1, olen, res);
    return 0;
}