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

/* Must include an2k.h first. */

/* definitions related to logger.c
**********************************/

/* Definitions of error levels and types used in logging the results. */
/* The default use, and re-use of numberical values near zero is
   overridden in order to make it less likely that any errors, like using
   unitialized variables or assigning the wrong kind of enumerated value,
   will be detected and fixed. */
#define LOGL_BASE 100
typedef enum error_levels_e {
   LOGL_FATAL = LOGL_BASE,
   LOGL_ERROR,
   LOGL_WARNING,
   LOGL_INFO,
   LOGL_DEBUG
} LOGL;
#define LOGL_COUNT (LOGL_DEBUG + 1 - LOGL_BASE)

#define LOGTP_BASE 200
typedef enum error_types_e {
   LOGTP_EXEC = LOGTP_BASE,
   LOGTP_CONFIG,
   LOGTP_CHECK
} LOGTP;
#define LOGTP_COUNT (LOGTP_CHECK + 1 - LOGTP_BASE)

/* Type indices run from 0 through 99.  Type 0 is not defined.  Types 11,
   12, and 18 through 98 are reserved.  Allocating an array of size 100
   takes some extra space but makes indexing much simpler and allows easy
   addition of new types that may be defined in the future. */
#define NUM_RECORD_TYPE_SLOTS 100

/* The DUP_FLD_IDX structure is used to store information about the fields
   that are encountered while processing a record.  Since field numbers can
   be anything from 1 through 999 or more, up to 9 digits, it is not
   practical to use an array as with records.  Each field should be
   encountered only once per record.  Subfields are used when multiple
   instances of the field contents are needed. */
typedef struct duplicate_field_index_s {
   int field_num;
   int index;
} DUP_FLD_IDX;

/* The CAN_CONTEXT structure stores context information like the name of
   the file being processed, and statistics like the types and numbers of
   errors encountered, and the number of lines, records, fields, or items
   processed, skipped, etc. */
typedef struct can_result_accumulator_s {
   const char *name;
   FILE *fp;

   int issue[LOGTP_COUNT][LOGL_COUNT];
   int line_total, line_skip, line_check; /* for config files */
   int rec_total, rec_skip, rec_check;
   int record[NUM_RECORD_TYPE_SLOTS];
   int fld_total, fld_skip, fld_check;
   int itm_total, itm_skip, itm_check;
   /* for single record use only, not aggregated or reported */
   /* count instances of fields -- used to verify required fields are all
      present and to detect duplicates */
   int rec_fld_used;
   int rec_fld_alloc;
   DUP_FLD_IDX *rec_fld_types;
} CAN_CONTEXT;

/* definitions related to config.c
**********************************/

#define DEFAULT_CONFIG_DIR   AN2K_RUNTIME_DATA_DIR "/chkan2k"
#define DEFAULT_CONFIG_FILE  DEFAULT_CONFIG_DIR "/default.conf"
#define MAX_CONFIG_FILES 10

/* The structures defined here are used to represent the standards and
   supplemental specifications from the configuration file. */

/* Item values may be strings or numbers.  Although the ANSI_NIST structure
   represents almost everything as strings, it makes sense to convert the
   numerical allowed item values once, instead of each time they might be
   compared with an actual item value.  All values are represented as a
   sequence of bytes.  In addition, numerical values can be converted into
   numbers.  Absence of a valid floating point value is indicated by NaN,
   absense of a valid integer value by UNDEF (-1), and if the string value
   can be absent, it would be indicated by NULL.  (To define a flag and
   union to identify and store either an integer or floating point value
   would take as much space but would be more complicated.) */
typedef struct chkan2k_item_s   ITEM_SPEC;
typedef struct chkan2k_field_s  FIELD_SPEC;
typedef struct chkan2k_record_s RECORD_SPEC;
typedef struct chkan2k_config_s CAN_CONFIG;

