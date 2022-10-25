#include "converter.h"
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
#include <imgutil.h>
#include <png_dec.h>
#include <jpeg2k.h>

int debug = 0;

// Append a field to a Type-1 record, branching to 'err_out' on error.
#define APPEND_TYPE1_FIELD(record, fieldname, value)            \
    do {                                \
        FIELD *__field = NULL;                    \
        if(value2field(&__field, TYPE_1_ID, fieldname, value) != 0)\
            goto err_out;                    \
        if(append_ANSI_NIST_record(record, __field) != 0)    \
            goto err_out;                    \
    } while (0)

#define APPEND_TYPE2_FIELD(record, fieldname, value)            \
    do {                                \
        FIELD *__field = NULL;                    \
        if(value2field(&__field, TYPE_2_ID, fieldname, value) != 0)\
            goto err_out;                    \
        if(append_ANSI_NIST_record(record, __field) != 0)    \
            goto err_out;                    \
    } while (0)

#define APPEND_TYPE9_FIELD(record, fieldname, value)            \
    do {                                \
        FIELD *__field = NULL;                    \
        if(value2field(&__field, TYPE_9_ID, fieldname, value) != 0)\
            goto err_out;                    \
        if(append_ANSI_NIST_record(record, __field) != 0)    \
            goto err_out;                    \
    } while (0)

// The maximum field size for a Type-2 record. This value is specified in
// ELECTRONIC FINGERPRINT TRANSMISSION SPECIFICATION (CJIS-RS-0010),
// January 1999.
#define MAX_TYPE2_FIELD_SIZE    120

void
convert_xy(unsigned short x_size, unsigned short y_size,
           unsigned short x_res, unsigned short y_res,
           unsigned short ansi_x, unsigned short ansi_y,
           unsigned short *fmr_x, unsigned short *fmr_y) {
    float factor, ty;

    // Resolution in FMR is expressed as pix/cm; AN2K uses pix/mm. Also,
    // the AN2K coordinate is a distance from the origin in .01mm units.
    // To convert from AN2K distance to pixel, convert pix/mm to pix/cm,
    // then factor out the .01mm units. This calculation is the same as
    // dividing the FMR resolution by 1000.
    if (x_res != 0) {
        factor = (float) x_res / 1000;
        *fmr_x = (unsigned short) (((float) ansi_x * factor) + 0.5);
    } else
        *fmr_x = 0;

    if (y_res != 0) {
        // Because the coordinate system is based at 0, subtract one
        // from size
        factor = (float) y_res / 1000;
        ty = (float) ansi_y * factor;
        *fmr_y = (unsigned short) (y_size - 1 - ty);
    } else
        *fmr_y = 0;

}

void
convert_theta(unsigned int ansi_theta, unsigned char *fmr_theta) {
    unsigned int ival;

    // FMR angles are in increments of 2, so 45 = 90 degrees.
    // Also, FMR angles are formed in the opposite manner as AN2K, so
    // flip the angle by 180 degrees.
    ival = ((ansi_theta + 180) % 360) / 2;
    *fmr_theta = (unsigned char) ival;
}

void
convert_quality(int ansi_qual, unsigned char *fmr_qual) {
    // XXX implement
    *fmr_qual = (unsigned char) ansi_qual;
}


void
convert_type(char ansi_type, unsigned char *fmr_type) {
    switch (ansi_type) {
        case 'A' :
            *fmr_type = FMD_MINUTIA_TYPE_RIDGE_ENDING;
            break;

        case 'B' :
            *fmr_type = FMD_MINUTIA_TYPE_BIFURCATION;
            break;

        default :
            *fmr_type = FMD_MINUTIA_TYPE_OTHER;
            break;
    }
}

void
convert_type4_ISR(RECORD *rec, unsigned short *x_res, unsigned short *y_res) {
    FIELD *field;
    int val;
    int idx;
    float f;

    if (lookup_ANSI_NIST_field(&field, &idx, ISR_ID, rec) == FALSE)
        ERR_OUT("Lookup of Type-4 ISR_ID");
    val = strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if ((val != 0) && (val != 1))
        ERR_OUT("Type-4 ISR is invalid");

    switch (val) {
        case 0:            // minimum resolution
            f = MIN_RESOLUTION * 10;    // px/mm -> px/cm
            *x_res = (unsigned short) (f + 0.5);
            f = MIN_RESOLUTION * 10;    // px/mm -> px/cm
            *y_res = (unsigned short) (f + 0.5);
            break;
            // May want to ERR out here
        default:
            *x_res = 0;
            *y_res = 0;
            break;
    }

    err_out:
    return;
}

void
convert_type13_SLC(RECORD *rec, unsigned short *x_res, unsigned short *y_res) {
    FIELD *field;
    int val;
    int hps, vps;
    int idx;
    float f;

    if (lookup_ANSI_NIST_field(&field, &idx, SLC_ID, rec) == FALSE)
        ERR_OUT("Lookup of SLC_ID");
    val = strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if ((val == 1) || (val == 2)) {
        if (lookup_ANSI_NIST_field(&field, &idx, HPS_ID, rec) == FALSE)
            ERR_OUT("Lookup of HPS_ID");
        hps = strtol((char *) field->subfields[0]->items[0]->value,
                     NULL, 10);
        if (lookup_ANSI_NIST_field(&field, &idx, VPS_ID, rec) == FALSE)
            ERR_OUT("Lookup of VPS_ID");
        vps = strtol((char *) field->subfields[0]->items[0]->value,
                     NULL, 10);
    }

    switch (val) {
        case 1:            // pixels/inch
            f = (float) hps / 2.54;
            *x_res = (unsigned short) f;
            f = (float) vps / 2.54;
            *y_res = (unsigned short) f;
            break;
        case 2:            // pixels/cm
            *x_res = hps;
            *y_res = vps;
            break;
            // May want to ERR out here
        default:
            *x_res = 0;
            *y_res = 0;
            break;
    }

    err_out:
    return;
}

