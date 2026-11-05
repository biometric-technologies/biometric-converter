#include "converter.h"
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lfs.h>
#ifndef CONVERTER_NO_IMAGE_DECODERS
#include <imgdecod.h>
#endif
#include <stdint.h>
#include <biomdi.h>
#include <biomdimacro.h>
#include <fmr.h>
#include <fmr2fmr.h>
#include <time.h>
#include <sys/param.h>
#ifndef CONVERTER_NO_IMAGE_DECODERS
#include <imgutil.h>
#include <png_dec.h>
#include <jpeg2k.h>
#endif
#include <pthread.h>
#include <stdarg.h>

int debug = 0;

// Message describing the last failure in the calling thread; returned by
// converter_last_error().
static __thread char last_error[256];

// The NBIS image decoders (WSQ in particular) and MINDTCT keep state in
// global variables, and the AN2K date routine uses ctime(). Everything that
// runs NBIS code is therefore serialized with this lock.
static pthread_mutex_t nbis_lock = PTHREAD_MUTEX_INITIALIZER;

static void
set_error(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(last_error, sizeof(last_error), fmt, ap);
    va_end(ap);
}

// Record an error message and branch to 'err_out'.
#define CONV_ERR(...)                       \
    do {                                    \
        set_error(__VA_ARGS__);             \
        goto err_out;                       \
    } while (0)

#define CONV_ALLOC_ERR(what) CONV_ERR("could not allocate %s", what)

const char *
converter_last_error(void) {
    return last_error;
}