/* Items can have different kinds of values, as specified here. */
typedef enum chkan2k_item_type_e {
   ITM_NUM = 300,		/* non-negative integer */
   ITM_SNUM,			/* signed integer */
   ITM_CNUM,			/* multiple numbers combined into one item */
   ITM_HEX,			/* hexadecimal integer */
   ITM_FP,			/* floating point number */
   ITM_STR,
   ITM_BIN,			/* arbitrary binary data */
   ITM_DATE,			/* a date of the form YYYYMMDD, as
				   supported by the standard for all dates
				   except Type-1.014 (GMT). */
   ITM_GMT,			/* a GMT date, as per Type-1.014 */
   ITM_IMAGE			/* image data */
} ITEM_SPEC_TYPE;

/* This structure keeps the item-value type and data together. */
typedef struct chkan2k_item_value_s {
   char *str;
   ITEM_SPEC_TYPE type;
   union {
      int num;
      double fp;
   } u;
} ITEM_SPEC_VAL;

/* This structure supports optionally named lists of item values. */
typedef struct chkan2k_list_s {
   char *tag;
   int num_vals, alloc_vals;
   ITEM_SPEC_VAL **vals;
} CAN_LIST;

/* Numerical limits are used in serveral places, so it makes sense to
   extract them out into a seperate structure. */
typedef struct chkan2k_limits_s {
   int min, max;
} CAN_LIMITS;

/* New record types and fields have been added since the original version
   of the ANSI/NIST standard, but nothing has been removed, so it suffices
   to specifiy version of the standard where a particular thing is defined. */
typedef struct chkan2k_standard_s {
   char *tag;
   char *name;
   ITEM_SPEC_VAL ver;
   char *ref;
   char *date;
   const struct chkan2k_standard_s *parent;
} CAN_STANDARD;

typedef void
(CHECK_ITEM_FUNC)(const CAN_CONFIG *const, const FIELD *,
		  const int, const int, const int, const int,
		  const ITEM_SPEC *const, const FIELD_SPEC *const,
		  const RECORD_SPEC *, const ANSI_NIST *const,
		  CAN_CONTEXT *const);

typedef struct chkan2k_item_type_desc_s {
   ITEM_SPEC_TYPE type;
   char *name;
   CHECK_ITEM_FUNC *func;
} ITEM_SPEC_TYPE_DESC;

/* ITEM_SPEC */
struct chkan2k_item_s {
   char *tag;
   CAN_STANDARD *std;
   CAN_LIMITS occ, size;
   ITEM_SPEC_VAL *min;
   ITEM_SPEC_VAL *max;
   ITEM_SPEC_TYPE type;
   CAN_LIST *enum_vals;
};

typedef void 
(CHECK_FIELD_FUNC)(const CAN_CONFIG *const, const FIELD *const,
		   const FIELD_SPEC *const, const int, const int,
		   const ANSI_NIST *const, CAN_CONTEXT *const);

/* FIELD_SPEC */
struct chkan2k_field_s {
   char *tag;
   int idnum;
   CAN_STANDARD *std;
   int num_records, alloc_records;
   RECORD_SPEC **records;
   CAN_LIMITS occ, size;
   int num_items, alloc_items;
   ITEM_SPEC **items;
   CHECK_FIELD_FUNC *check;
};

typedef enum chkan2k_record_data_type_e {
   RDT_ASCII = 'A',
   RDT_BINARY,
   RDT_ASCBIN
} CAN_REC_DATA_TYPE;
   
/* RECORD_SPEC */
struct chkan2k_record_s {
   char *name;
   int idnum;
   CAN_STANDARD *std;
   CAN_REC_DATA_TYPE data_type;
   int alloc_fields, num_fields;
   FIELD_SPEC **fields;
};

typedef struct chkan2k_option_s {
   char *name;
   char *value;
} CAN_OPTION;

