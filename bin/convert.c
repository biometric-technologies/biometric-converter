
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
    fmr->product_identifier_owner = 0; // XXX: replace with something valid?
    fmr->product_identifier_type = 0; // XXX: replace with something valid?
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

void ansi2fmr(ANSI_NIST *ansi_nist, struct finger_minutiae_record **fmr, struct finger_view_minutiae_record **fvmr) {

    FIELD *field;
    int i;
    int idc;
    int idx;
    int type9_count;

    type9_count = 0;
    for (i = 1; i < ansi_nist->num_records; i++) {
        if (ansi_nist->records[i]->type == TYPE_9_ID) {
            type9_count++;

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

void
usage() {
    printf(
            "usage:\n\tconvert -i <wsqfile> -o <resfile> -t ISO [-v] \n"
            "\t\t -i:  Specifies the WSQ input file\n"
            "\t\t -o:  Specifies the output file\n"
            "\t\t -t:  Specifies the type of output file (available types: ISO, ANSI)\n"
            "\t\t -v:  Verify template before saving (default: false)\n"
    );
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

static int
update_type1(ANSI_NIST *ansi_nist, RECORD *record, unsigned int type,
             unsigned int idc) {
    SUBFIELD *subfield = NULL;
    FIELD *field = NULL;
    ITEM *item = NULL;
    int field_idx;
    int saved_len;
    char buf[8];

    if (lookup_ANSI_NIST_field(&field, &field_idx, CNT_ID, record) == FALSE)
        ERR_OUT("locating CNT field of Type-1 record");

    // Save away the current length of the field so we can add
    // the change in length to the record
    saved_len = field->num_bytes;

    // Create a new subfield to contain the new logical record identifier
    snprintf(buf, sizeof(buf), "%d", type);
    if (value2subfield(&subfield, buf) != 0)
        ERR_OUT("creating new subfield");

    // Add the second item to the subfield, the IDC
    snprintf(buf, sizeof(buf), IDC_FMT, idc);
    if (value2item(&item, buf) != 0)
        ERR_OUT("creating new item");
    if (append_ANSI_NIST_subfield(subfield, item) != 0)
        ERR_OUT("appending item to subfield");

    // Add the subfield to the field
    if (append_ANSI_NIST_field(field, subfield) != 0)
        ERR_OUT("adding subfield to field");

    // Update the record length with the change in field length
    record->num_bytes += field->num_bytes - saved_len;
    if (update_ANSI_NIST_tagged_record_LEN(record) != 0)
        goto err_out2;

    // Update the sum of the Type-2 to Type-16 logical records, contained
    // in the first subfield of the Type-1 record.
    // The index numbeers are off-by-one.
    if (increment_numeric_item(0,        // record index
                               2,        // field index
                               0,        // subfield index
                               1,        // item index
                               ansi_nist,    // ansi_nist record
                               NULL) < 0)
        goto err_out2;

    return 0;

    err_out:
    if (item != NULL)
        free(item);
    if (subfield != NULL)
        free(subfield);

    return -1;

    err_out2:    // This exit point doesn't free any memory because the
    // fields have been shoved into the record
    return -1;
}

static int
create_type2(RECORD **anrecord, FILE *fp, unsigned int idc) {
    FIELD *field = NULL;
    SUBFIELD *subfield = NULL;
    RECORD *lrecord;    // For local convenience
    int field_num;
    char buf[MAX_TYPE2_FIELD_SIZE + 1];
    char *s;

    if (new_ANSI_NIST_record(anrecord, TYPE_2_ID) != 0)
        ALLOC_ERR_EXIT("Type-2 Record");

    lrecord = *anrecord;

    /*** 2.001 - Length                                    ***/
    // Set to 0 now, will recalculate later
    APPEND_TYPE2_FIELD(lrecord, LEN_ID, "0");

    /*** 2.002 - IDC value                                 ***/
    snprintf(buf, sizeof(buf), IDC_FMT, idc);
    APPEND_TYPE2_FIELD(lrecord, IDC_ID, buf);

    while (1) {
        // Read the field number, followed by single whitespace char,
        // followed by the string to place in the field.
        if (fscanf(fp, "%d", &field_num) < 0) {
            if (feof(fp)) {
                break;
            } else {
                ERR_OUT("reading Type-2 buf file.\n");
            }
        }
        fgetc(fp);    // skip the single whitespace char
        s = fgets(buf, MAX_TYPE2_FIELD_SIZE, fp);
        buf[strlen(buf) - 1] = '\0';
        if (s == NULL) {
            if (feof(fp)) {
                break;
            } else {
                ERR_OUT("reading Type-2 buf file.\n");
            }
        }
        /*** 2.xxx - User-defined field   ***/
        if (value2subfield(&subfield, buf) != 0)
            ERR_OUT("creating new Type-2 subfield");
        if (new_ANSI_NIST_field(&field, TYPE_2_ID, field_num) != 0)
            ERR_OUT("creating new Type-2 field");
        if (append_ANSI_NIST_field(field, subfield) != 0)
            ERR_OUT("appending Type-2 subfield");
        if (append_ANSI_NIST_record(lrecord, field) != 0)
            ERR_OUT("appending Type-2 field");

    }

    // Calculate and update the record length field
    if (update_ANSI_NIST_tagged_record_LEN(lrecord) != 0)
        ERR_OUT("updating Type-2 record length");

    return 0;

    err_out:
    return -1;
}

static int
create_type9(RECORD **anrecord, struct finger_view_minutiae_record *fvmr,
             unsigned int idc) {
    FIELD *field = NULL;
    SUBFIELD *subfield = NULL;
    ITEM *item = NULL;
    RECORD *lrecord;    // For local convenience
    struct finger_minutiae_data **fmds = NULL;
    struct ridge_count_data **rcds = NULL;
    struct core_data **cds;
    struct delta_data **dds;
    char buf[16];
    int mincnt, minidx, rdgcnt;
    int cnt, i;
    unsigned int x, y;

    if (new_ANSI_NIST_record(anrecord, TYPE_9_ID) != 0)
        ALLOC_ERR_EXIT("Type-9 Record");

    lrecord = *anrecord;

    /*** 9.001 - Length                                    ***/
    // Set to 0 now, will recalculate later
    APPEND_TYPE9_FIELD(lrecord, LEN_ID, "0");

    /*** 9.002 - IDC value                                 ***/
    snprintf(buf, sizeof(buf), IDC_FMT, idc);
    APPEND_TYPE9_FIELD(lrecord, IDC_ID, buf);

    /*** 9.003 - Impression type                           ***/
    CRW(fvmr->impression_type, MIN_TABLE_5_CODE, MAX_TABLE_5_CODE,
        "Impression type");
    snprintf(buf, sizeof(buf), "%d", fvmr->impression_type);
    APPEND_TYPE9_FIELD(lrecord, IMP_ID, buf);

    /*** 9.004 - Minutiae format                           ***/
    APPEND_TYPE9_FIELD(lrecord, FMT_ID, STD_STR);

    /*** 9.005 - Originating fingerprint reading system    ***/
    if (value2subfield(&subfield, "EXISTING IMAGE") != 0)
        ERR_OUT("creating Type-9 subfield");
    if (value2item(&item, AUTO_STR) != 0)
        ERR_OUT("creating Type-9 item");
    if (append_ANSI_NIST_subfield(subfield, item) != 0)
        ERR_OUT("appending Type-9 item");
    if (new_ANSI_NIST_field(&field, TYPE_9_ID, OFR_ID) != 0)
        ERR_OUT("creating Type-9 field");
    if (append_ANSI_NIST_field(field, subfield) != 0)
        ERR_OUT("appending Type-9 subfield");
    if (append_ANSI_NIST_record(lrecord, field) != 0)
        ERR_OUT("appending Type-9 field");

    /*** 9.006 - Finger position                           ***/
    snprintf(buf, sizeof(buf), "%02d", fvmr->finger_number);
    APPEND_TYPE9_FIELD(lrecord, FGP2_ID, buf);

    /*** 9.007 - Fingerprint pattern classification        ***/
    if (value2subfield(&subfield, TBL_STR) != 0)
        ERR_OUT("creating Type-9 subfield");
    if (value2item(&item, "UN") != 0)
        ERR_OUT("creating Type-9 item");
    if (append_ANSI_NIST_subfield(subfield, item) != 0)
        ERR_OUT("appending Type-9 item");
    if (new_ANSI_NIST_field(&field, TYPE_9_ID, FPC_ID) != 0)
        ERR_OUT("creating Type-9 field");
    if (append_ANSI_NIST_field(field, subfield) != 0)
        ERR_OUT("appending Type-9 subfield");
    if (append_ANSI_NIST_record(lrecord, field) != 0)
        ERR_OUT("appending Type-9 field");

    /*** 9.008 - Core position                             ***/
    cnt = get_core_count(fvmr);
    if (cnt > 0) {
        if (new_ANSI_NIST_field(&field, TYPE_9_ID, CRP_ID) != 0)
            ERR_OUT("allocating field");

        cds = (struct core_data **) malloc(
                cnt * sizeof(struct core_data **));
        if (cds == NULL)
            ALLOC_ERR_EXIT("Core data");

        if (get_cores(fvmr, cds) != cnt)
            ERR_OUT("retrieving core data");

        for (i = 0; i < cnt; i++) {
            convert_xy(fvmr->fmr->x_image_size,
                       fvmr->fmr->y_image_size,
                       fvmr->fmr->x_resolution,
                       fvmr->fmr->y_resolution,
                       cds[i]->x_coord,
                       cds[i]->y_coord,
                       &x, &y);
            snprintf(buf, sizeof(buf), "%04u%04u", x, y);
            if (value2subfield(&subfield, buf) != 0)
                ERR_OUT("creating subfield");
            if (append_ANSI_NIST_field(field, subfield) != 0)
                ERR_OUT("appending subfield");
        }
        if (append_ANSI_NIST_record(lrecord, field) != 0)
            ERR_OUT("adding field to record");

    } else if (cnt < 0)
        ERR_OUT("getting core record count");

    /*** 9.009 - Delta(s) position                         ***/
    cnt = get_delta_count(fvmr);
    if (cnt > 0) {
        if (new_ANSI_NIST_field(&field, TYPE_9_ID, DLT_ID) != 0)
            ERR_OUT("creating Type-9 field");

        dds = (struct delta_data **) malloc(
                cnt * sizeof(struct delta_data **));
        if (dds == NULL)
            ALLOC_ERR_EXIT("Delta data");

        if (get_deltas(fvmr, dds) != cnt)
            ERR_OUT("retrieving delta data");

        for (i = 0; i < cnt; i++) {
            convert_xy(fvmr->fmr->x_image_size,
                       fvmr->fmr->y_image_size,
                       fvmr->fmr->x_resolution,
                       fvmr->fmr->y_resolution,
                       dds[i]->x_coord,
                       dds[i]->y_coord,
                       &x, &y);
            snprintf(buf, sizeof(buf), "%04u%04u", x, y);
            if (value2subfield(&subfield, buf) != 0)
                ERR_OUT("creating subfield");
            if (append_ANSI_NIST_field(field, subfield) != 0)
                ERR_OUT("appending subfield");
        }
        if (append_ANSI_NIST_record(lrecord, field) != 0)
            ERR_OUT("adding field to record");

    } else if (cnt < 0)
        ERR_OUT("getting delta record count");

    /*** 9.010 - Number of minutiae                        ***/
    mincnt = get_fmd_count(fvmr);
    if (mincnt < 0)
        ERR_OUT("getting minutiae count");

    snprintf(buf, sizeof(buf), "%d", mincnt);
    APPEND_TYPE9_FIELD(lrecord, MIN_ID, buf);

    /*** 9.011 - Minutiae ridge count indicator            ***/
    rdgcnt = get_rcd_count(fvmr);
    if (rdgcnt > 0) {
        rcds = (struct ridge_count_data **) malloc(
                rdgcnt * sizeof(struct ridge_count_data **));
        if (rcds == NULL)
            ALLOC_ERR_EXIT("Ridge Count data");

        if (get_rcds(fvmr, rcds) != rdgcnt)
            ERR_OUT("retrieving ridge count data");

        APPEND_TYPE9_FIELD(lrecord, RDG_ID, "1");
    } else if (rdgcnt < 0)
        ERR_OUT("getting ridge record count");
    else
        APPEND_TYPE9_FIELD(lrecord, RDG_ID, "0");

    /*** 9.012 - Minutiae and ridge count data             ***/
    fmds = (struct finger_minutiae_data **) malloc(
            mincnt * sizeof(struct finger_minutiae_data **));
    if (fmds == NULL)
        ALLOC_ERR_EXIT("Finger Minutiae data");

    if (get_fmds(fvmr, fmds) != mincnt)
        ERR_OUT("retrieving minutiae data");

    if (new_ANSI_NIST_field(&field, TYPE_9_ID, MRC_ID) != 0)
        ERR_OUT("creating Type-9 field");

    for (minidx = 0; minidx < mincnt; minidx++) {
        unsigned int theta, rdgidx, minqual;
        char mintype;
        int idxnum = minidx + 1;

        // Index number
        snprintf(buf, sizeof(buf), "%03d", idxnum);
        if (value2subfield(&subfield, buf) != 0)
            ERR_OUT("creating Type-9 subfield");

        // X, Y, and theta values
        convert_xy(fvmr->fmr->x_image_size, fvmr->fmr->y_image_size,
                   fvmr->fmr->x_resolution, fvmr->fmr->y_resolution,
                   fmds[minidx]->x_coord, fmds[minidx]->y_coord,
                   &x, &y);
        convert_theta(fmds[minidx]->angle, &theta);
        snprintf(buf, sizeof(buf), "%04u%04u%03u", x, y, theta);
        if (value2item(&item, buf) != 0)
            ERR_OUT("creating Type-9 item");
        if (append_ANSI_NIST_subfield(subfield, item) != 0)
            ERR_OUT("appending Type-9 item");

        // Quality measure
        convert_quality(fmds[minidx]->quality, &minqual);
        snprintf(buf, sizeof(buf), "%u", minqual);
        if (value2item(&item, buf) != 0)
            ERR_OUT("creating Type-9 item");
        if (append_ANSI_NIST_subfield(subfield, item) != 0)
            ERR_OUT("appending Type-9 item");

        // Minutia type designation
        convert_type(fmds[minidx]->type, &mintype);
        snprintf(buf, sizeof(buf), "%c", mintype);
        if (value2item(&item, buf) != 0)
            ERR_OUT("creating Type-9 item");
        if (append_ANSI_NIST_subfield(subfield, item) != 0)
            ERR_OUT("appending Type-9 item");

        // Ridge count data: If the one of the index numbers
        // in the record matches the minutia index, then add that
        // ridge count data to the Type-9 record, using the index
        // number that is the 'other'.
        for (rdgidx = 0; rdgidx < rdgcnt; rdgidx++) {
            if ((rcds[rdgidx]->index_one == idxnum) ||
                (rcds[rdgidx]->index_two == idxnum)) {
                snprintf(buf, sizeof(buf), "%u,%u",
                         (rcds[rdgidx]->index_one == idxnum) ?
                         rcds[rdgidx]->index_two :
                         rcds[rdgidx]->index_one,
                         rcds[rdgidx]->count);

                if (value2item(&item, buf) != 0)
                    ERR_OUT("creating Type-9 item");
                if (append_ANSI_NIST_subfield(subfield, item) != 0)
                    ERR_OUT("appending Type-9 item");
            }
        }

        if (append_ANSI_NIST_field(field, subfield) != 0)
            ERR_OUT("appending Type-9 subfield");
    }
    free(fmds);
    if (append_ANSI_NIST_record(lrecord, field) != 0)
        ERR_OUT("appending Type-9 field");
    /*** End of minutiae and ridge count                 */

    // Calculate and update the record length field
    if (update_ANSI_NIST_tagged_record_LEN(lrecord) != 0)
        ERR_OUT("updating Type-9 record length");

    return 0;

    err_out:
    fprintf(stderr, "Error creating Type-9 record\n");
    if (item != NULL)
        free_ANSI_NIST_item(item);
    if (subfield != NULL)
        free_ANSI_NIST_subfield(subfield);
    if (field != NULL)
        free_ANSI_NIST_field(field);
    if (lrecord != NULL)
        free_ANSI_NIST_record(lrecord);
    if (fmds != NULL)
        free(fmds);

    return -1;
}

static int
create_type13(RECORD **anrecord, struct finger_view_minutiae_record *fvmr,
              FILE *fp, unsigned int idc) {
    char fn[MAXPATHLEN];
    unsigned char *imgdata;
    int imgsize;

    /*** 13.999 - Image data                               ***/
    // fp parameter is for the file containing the list of
    // image files
    if (fscanf(fp, "%s", fn) < 0)
        ERR_OUT("reading image list file.\n");

    printf("reading image from file %s\n", fn);
    if (read_binary_image_data(fn, &imgdata, &imgsize) != 0)
        ERR_OUT("reading image data");

    if (image2type_13(anrecord, imgdata, imgsize,
                      fvmr->fmr->x_image_size, fvmr->fmr->y_image_size,
                      8, (double) (fvmr->fmr->x_resolution / 10.0),
                      "NONE",    // Compression String
                      idc, fvmr->impression_type,
                      "NIST 894.03") != 0)
        ERR_OUT("converting image to Type-13 record");

    // Calculate and update the record length field
    if (update_ANSI_NIST_tagged_record_LEN(*anrecord) != 0)
        ERR_OUT("updating Type-13 record length");

    return 0;

    err_out:
    return -1;
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

int main(int argc, char *argv[]) {

    char *input_file, *output_file, *output_type;
    int validate;
    get_options(argc, argv, &input_file, &output_file, &output_type, &validate);

    unsigned char *idata;
    int img_type;
    int ilen, iw, ih, id, ippi;
    double ippmm;

    if (read_and_decode_grayscale_image(input_file, &img_type, &idata, &ilen, &iw, &ih, &id, &ippi) != 0)
        ERR_EXIT("cannot open input image file");

    if (ippi == UNDEFINED)
        ippmm = DEFAULT_PPI / (double) MM_PER_INCH;
    else
        ippmm = ippi / (double) MM_PER_INCH;

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
        free(idata);
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

    struct finger_minutiae_record *fmr;
    struct finger_view_minutiae_record *fvmr;

    if (new_fmr(FMR_STD_ANSI, &fmr) != 0)
        ALLOC_ERR_EXIT("FMR");
    if (new_fvmr(FMR_STD_ANSI, &fvmr) != 0)
        ALLOC_ERR_EXIT("FVMR");
    add_fvmr_to_fmr(fvmr, fmr);

    ansi2fmr(ansi_nist, &fmr, &fvmr);
    free_ANSI_NIST(ansi_nist);

    FILE *fmr_fp = NULL;
    if ((fmr_fp = fopen(output_file, "wb")) == NULL)
        OPEN_ERR_EXIT(output_file);

    if (strcmp(output_type, "ISO") == 0) {
        struct finger_minutiae_record *ofmr;
        struct finger_view_minutiae_record *ofvmr;

        if (new_fmr(FMR_STD_ISO, &ofmr) != 0)
            ALLOC_ERR_EXIT("FMR");
        if (new_fvmr(FMR_STD_ISO, &ofvmr) != 0)
            ALLOC_ERR_EXIT("FVMR");
        add_fvmr_to_fmr(fvmr, ofmr);

        unsigned int fmr_len;
        ansi2iso_fvmr(fvmr, ofvmr, &fmr_len, fmr->x_resolution, fmr->y_resolution);

        COPY_FMR(ofmr, fmr);
        COPY_FVMR(ofvmr, fvmr);
    }
    fvmr->impression_type = 0;

    if (validate == 1 && validate_fvmr(fvmr) != VALIDATE_OK)
        ERR_EXIT("Validation failed");

    if (write_fmr(fmr_fp, fmr) != 0) {
        fclose(fmr_fp);
        ERR_EXIT("Could not write finger minutiae record");
    }

    printf("Converting from [WSQ] to [%s] successfully done\n", output_type);

    exit(EXIT_SUCCESS);
}