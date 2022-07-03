#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lfs.h>
#include <imgdecod.h>
#include <stdint.h>
#include <biomdi.h>
#include <biomdimacro.h>
#include <fmr.h>
#include <fmr2fmr.h>
#include <time.h>
#include <sys/param.h>
#include <converter.h>
#include "img_io.h"

void
usage() {
    printf(
            "usage:\n\tconvert -i <imagefile> -o <resfile> -t <outtype> [-v] \n"
            "\t\t -i:  Specifies the input file (ANSI-381, WSQ, JPEG) \n"
            "\t\t -o:  Specifies the output file path\n"
            "\t\t -t:  Specifies the type of output file (available types: ISO, ISOC, ISOCC, ANSI)\n"
    );
}

void
get_options(int argc, char *argv[], char **in, char **out, char **type, int *validate) {
    char ch;
    *validate = 0;
    *in = "";
    *out = "";
    *type = "";
    while ((ch = getopt(argc, argv, "i:o:t:v")) != -1) {
        switch (ch) {
            case 'i':
                *in = malloc(strlen(optarg) + 1);
                strcpy(*in, optarg);
                break;
            case 'o':
                *out = malloc(strlen(optarg) + 1);
                strcpy(*out, optarg);
                break;
            case 't':
                *type = malloc(strlen(optarg) + 1);
                strcpy(*type, optarg);
                break;
            case 'v':
                *validate = 1;
                break;
            case '?':
            default:
                usage();
                break;
        }
    }

    if (strlen(*in) == 0 || strlen(*out) == 0 || strlen(*type) == 0) {
        usage();
        goto err_out;
    }
    return;

    err_out:
    exit(EXIT_FAILURE);
}


void write_data_to_file(FILE *fmr_fp, struct finger_minutiae_record *fmr) {
    if (write_fmr(fmr_fp, fmr) != 0) {
        fclose(fmr_fp);
        ERR_EXIT("Could not write finger minutiae record");
    }
    fclose(fmr_fp);
}

int main(int argc, char *argv[]) {
    char *input_file, *output_file, *output_type;
    int validate;

    get_options(argc, argv, &input_file, &output_file, &output_type, &validate);

    unsigned char *idata, *odata;
    int ilen, olen;

    if (read_raw_from_filesize(input_file, &idata, &ilen) != 0)
        OPEN_ERR_EXIT(input_file);

    /*
     * perform convert
     * */
    convert(idata, ilen, output_type, &odata, &olen);

    FILE *fmr_fp = NULL;
    if ((fmr_fp = fopen(output_file, "wb")) == NULL)
        OPEN_ERR_EXIT(output_file);

    int n;
    if ((n = fwrite(odata, 1, olen, fmr_fp)) != olen)
        ERR_EXIT("Could not write data to file");

    printf("Converting to [%s] successfully done\n", output_type);
    exit(EXIT_SUCCESS);
}