/* CAN_CONFIG */
struct chkan2k_config_s {
   const char *name;
   int num_standards, num_records, num_fields, num_items,
      num_lists, num_options;
   int alloc_standards, alloc_records, alloc_fields, alloc_items,
      alloc_lists, alloc_options;
   CAN_STANDARD **standards;
   RECORD_SPEC  **records;
   FIELD_SPEC   **fields;
   ITEM_SPEC    **items;
   CAN_LIST     **lists;
   CAN_OPTION   **options;
   struct chkan2k_config_s *parent;
};

/* The configuration file is read line by line, and the lines are parsed
   into tokens, using the CAN_TOKEN structure to point to the beginning and
   end of the token within the line buffer. */
typedef enum token_type_e { 
   TOK_SIMPLE='s',
   TOK_STRING,
   TOK_LIST,
   TOK_ITEM,
   TOK_EMPTY,
   TOK_COMMENT
} CAN_TOKEN_TYPE;

typedef struct chkan2k_token_s {
   const char *val, *end;
   CAN_TOKEN_TYPE type;
} CAN_TOKEN;

#define TOK_FMT "\"%.*s\""
#define TOK_ARGS(x) ((x)->end-(x)->val), ((x)->val)

/* functions defined in config.c
********************************/
const CAN_CONFIG *read_config
   (CAN_CONTEXT *const, const CAN_CONFIG *const);
CAN_STANDARD *lookup_standard_in_cfg_by_anver_deep
   (const CAN_CONFIG *const, const int);
RECORD_SPEC *lookup_record_in_cfg_by_type_num_deep
   (const CAN_CONFIG *const, const int);
FIELD_SPEC *lookup_field_in_cfg_by_tf_ids_deep
   (const CAN_CONFIG *const, const int, const int);
CAN_OPTION *lookup_option_in_cfg_by_name_deep
   (const CAN_CONFIG *const, const char *);
ITEM_SPEC_VAL *lookup_val_in_lst_by_str_flat
   (const CAN_LIST *const, const char *const);
ITEM_SPEC_VAL *lookup_val_in_lst_by_num_flat
   (const CAN_LIST *const, const int);

/* functions defined in chkfile.c
*********************************/
/* arbitrarily limit the maximum number of items allowed in a field */
#define MAX_ITEMS 10000

/* These functions are used to perform additional checks on the
   validity of a field. */
/* CHECK_FIELD_FUNC chk_ver_fld, chk_idc_fld, chk_date_fld; */
void check_ansi_nist(const CAN_CONFIG *const, const ANSI_NIST *const,
		     CAN_CONTEXT *const);
const ITEM_SPEC_TYPE_DESC *lookup_item_type_desc_by_num(const int);
int parse_item_numeric(const CAN_CONFIG *const, const FIELD *const,
		       const FIELD_SPEC *const, const int, const int, 
		       const int, const int, int *const, CAN_CONTEXT *const);

/* functions defined in logger.c
********************************/
int set_log_level(const char *const arg);
void can_log(const LOGL, const LOGTP, const CAN_CONFIG *const, 
	     CAN_CONTEXT *const, const char *const, ...);
void log_chk(const LOGL, const CAN_CONFIG *const, const FIELD *const, 
	     const FIELD_SPEC *const, const int, const int, const int, const int,
	     CAN_CONTEXT *const, const char *const, ...);
void reset_result_accumulator(CAN_CONTEXT *const, const char *const);
void init_result_accumulator(CAN_CONTEXT *const, const char *const);
void aggregate_result_accumulator(CAN_CONTEXT *const, const CAN_CONTEXT*const);
void report_result_accumulator(const CAN_CONFIG *const, 
			       CAN_CONTEXT *const, const int);
void reset_record_field_accumulator(CAN_CONTEXT *const);
int check_for_duplicate_fields(CAN_CONTEXT *const, const FIELD *const,
			       const int);

/* functions defined in combinations.c
**************************************/
void check_record_combinations(const CAN_CONFIG *const, 
			       const ANSI_NIST *const, CAN_CONTEXT *const);
