/*******************************************************************************

License: 
This software and/or related materials was developed at the National Institute
of Standards and Technology (NIST) by employees of the Federal Government
in the course of their official duties. Pursuant to title 17 Section 105
of the United States Code, this software is not subject to copyright
protection and is in the public domain. 

This software and/or related materials have been determined to be not subject
to the EAR (see Part 734.3 of the EAR for exact details) because it is
a publicly available technology and software, and is freely distributed
to any interested party with no licensing requirements.  Therefore, it is 
permissible to distribute this software as a free download from the internet.

Disclaimer: 
This software and/or related materials was developed to promote biometric
standards and biometric technology testing for the Federal Government
in accordance with the USA PATRIOT Act and the Enhanced Border Security
and Visa Entry Reform Act. Specific hardware and software products identified
in this software were used in order to perform the software development.
In no case does such identification imply recommendation or endorsement
by the National Institute of Standards and Technology, nor does it imply that
the products and equipment identified are necessarily the best available
for the purpose.

This software and/or related materials are provided "AS-IS" without warranty
of any kind including NO WARRANTY OF PERFORMANCE, MERCHANTABILITY,
NO WARRANTY OF NON-INFRINGEMENT OF ANY 3RD PARTY INTELLECTUAL PROPERTY
or FITNESS FOR A PARTICULAR PURPOSE or for any purpose whatsoever, for the
licensed product, however used. In no event shall NIST be liable for any
damages and/or costs, including but not limited to incidental or consequential
damages of any kind, including economic damage or injury to property and lost
profits, regardless of whether NIST shall be advised, have reason to know,
or in fact shall know of the possibility.

By using this software, you agree to bear all risk relating to quality,
use and performance of the software and/or related materials.  You agree
to hold the Government harmless from any claim arising from your use
of the software.

*******************************************************************************/

/***********************************************************************
      PACKAGE: ANSI/NIST 2007 Standard Reference Implementation

      FILE:    AN2K.H

      AUTHORS: Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/08/2005
               10/10/2007 (Kenneth Ko)
               12/12/2007 (Kenneth Ko)
               01/31/2008 (Kenneth Ko)
               02/27/2008 Joseph C. Konczal
      UPDATE:  01/26/2008 jck - report more details when things go wrong

***********************************************************************/
#ifndef _AN2K_H
#define _AN2K_H

#include <stdio.h>  /* Needed for references to (FILE *) below */
#include <stdlib.h> /* Added by MDG on 03-08-05 */
#include <string.h> /* Added by MDG on 03-08-05 */
#include <stdarg.h> /* Added by JCK on 03-06-08 - for varargs macros */
#include <errno.h>  /* Added by JCK on 01-22-09 */

#define AN2K_RUNTIME_DATA_DIR	"SED_INSTALL_DATA_DIR_STRING/an2k"

#define SHORT_READ_ERR_MSG(fp) ((ferror(fp) != 0) ? strerror(errno) : "premature EOF")

#define SHORT_SCAN_READ_ERR_MSG(fp, bdb) ((fp != NULL) ?		\
    ((ferror(fp) != 0) ? strerror(errno) : "premature EOF")		\
     : "buffer exhausted")

/*
 * A structure to represent the buffer is wrapped for safety, and contains
 * the start, current, end pointer values.
 */
typedef struct basic_data_buffer {
    int bdb_size;		/* Max size of the buffer */
    unsigned char *bdb_start;	/* Beginning read/write location */
    unsigned char *bdb_end;	/* End read/write location */
    unsigned char *bdb_current;	/* Current read/write location */
} AN2KBDB;

#define INIT_AN2KBDB(bdb, ptr, size) do {		\
    (bdb)->bdb_size = size;				\
    (bdb)->bdb_start = (bdb)->bdb_current = ptr;	\
    (bdb)->bdb_end = ptr + size;			\
} while (0)

/* characters in items */
typedef struct item{
   int num_bytes;   /* Always contains the current byte size of the entire */
                    /* item including any trailing US separator.           */
   int num_chars;   /* Number of characters currently in value, NOT  */
                    /* including the NULL terminator.                */
   int alloc_chars; /* Number of allocated characters for the value, */
                    /* including the NULL terminator.                */
   unsigned char *value;  /* Must keep NULL terminated.              */
   int us_char;
} ITEM;

/* items in subfields */
typedef struct subfield{
   int num_bytes;
   int num_items;
   int alloc_items;
   ITEM **items;
   int rs_char;
} SUBFIELD;

/* subfields in fields */
typedef struct field{
   char *id;
   unsigned int record_type;
   unsigned int field_int;
   int num_bytes;
   int num_subfields;
   int alloc_subfields;
   SUBFIELD **subfields;
   int gs_char;
} FIELD;

/* fields in records */
typedef struct record{
   unsigned int type;
   int total_bytes;
   int num_bytes;
   int num_fields;
   int alloc_fields;
   FIELD **fields;
   int fs_char;
} RECORD;

/* records in ANSI_NIST file */
typedef struct ansi_nist{
   unsigned int version;
   int num_bytes;
   int num_records;
   int alloc_records;
   RECORD **records;
} ANSI_NIST;