int
set_fmr_img(struct finger_minutiae_record *fmr, RECORD *rec) {
    FIELD *field;
    int idx;
    int factor;

    // Some image record types give resolution as half the minimum or
    // native, so set a factor for the resolution
    switch (rec->type) {
        case TYPE_3_ID:
        case TYPE_5_ID:
            factor = 2;
            break;
        default:
            factor = 1;
            break;
    }

    switch (rec->type) {
        case TYPE_13_ID:
            convert_type13_SLC(rec, &fmr->x_resolution, &fmr->y_resolution);
            break;
        case TYPE_4_ID:
            convert_type4_ISR(rec, &fmr->x_resolution, &fmr->y_resolution);
            break;
        default:
            // XXX: implement
            fmr->x_resolution = 0;
            fmr->y_resolution = 0;
            break;
    }

    if (lookup_ANSI_NIST_field(&field, &idx, HLL_ID, rec) == FALSE)
        ERR_OUT("Lookup of field HLL_ID");
    fmr->x_image_size =
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if (lookup_ANSI_NIST_field(&field, &idx, VLL_ID, rec) == FALSE)
        ERR_OUT("Lookup of VLL_ID");
    fmr->y_image_size =
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);;

    return 0;

    err_out:
    return -1;
}

int
init_fmr(struct finger_minutiae_record *fmr, ANSI_NIST *ansi_nist, int idc) {
    RECORD *rec;
    int idx;
    int ret;

    strcpy(fmr->format_id, FMR_FORMAT_ID);
    strcpy(fmr->spec_version, FMR_ANSI_SPEC_VERSION);
    fmr->record_length = FMR_ANSI_SMALL_HEADER_LENGTH;
    fmr->record_length_type = FMR_ANSI_SMALL_HEADER_TYPE;
    fmr->product_identifier_owner = 1; // XXX: replace with something valid?
    fmr->product_identifier_type = 1; // XXX: replace with something valid?
    fmr->scanner_id = 0;
    fmr->compliance = 0;

    ret = lookup_fingerprint_with_IDC(&rec, &idx, idc, 1, ansi_nist);
    if (ret < 0)
        ERR_OUT("System error locating image record");
    if (ret == TRUE) {
        if (set_fmr_img(fmr, rec) != 0)
            ERR_OUT("Initializing FMR image info");
    } else {
        fmr->x_image_size = 0;
        fmr->y_image_size = 0;
        fmr->x_resolution = 0;
        fmr->y_resolution = 0;
    }

    fmr->num_views = 0;

    return 0;

    err_out:
    return -1;
}

