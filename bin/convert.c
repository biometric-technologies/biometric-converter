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
            "\t\t -ti:  Specifies the type of the input file (image, 8bit depth only: WSQ, JPEG, IHEAD, JPEG2000, PNG or minutiae: ISO, ISONC, ISOCC, ANSI) \n"
            "\t\t -o:  Specifies the output file path\n"
            "\t\t -to:  Specifies the type of the output file (ISO, ISONC, ISOCC, ANSI)\n"
            "\t\t -v:  Accepted for compatibility; has no effect\n"
            "\t\t ISONC and ISOCC input is taken to be at 500 ppi\n"
    );
}

void
get_options(int argc, char *argv[], char **in, char **itype, char **out, char **otype, int *validate) {
    int ch;
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
                // -ti and -to: getopt sees option 't' with the argument
                // 'i' or 'o'; the type is the next word
                nch = *(char *) optarg;
                if (optind >= argc) {
                    usage();
                    goto err_out;
                }
                switch (nch) {
                    case 'i':
                        *itype = argv[optind++];
                        break;
                    case 'o':
                        *otype = argv[optind++];
                        break;
                    default:
                        usage();
                        goto err_out;
                }
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
    int ret;

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
        if (idata == NULL)
            ERR_EXIT("Could not allocate memory for the input");
        if (fread(idata, sizeof(unsigned char), ilen, i_fp) != (size_t) ilen)
            ERR_EXIT("Could not read %s", input_file);
        fclose(i_fp);

        ret = fmr2fmr(idata, ilen, &odata, &olen, input_type, output_type);
    } else {

        if (read_raw_from_filesize(input_file, &idata, &ilen) != 0)
            OPEN_ERR_EXIT(input_file);
        ret = img2fmr(idata, ilen, output_type, &odata, &olen);
    }
    free(idata);

    if (ret != 0) {
        fprintf(stderr, "Converting from [%s] to [%s] failed: %s\n",
                input_type, output_type, converter_last_error());
        exit(EXIT_FAILURE);
    }

    FILE *fmr_fp = NULL;
    if ((fmr_fp = fopen(output_file, "wb")) == NULL)
        OPEN_ERR_EXIT(output_file);

    if ((fwrite(odata, 1, olen, fmr_fp)) != olen)
        ERR_EXIT("Could not write data to file");
    if (fclose(fmr_fp) != 0)
        ERR_EXIT("Could not write data to file");
    converter_free(odata);

    printf("Converting from [%s] to [%s] successfully done\n", input_type, output_type);
    exit(EXIT_SUCCESS);
}