/* criteria used to select records of interest:

  These structures are designed to represent combinations of criteria
  used to select single or multiple records out of an2k files.
  Criteria can be combined using boolean operations such as 'and'
  and/or 'or'.  These structures can be nested in order to represent
  various combinations of criteria combined using different boolean
  operators.
 */
typedef enum rec_sel_type_e {
   rs_and = 1000,
   rs_or,
   rs_lrt,			/* logical record type */
   rs_fgplp,			/* finger or palm position */
   rs_fgp,			/* finger position */
   rs_plp,			/* palm position */
   rs_imp,			/* impression type */
   rs_idc,			/* image descriptor chararacter */
   rs_nqm,			/* NIST quality metric */
   rs_imt,
   rs_pos			/* subject pose */
} REC_SEL_TYPE;

typedef enum rec_sel_value_type_e {
   rsv_rs = 2000,
   rsv_num,
   rsv_str
} REC_SEL_VALUE_TYPE;

typedef union rec_sel_value_u {
   long num;  /* initialization assumes a pointer is never larger than a long */
   char *str;
   struct rec_sel_s **rs;
} REC_SEL_VALUE;

typedef struct rec_sel_s {
   REC_SEL_TYPE type;
   int alloc_values;
   int num_values;
   REC_SEL_VALUE value;
} REC_SEL;

/* end of record selection criteria */

/* Structures to hold segmentation data */
typedef struct polygon_s {
   int fgp;
   int num_points;
   int *x;
   int *y;
} POLYGON;

typedef struct segments_s {
   int num_polygons;
   POLYGON *polygons;
} SEGMENTS;
/* End of segmentation data structures */

#define ANSI_NIST_CHUNK   100
#ifndef TRUE
#define TRUE              1
#define FALSE             0
#endif
#define UNSET             -1
#define DONE              2
#define MORE              3
#define UNDEFINED_INT     -1
#ifndef IGNORE
#define IGNORE            2
#endif

#define TYPE_1_ID         1
#define TYPE_1_NUM_MANDATORY_FIELDS 9
#define TYPE_2_ID         2
#define TYPE_3_ID         3
#define TYPE_4_ID         4
#define TYPE_5_ID         5
#define TYPE_6_ID         6
#define TYPE_7_ID         7
#define TYPE_8_ID         8
#define TYPE_9_ID         9
#define TYPE_10_ID       10
#define TYPE_11_ID       11
#define TYPE_12_ID       12
#define TYPE_13_ID       13
#define TYPE_14_ID       14
#define TYPE_15_ID       15
#define TYPE_16_ID       16
#define TYPE_17_ID       17
#define TYPE_99_ID       99

/* Type-1 Field IDs */
#define LEN_ID            1
#define VER_ID            2
#define CNT_ID            3
#define TOT_ID            4
#define DAT_ID            5
#define PRY_ID            6
#define DAI_ID            7
#define ORI_ID            8
#define TCN_ID            9
#define TCR_ID           10
#define NSR_ID           11
#define NTR_ID           12
#define DOM_ID           13
#define GMT_ID           14
#define DCS_ID           15

#define IDC_FMT           "%02d"
#define FLD_FMT           "%d.%03d:"
#define ASCII_CSID        0


/* ANSI/NIST-CSL 1-1993 */
#define VERSION_0200      200
/* ANSI/NIST-ITL 1a-1997 */
#define VERSION_0201      201
/* ANSI/NIST-ITL 1-2000 */
#define VERSION_0300      300
/* ANSI/NIST-ITL 1-2007 */
#define VERSION_0400      400
/* ANSI/NIST-ITL 1-2011 */
#define VERSION_0500      500
/* ANSI/NIST-ITL 1-2011 Update 2013 */
#define VERSION_0501      501
/* ANSI/NIST-ITL 1-2011 Update 2015 */
#define VERSION_0502      502

#define FS_CHAR           0x1C
#define GS_CHAR           0x1D
#define RS_CHAR           0x1E
#define US_CHAR           0x1F

extern unsigned int biomeval_nbis_tagged_records[];
#define NUM_TAGGED_RECORDS           10

extern unsigned int biomeval_nbis_binary_records[];
#define NUM_BINARY_RECORDS           6

extern unsigned int tagged_image_records[];
#define NUM_TAGGED_IMAGE_RECORDS     7
#define IMAGE_FIELD                999

extern unsigned int biomeval_nbis_binary_image_records[];
#define NUM_BINARY_IMAGE_RECORDS     5
#define BINARY_LEN_BYTES             4
#define BINARY_IDC_BYTES             1
#define BINARY_IMP_BYTES             1
#define BINARY_FGP_BYTES             6
#define BINARY_ISR_BYTES             1
#define BINARY_HLL_BYTES             2
#define BINARY_VLL_BYTES             2
#define BINARY_CA_BYTES              1
#define NUM_BINARY_IMAGE_FIELDS      9

/* Type-3,4,5,6 Field IDs */
/* LEN_ID       1 defined above */
#define IDC_ID                       2
#define IMP_ID                       3
#define FGP_ID                       4
#define ISR_ID                       5
#define HLL_ID                       6
#define VLL_ID                       7
#define BIN_CA_ID                    8
#define BIN_IMAGE_ID                 9