int
init_fvmr(struct finger_view_minutiae_record *fvmr, RECORD *anrecord) {
    int idx;
    int subfield, item;
    unsigned short x, y, q;
    int tval;
    char buf[8];
    struct finger_minutiae_data *fmd;
    struct finger_extended_data *fed;
    struct finger_extended_data_block *fedb;
    struct ridge_count_data *rcd;
    struct core_data *cd;
    struct delta_data *dd;
    int have_fedb = 0;    /* So we have only one extended data block */
    int have_rcdb = 0;
    int have_cddb = 0;
    FIELD *field;

    /*
     * Create an array of counts for the finger positions; used to
     * maintain the view number for the finger across calls.
     */
    static int fgp_view[MAX_TABLE_6_CODE] = {0}; // from an2k.h

    /*** Finger number                 ***/
    if (lookup_ANSI_NIST_field(&field, &idx, FGP2_ID, anrecord) == FALSE)
        ERR_OUT("FGP field not found");
    fvmr->finger_number = (unsigned char)
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    /*** View number/impression type    ***/
    // XXX: Check finger_number in range
    fvmr->view_number = (unsigned char) fgp_view[fvmr->finger_number];
    fgp_view[fvmr->finger_number]++;

    if (lookup_ANSI_NIST_field(&field, &idx, IMP_ID, anrecord) == FALSE)
        ERR_OUT("IMP_ID field not found");
    fvmr->impression_type = (unsigned char)
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    // What it should be set to?
    fvmr->impression_type = 0;

    /*** Finger quality                 ***/
    // XXX: What should the overall finger quality be set to?
    fvmr->finger_quality = 0;

    /* Add the core records */
    fed = NULL;
    if (lookup_ANSI_NIST_field(&field, &idx, CRP_ID, anrecord) == TRUE) {
        if (have_fedb == 0) {
            if (new_fedb(FMR_STD_ANSI, &fedb) != 0)
                ALLOC_ERR_OUT("Extended Data Block");
            have_fedb = 1;
        }
        /* The FEDB length does NOT include the
         * block length field itself, so we add the
         * the new core/delta data block length only.
         */
        if (have_cddb == 0) {
            if (new_fed(FMR_STD_ANSI, &fed, FED_CORE_AND_DELTA,
                        FED_HEADER_LENGTH) != 0)
                ALLOC_ERR_EXIT("Extended Data record");
            have_cddb = 1;
        }
        fed->length += CORE_DATA_HEADER_LENGTH;

        for (subfield = 0; subfield < field->num_subfields;
             subfield++) {
            if (new_cd(FMR_STD_ANSI, &cd) != 0)
                ALLOC_ERR_EXIT("Core Data");
            /* The x,y coordinates are strung together;
             * separate them. */
            memcpy(buf,
                   field->subfields[subfield]->items[0]->value, 4);
            buf[4] = '\0';
            cd->x_coord =
                    (unsigned short) strtoul(buf, (char **) NULL, 10);
            memcpy(buf,
                   &field->subfields[subfield]->items[0]->value[4], 4);
            buf[4] = '\0';
            cd->y_coord =
                    (unsigned short) strtoul(buf, (char **) NULL, 10);
            /* No angle in AN2K; leave at default of 0 */
            fed->length += CORE_DATA_MIN_LENGTH;
            fed->cddb->num_cores++;
            add_cd_to_cddb(cd, fed->cddb);
        }
    }
    /* Add the delta records */
    if (lookup_ANSI_NIST_field(&field, &idx, DLT_ID, anrecord) == TRUE) {
        if (have_fedb == 0) {
            if (new_fedb(FMR_STD_ANSI, &fedb) != 0)
                ALLOC_ERR_OUT("Extended Data Block");
            have_fedb = 1;
        }
        /* The FEDB length does NOT include the
         * block length field itself, so we add the
         * the new core/delta data block length only.
         */
        if (have_cddb == 0) {
            if (new_fed(FMR_STD_ANSI, &fed, FED_CORE_AND_DELTA,
                        FED_HEADER_LENGTH) != 0)
                ALLOC_ERR_EXIT("Extended Data record");
            have_cddb = 1;
        }
        fed->length += DELTA_DATA_HEADER_LENGTH;

        for (subfield = 0; subfield < field->num_subfields;
             subfield++) {
            if (new_dd(FMR_STD_ANSI, &dd) != 0)
                ALLOC_ERR_EXIT("Delta Data");
            memcpy(buf,
                   field->subfields[subfield]->items[0]->value, 4);
            buf[4] = '\0';
            dd->x_coord =
                    (unsigned short) strtoul(buf, (char **) NULL, 10);
            memcpy(buf,
                   &field->subfields[subfield]->items[0]->value[4], 4);
            buf[4] = '\0';
            dd->y_coord =
                    (unsigned short) strtoul(buf, (char **) NULL, 10);
            /* No angles in AN2K; leave at default of 0 */
            fed->length += DELTA_DATA_MIN_LENGTH;
            fed->cddb->num_deltas++;
            add_dd_to_cddb(dd, fed->cddb);
        }
    }
    if (have_cddb) {
        fedb->block_length += fed->length;
        add_fed_to_fedb(fed, fedb);
    }

    /*** Number of minutiae             ***/
    if (lookup_ANSI_NIST_field(&field, &idx, MIN_ID, anrecord) == FALSE)
        ERR_OUT("Number of minutiae field not found");
    tval = (int) strtol((char *) field->subfields[0]->items[0]->value,
                        NULL, 10);
    if (tval > FMR_MAX_NUM_MINUTIAE) {
        tval = FMR_MAX_NUM_MINUTIAE;
    }
    fvmr->number_of_minutiae = (unsigned char) tval;

    /*** Finger minutiae data           ***/
    if (lookup_ANSI_NIST_field(&field, &idx, MRC_ID, anrecord) == FALSE)
        ERR_OUT("Minutiae and ridge count data field not found");

    /* For each minutiae index number, create the minutiae data records */
    for (subfield = 0; subfield < fvmr->number_of_minutiae; subfield++) {
        if (new_fmd(FMR_STD_ANSI, &fmd, subfield) != 0)
            ALLOC_ERR_OUT("finger minutiae data record");

        /* The x,y,theta values are in the second item,
         * strung together; separate them.
         */
        memcpy(buf, field->subfields[subfield]->items[1]->value, 4);
        buf[4] = '\0';
        x = (unsigned short) strtoul(buf, (char **) NULL, 10);

        memcpy(buf, &field->subfields[subfield]->items[1]->value[4], 4);
        buf[4] = '\0';
        y = (unsigned short) strtoul(buf, (char **) NULL, 10);

        convert_xy(fvmr->fmr->x_image_size, fvmr->fmr->y_image_size,
                   fvmr->fmr->x_resolution, fvmr->fmr->y_resolution,
                   x, y,
                   &fmd->x_coord, &fmd->y_coord);

        memcpy(buf, &field->subfields[subfield]->items[1]->value[8], 3);
        buf[3] = '\0';
        convert_theta(strtoul(buf, (char **) NULL, 10), &fmd->angle);

        q = (unsigned short) strtoul(
                (char *) field->subfields[subfield]->items[2]->value,
                (char **) NULL, 10);
        convert_quality(q, &fmd->quality);

        convert_type(field->subfields[subfield]->items[3]->value[0],
                     &fmd->type);

        /* Ridge count data is stored as items 5 .. num_items in
         * 'second-index,count' format. The first index is stored
         * in the first item of the field.
         */
        if (field->subfields[subfield]->num_items > 4) {
            if (have_fedb == 0) {
                if (new_fedb(FMR_STD_ANSI, &fedb) != 0)
                    ALLOC_ERR_OUT("Extended Data Block");
                have_fedb = 1;
            }
            /* The FEDB length does NOT include the
             * block length field itself, so we add the
             * the new ridge count data block length only.
             */
            if (have_rcdb == 0) {
                if (new_fed(FMR_STD_ANSI, &fed, FED_RIDGE_COUNT,
                            FED_HEADER_LENGTH) != 0)
                    ALLOC_ERR_EXIT("Extended Data record");
                have_rcdb = 1;
                fed->length += RIDGE_COUNT_HEADER_LENGTH;
                // XXX Set fed->rcdb->method
            }

            for (item = 4;
                 item < field->subfields[subfield]->num_items;
                 item++) {
                char *c;
                if (new_rcd(&rcd) != 0)
                    ALLOC_ERR_EXIT("Ridge Count Data");
                rcd->index_one = (unsigned short) strtoul(
                        (char *) field->subfields[subfield]->items[0]->value,
                        (char **) NULL, 10);
                c = strtok(
                        (char *) field->subfields[subfield]->items[item]->value,
                        ",");
                rcd->index_two = (unsigned short) strtoul(c,
                                                          (char **) NULL, 10);
                c = strtok(NULL, ",");
                rcd->count = (unsigned short) strtoul(c,
                                                      (char **) NULL, 10);
                fed->length += RIDGE_COUNT_DATA_LENGTH;
                add_rcd_to_rcdb(rcd, fed->rcdb);
            }
        }
        add_fmd_to_fvmr(fmd, fvmr);
    }
    if (have_rcdb) {
        fedb->block_length += fed->length;
        add_fed_to_fedb(fed, fedb);
    }

    /* There is only one extended data block per FVMR */
    if (have_fedb)
        add_fedb_to_fvmr(fedb, fvmr);

    return 0;

    err_out:
    //XXX free memory for FEDs, FEDBs, RCDBs
    return -1;
}