void
converter_free(void *data) {
    free(data);
}

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
convert_xy(unsigned short y_size, double ppmm,
           unsigned short ansi_x, unsigned short ansi_y,
           unsigned short *fmr_x, unsigned short *fmr_y) {
    double factor;
    int h_units;

    // The AN2K coordinate is a distance from the bottom left corner in
    // .01mm units. NBIS minutiae2type_9() computes it from the MINDTCT
    // pixel position (x, y) as x_units = x / ppmm * 100 and
    // y_units = h_units - 1 - y / ppmm * 100, where h_units is the image
    // height in .01mm. Invert that with the same scan resolution, so the
    // record gets the pixel positions MINDTCT found.
    factor = ppmm / 100.0;
    *fmr_x = (unsigned short) (((double) ansi_x * factor) + 0.5);

    h_units = (int) (((double) y_size / factor) + 0.5);
    *fmr_y = (unsigned short) (((double) (h_units - 1 - ansi_y) * factor) + 0.5);
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
    int q;

    // NBIS minutiae2type_9() stores the MINDTCT reliability R (0.0-1.0) as
    // 63 - 61 * R, so 2 is the best and 63 the worst quality. The FMR
    // minutia quality runs from 0 to 100 with 100 the best; write R * 100,
    // the value MINDTCT itself reports.
    q = (int) (((63 - ansi_qual) * 100.0 / 61.0) + 0.5);
    if (q < 0)
        q = 0;
    if (q > FMD_MAX_MINUTIA_QUALITY)
        q = FMD_MAX_MINUTIA_QUALITY;
    *fmr_qual = (unsigned char) q;
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
        CONV_ERR("Lookup of Type-4 ISR_ID");
    val = strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if ((val != 0) && (val != 1))
        CONV_ERR("Type-4 ISR is invalid");

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
        CONV_ERR("Lookup of SLC_ID");
    val = strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if ((val == 1) || (val == 2)) {
        if (lookup_ANSI_NIST_field(&field, &idx, HPS_ID, rec) == FALSE)
            CONV_ERR("Lookup of HPS_ID");
        hps = strtol((char *) field->subfields[0]->items[0]->value,
                     NULL, 10);
        if (lookup_ANSI_NIST_field(&field, &idx, VPS_ID, rec) == FALSE)
            CONV_ERR("Lookup of VPS_ID");
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
        CONV_ERR("Lookup of field HLL_ID");
    fmr->x_image_size =
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    if (lookup_ANSI_NIST_field(&field, &idx, VLL_ID, rec) == FALSE)
        CONV_ERR("Lookup of VLL_ID");
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
        CONV_ERR("System error locating image record");
    if (ret == TRUE) {
        if (set_fmr_img(fmr, rec) != 0)
            CONV_ERR("Initializing FMR image info");
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

// Return the extended data block of a finger view, creating it and
// attaching it to the view on first use; NULL if it cannot be allocated.
static struct finger_extended_data_block *
fvmr_fedb(struct finger_view_minutiae_record *fvmr) {
    struct finger_extended_data_block *fedb;

    if (fvmr->extended == NULL) {
        if (new_fedb(FMR_STD_ANSI, &fedb) != 0)
            return NULL;
        add_fedb_to_fvmr(fedb, fvmr);
    }
    return fvmr->extended;
}

// Fill a finger view from an AN2K Type-9 record. Every allocated item is
// attached to the view as soon as it is created, so freeing the parent FMR
// releases everything, also on error. ppmm is the scan resolution the
// minutiae were detected with; fgp_view counts the views per finger
// position and has MAX_TABLE_6_CODE entries.
int
init_fvmr(struct finger_view_minutiae_record *fvmr, RECORD *anrecord,
          double ppmm, int *fgp_view) {
    int idx;
    int subfield, item;
    unsigned short x, y, q;
    int tval;
    char buf[8];
    char *c, *saveptr;
    struct finger_minutiae_data *fmd;
    struct finger_extended_data *cd_fed = NULL;
    struct finger_extended_data *rc_fed = NULL;
    struct finger_extended_data *fed;
    struct finger_extended_data_block *fedb;
    struct ridge_count_data *rcd;
    struct core_data *cd;
    struct delta_data *dd;
    FIELD *field;

    /*** Finger number                 ***/
    if (lookup_ANSI_NIST_field(&field, &idx, FGP2_ID, anrecord) == FALSE)
        CONV_ERR("FGP field not found");
    tval = (int) strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);
    if ((tval < 0) || (tval > FMR_MAX_FINGER_CODE) || (tval >= MAX_TABLE_6_CODE))
        CONV_ERR("finger position %d out of range", tval);
    fvmr->finger_number = (unsigned char) tval;

    /*** View number/impression type    ***/
    fvmr->view_number = (unsigned char) fgp_view[fvmr->finger_number];
    fgp_view[fvmr->finger_number]++;

    if (lookup_ANSI_NIST_field(&field, &idx, IMP_ID, anrecord) == FALSE)
        CONV_ERR("IMP_ID field not found");
    fvmr->impression_type = (unsigned char)
            strtol((char *) field->subfields[0]->items[0]->value, NULL, 10);

    // What it should be set to?
    fvmr->impression_type = 0;

    /*** Finger quality                 ***/
    // XXX: What should the overall finger quality be set to?
    fvmr->finger_quality = 0;

    /* Add the core records */
    if (lookup_ANSI_NIST_field(&field, &idx, CRP_ID, anrecord) == TRUE) {
        if ((fedb = fvmr_fedb(fvmr)) == NULL)
            CONV_ALLOC_ERR("extended data block");
        /* The FEDB length does NOT include the
         * block length field itself; it is summed up below.
         */
        if (cd_fed == NULL) {
            if (new_fed(FMR_STD_ANSI, &cd_fed, FED_CORE_AND_DELTA,
                        FED_HEADER_LENGTH) != 0)
                CONV_ALLOC_ERR("extended data record");
            add_fed_to_fedb(cd_fed, fedb);
        }
        cd_fed->length += CORE_DATA_HEADER_LENGTH;

        for (subfield = 0; subfield < field->num_subfields;
             subfield++) {
            if (new_cd(FMR_STD_ANSI, &cd) != 0)
                CONV_ALLOC_ERR("core data");
            add_cd_to_cddb(cd, cd_fed->cddb);
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
            cd_fed->length += CORE_DATA_MIN_LENGTH;
            cd_fed->cddb->num_cores++;
        }
    }
    /* Add the delta records */
    if (lookup_ANSI_NIST_field(&field, &idx, DLT_ID, anrecord) == TRUE) {
        if ((fedb = fvmr_fedb(fvmr)) == NULL)
            CONV_ALLOC_ERR("extended data block");
        if (cd_fed == NULL) {
            if (new_fed(FMR_STD_ANSI, &cd_fed, FED_CORE_AND_DELTA,
                        FED_HEADER_LENGTH) != 0)
                CONV_ALLOC_ERR("extended data record");
            add_fed_to_fedb(cd_fed, fedb);
        }
        cd_fed->length += DELTA_DATA_HEADER_LENGTH;

        for (subfield = 0; subfield < field->num_subfields;
             subfield++) {
            if (new_dd(FMR_STD_ANSI, &dd) != 0)
                CONV_ALLOC_ERR("delta data");
            add_dd_to_cddb(dd, cd_fed->cddb);
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
            cd_fed->length += DELTA_DATA_MIN_LENGTH;
            cd_fed->cddb->num_deltas++;
        }
    }

    /*** Number of minutiae             ***/
    if (lookup_ANSI_NIST_field(&field, &idx, MIN_ID, anrecord) == FALSE)
        CONV_ERR("number of minutiae field not found");
    tval = (int) strtol((char *) field->subfields[0]->items[0]->value,
                        NULL, 10);
    if (tval > FMR_MAX_NUM_MINUTIAE) {
        tval = FMR_MAX_NUM_MINUTIAE;
    }

    /*** Finger minutiae data           ***/
    if (lookup_ANSI_NIST_field(&field, &idx, MRC_ID, anrecord) == FALSE)
        CONV_ERR("minutiae and ridge count data field not found");
    if (tval > field->num_subfields)
        tval = field->num_subfields;
    fvmr->number_of_minutiae = (unsigned char) tval;

    /* For each minutiae index number, create the minutiae data records */
    for (subfield = 0; subfield < fvmr->number_of_minutiae; subfield++) {
        if (new_fmd(FMR_STD_ANSI, &fmd, subfield) != 0)
            CONV_ALLOC_ERR("finger minutiae data record");
        add_fmd_to_fvmr(fmd, fvmr);

        /* The x,y,theta values are in the second item,
         * strung together; separate them.
         */
        memcpy(buf, field->subfields[subfield]->items[1]->value, 4);
        buf[4] = '\0';
        x = (unsigned short) strtoul(buf, (char **) NULL, 10);

        memcpy(buf, &field->subfields[subfield]->items[1]->value[4], 4);
        buf[4] = '\0';
        y = (unsigned short) strtoul(buf, (char **) NULL, 10);

        convert_xy(fvmr->fmr->y_image_size, ppmm, x, y,
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
            if ((fedb = fvmr_fedb(fvmr)) == NULL)
                CONV_ALLOC_ERR("extended data block");
            if (rc_fed == NULL) {
                if (new_fed(FMR_STD_ANSI, &rc_fed, FED_RIDGE_COUNT,
                            FED_HEADER_LENGTH) != 0)
                    CONV_ALLOC_ERR("extended data record");
                add_fed_to_fedb(rc_fed, fedb);
                rc_fed->length += RIDGE_COUNT_HEADER_LENGTH;
                // XXX Set rc_fed->rcdb->method
            }

            for (item = 4;
                 item < field->subfields[subfield]->num_items;
                 item++) {
                if (new_rcd(&rcd) != 0)
                    CONV_ALLOC_ERR("ridge count data");
                add_rcd_to_rcdb(rcd, rc_fed->rcdb);
                rcd->index_one = (unsigned short) strtoul(
                        (char *) field->subfields[subfield]->items[0]->value,
                        (char **) NULL, 10);
                c = strtok_r(
                        (char *) field->subfields[subfield]->items[item]->value,
                        ",", &saveptr);
                if (c == NULL)
                    CONV_ERR("malformed ridge count item");
                rcd->index_two = (unsigned short) strtoul(c,
                                                          (char **) NULL, 10);
                c = strtok_r(NULL, ",", &saveptr);
                if (c == NULL)
                    CONV_ERR("malformed ridge count item");
                rcd->count = (unsigned short) strtoul(c,
                                                      (char **) NULL, 10);
                rc_fed->length += RIDGE_COUNT_DATA_LENGTH;
            }
        }
    }

    /* There is only one extended data block per FVMR; its length is the
     * sum of the extended data records it holds. */
    if (fvmr->extended != NULL) {
        fvmr->extended->block_length = 0;
        TAILQ_FOREACH(fed, &fvmr->extended->extended_data, list)
            fvmr->extended->block_length += fed->length;
    }

    return 0;

    err_out:
    return -1;
}

// Build an ANSI INCITS 378 record from the Type-9 record in an AN2K
// structure. iw and ih are the image size in pixels, ppmm the scan
// resolution the minutiae were detected with.
static int
ansi2fmr(ANSI_NIST *ansi_nist, int iw, int ih, double ppmm,
         struct finger_minutiae_record **ofmr) {
    struct finger_minutiae_record *fmr = NULL;
    struct finger_view_minutiae_record *fvmr;
    FIELD *field;
    int i;
    int idc;
    int idx;

    // Number of views per finger position, for the view number
    int fgp_view[MAX_TABLE_6_CODE] = {0}; // from an2k.h

    for (i = 1; i < ansi_nist->num_records; i++) {
        if (ansi_nist->records[i]->type != TYPE_9_ID)
            continue;

        /*** Image designation character   ***/
        if (lookup_ANSI_NIST_field(&field, &idx, IDC_ID,
                                   ansi_nist->records[i]) == FALSE)
            CONV_ERR("IDC field not found");
        idc = strtol((char *) field->subfields[0]->items[0]->value,
                     NULL, 10);

        if (new_fmr(FMR_STD_ANSI, &fmr) != 0)
            CONV_ALLOC_ERR("FMR");

        if (init_fmr(fmr, ansi_nist, idc) != 0)
            goto err_out;

        // The AN2K structure holds no image record; take the size from the
        // image. The record resolution is in pixels per centimetre.
        fmr->x_image_size = (unsigned short) iw;
        fmr->y_image_size = (unsigned short) ih;
        fmr->x_resolution = (unsigned short) ((ppmm * 10.0) + 0.5);
        fmr->y_resolution = fmr->x_resolution;

        if (new_fvmr(FMR_STD_ANSI, &fvmr) != 0)
            CONV_ALLOC_ERR("FVMR");
        add_fvmr_to_fmr(fvmr, fmr);

        if (init_fvmr(fvmr, ansi_nist->records[i], ppmm, fgp_view) != 0)
            goto err_out;
        fmr->num_views++;

        // An image gives one Type-9 record
        *ofmr = fmr;
        return 0;
    }
    CONV_ERR("no minutiae found in the image");

    err_out:
    if (fmr != NULL)
        free_fmr(fmr);
    return -1;
}

static int
create_type1(RECORD **anrecord) {
    ITEM *item = NULL;
    SUBFIELD *subfield = NULL;
    FIELD *field = NULL;
    RECORD *lrecord = NULL;    // For local convenience
    char buf[32];
    char *date_str;
    time_t tod;
    struct tm tm;

    if (new_ANSI_NIST_record(anrecord, TYPE_1_ID) != 0)
        CONV_ALLOC_ERR("Type-1 record");

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
        CONV_ERR("allocating Type-1 subfield");
    // Add the second item to the subfield, the count of remaining records
    if (value2item(&item, "0") != 0)
        CONV_ERR("allocating Type-1 item");
    if (append_ANSI_NIST_subfield(subfield, item) != 0)
        CONV_ERR("appending Type-1 item");
    item = NULL;
    // Add the subfield to the field
    if (new_ANSI_NIST_field(&field, TYPE_1_ID, CNT_ID) != 0)
        CONV_ERR("allocating Type-1 field");
    if (append_ANSI_NIST_field(field, subfield) != 0)
        CONV_ERR("appending Type-1 subfield");
    subfield = NULL;

    // Add the entire field to the record
    if (append_ANSI_NIST_record(lrecord, field) != 0)
        CONV_ERR("appending Type-1 field");
    field = NULL;

    /*** 1.004 - Type of transaction ***/
    APPEND_TYPE1_FIELD(lrecord, 4, "LFFS");

    /*** 1.005 - Date ***/
    if (get_ANSI_NIST_date(&date_str) != 0)
        CONV_ERR("getting ANSI/NIST date");
    APPEND_TYPE1_FIELD(lrecord, DAT_ID, date_str);
    free(date_str);

    /*** 1.007 - Destination agency identifier ***/
    APPEND_TYPE1_FIELD(lrecord, 7, "ANSI/NIST");

    /*** 1.008 - Originating agency identifier ***/
    APPEND_TYPE1_FIELD(lrecord, 8, "M1/FMR");

    /*** 1.009 - Transaction control number ***/
    // Use the current UTC time string YYYYMMDDHHMMSS
    tod = time(NULL);
    gmtime_r(&tod, &tm);
    snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec);

    APPEND_TYPE1_FIELD(lrecord, 9, buf);

    /*** 1.011 - Native scanning resolution ***/
    // XXX Set the NSR to the minimum, but we may want to base this
    // XXX value on something from the FMR
    snprintf(buf, sizeof(buf), "%5.2f", MIN_RESOLUTION);
    APPEND_TYPE1_FIELD(lrecord, 11, buf);

    /*** 1.012 - Nominal transmitting resolution ***/
    APPEND_TYPE1_FIELD(lrecord, 12, buf);

    if (update_ANSI_NIST_tagged_record_LEN(lrecord) != 0)
        CONV_ERR("updating record length");

    return 0;

    err_out:
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

// Image decoding needs the NBIS decoders and the libraries they use (libjpeg,
// OpenJPEG, libpng, zlib). Builds that get pixels elsewhere, such as the
// WebAssembly build, where the browser decodes the image, define
// CONVERTER_NO_IMAGE_DECODERS and use raw2fmr() only.
#ifndef CONVERTER_NO_IMAGE_DECODERS
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
                free(ndata);
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
                free(ndata);
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

// Read the resolution from the pHYs chunk of a PNG file; the NBIS PNG
// decoder does not report it. Returns pixels per inch, or UNDEFINED when the
// file has no pHYs chunk or gives only an aspect ratio.
static int
png_ppi(unsigned char *data, int len) {
    int pos = 8;            // skip the PNG signature
    uint32_t clen, ppu;

    while (pos + 8 <= len) {
        clen = ((uint32_t) data[pos] << 24) | ((uint32_t) data[pos + 1] << 16) |
               ((uint32_t) data[pos + 2] << 8) | (uint32_t) data[pos + 3];
        // pHYs must come before the image data
        if ((memcmp(&data[pos + 4], "IDAT", 4) == 0) ||
            (memcmp(&data[pos + 4], "IEND", 4) == 0))
            break;
        if (memcmp(&data[pos + 4], "pHYs", 4) == 0) {
            if ((clen < 9) || (pos + 8 + 9 > len))
                break;
            ppu = ((uint32_t) data[pos + 8] << 24) | ((uint32_t) data[pos + 9] << 16) |
                  ((uint32_t) data[pos + 10] << 8) | (uint32_t) data[pos + 11];
            // Unit 1 is pixels per metre; 0 means an aspect ratio only
            if ((data[pos + 16] != 1) || (ppu == 0))
                break;
            return (int) ((ppu * 0.0254) + 0.5);
        }
        if (clen > (uint32_t) (len - pos - 12))
            break;
        pos += 12 + (int) clen;    // length, type, data and CRC
    }
    return UNDEFINED;
}

static int
read_image(unsigned char *indata, int ilen,
           unsigned char **odata, int *olen,
           int *img_type,
           int *iw, int *ih, int *id, int *ippi,
           double *ippmm) {
    int ret;

    *odata = NULL;
    if ((image_type(img_type, indata, ilen) != 0) || (*img_type == UNKNOWN_IMG))
        CONV_ERR("unrecognized image format (WSQ, JPEG, JPEG 2000, PNG and "
                 "NIST IHead are supported)");

    if ((ret = scan_and_decode_image(indata, ilen, img_type, odata, olen,
                                     iw, ih, id, ippi)) != 0) {
        *odata = NULL;
        CONV_ERR("could not decode the input image (NBIS error %d)", ret);
    }

    // MINDTCT works on 8-bit grey-scale images only
    if (*id != 8) {
        free(*odata);
        *odata = NULL;
        CONV_ERR("the image depth is %d bits; only 8-bit grey-scale images "
                 "are supported", *id);
    }

    if ((*ippi == UNDEFINED) && (*img_type == PNG_IMG))
        *ippi = png_ppi(indata, ilen);

    if (*ippi <= 0) {
        *ippi = UNDEFINED;
        *ippmm = DEFAULT_PPI / (double) MM_PER_INCH;
    } else
        *ippmm = *ippi / (double) MM_PER_INCH;
    return 0;

    err_out:
    return -1;
}

#endif // CONVERTER_NO_IMAGE_DECODERS

static int
read_minutiae_to_ansi_fmr(unsigned char *idata, int iw, int ih, int id, double ippmm,
                          struct finger_minutiae_record **fmr) {
    unsigned char *bdata = NULL;
    int bw, bh, bd;
    int *direction_map = NULL, *low_contrast_map = NULL, *low_flow_map = NULL;
    int *high_curve_map = NULL, *quality_map = NULL;
    int map_w, map_h;
    MINUTIAE *minutiae = NULL;
    ANSI_NIST *ansi_nist = NULL;
    RECORD *type1, *type9;
    int img_idc = 1;
    int ret = -1;

    if (get_minutiae(&minutiae, &quality_map, &direction_map,
                     &low_contrast_map, &low_flow_map, &high_curve_map,
                     &map_w, &map_h, &bdata, &bw, &bh, &bd,
                     idata, iw, ih, id, ippmm, &lfsparms_V2) != 0) {
        set_error("minutiae detection failed");
        return -1;
    }

    if (alloc_ANSI_NIST(&ansi_nist) != 0) {
        ansi_nist = NULL;
        CONV_ALLOC_ERR("AN2K record");
    }

    if (create_type1(&type1) != 0)
        CONV_ERR("could not create the AN2K Type-1 record");
    if (update_ANSI_NIST(ansi_nist, type1) != 0) {
        free_ANSI_NIST_record(type1);
        CONV_ERR("could not insert the AN2K Type-1 record");
    }

    // Only the Type-9 minutiae record is needed. NBIS
    // update_ANSI_NIST_lfs_results() would also store a copy of the image
    // as a Type-14 record, and leaks memory doing so.
    if (minutiae->num < 1)
        CONV_ERR("no minutiae found in the image");
    if (minutiae2type_9(&type9, img_idc, minutiae, iw, ih, ippmm) != 0)
        CONV_ERR("could not create the AN2K Type-9 record from the minutiae");
    if (insert_ANSI_NIST_record_frmem(ansi_nist->num_records, type9, ansi_nist) != 0) {
        free_ANSI_NIST_record(type9);
        CONV_ERR("could not insert the AN2K Type-9 record");
    }

    if (ansi2fmr(ansi_nist, iw, ih, ippmm, fmr) != 0)
        goto err_out;

    ret = 0;

    err_out:
    free_minutiae(minutiae);
    free(quality_map);
    free(direction_map);
    free(low_contrast_map);
    free(low_flow_map);
    free(high_curve_map);
    free(bdata);
    if (ansi_nist != NULL)
        free_ANSI_NIST(ansi_nist);
    return ret;
}

static int
str_to_type(char *stdstr) {
    if (stdstr == NULL)
        return (-1);
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

#define IS_ISO_CARD(type) \
    (((type) == FMR_STD_ISO_NORMAL_CARD) || ((type) == FMR_STD_ISO_COMPACT_CARD))

// Resolution assumed for ISO card input when none is given: 500 ppi, in
// pixels per centimetre as in the record header
#define DEFAULT_RESOLUTION_PPCM 197

// Number of minutiae in the list of a finger view; get_fmd_count() returns
// the number_of_minutiae field instead.
static int
count_fmds(struct finger_view_minutiae_record *fvmr) {
    struct finger_minutiae_data *fmd;
    int count = 0;

    TAILQ_FOREACH(fmd, &fvmr->minutiae_data, list)
        count++;
    return count;
}

// The compact card format stores each coordinate in one byte, in units of
// 0.1 mm, so it covers 25.5 mm by 25.5 mm from the origin. Leave out the
// minutiae beyond that rather than write wrapped-around positions.
static void
drop_isocc_out_of_range(struct finger_view_minutiae_record *fvmr) {
    struct finger_minutiae_data *fmd, *next;

    for (fmd = TAILQ_FIRST(&fvmr->minutiae_data); fmd != NULL; fmd = next) {
        next = TAILQ_NEXT(fmd, list);
        if ((fmd->x_coord > 255) || (fmd->y_coord > 255)) {
            TAILQ_REMOVE(&fvmr->minutiae_data, fmd, list);
            free_fmd(fmd);
        }
    }
    fvmr->number_of_minutiae = (unsigned char) count_fmds(fvmr);
}

// Size of one coordinate unit of an ISO format in millimetres. The record
// format counts pixels, whose size follows from the resolution in pixels
// per centimetre.
static double
iso_coord_unit_mm(unsigned int format_std, unsigned short res) {
    switch (format_std) {
        case FMR_STD_ISO_NORMAL_CARD:
            return 0.01;
        case FMR_STD_ISO_COMPACT_CARD:
            return 0.1;
        default:
            return 10.0 / (double) res;
    }
}

static double
iso_angle_unit(unsigned int format_std) {
    if (format_std == FMR_STD_ISO_COMPACT_CARD)
        return FMD_ISOCC_ANGLE_UNIT;
    return FMD_ISO_ANGLE_UNIT;
}

// Convert a finger view between the ISO record, normal card and compact card
// formats. BiomDI has no routine for this, and going through ANSI would round
// every angle to 2 degrees. Positions are converted through millimetres,
// angles between the 256-step and the 64-step units. xres and yres are in
// pixels per centimetre.
static int
iso2iso_fvmr(struct finger_view_minutiae_record *ifvmr,
             struct finger_view_minutiae_record *ofvmr,
             unsigned short xres, unsigned short yres) {
    struct finger_minutiae_data *ifmd, *ofmd;
    double xscale, yscale, ascale;
    int steps;

    xscale = iso_coord_unit_mm(ifvmr->format_std, xres) /
             iso_coord_unit_mm(ofvmr->format_std, xres);
    yscale = iso_coord_unit_mm(ifvmr->format_std, yres) /
             iso_coord_unit_mm(ofvmr->format_std, yres);
    ascale = iso_angle_unit(ifvmr->format_std) / iso_angle_unit(ofvmr->format_std);
    steps = (int) (360.0 / iso_angle_unit(ofvmr->format_std) + 0.5);

    COPY_FVMR(ifvmr, ofvmr);
    TAILQ_FOREACH(ifmd, &ifvmr->minutiae_data, list) {
        if (new_fmd(ofvmr->format_std, &ofmd, ifmd->index) != 0)
            CONV_ALLOC_ERR("output FMD");
        COPY_FMD(ifmd, ofmd);
        add_fmd_to_fvmr(ofmd, ofvmr);

        ofmd->x_coord = (unsigned short) (((double) ifmd->x_coord * xscale) + 0.5);
        ofmd->y_coord = (unsigned short) (((double) ifmd->y_coord * yscale) + 0.5);
        ofmd->angle = (unsigned char)
                (((int) (((double) ifmd->angle * ascale) + 0.5)) % steps);
    }
    ofvmr->number_of_minutiae = (unsigned char) count_fmds(ofvmr);
    return 0;

    err_out:
    return -1;
}

static int
copy_without_conversion(FMR *ifmr, FMR *ofmr, int fmr_type) {
    FVMR *ifvmr, *ofvmr;
    FMD *ifmd, *ofmd;

    COPY_FMR(ifmr, ofmr);

    if (get_fvmr_count(ifmr) < 1)
        CONV_ERR("there are no finger views in the input record");

    TAILQ_FOREACH(ifvmr, &ifmr->finger_views, list) {
        if (new_fvmr(fmr_type, &ofvmr) < 0)
            CONV_ALLOC_ERR("output FVMR");
        add_fvmr_to_fmr(ofvmr, ofmr);

        COPY_FVMR(ifvmr, ofvmr);
        TAILQ_FOREACH(ifmd, &ifvmr->minutiae_data, list) {
            if (new_fmd(fmr_type, &ofmd, ifmd->index) != 0)
                CONV_ALLOC_ERR("output FMD");
            COPY_FMD(ifmd, ofmd);
            add_fmd_to_fvmr(ofmd, ofvmr);
        }
        // XXX Copy the FEDB to the output fmr; extended data is dropped
    }
    return 0;

    err_out:
    return -1;
}

/*
 * Copy an FMR with conversion.
 */
static int
copy_with_conversion(FMR *ifmr, FMR *ofmr, int in_type, int out_type) {
    FVMR *ifvmr, *ofvmr;
    unsigned int fvmr_len;
    int rcount, rc;

    if (in_type == out_type)
        CONV_ERR("input and output types are the same");

    COPY_FMR(ifmr, ofmr);

    /* Fix up the output FMR header for those input types that don't
     * have all the needed information.
     */
    if (((out_type == FMR_STD_ANSI) || (out_type == FMR_STD_ISO)) &&
        IS_ISO_CARD(in_type)) {
        strncpy(ofmr->format_id, FMR_FORMAT_ID, FMR_FORMAT_ID_LEN);
        strncpy(ofmr->spec_version,
                out_type == FMR_STD_ANSI ? FMR_ANSI_SPEC_VERSION : FMR_ISO_SPEC_VERSION,
                FMR_SPEC_VERSION_LEN);
    }

    /* ISO records carry no CBEFF product identifier, which ANSI requires;
     * write the same placeholder as for image input.
     */
    if ((out_type == FMR_STD_ANSI) && (in_type != FMR_STD_ANSI)) {
        ofmr->product_identifier_owner = 1;
        ofmr->product_identifier_type = 1;
    }

    rcount = get_fvmr_count(ifmr);
    if (rcount < 1)
        CONV_ERR("there are no finger views in the input record");
    if (IS_ISO_CARD(out_type) && (rcount > 1))
        CONV_ERR("the ISO card formats hold one finger view; the input "
                 "record has %d", rcount);

    TAILQ_FOREACH(ifvmr, &ifmr->finger_views, list) {
        if (new_fvmr(out_type, &ofvmr) < 0)
            CONV_ALLOC_ERR("output FVMR");
        add_fvmr_to_fmr(ofvmr, ofmr);

        if (in_type == FMR_STD_ANSI) {
            if (out_type == FMR_STD_ISO_COMPACT_CARD)
                rc = ansi2isocc_fvmr(ifvmr, ofvmr, &fvmr_len,
                                     ifmr->x_resolution, ifmr->y_resolution);
            else
                rc = ansi2iso_fvmr(ifvmr, ofvmr, &fvmr_len,
                                   ifmr->x_resolution, ifmr->y_resolution);
        } else if (out_type == FMR_STD_ANSI) {
            if (in_type == FMR_STD_ISO_COMPACT_CARD)
                rc = isocc2ansi_fvmr(ifvmr, ofvmr, &fvmr_len,
                                     ifmr->x_resolution, ifmr->y_resolution);
            else
                rc = iso2ansi_fvmr(ifvmr, ofvmr, &fvmr_len,
                                   ifmr->x_resolution, ifmr->y_resolution);
        } else
            rc = iso2iso_fvmr(ifvmr, ofvmr,
                              ifmr->x_resolution, ifmr->y_resolution);
        if (rc != 0)
            CONV_ERR("could not convert the finger view");

        // XXX Copy the FEDB to the output fmr; extended data is dropped
        ofvmr->extended = NULL;

        if (out_type == FMR_STD_ISO_COMPACT_CARD)
            drop_isocc_out_of_range(ofvmr);
    }
    return 0;

    err_out:
    return -1;
}

// Set the record length from the content of the record and serialize the
// record into a new buffer, which the caller frees.
static int
write_fmr_to_buffer(struct finger_minutiae_record *fmr,
                    unsigned char **odata, int *olen) {
    struct finger_view_minutiae_record *fvmr;
    unsigned int len, count;
    uint8_t *buf = NULL;
    BDB bdb;

    switch (fmr->format_std) {
        case FMR_STD_ANSI:
            len = FMR_ANSI_SMALL_HEADER_LENGTH;
            break;
        case FMR_STD_ISO:
            len = FMR_ISO_HEADER_LENGTH;
            break;
        default:
            len = 0;    // the card formats have no header
            break;
    }

    fmr->num_views = 0;
    TAILQ_FOREACH(fvmr, &fmr->finger_views, list) {
        count = count_fmds(fvmr);
        fvmr->number_of_minutiae = (unsigned char) count;
        fmr->num_views++;
        switch (fmr->format_std) {
            case FMR_STD_ISO_NORMAL_CARD:
                len += count * FMD_ISO_NORMAL_DATA_LENGTH;
                break;
            case FMR_STD_ISO_COMPACT_CARD:
                len += count * FMD_ISO_COMPACT_DATA_LENGTH;
                break;
            default:
                // The extended data block length is always written
                len += FVMR_HEADER_LENGTH + (count * FMD_DATA_LENGTH) +
                       FEDB_HEADER_LENGTH;
                if (fvmr->extended != NULL)
                    len += fvmr->extended->block_length;
                break;
        }
    }

    if (fmr->format_std == FMR_STD_ANSI) {
        fmr->record_length_type = FMR_ANSI_SMALL_HEADER_TYPE;
        if (len > FMR_ANSI_MAX_SHORT_LENGTH) {
            len += FMR_ANSI_LARGE_HEADER_LENGTH - FMR_ANSI_SMALL_HEADER_LENGTH;
            fmr->record_length_type = FMR_ANSI_LARGE_HEADER_TYPE;
        }
    } else if (fmr->format_std == FMR_STD_ISO)
        fmr->record_length_type = FMR_ISO_HEADER_TYPE;
    fmr->record_length = len;

    // A card record without minutiae is empty; malloc(0) may return NULL
    if ((buf = (uint8_t *) malloc(len > 0 ? len : 1)) == NULL)
        CONV_ALLOC_ERR("output buffer");
    INIT_BDB(&bdb, buf, len);

    if (push_fmr(&bdb, fmr) != WRITE_OK)
        CONV_ERR("could not write the output record");
    if ((unsigned int) (bdb.bdb_current - bdb.bdb_start) != len)
        CONV_ERR("output record length mismatch (%u of %u bytes written)",
                 (unsigned int) (bdb.bdb_current - bdb.bdb_start), len);

    *odata = buf;
    *olen = (int) len;
    return 0;

    err_out:
    free(buf);
    return -1;
}

// Detect the minutiae in an 8-bit grey-scale image and write them as a record
// of type out_type. Called with nbis_lock held for the detection only.
static int
pixels_to_fmr(unsigned char *pixels, int iw, int ih, double ippmm, int out_type,
              unsigned char **odata, int *olen) {
    struct finger_minutiae_record *fmr = NULL;
    struct finger_minutiae_record *ofmr = NULL;
    int rc, ret = -1;

    pthread_mutex_lock(&nbis_lock);
    rc = read_minutiae_to_ansi_fmr(pixels, iw, ih, 8, ippmm, &fmr);
    pthread_mutex_unlock(&nbis_lock);
    if (rc != 0)
        goto err_out;

    if (out_type == FMR_STD_ANSI) {
        ofmr = fmr;
        fmr = NULL;
    } else {
        if (new_fmr(out_type, &ofmr) != 0)
            CONV_ALLOC_ERR("output FMR");
        if (copy_with_conversion(fmr, ofmr, FMR_STD_ANSI, out_type) != 0)
            goto err_out;
    }

    if (write_fmr_to_buffer(ofmr, odata, olen) != 0)
        goto err_out;

    // Some helpers record a message for conditions they recover from
    last_error[0] = '\0';
    ret = 0;

    err_out:
    if (fmr != NULL)
        free_fmr(fmr);
    if (ofmr != NULL)
        free_fmr(ofmr);
    return ret;
}

// Check the output arguments and type shared by img2fmr() and raw2fmr().
static int
start_image_conversion(char *otype, unsigned char **odata, int *olen,
                       int *out_type) {
    last_error[0] = '\0';
    if ((odata == NULL) || (olen == NULL))
        CONV_ERR("no output location given");
    *odata = NULL;
    *olen = 0;
    if ((*out_type = str_to_type(otype)) < 0)
        CONV_ERR("unknown output type '%s' (use ANSI, ISO, ISONC or ISOCC)",
                 otype != NULL ? otype : "");
    return 0;

    err_out:
    return -1;
}

int raw2fmr(unsigned char *pixels, int width, int height, int ppi, char *otype,
            unsigned char **odata, int *olen) {
    int out_type;

    if (start_image_conversion(otype, odata, olen, &out_type) != 0)
        return -1;
    if ((pixels == NULL) || (width <= 0) || (height <= 0)) {
        set_error("no input image");
        return -1;
    }
    if (ppi <= 0)
        ppi = DEFAULT_PPI;
    return pixels_to_fmr(pixels, width, height, ppi / (double) MM_PER_INCH,
                         out_type, odata, olen);
}

#ifndef CONVERTER_NO_IMAGE_DECODERS
int img2fmr(unsigned char *idata, int ilen, char *otype, unsigned char **odata, int *olen) {
    unsigned char *imdata = NULL;
    int img_len, img_type, iw, ih, id, ippi, out_type, rc;
    double ippmm;

    if (start_image_conversion(otype, odata, olen, &out_type) != 0)
        return -1;
    if ((idata == NULL) || (ilen <= 0)) {
        set_error("no input data");
        return -1;
    }

    // The decoders keep global state too
    pthread_mutex_lock(&nbis_lock);
    rc = read_image(idata, ilen, &imdata, &img_len, &img_type,
                    &iw, &ih, &id, &ippi, &ippmm);
    pthread_mutex_unlock(&nbis_lock);
    if (rc != 0)
        return -1;

    rc = pixels_to_fmr(imdata, iw, ih, ippmm, out_type, odata, olen);
    free(imdata);
    return rc;
}
#endif // CONVERTER_NO_IMAGE_DECODERS

int fmr2fmr(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
            char *in_type_str, char *out_type_str) {
    return fmr2fmr_iso_card(idata, ilen, odata, olen, in_type_str, out_type_str, 0, 0);
}

int fmr2fmr_iso_card(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                     char *in_type_str, char *out_type_str, int iso_c_xres, int iso_c_yres) {

    BDB rbdb;
    struct finger_minutiae_record *ifmr = NULL;
    struct finger_minutiae_record *ofmr = NULL;
    int in_type, out_type, rc;
    int ret = -1;

    last_error[0] = '\0';
    if ((odata == NULL) || (olen == NULL))
        CONV_ERR("no output location given");
    *odata = NULL;
    *olen = 0;
    if ((idata == NULL) || (ilen <= 0))
        CONV_ERR("no input data");
    if ((in_type = str_to_type(in_type_str)) < 0)
        CONV_ERR("unknown input type '%s' (use ANSI, ISO, ISONC or ISOCC)",
                 in_type_str != NULL ? in_type_str : "");
    if ((out_type = str_to_type(out_type_str)) < 0)
        CONV_ERR("unknown output type '%s' (use ANSI, ISO, ISONC or ISOCC)",
                 out_type_str != NULL ? out_type_str : "");

    // The ANSI and ISO record formats start with the format identifier
    if (!IS_ISO_CARD(in_type) &&
        ((ilen < FMR_FORMAT_ID_LEN) ||
         (memcmp(idata, FMR_FORMAT_ID, FMR_FORMAT_ID_LEN) != 0)))
        CONV_ERR("the input is not an %s finger minutiae record (no FMR "
                 "identifier)", in_type_str);

    if ((new_fmr(in_type, &ifmr) != 0) || (new_fmr(out_type, &ofmr) != 0))
        CONV_ALLOC_ERR("FMR");

    INIT_BDB(&rbdb, idata, ilen);
    if (scan_fmr(&rbdb, ifmr) != READ_OK)
        CONV_ERR("could not read the input as a record of type %s", in_type_str);

    /* ISO card formats have no input resolution, so set it here
     * from the arguments, or assume 500 ppi.
     */
    if (IS_ISO_CARD(in_type)) {
        ifmr->x_resolution = iso_c_xres > 0 ? iso_c_xres : DEFAULT_RESOLUTION_PPCM;
        ifmr->y_resolution = iso_c_yres > 0 ? iso_c_yres : DEFAULT_RESOLUTION_PPCM;
    } else if (IS_ISO_CARD(out_type) &&
               ((ifmr->x_resolution == 0) || (ifmr->y_resolution == 0)))
        CONV_ERR("the input record gives no resolution, which the "
                 "conversion to a card format needs");

    /* If the input and output file types are the same,
     * do a straight copy.
     */
    if (in_type == out_type)
        rc = copy_without_conversion(ifmr, ofmr, in_type);
    else
        rc = copy_with_conversion(ifmr, ofmr, in_type, out_type);
    if (rc != 0)
        goto err_out;

    if (write_fmr_to_buffer(ofmr, odata, olen) != 0)
        goto err_out;

    // Some helpers record a message for conditions they recover from
    last_error[0] = '\0';
    ret = 0;

    err_out:
    if (ifmr != NULL)
        free_fmr(ifmr);
    if (ofmr != NULL)
        free_fmr(ofmr);
    return ret;
}