extern unsigned int biomeval_nbis_binary_signature_records[];
#define NUM_BINARY_SIGNATURE_RECORDS 1
#define BINARY_SIG_BYTES             1
#define BINARY_SRT_BYTES             1
#define NUM_BINARY_SIGNATURE_FIELDS  8

/* Type-8 Field IDs */
/* LEN_ID       1 defined above */
/* IDC_ID       2 defined above */
#define SIG_ID                       3
#define SRT_ID                       4
/* ISR_ID       5 defined above */
/* HLL_ID       6 defined above */
/* VLL_ID       7 defined above */

/* Type-10,13,14,15,16 Field IDs */
/* LEN_ID       1 defined above */
/* IDC_ID       2 defined above */
/* IMP_ID       3 defined above */
#define SRC_ID                        4
#define CD_ID                         5
/* HLL_ID       6 defined above */
/* VLL_ID       7 defined above */
#define SLC_ID                        8
#define HPS_ID                        9
#define VPS_ID                       10
#define TAG_CA_ID                    11
#define CSP_ID                       12
#define CSP_ID_Type_17               13
#define BPX_ID                       12
#define FGP3_ID                      13
#define DAT2_ID             IMAGE_FIELD

/* Type-10 field IDs, in addition the the common subset above... jck */
#define IMT_ID                        3
#define PHD_ID                        5
/* 6 HLL, 7 VLL, 8 SLC, 9 HPS, 10 VPS, 11 CGA (TAG_CA_ID), 12CSP */
#define SAP_ID                       13
/* 14 and 15 are reserved */
/* 16 SHPS, 17 SVPS */
/* 18 and 19 are reserved */
#define POS_ID                       20
#define POA_ID                       21
#define PXS_ID                       22
#define PAS_ID                       23
#define SQS_ID                       24
#define SPA_ID                       25
#define SXS_ID                       26
#define SEC_ID                       27
#define SHC_ID                       28
#define FFP_ID                       29
#define DMM_ID                       30
/* 31 through 39 are reserved */
#define SMT_ID                       40
#define SMS_ID                       41
#define SMD_ID                       42
#define COL_ID                       43
/* 44 through 199 reserved */

/* Type-13,14,15 field IDs, in addition to the common subset above... jck */
/* Type-13,14 respecively, reserved in type 15... */
#define SPD_ID                       14
#define PPD_ID                       14

/* Type-13,14, reserved in type 15... */
#define PPC_ID                       15

/* Type-13,14,15... */
#define SHPS_ID                      16
#define SVPS_ID                      17

/* Type-14 only, reserved in Type-13,15... */
#define AMP_ID                       18

/* 19 is reserved in Type-13,14,15. */
/* Type-13,14,15...*/
#define COM_ID                       20

/* Type-14 only, reserved in Type-13,15 */
#define SEG_ID                       21
#define NQM_ID                       22
#define SQM_ID                       23

/* Type-13,14,15 respecively... */
#define LQM_ID                       24
#define FQM_ID                       24
#define PQM_ID                       24

/* Type-14 only, reserved in Type-13,15... */
#define ASEG_ID                      25

/* 26 through 29 are reserved in Type-13,14,15. */

/* Type-14,15, reserved in Type-13... */
#define DMM_ID                      30
/* End of Type-13,14,15 field IDs. */

/* Type-9 Standard Field IDs */
/* LEN_ID       1 defined above */
/* IDC_ID       2 defined above */
/* IMP_ID       3 defined above */
#define FMT_ID                        4
#define OFR_ID                        5
#define FGP2_ID                       6
#define FPC_ID                        7
#define CRP_ID                        8
#define DLT_ID                        9
#define MIN_ID                       10
#define RDG_ID                       11
#define MRC_ID                       12
/* Type-9 FBI/IAFIS Field IDs  */
/* EFTS Field 13 Non-standard! */
#define FGN_ID                       14
#define NMN_ID                       15
#define FCP_ID                       16
#define APC_ID                       17
#define ROV_ID                       18
#define COF_ID                       19
#define ORN_ID                       20
#define CRA_ID                       21
#define DLA_ID                       22
#define MAT_ID                       23

/* Maximum number of minutiae in an FBI/IAFIS Type-9 record. */
#define MAX_IAFIS_MINUTIAE          254
/* Maximum number of pattern classes in an FBI/IAFIS Type-9 record. */
#define MAX_IAFIS_PATTERN_CLASSES     3
/* Maximum number of cores in an FBI/IAFIS Type-9 record. */
#define MAX_IAFIS_CORES               2
/* Maximum number of deltas in an FBI/IAFIS Type-9 record. */
#define MAX_IAFIS_DELTAS              2
/* Maximum number of items in FBI/IAFIS minutia subfield. */
#define MAX_IAFIS_MINUTIA_ITEMS      13
/* Number of characters in an FBI/IAFIS method string. */
#define IAFIS_METHOD_STRLEN           3

/* Minimum Table 5 Impression Code. */
#define MIN_TABLE_5_CODE              0
/* Maximum Table 5 Impression Code. */
#define MAX_TABLE_5_CODE              29

/* Minimum Table 6 Finger Position Code. */
#define MIN_TABLE_6_CODE              0
/* Maximum Table 6 Finger Position Code. */
#define MAX_TABLE_6_CODE             16