void ansi2fmr(ANSI_NIST *ansi_nist, struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr,
              int ppi) {

    FIELD *field;
    int i;
    int idc;
    int idx;

    for (i = 1; i < ansi_nist->num_records; i++) {
        if (ansi_nist->records[i]->type == TYPE_9_ID) {

            /*** Image designation character   ***/
            if (lookup_ANSI_NIST_field(&field, &idx, IDC_ID,
                                       ansi_nist->records[i]) == FALSE)
                ERR_OUT("IDC field not found");
            idc = strtol((char *) field->subfields[0]->items[0]->value,
                         NULL, 10);

            if (new_fmr(FMR_STD_ANSI, fmr) != 0)
                ALLOC_ERR_EXIT("FMR");

            if (init_fmr(*fmr, ansi_nist, idc) != 0)
                ERR_OUT("Initializing FMR");

            if (new_fvmr(FMR_STD_ANSI, fvmr) != 0)
                ALLOC_ERR_EXIT("FVMR");
            add_fvmr_to_fmr(*fvmr, *fmr);

            if (init_fvmr(*fvmr, ansi_nist->records[i]) != 0)
                ERR_OUT("Could not convert Type-9 record");

            (*fmr)->x_resolution = ppi;
            (*fmr)->y_resolution = ppi;
            (*fmr)->num_views++;
            (*fmr)->record_length += FVMR_HEADER_LENGTH +
                                     (FMD_DATA_LENGTH * (*fvmr)->number_of_minutiae);
            if ((*fvmr)->extended != NULL)
                (*fmr)->record_length += FEDB_HEADER_LENGTH +
                                         (*fvmr)->extended->block_length;
        }
    }
    return;

    err_out:
    exit(EXIT_FAILURE);
}


static int
create_type1(RECORD **anrecord) {
    ITEM *item = NULL;
    SUBFIELD *subfield = NULL;
    FIELD *field = NULL;
    RECORD *lrecord;    // For local convenience
    char buf[32];
    char *date_str;
    time_t tod;
    struct tm *tm;

    if (new_ANSI_NIST_record(anrecord, TYPE_1_ID) != 0)
        ALLOC_ERR_EXIT("Type-1 Record");

    lrecord = *anrecord;

    /*** 1.001 Logical record length ***/
    // Set the length to 0 for now; it will be updated when the record
    // is closed
    APPEND_TYPE1_FIELD(lrecord, LEN_ID, "0");

    /*** 1.002 - Version number ***/
    snprintf(buf, sizeof(buf), "%04d", VERSION_0300);
    APPEND_TYPE1_FIELD(lrecord, VER_ID, buf);

    /*** 1.003 - File content ***/
    snprintf(buf, sizeof(buf), "%d", TYPE_1_ID);
    // Allocate a new subfield and set the first item
    if (value2subfield(&subfield, buf) != 0)
        ERR_OUT("allocating Type-1 subfield");
    // Add the second item to the subfield, the count of remaining records
    if (value2item(&item, "0") != 0)
        ERR_OUT("allocating Type-1 item");
    if (append_ANSI_NIST_subfield(subfield, item) != 0)
        ERR_OUT("appending Type-1 item");
    // Add the subfield to the field
    if (new_ANSI_NIST_field(&field, TYPE_1_ID, CNT_ID) != 0)
        ERR_OUT("allocating Type-1 field");
    if (append_ANSI_NIST_field(field, subfield) != 0)
        ERR_OUT("appending Type-1 subfield");

    // Add the entire field to the record
    if (append_ANSI_NIST_record(lrecord, field) != 0)
        ERR_OUT("appending Type-1 field");

    /*** 1.004 - Type of transaction ***/
    APPEND_TYPE1_FIELD(lrecord, 4, "LFFS");

    /*** 1.005 - Date ***/
    if (get_ANSI_NIST_date(&date_str) != 0)
        ERR_OUT("getting ANSI/NIST date");
    APPEND_TYPE1_FIELD(lrecord, DAT_ID, date_str);
    free(date_str);

    /*** 1.007 - Destination agency identifier ***/
    APPEND_TYPE1_FIELD(lrecord, 7, "ANSI/NIST");

    /*** 1.008 - Originating agency identifier ***/
    APPEND_TYPE1_FIELD(lrecord, 8, "M1/FMR");

    /*** 1.009 - Transaction control number ***/
    // Use the current UTC time string YYYYMMDDHHMMSS
    tod = time(NULL);
    tm = gmtime(&tod);
    snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
             tm->tm_min, tm->tm_sec);

    APPEND_TYPE1_FIELD(lrecord, 9, buf);

    /*** 1.011 - Native scanning resolution ***/
    // XXX Set the NSR to the minimum, but we may want to base this
    // XXX value on something from the FMR
    snprintf(buf, sizeof(buf), "%5.2f", MIN_RESOLUTION);
    APPEND_TYPE1_FIELD(lrecord, 11, buf);

    /*** 1.012 - Nominal transmitting resolution ***/
    APPEND_TYPE1_FIELD(lrecord, 12, buf);

    if (update_ANSI_NIST_tagged_record_LEN(lrecord) != 0)
        ERR_OUT("updating record length");

    return 0;

    err_out:
    fprintf(stderr, "Error creating Type-1 record\n");
    if (item != NULL)
        free_ANSI_NIST_item(item);
    if (subfield != NULL)
        free_ANSI_NIST_subfield(subfield);
    if (field != NULL)
        free_ANSI_NIST_field(field);
    if (lrecord != NULL)
        free_ANSI_NIST_record(lrecord);

    return -1;
}

