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
#include <img_io.h>

void
usage() {
    printf(
            "usage:\n\tconvert -i <input file> -ti <input type> -o <output file> -to <output type> [-v] \n"
            "\t\t -i:  Specifies the input file path \n"
            "\t\t -ti:  Specifies the type of the input file (image, 8bit depth only: WSQ, JPEG, IHEAD, JPEG2000, PNG or minutiae: ISO, ISOC, ISOCC, ANSI) \n"
            "\t\t -o:  Specifies the output file path\n"
            "\t\t -to:  Specifies the type of the output file (ISO, ISOC, ISOCC, ANSI)\n"
            "\t\t -v:  Validate output result [0 - validate, 1 - skip] (Optional, defaults to 0)\n"
    );
}

void
get_options(int argc, char *argv[], char **in, char **itype, char **out, char **otype, int *validate) {
    char ch;
    char nch;
    *validate = 0;
    *in = "";
    *out = "";
    *itype = "";
    *otype = "";
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
                nch = *(char *) optarg;
                switch (nch) {
                    case 'i':
                        *itype = malloc(strlen(optarg) + 1);
                        strcpy(*itype, argv[optind]);
                        break;
                    case 'o':
                        *otype = malloc(strlen(optarg) + 1);
                        strcpy(*otype, argv[optind]);
                        break;
                    default:
                        usage();
                        return;
                }
            case 'v':
                *validate = 1;
                break;
            case '?':
            default:
                usage();
                break;
        }
    }

    if (strlen(*in) == 0 || strlen(*out) == 0 || strlen(*itype) == 0 || strlen(*otype) == 0) {
        usage();
        goto err_out;
    }
    return;

    err_out:
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *input_file, *input_type, *output_file, *output_type;
    int validate;

    get_options(argc, argv, &input_file, &input_type, &output_file, &output_type, &validate);
    printf("input=%s, output=%s\n", input_type, output_type);

    unsigned char *idata, *odata;
    int ilen, olen;
    if (strcmp(input_type, "ANSI") == 0
        || strcmp(input_type, "ISO") == 0
        || strcmp(input_type, "ISONC") == 0
        || strcmp(input_type, "ISOCC") == 0) {

        FILE *i_fp = NULL;
        if ((i_fp = fopen(input_file, "rb")) == NULL)
            OPEN_ERR_EXIT(input_file);

        fseek(i_fp, 0, SEEK_END);
        ilen = ftell(i_fp);
        fseek(i_fp, 0, SEEK_SET);
        idata = (unsigned char *) malloc((ilen + 1) * sizeof(unsigned char));
        fread(idata, sizeof(unsigned char), ilen, i_fp);
        fclose(i_fp);

        fmr2fmr(idata, ilen, &odata, &olen, input_file, output_type);
    } else {

        if (read_raw_from_filesize(input_file, &idata, &ilen) != 0)
            OPEN_ERR_EXIT(input_file);
        img2fmr(idata, ilen, output_type, &odata, &olen);
    }

    FILE *fmr_fp = NULL;
    if ((fmr_fp = fopen(output_file, "wb")) == NULL)
        OPEN_ERR_EXIT(output_file);

    if ((fwrite(odata, 1, olen, fmr_fp)) != olen)
        ERR_EXIT("Could not write data to file");

    printf("Converting from [%s] to [%s] successfully done\n", input_type, output_type);
    exit(EXIT_SUCCESS);
}