/* Minimum Table 19 Palm Code. */
#define MIN_TABLE_19_CODE            20
/* Maximum Table 19 Palm Code. */
#define MAX_TABLE_19_CODE            30

/* Minimum Minutia Quality value. */
#define MIN_QUALITY_VALUE             0
/* Maximum Minutia Quality value. */
#define MAX_QUALITY_VALUE            63

/* Minimum scanning resolution in pixels/mm (500 dpi). */
#define MIN_RESOLUTION               19.69
/* Minimum scanning resolution in as stored in tagged field images. */
#define MIN_TAGGED_RESOLUTION        19.7
/* Scan resolution tolerance in mm's. */
#define MM_TOLERANCE                  0.2

#define FIELD_NUM_LEN              9
#define ITEM_START                 '='
#define ITEM_END                   US_CHAR

#define STD_STR                    "S"
#define USER_STR                   "U"
#define TBL_STR                    "T"
#define AUTO_STR                   "A"
#define PPI_STR                    "1"
#define PP_CM                      "2"

#define DEL_OP                     'd'
#define INS_OP                     'i'
#define PRN_OP                     'p'
#define SUB_OP                     's'

#define DEFAULT_FPOUT              stdout

#ifndef MAX_UINT_CHARS
#define MAX_UINT_CHARS   10
#define MAX_USHORT_CHARS  5
#define MAX_UCHAR_CHARS   3
#endif

#define UNUSED_STR                 "255"

#define MM_PER_INCH                 25.4

#define UNKNOWN_HAND                 0
#define RIGHT_HAND                   1
#define LEFT_HAND                    2

#define COMP_NONE                  "NONE"
#define BIN_COMP_NONE              "0"
#define COMP_WSQ                   "WSQ20"
#define BIN_COMP_WSQ               "1"
#define COMP_JPEGB                 "JPEGB"
#define BIN_COMP_JPEGB             "2"
#define COMP_JPEGL                 "JPEGL"
#define BIN_COMP_JPEGL             "3"
#define COMP_JPEG2K                "JP2"
#define BIN_COMP_JPEG2K            "4"
#define COMP_JPEG2KL               "JP2L"
#define BIN_COMP_JPEG2KL           "5"
#define COMP_PNG                   "PNG"
#define BIN_COMP_PNG               "6"
#define CSP_GRAY                   "GRAY"
#define CSP_RGB                    "RGB"
#define CSP_YCC                    "YCC"
#define CSP_SRGB                   "SRGB"
#define CSP_SYCC                   "SYCC"

/***********************************************************************/
/* ALLOC.C : ALLOCATION ROUTINES */
extern int biomeval_nbis_alloc_ANSI_NIST(ANSI_NIST **);
extern int biomeval_nbis_new_ANSI_NIST_record(RECORD **, const int);
extern int biomeval_nbis_alloc_ANSI_NIST_record(RECORD **);
extern int biomeval_nbis_new_ANSI_NIST_field(FIELD **, const int, const int);
extern int biomeval_nbis_alloc_ANSI_NIST_field(FIELD **);
extern int biomeval_nbis_alloc_ANSI_NIST_subfield(SUBFIELD **);
extern int biomeval_nbis_alloc_ANSI_NIST_item(ITEM **);
extern void biomeval_nbis_free_ANSI_NIST(ANSI_NIST *);
extern void biomeval_nbis_free_ANSI_NIST_record(RECORD *);
extern void biomeval_nbis_free_ANSI_NIST_field(FIELD *);
extern void biomeval_nbis_free_ANSI_NIST_subfield(SUBFIELD *);
extern void biomeval_nbis_free_ANSI_NIST_item(ITEM *);

/***********************************************************************/
/* APPEND.C : APPEND ROUTINES */
extern int biomeval_nbis_append_ANSI_NIST_record(RECORD *, FIELD *);
extern int biomeval_nbis_append_ANSI_NIST_field(FIELD *, SUBFIELD *);
extern int biomeval_nbis_append_ANSI_NIST_subfield(SUBFIELD *, ITEM *);

/***********************************************************************/
/* COPY.C : COPY ROUTINES */
extern int biomeval_nbis_copy_ANSI_NIST(ANSI_NIST **, ANSI_NIST *);
extern int biomeval_nbis_copy_ANSI_NIST_record(RECORD **, RECORD *);
extern int biomeval_nbis_copy_ANSI_NIST_field(FIELD **, FIELD *);
extern int biomeval_nbis_copy_ANSI_NIST_subfield(SUBFIELD **, SUBFIELD *);
extern int biomeval_nbis_copy_ANSI_NIST_item(ITEM **, ITEM *);

/***********************************************************************/
/* DATE.C : DATE ROUTINES */
extern int biomeval_nbis_get_ANSI_NIST_date(char **);

/***********************************************************************/
/* DECODE.C : IMAGE RECORD DECODER ROUTINES */
extern int biomeval_nbis_decode_ANSI_NIST_image(unsigned char **, int *, int *, int *,
                     double *, const ANSI_NIST *, const int, const int);
extern int biomeval_nbis_decode_binary_field_image(unsigned char **, int *, int *, int *,
                     double *, const ANSI_NIST *, const int);