int scan_and_decode_image(unsigned char *idata, int ilen, int *oimg_type,
                          unsigned char **odata, int *olen,
                          int *ow, int *oh, int *od, int *oppi) {
    int ret, i;
    unsigned char *ndata;
    int img_type, nlen;
    int w, h, d, ppi, lossyflag, intrlvflag = 0, n_cmpnts;
    IMG_DAT *img_dat;

    if ((ret = image_type(&img_type, idata, ilen))) {
        return (ret);
    }

    switch (img_type) {
        case UNKNOWN_IMG:
            /* Return raw image data as read from file. */
            *oimg_type = img_type;
            *odata = idata;
            *olen = ilen;
            *ow = -1;
            *oh = -1;
            *od = -1;
            *oppi = -1;
            return (0);
        case WSQ_IMG:
            if ((ret = wsq_decode_mem(&ndata, &w, &h, &d, &ppi, &lossyflag,
                                      idata, ilen))) {
                return (ret);
            }
            nlen = w * h;
            break;
        case JPEGL_IMG:
            if ((ret = jpegl_decode_mem(&img_dat, &lossyflag, idata, ilen))) {
                return (ret);
            }
            if ((ret = get_IMG_DAT_image(&ndata, &nlen, &w, &h, &d, &ppi,
                                         img_dat))) {
                free_IMG_DAT(img_dat, FREE_IMAGE);
                return (ret);
            }
            free_IMG_DAT(img_dat, FREE_IMAGE);
            break;
        case JPEGB_IMG:
            if ((ret = jpegb_decode_mem(&ndata, &w, &h, &d, &ppi, &lossyflag,
                                        idata, ilen))) {
                return (ret);
            }
            if (d == 8) {
                n_cmpnts = 1;
                intrlvflag = 0;
            } else if (d == 24) {
                n_cmpnts = 3;
                intrlvflag = 1;
            } else {
                fprintf(stderr, "ERROR : read_and_decode_image : ");
                fprintf(stderr, "JPEGB decoder returned d=%d ", d);
                fprintf(stderr, "not equal to 8 or 24\n");
                free(idata);
                return (-2);
            }
            nlen = w * h * (d >> 3);
            break;
        case IHEAD_IMG:
            if ((ret = ihead_decode_mem(&ndata, &w, &h, &d, &ppi, &lossyflag,
                                        idata, ilen))) {
                return (ret);
            }

            nlen = SizeFromDepth(w, h, d);
            if ((d == 1) || (d == 8)) {
                n_cmpnts = 1;
                intrlvflag = 0;
            } else if (d == 24) {
                n_cmpnts = 3;
                intrlvflag = 1;
            } else {
                fprintf(stderr, "ERROR : read_and_decode_image : ");
                fprintf(stderr, "IHead decoder returned d=%d ", d);
                fprintf(stderr, "not equal to {1,8,24}\n");
                return (-2);
            }
            break;
        case JP2_IMG:
            if ((ret = openjpeg2k_decode_mem(&img_dat, &lossyflag, idata, ilen))) {
                return (ret);
            }
            if ((ret = get_IMG_DAT_image(&ndata, &nlen, &w, &h, &d, &ppi,
                                         img_dat))) {
                free_IMG_DAT(img_dat, FREE_IMAGE);
                return (ret);
            }
            free_IMG_DAT(img_dat, FREE_IMAGE);
            break;

        case PNG_IMG:
            if ((ret = png_decode_mem(&img_dat, &lossyflag, idata, ilen))) {
                return (ret);
            }
            if ((ret = get_IMG_DAT_image(&ndata, &nlen, &w, &h, &d, &ppi,
                                         img_dat))) {
                free_IMG_DAT(img_dat, FREE_IMAGE);
                return (ret);
            }
            free_IMG_DAT(img_dat, FREE_IMAGE);
            break;

        default:
            fprintf(stderr, "ERROR : read_and_decode_image : ");
            fprintf(stderr, "illegal image type = %d\n", img_type);
            return (-3);
    }

    *oimg_type = img_type;
    *odata = ndata;
    *olen = nlen;
    *ow = w;
    *oh = h;
    *od = d;
    *oppi = ppi;

    return (0);
}


void read_image(unsigned char *indata, int ilen,
                unsigned char **odata, int *olen,
                int *img_type,
                int *iw, int *ih, int *id, int *ippi,
                double *ippmm) {

    if (scan_and_decode_image(indata, ilen, img_type, odata, olen, iw, ih, id, ippi) != 0)
        ERR_EXIT("cannot open input image file");

    if (*ippi == UNDEFINED)
        *ippmm = DEFAULT_PPI / (double) MM_PER_INCH;
    else
        *ippmm = *ippi / (double) MM_PER_INCH;
}

void read_minutiae_to_ansi_fmr(unsigned char *idata, int iw, int ih, int id, int ippi, double ippmm,
                               struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr) {
    unsigned char *bdata;
    int bw, bh, bd;
    int *direction_map, *low_contrast_map, *low_flow_map;
    int *high_curve_map, *quality_map;
    int map_w, map_h;

    MINUTIAE *minutiae;

    if (get_minutiae(&minutiae, &quality_map, &direction_map,
                     &low_contrast_map, &low_flow_map, &high_curve_map,
                     &map_w, &map_h, &bdata, &bw, &bh, &bd,
                     idata, iw, ih, id, ippmm, &lfsparms_V2) != 0) {
        ERR_EXIT("cannot read minutiae");
    }

    ANSI_NIST *ansi_nist;
    RECORD *type1;
    int img_idc, img_imp = 0;

    if (alloc_ANSI_NIST(&ansi_nist) != 0)
        ALLOC_ERR_EXIT("AN2K record");

    create_type1(&type1);
    if (update_ANSI_NIST(ansi_nist, type1) != 0)
        ALLOC_ERR_EXIT("inserting Type-1 Record");

    if (update_ANSI_NIST_lfs_results(ansi_nist, minutiae, bdata, bw, bh, bd, ippmm, img_idc, img_imp) != 0) {
        free_minutiae(minutiae);
        free(bdata);
        ERR_EXIT("could not create ANSI record from minutiae");
    }

    free_minutiae(minutiae);
    free(bdata);


    if (new_fmr(FMR_STD_ANSI, fmr) != 0)
        ALLOC_ERR_EXIT("FMR");
    if (new_fvmr(FMR_STD_ANSI, fvmr) != 0)
        ALLOC_ERR_EXIT("FVMR");
    add_fvmr_to_fmr(*fvmr, *fmr);

    ansi2fmr(ansi_nist, fmr, fvmr, ippi);
    free_ANSI_NIST(ansi_nist);
}

void convert_ansi2iso(struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr, int ippi) {
    struct finger_minutiae_record *ofmr;
    struct finger_view_minutiae_record *ofvmr;

    if (new_fmr(FMR_STD_ISO, &ofmr) != 0)
        ALLOC_ERR_EXIT("FMR");
    if (new_fvmr(FMR_STD_ISO, &ofvmr) != 0)
        ALLOC_ERR_EXIT("FVMR");
    add_fvmr_to_fmr(ofvmr, ofmr);

    COPY_FMR((*fmr), ofmr);
    ofmr->record_length = FMR_ISO_HEADER_LENGTH;
    ofmr->record_length_type = FMR_ISO_HEADER_TYPE;

    unsigned int fmr_len;
    ansi2iso_fvmr(*fvmr, ofvmr, &fmr_len, ippi, ippi);
    ofmr->record_length = fmr_len;

    *fmr = ofmr;
    *fvmr = ofvmr;
}

void convert_ansi2iso_c(struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr, int ippi) {
    struct finger_minutiae_record *ofmr;
    struct finger_view_minutiae_record *ofvmr;

    if (new_fmr(FMR_STD_ISO_NORMAL_CARD, &ofmr) != 0)
        ALLOC_ERR_EXIT("FMR");
    if (new_fvmr(FMR_STD_ISO_NORMAL_CARD, &ofvmr) != 0)
        ALLOC_ERR_EXIT("FVMR");
    add_fvmr_to_fmr(ofvmr, ofmr);

    COPY_FMR((*fmr), ofmr);
    ofmr->record_length = FMR_ISO_HEADER_LENGTH;
    ofmr->record_length_type = FMR_ISO_HEADER_TYPE;

    unsigned int fmr_len;
    ansi2iso_fvmr(*fvmr, ofvmr, &fmr_len, ippi, ippi);
    ofmr->record_length = fmr_len;

    *fmr = ofmr;
    *fvmr = ofvmr;
}

void convert_ansi2iso_cc(struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr, int ippi) {
    struct finger_minutiae_record *ofmr;
    struct finger_view_minutiae_record *ofvmr;

    if (new_fmr(FMR_STD_ISO_COMPACT_CARD, &ofmr) != 0)
        ALLOC_ERR_EXIT("FMR");
    if (new_fvmr(FMR_STD_ISO_COMPACT_CARD, &ofvmr) != 0)
        ALLOC_ERR_EXIT("FVMR");
    add_fvmr_to_fmr(ofvmr, ofmr);

    COPY_FMR((*fmr), ofmr);
    ofmr->record_length = FMR_ISO_HEADER_LENGTH;
    ofmr->record_length_type = FMR_ISO_HEADER_TYPE;

    unsigned int fmr_len;
    ansi2isocc_fvmr(*fvmr, ofvmr, &fmr_len, ippi, ippi);
    ofmr->record_length = fmr_len;

    *fmr = ofmr;
    *fvmr = ofvmr;
}