extern int biomeval_nbis_decode_tagged_field_image(unsigned char **, int *, int *, int *,
                     double *, const ANSI_NIST *, const int, const int);

/***********************************************************************/
/* DELETE.C : DELETE ROUTINES */
extern int biomeval_nbis_do_delete(const char *, const int, const int, const int,
              const int, ANSI_NIST *);
extern int biomeval_nbis_delete_ANSI_NIST_select(const int, const int, const int,
              const int, ANSI_NIST *);
extern int biomeval_nbis_delete_ANSI_NIST_record(const int, ANSI_NIST *);
extern int biomeval_nbis_adjust_delrec_CNT_IDCs(const int, ANSI_NIST *);
extern int biomeval_nbis_delete_ANSI_NIST_field(const int, const int, ANSI_NIST *);
extern int biomeval_nbis_delete_ANSI_NIST_subfield(const int, const int, const int,
              ANSI_NIST *);
extern int biomeval_nbis_delete_ANSI_NIST_item(const int, const int, const int,
              const int, ANSI_NIST *);

/***********************************************************************/
/* FLIP.C : FLIP COORDS & DIRECTION ROUTINES */
extern int biomeval_nbis_flip_y_coord(char *, const int, const int, const double);
extern int biomeval_nbis_flip_direction(char *, const int);