static int
copy_without_conversion(FMR *ifmr, FMR *ofmr, int fmr_type) {
    FVMR *ofvmr;
    FVMR **ifvmrs = NULL;
    FMD **ifmds = NULL;
    FMD *ofmd;
    int r, rcount, m, mcount;
    int retval;

    retval = -1;            /* Assume failure, for now */

    COPY_FMR(ifmr, ofmr);

    /* Get all of the finger view records */
    rcount = get_fvmr_count(ifmr);
    if (rcount > 0) {
        ifvmrs = (FVMR **) malloc(rcount * sizeof(FVMR *));
        if (ifvmrs == NULL)
            ALLOC_ERR_OUT("FVMR Array");
        if (get_fvmrs(ifmr, ifvmrs) != rcount)
            ERR_OUT("getting FVMRs from FMR");

        for (r = 0; r < rcount; r++) {
            if (new_fvmr(fmr_type, &ofvmr) < 0)
                ALLOC_ERR_RETURN("Output FVMR");

            COPY_FVMR(ifvmrs[r], ofvmr);
            mcount = get_fmd_count(ifvmrs[r]);
            if (mcount != 0) {
                ifmds = (FMD **) malloc(mcount * sizeof(FMD *));
                if (ifmds == NULL)
                    ALLOC_ERR_RETURN("FMD array");
                if (get_fmds(ifvmrs[r], ifmds) != mcount)
                    ERR_OUT("getting FMDs from FVMR");

                for (m = 0; m < mcount; m++) {
                    if (new_fmd(FMR_STD_ISO, &ofmd, m) != 0)
                        ALLOC_ERR_RETURN("Output FMD");
                    COPY_FMD(ifmds[m], ofmd);
                    add_fmd_to_fvmr(ofmd, ofvmr);
                }
            }

            /* Subtract off the length of the extended data block,
             * if present, because we don't copy extended data yet.
             */
            if (ifvmrs[r]->extended != NULL) {
                ofmr->record_length -=
                        ifvmrs[r]->extended->block_length;

            }
            add_fvmr_to_fmr(ofvmr, ofmr);
            // XXX Copy the FEDB to the output fmr
        }

    } else {
        if (rcount == 0)
            ERR_OUT("there are no FVMRs in the input FMR");
        else
            ERR_OUT("retrieving FVMRs from input FMR");
    }
    retval = 0;

    err_out:
    if (ifvmrs != NULL)
        free(ifvmrs);
    if (ifmds != NULL)
        free(ifmds);
    return (retval);
}

/*
 * Copy an FMR with conversion.
 */
static int
copy_with_conversion(FMR *ifmr, FMR *ofmr, int in_type, int out_type) {
    FVMR *ofvmr;
    FVMR **ifvmrs = NULL;
    int r, rcount;
    unsigned int fmr_len, fvmr_len;
    int rc, retval;
    char *ver;

    retval = -1;            /* Assume failure, for now */

    if (in_type == out_type)
        return (-1);

    COPY_FMR(ifmr, ofmr);
    switch (out_type) {
        case FMR_STD_ANSI:
            fmr_len = FMR_ANSI_SMALL_HEADER_LENGTH;
            ver = FMR_ANSI_SPEC_VERSION;
            break;
        case FMR_STD_ANSI07:
            fmr_len = FMR_ANSI07_HEADER_LENGTH;
            ver = FMR_ANSI07_SPEC_VERSION;
            break;
        case FMR_STD_ISO:
            fmr_len = FMR_ISO_HEADER_LENGTH;
            ver = FMR_ISO_SPEC_VERSION;
            break;
    }

    /* Fix up the output FMR header for those input types that don't
     * have all the needed information.
     */

    if ((out_type == FMR_STD_ANSI) || (out_type == FMR_STD_ISO) ||
        (out_type == FMR_STD_ANSI07))
        if ((in_type == FMR_STD_ISO_NORMAL_CARD) ||
            (in_type == FMR_STD_ISO_COMPACT_CARD)) {
            strncpy(ofmr->format_id, FMR_FORMAT_ID,
                    FMR_FORMAT_ID_LEN);
            strncpy(ofmr->spec_version, ver,
                    FMR_SPEC_VERSION_LEN);
        }

    /* Get all of the finger view records */
    rcount = get_fvmr_count(ifmr);
    if (rcount > 0) {
        ifvmrs = (FVMR **) malloc(rcount * sizeof(FVMR *));
        if (ifvmrs == NULL)
            ALLOC_ERR_OUT("FVMR Array");
        if (get_fvmrs(ifmr, ifvmrs) != rcount)
            ERR_OUT("getting FVMRs from FMR");

        for (r = 0; r < rcount; r++) {
            if (new_fvmr(out_type, &ofvmr) < 0)
                ALLOC_ERR_RETURN("Output FVMR");

            rc = -1;
            switch (in_type) {
                case FMR_STD_ANSI07:
                    /* The coord and resolution info is stored
                     * in the FVMR; copy it to the FMR header,
                     * using the first FVMR as the canonical set.
                     */
                    ofmr->x_image_size = ifvmrs[0]->x_image_size;
                    ofmr->y_image_size = ifvmrs[0]->y_image_size;
                    ofmr->x_resolution = ifvmrs[0]->x_resolution;
                    ofmr->y_resolution = ifvmrs[0]->y_resolution;
                case FMR_STD_ANSI:
                    switch (out_type) {
                        case FMR_STD_ISO:
                        case FMR_STD_ISO_NORMAL_CARD:
                            rc = ansi2iso_fvmr(ifvmrs[r], ofvmr,
                                               &fvmr_len, ifmr->x_resolution,
                                               ifmr->y_resolution);
                            break;
                        case FMR_STD_ISO_COMPACT_CARD:
                            rc = ansi2isocc_fvmr(ifvmrs[r], ofvmr,
                                                 &fvmr_len, ifmr->x_resolution,
                                                 ifmr->y_resolution);
                            break;
                        default:
                            ERR_OUT("Invalid output type");
                    }
                    break;

                    /* XXX Eventually handle ISO->ISO conversions */
                case FMR_STD_ISO:
                    /* For ANSI07, the coord and resolution info
                     * is copied by iso2ansi_fvmr().
                     */
                case FMR_STD_ISO_NORMAL_CARD:
                    rc = iso2ansi_fvmr(ifvmrs[r], ofvmr, &fvmr_len,
                                       ifmr->x_resolution, ifmr->y_resolution);
                    break;
                case FMR_STD_ISO_COMPACT_CARD:
                    rc = isocc2ansi_fvmr(ifvmrs[r], ofvmr,
                                         &fvmr_len, ifmr->x_resolution,
                                         ifmr->y_resolution);
                    break;
            }
            if (rc != 0)
                ERR_OUT("Modifying FVMR");

            fmr_len += fvmr_len;
            // XXX Copy the FEDB to the output fmr
            ofvmr->extended = NULL;
            add_fvmr_to_fmr(ofvmr, ofmr);

            fmr_len += FEDB_HEADER_LENGTH;
        }

    } else {
        if (rcount == 0)
            ERR_OUT("there are no FVMRs in the input FMR");
        else
            ERR_OUT("retrieving FVMRs from input FMR");
    }

    ofmr->record_length = fmr_len;
    retval = 0;
    err_out:
    if (ifvmrs != NULL)
        free(ifvmrs);
    return (retval);
}