/***********************************************************************/
/* FMTSTD.C : ANSI_NIST FORMAT READ ROUTINES */
extern int biomeval_nbis_read_ANSI_NIST_file(const char *, ANSI_NIST **);
extern int biomeval_nbis_read_ANSI_NIST(FILE *, ANSI_NIST *);
extern int biomeval_nbis_read_Type1_record(FILE *, RECORD **, unsigned int *);
extern int biomeval_nbis_read_ANSI_NIST_remaining_records(FILE *, ANSI_NIST *);
extern int biomeval_nbis_read_ANSI_NIST_record(FILE *, RECORD **, const unsigned int);
extern int biomeval_nbis_read_ANSI_NIST_tagged_record(FILE *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_read_ANSI_NIST_record_length(FILE *, int *, FIELD **);
extern int biomeval_nbis_read_ANSI_NIST_version(FILE *, int *, FIELD **);
extern int biomeval_nbis_read_ANSI_NIST_integer_field(FILE *, int *, FIELD **);
extern int biomeval_nbis_read_ANSI_NIST_remaining_fields(FILE *, RECORD *);
extern int biomeval_nbis_read_ANSI_NIST_field(FILE *, FIELD **, int);
extern int biomeval_nbis_read_ANSI_NIST_image_field(FILE *, FIELD **, char *, const int,
              const int, int); /* Added by MDG 03-08-05 */
extern int biomeval_nbis_read_ANSI_NIST_tagged_field(FILE *, FIELD **, char *, const int,
              const int, int);
extern int biomeval_nbis_read_ANSI_NIST_field_ID(FILE *, char **, unsigned int *,
              unsigned int *);
extern int biomeval_nbis_parse_ANSI_NIST_field_ID(unsigned char **, unsigned char *,
                                    char **, unsigned int *, unsigned int *);
extern int biomeval_nbis_read_ANSI_NIST_subfield(FILE *, SUBFIELD **);
extern int biomeval_nbis_read_ANSI_NIST_item(FILE *, ITEM **);
extern int biomeval_nbis_read_ANSI_NIST_binary_image_record(FILE *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_read_ANSI_NIST_binary_signature_record(FILE *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_read_ANSI_NIST_binary_field(FILE *, FIELD **, const int);

/***********************************************************************/
/* FMTSTD.C : ANSI_NIST FORMAT BUFFER SCAN ROUTINES */
extern int biomeval_nbis_scan_ANSI_NIST(AN2KBDB *, ANSI_NIST *);
extern int biomeval_nbis_scan_Type1_record(AN2KBDB *, RECORD **, unsigned int *);
extern int biomeval_nbis_scan_ANSI_NIST_remaining_records(AN2KBDB *, ANSI_NIST *);
extern int biomeval_nbis_scan_ANSI_NIST_record(AN2KBDB *, RECORD **, const unsigned int);
extern int biomeval_nbis_scan_ANSI_NIST_tagged_record(AN2KBDB *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_scan_ANSI_NIST_record_length(AN2KBDB *, int *, FIELD **);
extern int biomeval_nbis_scan_ANSI_NIST_version(AN2KBDB *, int *, FIELD **);
extern int biomeval_nbis_scan_ANSI_NIST_integer_field(AN2KBDB *, int *, FIELD **);
extern int biomeval_nbis_scan_ANSI_NIST_remaining_fields(AN2KBDB *, RECORD *);
extern int biomeval_nbis_scan_ANSI_NIST_field(AN2KBDB *, FIELD **, int);
extern int biomeval_nbis_scan_ANSI_NIST_image_field(AN2KBDB *, FIELD **, char *, const int,
              const int, int); /* Added by MDG 03-08-05 */
extern int biomeval_nbis_scan_ANSI_NIST_tagged_field(AN2KBDB *, FIELD **, char *, const int,
              const int, int);
extern int biomeval_nbis_scan_ANSI_NIST_field_ID(AN2KBDB *, char **, unsigned int *,
              unsigned int *);
extern int biomeval_nbis_scan_ANSI_NIST_subfield(AN2KBDB *, SUBFIELD **);
extern int biomeval_nbis_scan_ANSI_NIST_item(AN2KBDB *, ITEM **);
extern int biomeval_nbis_scan_ANSI_NIST_binary_image_record(AN2KBDB *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_scan_ANSI_NIST_binary_signature_record(AN2KBDB *, RECORD **,
              const unsigned int);
extern int biomeval_nbis_scan_ANSI_NIST_binary_field(AN2KBDB *, FIELD **, const int);

/***********************************************************************/
/* FMTSTD.C : ANSI_NIST FORMAT WRITE ROUTINES */
extern int biomeval_nbis_write_ANSI_NIST_file(const char *, const ANSI_NIST *);
extern int biomeval_nbis_write_ANSI_NIST(FILE *, const ANSI_NIST *);
extern int biomeval_nbis_write_ANSI_NIST_record(FILE *, RECORD *);
extern int biomeval_nbis_write_ANSI_NIST_tagged_field(FILE *, const FIELD *);
extern int biomeval_nbis_write_ANSI_NIST_tagged_subfield(FILE *, const SUBFIELD *);
extern int biomeval_nbis_write_ANSI_NIST_tagged_item(FILE *, const ITEM *);
extern int biomeval_nbis_write_ANSI_NIST_separator(FILE *, const char);
extern int biomeval_nbis_write_ANSI_NIST_binary_field(FILE *, const FIELD *);
extern int biomeval_nbis_write_ANSI_NIST_binary_subfield(FILE *, const SUBFIELD *);
extern int biomeval_nbis_write_ANSI_NIST_binary_item(FILE *, const ITEM *);

/***********************************************************************/
/* FMTTEXT.C : READ FORMATTED TEXT ROUTINES */
extern int biomeval_nbis_read_fmttext_file(const char *, ANSI_NIST **);
extern int biomeval_nbis_read_fmttext(FILE *, ANSI_NIST *);
extern int biomeval_nbis_read_fmttext_item(FILE *, int *, int *, int *, int *, int *,
              int *, char **);
/* FMTTEXT.C : WRITE FORMATTED TEXT ROUTINES */
extern int biomeval_nbis_write_fmttext_file(const char *, const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext(FILE *, const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext_record(FILE *, const int, const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext_field(FILE *, const int, const int,
              const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext_image_field(FILE *, const int, const int,
                      const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext_subfield(FILE *, const int, const int, const int,
              const ANSI_NIST *);
extern int biomeval_nbis_write_fmttext_item(FILE *, const int, const int, const int,
              const int, const ANSI_NIST *);

/***********************************************************************/
/* GETIMG.C : LOCATE & RETURN IMAGE DATA ROUTINES */
extern int biomeval_nbis_get_first_grayprint(unsigned char **, int *, int *, int *,
                               double *, int *, int *,
                               RECORD **, int *, const ANSI_NIST *);

/***********************************************************************/
/* INSERT.C : INSERT ROUTINES */
extern int biomeval_nbis_do_insert(const char *, const int, const int, const int,
              const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_select(const int, const int, const int,
              const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_record(const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_record_frmem(const int, RECORD *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_record_core(const int, RECORD *, const int, 
	      ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_field(const int, const int, const char *,
              ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_field_frmem(const int, const int, FIELD *,
              ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_field_core(const int, const int, FIELD *,
              ANSI_NIST *);
extern int biomeval_nbis_adjust_insrec_CNT_IDCs(const int, const int, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_subfield(const int, const int, const int,
              const char *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_subfield_frmem(const int, const int, const int,
              SUBFIELD *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_subfield_core(const int, const int, const int,
              SUBFIELD *, ANSI_NIST *);
extern int biomeval_nbis_insert_ANSI_NIST_item(const int, const int, const int, const int,
              const char *, ANSI_NIST *);

/***********************************************************************/
/* IS_AN2K.C : AN2K FORMAT TESTS */
extern int biomeval_nbis_is_ANSI_NIST_file(const char *const);
extern int biomeval_nbis_is_ANSI_NIST(unsigned char *, const int);

/***********************************************************************/
/* LOOKUP.C : LOOKUP ROUTINES */
extern int biomeval_nbis_lookup_ANSI_NIST_field(FIELD **, int *const,
		  const unsigned int, const RECORD *const);
extern int biomeval_nbis_lookup_ANSI_NIST_subfield(SUBFIELD **, const unsigned int,
		  const FIELD *const);
extern int biomeval_nbis_lookup_ANSI_NIST_item(ITEM **, const unsigned int,
                                 const SUBFIELD *const);
extern int biomeval_nbis_lookup_ANSI_NIST_image(RECORD **, int *const, const int,
              const ANSI_NIST *const);
extern int biomeval_nbis_lookup_ANSI_NIST_image_ppmm(double *const, const ANSI_NIST *const,
				       const int);
extern int biomeval_nbis_lookup_binary_field_image_ppmm(double *const, const ANSI_NIST *const,
              const int );
extern int biomeval_nbis_lookup_tagged_field_image_ppmm(double *const, const RECORD *const);
extern int biomeval_nbis_lookup_ANSI_NIST_fingerprint(RECORD **, int *const, const int,
              const ANSI_NIST *const);
extern int biomeval_nbis_lookup_ANSI_NIST_grayprint(RECORD **, int *const,
                                      const int, const ANSI_NIST *);
extern int biomeval_nbis_lookup_binary_field_fingerprint(RECORD **, int *const, const int,
              const ANSI_NIST *const);
extern int biomeval_nbis_lookup_tagged_field_fingerprint(RECORD **, int *const, const int,
              const ANSI_NIST *const);
extern int biomeval_nbis_lookup_fingerprint_with_IDC(RECORD **, int *const, const int,
				       const int, const ANSI_NIST *const);
extern int biomeval_nbis_lookup_FGP_field(FIELD **, int *const, const RECORD *const);
extern int biomeval_nbis_lookup_IMP_field(FIELD **, int *const, const RECORD *const);
extern int biomeval_nbis_lookup_minutiae_format(char *const, const RECORD *const);
extern int biomeval_nbis_lookup_ANSI_NIST_record(RECORD **, int *const, const int,
				   const ANSI_NIST *const, 
				   const REC_SEL *const);

/***********************************************************************/
/* PRINT.C : PRINT ROUTINES */
extern int biomeval_nbis_do_print(const char *, const int, const int, const int,
              const int, ANSI_NIST *);
extern int biomeval_nbis_print_ANSI_NIST_select(FILE *, const int, const int, const int,
              const int, ANSI_NIST *);

/***********************************************************************/
/* READ.C : GENERAL FILE AND BUFFER UTILITIES */
extern int biomeval_nbis_fbgetc(FILE *, AN2KBDB *);
extern size_t biomeval_nbis_fbread(void *, size_t, size_t, FILE *, AN2KBDB *);
extern long biomeval_nbis_fbtell(FILE *, AN2KBDB *);

/***********************************************************************/
/* READ.C : GENERAL READ UTILITIES */
extern int biomeval_nbis_read_binary_item_data(FILE *, unsigned char **, const int);
extern int biomeval_nbis_read_binary_uint(FILE *, unsigned int *);
extern int biomeval_nbis_read_binary_ushort(FILE *, unsigned short *);
extern int biomeval_nbis_read_binary_uchar(FILE *, unsigned char *);
extern int biomeval_nbis_read_binary_image_data(const char *, unsigned char **, int *);
extern int biomeval_nbis_read_char(FILE *, const int);
extern int biomeval_nbis_read_string(FILE *, char **, const int);
extern int biomeval_nbis_read_integer(FILE *, int *, const int);
extern int biomeval_nbis_skip_white_space(FILE *);

/***********************************************************************/
/* READ.C : GENERAL BUFFER SCAN UTILITIES */
extern int biomeval_nbis_scan_binary_item_data(AN2KBDB *, unsigned char **, const int);
extern int biomeval_nbis_scan_binary_uint(AN2KBDB *, unsigned int *);
extern int biomeval_nbis_scan_binary_ushort(AN2KBDB *, unsigned short *);
extern int biomeval_nbis_scan_binary_uchar(AN2KBDB *, unsigned char *);

/***********************************************************************/
/* SIZE.C : FIELD BYTE SIZES */
extern int biomeval_nbis_binary_image_field_bytes(const int);
extern int biomeval_nbis_binary_signature_field_bytes(const int);

/***********************************************************************/
/* SUBSTITUTE.C : SUBSTITUTE ROUTINES */
extern int biomeval_nbis_do_substitute(const char *, const int, const int, const int,
              const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_substitute_ANSI_NIST_select(const int, const int, const int,
              const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_substitute_ANSI_NIST_record(const int, const char *, ANSI_NIST *);
extern int biomeval_nbis_substitute_ANSI_NIST_field(const int, const int, const char *,
              ANSI_NIST *);
extern int biomeval_nbis_substitute_ANSI_NIST_subfield(const int, const int, const int,
              const char *, ANSI_NIST *);
extern int biomeval_nbis_substitute_ANSI_NIST_item(const int, const int, const int,
              const int, const char *, ANSI_NIST *);

/***********************************************************************/
/* TO_IAFIS.C : ANSI/NIST 2007 TO FBI/IAFIS CONVERSION ROUTINES */
extern int biomeval_nbis_nist2iafis_fingerprints(ANSI_NIST *);
extern int biomeval_nbis_nist2iafis_fingerprint(RECORD **, RECORD *);
extern int biomeval_nbis_nist2iafis_type_9s(ANSI_NIST *);
extern int biomeval_nbis_nist2iafis_needed(RECORD *);
extern int biomeval_nbis_nist2iafis_type_9(RECORD **, ANSI_NIST *, const int);
extern int biomeval_nbis_nist2iafis_method(char **, char *);
extern int biomeval_nbis_nist2iafis_minutia_type(char **, char *);
extern int biomeval_nbis_nist2iafis_pattern_class(char **, char *, const int);
extern int biomeval_nbis_nist2iafis_ridgecount(char **, char *);

/***********************************************************************/
/* TO_NIST.C : FBI/IAFIS TO ANSI/NIST 2007 CONVERSION ROUTINES */
extern int biomeval_nbis_iafis2nist_fingerprints(ANSI_NIST *);
extern int biomeval_nbis_iafis2nist_fingerprint(RECORD **, ANSI_NIST *, const int);
extern int biomeval_nbis_iafis2nist_type_9s(ANSI_NIST *);
extern int biomeval_nbis_iafis2nist_needed(RECORD *);
extern int biomeval_nbis_iafis2nist_type_9(RECORD **, ANSI_NIST *, const int);
extern int biomeval_nbis_iafis2nist_method(char **, char *);
extern int biomeval_nbis_iafis2nist_minutia_type(char **, char *);
extern int biomeval_nbis_iafis2nist_pattern_class(char **, char *, const int);
extern int biomeval_nbis_iafis2nist_ridgecount(char **, char *);

/***********************************************************************/
/* TYPE.C : RECORD & FIELD TYPE TESTS */
extern int biomeval_nbis_tagged_record(const unsigned int);
extern int biomeval_nbis_binary_record(const unsigned int);
extern int tagged_image_record(const unsigned int);
extern int biomeval_nbis_binary_image_record(const unsigned int);
extern int biomeval_nbis_image_record(const unsigned int);
extern int biomeval_nbis_binary_signature_record(const unsigned int);
extern int biomeval_nbis_image_field(const FIELD *);
extern int biomeval_nbis_is_delimiter(const int);
extern int biomeval_nbis_which_hand(const int);

/***********************************************************************/
/* SELECT.C : RECORD SELECTION BASED ON VARIOUS EXTENSIBLE CRITERIA */
extern int biomeval_nbis_select_ANSI_NIST_record(RECORD *, const REC_SEL *);
extern int biomeval_nbis_new_rec_sel(REC_SEL **, const REC_SEL_TYPE, const int, ...);
extern int biomeval_nbis_alloc_rec_sel(REC_SEL **, const REC_SEL_TYPE, const int);
extern void biomeval_nbis_free_rec_sel(REC_SEL *);
extern int biomeval_nbis_add_rec_sel_num(REC_SEL **, const REC_SEL_TYPE, const int);
extern int biomeval_nbis_add_rec_sel(REC_SEL **, const REC_SEL *const);
extern int biomeval_nbis_parse_rec_sel_option(const REC_SEL_TYPE, const char *const,
				const char **, REC_SEL **, const int);
extern int biomeval_nbis_write_rec_sel(FILE *, const REC_SEL *const);
extern int biomeval_nbis_write_rec_sel_file(const char *const, const REC_SEL *const);
extern int biomeval_nbis_read_rec_sel(FILE *, REC_SEL **);
extern int biomeval_nbis_read_rec_sel_file(const char *const, REC_SEL **);
extern int biomeval_nbis_imp_is_rolled(const int);
extern int biomeval_nbis_imp_is_flat(const int);
extern int biomeval_nbis_imp_is_live_scan(const int);
extern int biomeval_nbis_imp_is_latent(const int);
extern int biomeval_nbis_simplify_rec_sel(REC_SEL **);

/***********************************************************************/
/* TYPE1314.C : Type-13 and Type-14 ROUTINES */
extern int biomeval_nbis_fingerprint2tagged_field_image(RECORD **, unsigned char *,
                  const int, const int, const int, const int, const double,
                  char *, const int, const int, char *);
extern int biomeval_nbis_image2type_13(RECORD **, unsigned char *, const int, const int,
                  const int, const int, const double, char *, const int,
                  const int, char *);
extern int biomeval_nbis_image2type_14(RECORD **, unsigned char *, const int, const int,
                  const int, const int, const double, char *, const int,
                  const int, char *);

/***********************************************************************/
/* UPDATE.C : UPDATE ROUTINES */
extern int biomeval_nbis_update_ANSI_NIST(ANSI_NIST *, RECORD *);
extern int biomeval_nbis_update_ANSI_NIST_record(RECORD *, FIELD *);
extern int biomeval_nbis_update_ANSI_NIST_field(FIELD *, SUBFIELD *);
extern int biomeval_nbis_update_ANSI_NIST_subfield(SUBFIELD *, ITEM *);
extern int biomeval_nbis_update_ANSI_NIST_item(ITEM *, const int);
extern int biomeval_nbis_update_ANSI_NIST_record_LENs(ANSI_NIST *);
extern int biomeval_nbis_update_ANSI_NIST_record_LEN(ANSI_NIST *, const int);
extern int biomeval_nbis_update_ANSI_NIST_binary_record_LEN(RECORD *);
extern int biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(RECORD *);
extern void biomeval_nbis_update_ANSI_NIST_field_ID(FIELD *, const int, const int);

/***********************************************************************/
/* UTIL.C : UTILITY ROUTINES */
extern int biomeval_nbis_increment_numeric_item(const int, const int, const int,
              const int, ANSI_NIST *, char *);
extern int biomeval_nbis_decrement_numeric_item(const int, const int, const int,
              const int, ANSI_NIST *, char *);

/***********************************************************************/
/* VALUE2.C : STRING TO STRUCTURE ROUTINES */
extern int biomeval_nbis_value2field(FIELD **, const int, const int, const char *);
extern int biomeval_nbis_value2subfield(SUBFIELD **, const char *);
extern int biomeval_nbis_value2item(ITEM **, const char *);

#endif /* !_AN2K_H */