int img2fmr(unsigned char *idata, int ilen, char *otype, unsigned char **odata, int *olen) {

    unsigned char *imdata;
    int img_len;
    int img_type;
    int iw, ih, id, ippi;
    double ippmm;

    read_image(idata, ilen, &imdata, &img_len, &img_type,
               &iw, &ih, &id, &ippi, &ippmm);

    struct finger_minutiae_record *fmr;
    struct finger_view_minutiae_record *fvmr;
    read_minutiae_to_ansi_fmr(idata, iw, ih, id, ippi, ippmm, &fmr, &fvmr);

    if (strcmp(otype, "ISO") == 0) {
        convert_ansi2iso(&fmr, &fvmr, ippi);
    }
    if (strcmp(otype, "ISONC") == 0) {
        convert_ansi2iso_c(&fmr, &fvmr, ippi);
    }
    if (strcmp(otype, "ISOCC") == 0) {
        convert_ansi2iso_cc(&fmr, &fvmr, ippi);
    }

    uint8_t *buf;
    BDB *bdb;
    buf = (uint8_t *) malloc(fmr->record_length);
    bdb = (BDB *) malloc(sizeof(BDB));
    INIT_BDB(bdb, buf, fmr->record_length);

    if (push_fmr(bdb, fmr) != WRITE_OK) {
        fprintf(stderr, "could not push FMR\n");
        exit(EXIT_FAILURE);
    }

    *odata = bdb->bdb_start;
    *olen = bdb->bdb_size;

    return 0;
}

static int
str_to_type(char *stdstr) {
    if (strcmp(stdstr, "ANSI") == 0)
        return (FMR_STD_ANSI);
    if (strcmp(stdstr, "ISO") == 0)
        return (FMR_STD_ISO);
    if (strcmp(stdstr, "ISONC") == 0)
        return (FMR_STD_ISO_NORMAL_CARD);
    if (strcmp(stdstr, "ISOCC") == 0)
        return (FMR_STD_ISO_COMPACT_CARD);
    return (-1);
}

int fmr2fmr(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
            char *in_type_str, char *out_type_str) {
    fmr2fmr_iso_card(idata, ilen, odata, olen, in_type_str, out_type_str, 0, 0);
}

int fmr2fmr_iso_card(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                     char *in_type_str, char *out_type_str, int iso_c_xres, int iso_c_yres) {

    BDB *rbdb;
    rbdb = (BDB *) malloc(sizeof(BDB));
    INIT_BDB(rbdb, idata, ilen);

    struct finger_minutiae_record *ifmr;
    struct finger_minutiae_record *ofmr;

    if (scan_fmr(rbdb, ifmr) != READ_OK) {
        fprintf(stderr, "Could not read FMR from file.\n");
        exit(EXIT_FAILURE);
    }

    int in_type = str_to_type(in_type_str);
    int out_type = str_to_type(out_type_str);

    /* ISO card formats have no input resolution, so set it here
	 * from the input options.
	 */
    unsigned short iso_xres, iso_yres;
    if ((in_type == FMR_STD_ISO_NORMAL_CARD) ||
        (in_type == FMR_STD_ISO_COMPACT_CARD)) {
        ifmr->x_resolution = iso_c_xres;
        ifmr->y_resolution = iso_c_yres;
    }
    /* If the input and output file types are the same,
     * do a straight copy.
     */
    if (in_type == out_type)
        copy_without_conversion(ifmr, ofmr, in_type);
    else
        copy_with_conversion(ifmr, ofmr, in_type, out_type);

    uint8_t *buf;
    BDB *bdb;
    buf = (uint8_t *) malloc(ofmr->record_length);
    bdb = (BDB *) malloc(sizeof(BDB));
    INIT_BDB(bdb, buf, ofmr->record_length);

    if (push_fmr(bdb, ofmr) != WRITE_OK) {
        fprintf(stderr, "could not push FMR\n");
        exit(EXIT_FAILURE);
    }

    *odata = bdb->bdb_start;
    *olen = bdb->bdb_size;

    return 0;
}