/*******************************************************************************

License: 
This software was developed at the National Institute of Standards and 
Technology (NIST) by employees of the Federal Government in the course 
of their official duties. Pursuant to title 17 Section 105 of the 
United States Code, this software is not subject to copyright protection 
and is in the public domain. NIST assumes no responsibility  whatsoever for 
its use by other parties, and makes no guarantees, expressed or implied, 
about its quality, reliability, or any other characteristic. 

This software has been determined to be outside the scope of the EAR
(see Part 734.3 of the EAR for exact details) as it has been created solely
by employees of the U.S. Government; it is freely distributed with no
licensing requirements; and it is considered public domain.  Therefore,
it is permissible to distribute this software as a free download from the
internet.

Disclaimer: 
This software was developed to promote biometric standards and biometric
technology testing for the Federal Government in accordance with the USA
PATRIOT Act and the Enhanced Border Security and Visa Entry Reform Act.
Specific hardware and software products identified in this software were used
in order to perform the software development.  In no case does such
identification imply recommendation or endorsement by the National Institute
of Standards and Technology, nor does it imply that the products and equipment
identified are necessarily the best available for the purpose.  

*******************************************************************************/


/***********************************************************************
      LIBRARY: AN2K - ANSI/NIST 2000 Reference Implementation

      FILE:    SELECT.C
      AUTHOR:  Joseph C. Konczal
      DATE:    01/30/2008

      Contains routines used to select records from an ANSI NIST file
      based on multiple criteria consisting of combinations of finger
      position, impression type, and other features.
***********************************************************************
               ROUTINES:
	                * biomeval_nbis_get_type_params_by_type()
                        * biomeval_nbis_get_type_params_by_name()
                        biomeval_nbis_select_ANSI_NIST_record()
                        biomeval_nbis_new_rec_sel()
                        biomeval_nbis_alloc_rec_sel()
                        biomeval_nbis_free_rec_sel()
                        biomeval_nbis_add_rec_sel_num()
                        biomeval_nbis_add_rec_sel_str()
                        biomeval_nbis_add_rec_sel()
                        biomeval_nbis_simplify_rec_sel()
                        * biomeval_nbis_validate_rec_sel_num_value()
                        * biomeval_nbis_rec_sel_usage()
                        biomeval_nbis_parse_rec_sel_option()
                        biomeval_nbis_write_rec_sel()
                        biomeval_nbis_write_rec_sel_file()
                        biomeval_nbis_read_rec_sel()
                        biomeval_nbis_read_rec_sel_file()
                        biomeval_nbis_imp_is_live_scan()
                        biomeval_nbis_imp_is_latent()
                        biomeval_nbis_imp_is_rolled()
                        biomeval_nbis_imp_is_flat()

      * The marked functions are delcared static and called only from
        within select.c.

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include <an2k.h>

#include <nbis_sysdeps.h>

/***********************************************************************
************************************************************************
Data Structures

   The following data structures and tables are used by
   biomeval_nbis_parse_rec_sel_option to decide how to parse record selection
   options and what limits to enforce.  To add the ability to parse
   options for another type, it might only be necessary to add it to
   these tables.  However, to actually use the values so entered, the
   biomeval_nbis_select_ANSI_NIST_record function would also need to be modified to
   handle the new type of value properly instead of warning that it is
   not implemented.

************************************************************************/

/* Th AR_SZ macro calculates the number of elements in an array. */
#define AR_SZ(x) (sizeof(x)/sizeof(*x))

/* The structure below holds a list of synonymous names the set of
   values to which they map.  */
typedef struct rec_sel_named_sets_s {
   const char *names[6]; /* name strings, e.g. "thumb", null terminated */
   int num_values;
   REC_SEL_VALUE values[13]; /* associated values, e.g. 1 and 6 */
} REC_SEL_NAMED_SETS;

/* Logical Record Types, based on ANSI/NIST-ITL 1-2007, Table 4, p. 13. */
static const REC_SEL_NAMED_SETS biomeval_nbis_lrt_names[] = {
   {{ "grey print", "gp", NULL },    3, {{.num = 4}, {.num = 13}, {.num = 14}}},
   {{ "Transaction information", "ti", NULL },            1, {{.num = 1}}},
   {{ "User-defined descriptive text", "udt", NULL },     1, {{.num = 2}}},
   {{ "Low-resolution grayscale fingerprint image", "lrgsfi", NULL }, 1, {{.num = 3}}},
   {{ "High-resolution grayscale fingerprint image", "hrgsfi",NULL }, 1, {{.num = 4}}},
   {{ "Low-resolution binary fingerprint image", "lrbfi", NULL },     1, {{.num = 5}}},
   {{ "High-resolution binary fingerprint image", "hrbfi", NULL },    1, {{.num = 6}}},
   {{ "User-defined image", "udi", NULL },                1, {{.num = 7}}},
   {{ "Signature image", "si", NULL },                    1, {{.num = 8}}},
   {{ "Minutiae data", "md", NULL },                      1, {{.num = 9}}},
   {{ "Facial & SMT image", "fsmti", NULL },              1, {{.num = 10}}},
   {{ "Variable-resolution latent image", "vrli", NULL }, 1, {{.num = 13}}},
   {{ "Variable-resolution fingerprint image", "vrfi", NULL }, 1, {{.num = 14}}},
   {{ "Variable-resolution palmprint image", "vrpi", NULL },   1, {{.num = 15}}},
   {{ "User-defined variable-resolution testing image","udvrti",NULL},1,{{.num = 16}}},
   {{ "Iris image", "ii", NULL },                         1, {{.num = 17}}},
   {{ "CBEFF Biometric data record", "cbdr", NULL },      1, {{.num = 99}}},
};
static const REC_SEL_VALUE biomeval_nbis_lrt_values[] = {
   {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}, {10}, /* 11 & 12 reserved */
   {13}, {14}, {15}, {16}, {17}, /* 18 through 98 reserved */
   {99}
};

/* The FGP and PLP selector types are complicated by the fact that
   some record types can have either one in one of the FGP fields, and
   some can have only one or the other.  They are defined below in a
   large, combined structure, which is split into two pieces for the
   separate items. */

/* Finger Positions, based on ANSI/NIST-ITL 1-2007, Table 12, p. 31. */
/* and Palm Position Codes, based on ANSI/NIST-ITL 1-2007, Table 35, p. 84. */

#define FGP_NAMES_COUNT 24
#define FGP_VALUES_COUNT 17

static const REC_SEL_NAMED_SETS biomeval_nbis_fgplp_names[] = {
   {{ "thumb", "t", NULL },                         4, {{.num = 1},{.num = 6},{.num = 11},{.num = 12}}}, /*0*/
   {{ "index finger", "i", "if", NULL },            2, {{.num = 2}, {.num = 7}}},       /*1*/
   {{ "middle finger", "m", "mf", NULL },           2, {{.num = 3}, {.num = 8}}},       /*2*/
   {{ "ring finger", "r", "rf", NULL },             2, {{.num = 4}, {.num = 9}}},       /*3*/
   {{ "little finger", "l", "lf", NULL },           2, {{.num = 5}, {.num = 10}}},      /*4*/
   {{ "right hand finger", "rhf", NULL }, 7, 
                                 {{.num = 1}, {.num = 2}, {.num = 3}, {.num = 4}, {.num = 5}, {.num = 11}, {.num = 13}}}, /*5*/
   {{ "left hand finger", "lhf", NULL },  7, 
                                 {{.num = 6}, {.num = 7}, {.num = 8}, {.num = 9}, {.num = 10}, {.num = 12}, {.num = 14}}}, /*6*/
   {{ "unknown finger", "uf", NULL },	            1, {{.num = 0}}},            /*7*/
   {{ "right thumb", "rt", NULL },                  2, {{.num = 1}, {.num = 11}}},      /*8*/
   {{ "right index finger", "rif", "ri", NULL },    1, {{.num = 2}}},            /*9*/
   {{ "right middle finger", "rmf", NULL },         1, {{.num = 3}}},           /*10*/
   {{ "right ring finger", "rrf", NULL },           1, {{.num = 4}}},           /*11*/
   {{ "right little finger", "rlf", "rl", NULL },   1, {{.num = 5}}},           /*12*/
   {{ "left thumb", "lt", NULL },                   2, {{.num = 6}, {.num = 12}}}, /*13*/
   {{ "left index finger", "lif", NULL },           1, {{.num = 7}}},           /*14*/
   {{ "left middle finger", "lmf", NULL },          1, {{.num = 8}}},           /*15*/
   {{ "left ring finger", "lrf", NULL },            1, {{.num = 9}}},           /*16*/
   {{ "left little finger", "llf", NULL },          1, {{.num = 10}}},          /*17*/
   {{ "plain right thumb", "prt", NULL },           1, {{.num = 11}}},          /*18*/
   {{ "plain left thumb", "plt", NULL },            1, {{.num = 12}}},          /*19*/
   {{ "plain right four fingers", "prff", "r4", NULL },      1, {{.num = 13}}}, /*20*/
   {{ "plain left four fingers", "plff", "l4", NULL },       1, {{.num = 14}}}, /*21*/
   {{ "left & right thumbs", "both thumbs", "rlt", "bt", "2t",NULL}, 1, {{.num = 15}}},
   {{ "eji", "tip", NULL },                                  1, {{.num = 19}}}, /*23*/
   /* End of Finger Position Codes, Beginning of Palmprint Codes. */
   {{ "interdigital", NULL },                   2, {{.num = 31}, {.num = 34}}},        /*24*/
   {{ "thenar", NULL },                         2, {{.num = 32}, {.num = 35}}},
   {{ "hypothenar", NULL },                     2, {{.num = 33}, {.num = 36}}},
   {{ "right palm", "rp", NULL },   8,
                             {{.num = 21}, {.num = 22}, {.num = 25}, {.num = 26}, {.num = 29}, {.num = 31}, {.num = 32}, {.num = 33}}},
   {{ "left palm", "lp", NULL },    8, 
                             {{.num = 23}, {.num = 24}, {.num = 27}, {.num = 28}, {.num = 30}, {.num = 34}, {.num = 35}, {.num = 36}}},
   {{ "unknown palm", "up", NULL },             1, {{.num = 20}}},
   {{ "right full palm", "rfp", NULL },         1, {{.num = 21}}},
   {{ "right writer's palm", "rwp", NULL },     1, {{.num = 22}}},
   {{ "left full palm", "lfp", NULL },          1, {{.num = 23}}},
   {{ "left writer's palm", "lwp", NULL },      1, {{.num = 24}}},
   {{ "full palm", "fp", NULL },                2, {{.num = 21}, {.num = 23}}},
   {{ "writer's palm", "wp", NULL },            2, {{.num = 22}, {.num = 24}}},
   {{ "right lower palm", "rlp", NULL },        1, {{.num = 25}}},
   {{ "right upper palm", "rup", NULL },        1, {{.num = 26}}},
   {{ "left lower palm", "llp", NULL },         1, {{.num = 27}}},
   {{ "left upper palm", "lup", NULL },         1, {{.num = 28}}},
   {{ "lower palm", "lp", NULL },               2, {{.num = 25}, {.num = 27}}},
   {{ "upper palm", "up", NULL },               2, {{.num = 26}, {.num = 28}}},
   {{ "right other", "ro", NULL },              1, {{.num = 29}}},
   {{ "left other", "lo", NULL },               1, {{.num = 30}}},
   {{ "other palm", "op", NULL },               2, {{.num = 29}, {.num = 30}}},
   {{ "right interdigital", "rin", NULL },      1, {{.num = 31}}},
   {{ "right thenar", "rthe", NULL },           1, {{.num = 32}}},
   {{ "right hypothenar", "rhy", NULL },        1, {{.num = 33}}},
   {{ "left interdigital", "lin", NULL },       1, {{.num = 34}}},
   {{ "left thenar", "lthe", NULL },            1, {{.num = 35}}},
   {{ "left hypothenar", "lhy", NULL },         1, {{.num = 36}}},
};
static const REC_SEL_VALUE biomeval_nbis_fgplp_values[] = {
     {0},  {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8},  {9}, 
    {10}, {11}, {12}, {13}, {14}, {15}, /*16-18 omitted*/ {19},
    /* Preceeding are FGP, following are PLP.  The PLP are contiguous */
    {20}, {21}, {22}, {23}, {24}, {25}, {26}, {27}, {28}, {29},
    {30}, {31}, {32}, {33}, {34}, {35}, {36}
};

/* Impression Type, based on ANSI/NIST-ITL {1}-{2007}, Table {11}, p. {30}. */
static const REC_SEL_NAMED_SETS biomeval_nbis_imp_names[] = {
   {{ "rolled", "r", NULL },   6, {{.num = 1}, {.num = 3}, {.num = 21}, {.num = 23}, {.num = 25}, {.num = 27}}},
   {{ "plain", "p", NULL },    6, {{.num = 0}, {.num = 2}, {.num = 20}, {.num = 22}, {.num = 24}, {.num = 26}}},
   {{ "latent", "lat", NULL }, 8, {{.num = 4}, {.num = 5}, {.num = 6}, {.num = 7}, {.num = 12}, {.num = 13}, {.num = 14}, {.num = 15}}},
   /* don't move live-scan without fixing the pointer definition below */
   {{ "live-scan", "live", NULL },   12, {{.num = 0},  {.num = 1},  {.num = 8},  {.num = 10}, {.num = 20}, {.num = 21},
				          {.num = 22}, {.num = 23}, {.num = 24}, {.num = 25}, {.num = 26}, {.num = 27}}},
   {{ "nonlive-scan", "nonlive", NULL },            3, {{.num = 2}, {.num = 3}, {.num = 11}}},
   {{ "palm", NULL },                               4, {{.num = 12},{.num = 13},{.num = 14},{.num = 15}}},
   {{ "live-scan plain", "lsp", "lspl", NULL },     1, {{.num = 0}}},
   {{ "live-scan rolled", "lsr", "lsro", NULL },    1, {{.num = 1}}},
   {{ "nonlive-scan plain", "nsp", "nspl", NULL },  1, {{.num = 2}}},
   {{ "nonlive-scan rolled", "nsr", "nsro", NULL }, 1, {{.num = 3}}},
   {{ "latent impression", "li", NULL },            1, {{.num = 4}}},
   {{ "latent tracing", "lt", NULL },               1, {{.num = 5}}},
   {{ "latent photo", "lp", NULL },                 1, {{.num = 6}}},
   {{ "latent lift", "ll", NULL },                  1, {{.num = 7}}},
   {{ "live-scan vertical swipe", "lsvs", NULL },   1, {{.num = 8}}},
   /* 9 omitted */
   {{ "live-scan palm", "lspa", NULL },             1, {{.num = 10}}},
   {{ "nonlive-scan palm", "nspa", NULL },          1, {{.num = 11}}},
   {{ "latent palm impression", "lpi", NULL },      1, {{.num = 12}}},
   {{ "latent palm tracing", "lpt", NULL },         1, {{.num = 13}}},
   {{ "latent palm photo", "lpp", NULL },           1, {{.num = 14}}},
   {{ "latent palm lift", "lpl", NULL },            1, {{.num = 15}}},
   /* 16 through 19 omitted */
   {{ "live-scan optical contact plain", "lsocp", NULL },           1, {{.num = 20}}},
   {{ "live-scan optical contact rolled", "lsocr", NULL },          1, {{.num = 21}}},
   {{ "live-scan non-optical contact plain", "lsnocp", NULL },      1, {{.num = 22}}},
   {{ "live-scan non-optical contact rolled", "lsnocr", NULL },     1, {{.num = 23}}},
   {{ "live-scan optical contactless plain", "lsoclp", NULL },      1, {{.num = 24}}},
   {{ "live-scan optical contactless rolled", "lsoclr", NULL },     1, {{.num = 25}}},
   {{ "live-scan non-optical contactless plain", "lsnoclp", NULL }, 1, {{.num = 26}}},
   {{ "live-scan non-optical contactless rolled", "lsnoclr", NULL },1, {{.num = 27}}},
   {{ "other", NULL },                             1, {{.num = 28}}},
   {{ "unknown", NULL },                           1, {{.num = 29}}},
};
static const REC_SEL_VALUE biomeval_nbis_imp_values[] = {
    {0},  {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8}, /* 9 omitted */
   {10}, {11}, {12}, {13}, {14}, {15}, /* 16 through 19 omitted */
   {20}, {21}, {22}, {23}, {24}, {25}, {26}, {27}, {28}, {29} 
};
static const REC_SEL_NAMED_SETS *biomeval_nbis_imp_rolled_set    = &biomeval_nbis_imp_names[0];
static const REC_SEL_NAMED_SETS *biomeval_nbis_imp_flat_set      = &biomeval_nbis_imp_names[1];
static const REC_SEL_NAMED_SETS *biomeval_nbis_imp_latent_set    = &biomeval_nbis_imp_names[2];
static const REC_SEL_NAMED_SETS *biomeval_nbis_imp_live_scan_set = &biomeval_nbis_imp_names[3];


/* NIST IR {7175}, August {2004}, Fingerprint Image Quality, Table {2}, p. {12}. */
static const REC_SEL_NAMED_SETS biomeval_nbis_nqm_names[] = {
   {{ "excellent", NULL },       1, {{.num = 1}}},
   {{ "very good", "vg", NULL }, 1, {{.num = 2}}},
   {{ "good", NULL },            1, {{.num = 3}}},
   {{ "fair", NULL },            1, {{.num = 4}}},
   {{ "poor", NULL },            1, {{.num = 5}}},
};

/* Image Type, based on ANSI/NIST-ITL 1-2007, Section 15.1.4, p. 45. */
static const REC_SEL_NAMED_SETS biomeval_nbis_imt_names[] = {
   {{ "face",   NULL }, 1, {{.str = "FACE"}}},
   {{ "scar",   NULL }, 1, {{.str = "SCAR"}}},
   {{ "mark",   NULL }, 1, {{.str = "MARK"}}},
   {{ "tattoo", NULL }, 1, {{.str = "TATTOO"}}},
};

/* Subject Pose, based on ANSI/NIST-ITL 1-2007, Table 11, p. 30.19, p. 51. */
static const REC_SEL_VALUE biomeval_nbis_pos_values[] = {
   {.str = "F"}, {.str = "R"}, {.str = "L"}, {.str = "A"}, {.str = "D"}
};

static const REC_SEL_NAMED_SETS biomeval_nbis_pos_names[] = {
   {{ "full face frontal", "fff", NULL },   1, {{.str = "F"}}},
   {{ "right profile", "rp", NULL },        1, {{.str = "R"}}},
   {{ "left profile", "lp", NULL },         1, {{.str = "L"}}},
   {{ "angled pose", "ap", NULL },          1, {{.str = "A"}}},
   {{ "determined 3d pose", "d3dp", NULL }, 1, {{.str = "D"}}},
};

/* The structure below ties together the structures and arrays defined
   above with more information about each type of record selector.
   The record selector types themselves are enumerated in an2k.h. */
typedef struct biomeval_nbis_rec_sel_type_params_s {
   const REC_SEL_TYPE type;
   const char *name;
   const char *description;
   const char *reference;
   const int min;
   const int max;
   const REC_SEL_VALUE_TYPE value_type;
   const int value_count;
   const REC_SEL_VALUE *enum_values;
   const int named_sets_count;
   const REC_SEL_NAMED_SETS *named_sets;
} REC_SEL_TYPE_PARAMS;

/* These types are enumerated in an2k.h. */
static const REC_SEL_TYPE_PARAMS biomeval_nbis_rec_sel_type_params[] = {
   { rs_and, "AND", "Boolean AND",
     NULL, /* no reference in std */
     0, 0, rsv_rs,
     0, NULL, 0, NULL },
   { rs_or,  "OR",  "Boolean OR",
     NULL,
     0, 0, rsv_rs,
     0, NULL, 0, NULL },
   { rs_lrt, "LRT", "Logical Record Type",
     "ANSI/NIST-ITL 1-2007, Table 4, p. 13.",
     1, 99, rsv_num, 
     AR_SZ(biomeval_nbis_lrt_values), biomeval_nbis_lrt_values, AR_SZ(biomeval_nbis_lrt_names), biomeval_nbis_lrt_names },
   { rs_fgplp, "FGPLP", "Finger or Palm Position",
     "ANSI/NIST-ITL 1-2007, Tables 12 and 35, pp. 31 and 84.",
     0, 36, rsv_num,
     AR_SZ(biomeval_nbis_fgplp_values), biomeval_nbis_fgplp_values, AR_SZ(biomeval_nbis_fgplp_names), biomeval_nbis_fgplp_names },
   { rs_fgp, "FGP", "Finger Position",
     "ANSI/NIST-ITL 1-2007, Table 12, p. 31.",
     0, 19, rsv_num,
     FGP_VALUES_COUNT, biomeval_nbis_fgplp_values, FGP_NAMES_COUNT, biomeval_nbis_fgplp_names },
   { rs_plp, "PLP", "Palmprint Position",
     "ANSI/NIST-ITL 1-2007, Table 35, p. 84.",
     20, 36, rsv_num,
     0, NULL, AR_SZ(biomeval_nbis_fgplp_names)-FGP_NAMES_COUNT, biomeval_nbis_fgplp_names+FGP_NAMES_COUNT },
   { rs_imp, "IMP", "Impression Type",
     "ANSI/NIST-ITL 1-2007, Table 11, p. 30.",
     0, 29, rsv_num,
     AR_SZ(biomeval_nbis_imp_values), biomeval_nbis_imp_values, AR_SZ(biomeval_nbis_imp_names), biomeval_nbis_imp_names },
   { rs_idc, "IDC", "Image Designation Character",
     "ANSI/NIST-ITL 1-2007.",
     0, 0, rsv_num,
     0, NULL, 0, NULL },
   { rs_nqm, "NQM", "NIST Quality Metric",
     "NIST IR 7151, August 2004, Fingerprint Image Quality",
     1, 5, rsv_num,
     0, NULL, AR_SZ(biomeval_nbis_nqm_names), biomeval_nbis_nqm_names },
   { rs_imt, "IMT", "Image Type",
     "ANSI/NIST-ITL 1-2007, Section 15.1.3, p. 45.",
     0, 0, rsv_str,
     0, NULL, AR_SZ(biomeval_nbis_imt_names), biomeval_nbis_imt_names },
   { rs_pos, "POS", "Subject Pose",
     "ANSI/NIST-ITL 1-2007, Table 19, p. 51.",
     0, 0, rsv_str,
     AR_SZ(biomeval_nbis_pos_values), biomeval_nbis_pos_values, AR_SZ(biomeval_nbis_pos_names), biomeval_nbis_pos_names },
};

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_get_type_params_by_name - Takes a string representation
#cat:               of a possible record selection parameter type name
#cat:               and returns the corresponding record selection
#cat:               type, if it exists.

   Input
      type        - the enumerated type value to look up
   Output
      params      - points to the type properties structure
   Return
      Zero        - success
      Nonzero     - not found or system error

************************************************************************/
static int
biomeval_nbis_get_type_params_by_type(const REC_SEL_TYPE_PARAMS **params,
			const REC_SEL_TYPE type)
#define ERRHDR "ERROR : biomeval_nbis_get_type_params_by_type : "
{
   const REC_SEL_TYPE_PARAMS *type_params;

   for (type_params = biomeval_nbis_rec_sel_type_params;
	type_params - biomeval_nbis_rec_sel_type_params < AR_SZ(biomeval_nbis_rec_sel_type_params);
	type_params++) {
      if (type == type_params->type)
	 break;
   }
   if (type_params - biomeval_nbis_rec_sel_type_params == AR_SZ(biomeval_nbis_rec_sel_type_params)) {
      fprintf(stderr, ERRHDR "parameters not specified for type %d\n", type);
      return -1;
   }

   *params = type_params;
   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_get_type_params_by_name - Takes a string representation
#cat:               of a possible record selection parameter type name
#cat:               and returns the corresponding record selection
#cat:               type, if it exists.

   Input
      type_name   - a character string containing the type name to look up
   Output
      params      - points to the type properties structure
   Return
      Zero        - success
      Nonzero     - not found or system error

************************************************************************/
static int
biomeval_nbis_get_type_params_by_name(const REC_SEL_TYPE_PARAMS **params,
			const char *const type_name)
#define ERRHDR "ERROR : biomeval_nbis_get_type_params_by_name : "
{
   const REC_SEL_TYPE_PARAMS *type_params;

   for (type_params = biomeval_nbis_rec_sel_type_params;
	type_params - biomeval_nbis_rec_sel_type_params < AR_SZ(biomeval_nbis_rec_sel_type_params);
	type_params++) {
      if (!strcmp(type_params->name, type_name)) /* match */
	 break;
   }
   if (type_params - biomeval_nbis_rec_sel_type_params == AR_SZ(biomeval_nbis_rec_sel_type_params)) {
      fprintf(stderr, ERRHDR "parameters not specified for type '%s'\n",
	      type_name);
      return -1;
   }
   
   *params = type_params;
   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_select_ANSI_NIST_record - Takes an ANSI/NIST record and a set of
#cat:               selection criteria and determines whether the
#cat:               record matches the criteria.

   Input:
      record     - record structure to be checked
      criteria   - criteria structure indicating what to check

   Return Code:
      TRUE       - record meets the criteria
      FALSE      - record does not meet the criteria
************************************************************************/
int biomeval_nbis_select_ANSI_NIST_record(RECORD *record,
			    const REC_SEL *const criteria)
#define ERRHDR "ERROR : biomeval_nbis_select_ANSI_NIST_record : "
{
   FIELD *field;
   int parm_i, subfield_i, field_i;
   int item_value;
   int fgp_count = 0, fgp_value[17];
   const REC_SEL_TYPE_PARAMS *type_params;
   
   /* To simplify composition of criteria, missing or empty criteria
      structures match by default. */
   if (!criteria || criteria->num_values == 0)
      return TRUE;		/* default unfiltered */

   /* Check the criteria. */

   for (parm_i = 0; parm_i < criteria->num_values; parm_i++) {

    /* In order to simplify recursion and produce a correct and robust
      implementation, each node contains either a collection of other
      nodes whose values are combined by a boolean operation, or a
      single value of a specific type. */

     switch (criteria->type) {

	 /* Both 'rs_and' and 'or' either short circuit or continue, others
	    return TRUE or FALSE immediatly. */
      case rs_and:
	 if (!biomeval_nbis_select_ANSI_NIST_record(record, criteria->value.rs[parm_i]))
	    return FALSE;	/* 'rs_and' short circuit */
	 break;

      case rs_or:
	 if (biomeval_nbis_select_ANSI_NIST_record(record, criteria->value.rs[parm_i]))
	    return TRUE;	/* 'rs_or' short circuit */
	 break;

      case rs_fgp:			/* finger position */
      case rs_plp:			/* palmprint position */
      case rs_fgplp:		/* finger or palmprint position */
	 /* First, find the values and convert them to integers... */
	 if (!biomeval_nbis_lookup_FGP_field(&field, &field_i, record))
	    return FALSE;
	 for (subfield_i=0;
	      subfield_i < field->num_subfields;
	      subfield_i++) {
	    /* In the files on disk, the FGP field in types 3-6 are
	       stored as binary integers, with 255 indicating unused
	       fields, and in types 9 and 13-15 as 1-2 byte ASCII
	       numbers, but they are all ASCII in the ANSI/NIST
	       structure. */
	    const int new_fgp_int
	       = atoi((char *)field->subfields[subfield_i]->items[0]->value);
	    
	    if (new_fgp_int == 255)
	       break;
	    if (subfield_i > AR_SZ(fgp_value)) {
	       fprintf(stderr, ERRHDR
		       "unsupported number of FGP items > %lu\n",
		       (unsigned long)AR_SZ(fgp_value));
	       return FALSE;
	    }
	    fgp_value[fgp_count++] = new_fgp_int;
	 }

	 /* ...then check for a match */
	 for (subfield_i=0; subfield_i < fgp_count; subfield_i++)
	    if (criteria->value.num == fgp_value[subfield_i])
	       return TRUE;
	 break;

      case rs_imp:			/* impression type */
	 if (!biomeval_nbis_lookup_IMP_field(&field, &field_i, record))
	    return FALSE;
	 item_value = atoi((char *)field->subfields[0]->items[0]->value);
	 if (criteria->value.num == item_value)
	    return TRUE;
	 break;

      case rs_idc:			/* image descriptor character */
	 if (record->type == TYPE_1_ID)
	    return FALSE;	/* Records of TYPE-1 have no IDC. */
	 if (!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IDC_ID, record))
	    return FALSE;
	 item_value = atoi((char *)field->subfields[0]->items[0]->value);
	 if (criteria->value.num == item_value)
	    return TRUE;
	 break;

      case rs_lrt:			/* logical record type */
	 if (record->type == criteria->value.num)
	    return TRUE;
	 break;

     case rs_nqm:
	if (record->type != TYPE_14_ID)
	   return FALSE;
	if (!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, NQM_ID, record))
	   return FALSE;
	for (subfield_i = 0;
	     subfield_i < field->num_subfields;
	     subfield_i++) {
	   /* const int new_fgp_int
	      = atoi((char *)field->subfields[subfield_i]->items[0]->value); */
	   const int new_nfiq_int
	      = atoi((char *)field->subfields[subfield_i]->items[1]->value);
	   if (subfield_i > 0) 
	      fprintf(stderr, "WARNING : select.c : "
		      "NQM subfield %d > 1, handling of multiple finger slaps "
		      "not completly implemented", subfield_i+1);
	   if (criteria->value.num == new_nfiq_int)
	      return TRUE;
	}
	break;

     case rs_imt:
	if (record->type != TYPE_10_ID)
	   return FALSE;
	
	break;

     case rs_pos:
	if (record->type != TYPE_10_ID)
	   return FALSE;
	if (!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, POS_ID, record))
	   return FALSE;
	for (subfield_i = 0;
	     subfield_i < field->num_subfields;
	     subfield_i++) {
	   const int new_pos_int
	      = atoi((char *)field->subfields[subfield_i]->items[0]->value);

	   if (subfield_i > 0) 
	      fprintf(stderr, "WARNING : select.c : "
		      "POS subfield %d > 1, handling of multiple finger slaps "
		      "not completly implemented", subfield_i+1);
	   if (criteria->value.num == new_pos_int)
	      return TRUE;
	}
	
	break;

      default:
	 if (biomeval_nbis_get_type_params_by_type(&type_params, criteria->type))
	    fprintf(stderr, ERRHDR 
		    "missing implementation of criterion type # %d\n",
		    criteria->type);
	 else
	    fprintf(stderr, ERRHDR
		    "incomplete implementation of criterion type %s (%s)\n",
		    type_params->name, type_params->description);
	 break;
      }
   }
   /* Either all the and-ed values were true, or none of the others were.. */
   return criteria->type == rs_and ? TRUE : FALSE;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_new_rec_sel - Allocates then fills a record selector structure
#cat:              designed to be combined with other similar
#cat:              structures to represent the criteria for selecting
#cat:              records from ANSI/NIST files.

   Input:
      type       - the type of record selector to create
      num_values - the number of values specified next
      ...        - values, either one number or some nested record selectors
   Output:
      recsel     - points to the allocated structure
   Return Code:
      Zero       - success
      Negative   - error
************************************************************************/
int biomeval_nbis_new_rec_sel(REC_SEL **rec_sel, const REC_SEL_TYPE type,
		const int num_values, ...)
#define ERRHDR "ERROR : biomeval_nbis_new_rec_sel : "
{
   va_list values;
   int ret, i, j;

   va_start(values, num_values);

   if (num_values < 1) {
      fprintf(stderr, ERRHDR "at least one value must be supplied");
      ret = -1;
   } else {
      ret = biomeval_nbis_alloc_rec_sel(rec_sel, type, num_values);
      if (!ret) {
	 if (type == rs_and || type == rs_or) { /* boolean combination */
	    /* i counts args, j counts stored non-NULL values */
	    for(i = j = 0; i < num_values; i++) {
	       REC_SEL *rs = va_arg(values, REC_SEL *);
	       if (rs != NULL)
		  (*rec_sel)->value.rs[j++] = rs;
	    }
	    (*rec_sel)->num_values = j;
	 } else if (num_values > 1) { /* error */
	    const REC_SEL_TYPE_PARAMS *type_params;
	    if (!biomeval_nbis_get_type_params_by_type(&type_params, type)) {
	       fprintf(stderr, ERRHDR "too many values %d for type %s\n",
		       num_values, type_params->name);
	    }
	    ret = -2;
	 } else {		/* numeric value */
	    (*rec_sel)->value.num = va_arg(values, int);
	    (*rec_sel)->num_values = 1;
	 }
      }
   }

   va_end(values);
   return ret;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_rec_sel - Allocates an empty initialized structure
#cat:              designed to be combined with other similar
#cat:              structures to represent the criteria for selecting
#cat:              records from ANSI/NIST files.

   Input:
      type       - the type of record selector to create
      alloc_values  - the size of array or number value of slots to allocate
   Output:
      recsel     - points to the allocated structure
   Return Code:
      Zero       - success
      Negative   - error
************************************************************************/
int biomeval_nbis_alloc_rec_sel(REC_SEL **rec_sel,
		  const REC_SEL_TYPE type,
		  const int alloc_values)
{
   REC_SEL *rs;
   int size;

   if (type == rs_and || type == rs_or)
      size = sizeof(REC_SEL) + alloc_values * sizeof(REC_SEL_VALUE);
   else
      size = sizeof(REC_SEL);

   rs = (REC_SEL *)malloc(size);
   if (!rs) {
      perror("ERROR : biomeval_nbis_alloc_rec_sel : cannot malloc %d bytes : ");
      return(-1);
   }
   rs->alloc_values = alloc_values;
   rs->type = type;
   rs->num_values = 0;
   if (type == rs_and || type == rs_or) {
      rs->value.rs = (REC_SEL**)(rs + 1);
   }
   *rec_sel = rs;

   return 0;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_rec_sel - Deallocate a record selection criteria structure
#cat:              and all the nested allocated structures.  Ignore
#cat:              NULL pointers.

   Input:
      critera	 - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_rec_sel(REC_SEL *criteria) {
   int i;

   if (criteria) {
      if (criteria->type == rs_and || criteria->type == rs_or) {
	 for (i = 0; i < criteria->num_values; i++) {
	    biomeval_nbis_free_rec_sel(criteria->value.rs[i]);
	 }
      }
      free(criteria);
   }
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_add_rec_sel_num - Create a record selection criteria structure
#cat:              of the specified type and numeric value, and add it to
#cat:              another record selection criteria structure, which
#cat:              has been created separately and assigned a logical
#cat:              operation to apply in combining the results of
#cat:              a record against the nested criteria structures.

   Input:
      head       - a pointer to the location of the containing structure
      type	 - the type of selection criterion value to add
      value      - the numeric value of the criterion
   Output:
      head  	 - the location of the containing structure, which could 
                   be reallocated to make enough space for the added value
   Return Code:
      zero  	 - success
      negative   - error
************************************************************************/
int biomeval_nbis_add_rec_sel_num(REC_SEL **head, const REC_SEL_TYPE type, const int value)
{
   REC_SEL *new_sel;
   int res;
   
   res = biomeval_nbis_new_rec_sel(&new_sel, type, 1, value);
   if (res < 0) {
      return res;
   }

   return biomeval_nbis_add_rec_sel(head, new_sel);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_add_rec_sel_str - Create a record selection criteria structure
#cat:              of the specified type and string value, and add it to
#cat:              another record selection criteria structure, which
#cat:              has been created separately and assigned a logical
#cat:              operation to apply in combining the results of
#cat:              a record against the nested criteria structures.

   Input:
      head       - a pointer to the location of the containing structure
      type	 - the type of selection criterion value to add
      value      - a pointer to the string value of the criterion
   Output:
      head  	 - the location of the containing structure, which could 
                   be reallocated to make enough space for the added value
   Return Code:
      zero  	 - success
      negative   - error
************************************************************************/
int biomeval_nbis_add_rec_sel_str(REC_SEL **head, const REC_SEL_TYPE type, const char* value)
{
   REC_SEL *new_sel;
   int res;

   res = biomeval_nbis_alloc_rec_sel(&new_sel, type, 1);
   if (res < 0) {
      return res;
   }
   new_sel->num_values = 1;
   new_sel->value.str = (char *)value;

   return biomeval_nbis_add_rec_sel(head, new_sel);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_add_rec_sel - Add specified record selection criteria
#cat:              structure to another record selection criteria
#cat:              structure, which has been created separately and
#cat:              assigned a logical operation to apply in combining
#cat:              the results of checking a record against the nested
#cat:              criteria structures.

   Input:
      head       - a pointer to the location of the containing structure
      new_sel	 - the selection criteria structure to add
   Output:
      head  	 - the location of the containing structure, which could 
                   be reallocated to make enough space for the added value
   Return Code:
      zero  	 - success
      negative   - error
************************************************************************/
int biomeval_nbis_add_rec_sel(REC_SEL **head, const REC_SEL *const new_sel)
#define ERRHDR "ERROR : biomeval_nbis_add_rec_sel : "
{
   REC_SEL* new_ptr;

   /* as usual, most of the code handles the exceptional case, here
      more space is needed */
   if ((*head)->num_values == (*head)->alloc_values) {
      /* double the number of available slots */
      int new_alloc_values = 2 * (*head)->alloc_values;
      int new_size 
	 = sizeof(REC_SEL) + new_alloc_values*sizeof(REC_SEL_VALUE);
      
      new_ptr = realloc(*head, new_size);
      if (new_ptr == NULL) {
	 fprintf(stderr, ERRHDR "connot realloc from %lu bytes to %d\n",
		 (unsigned long)((*head)->alloc_values * sizeof(REC_SEL_VALUE)),
		 new_size);
	 return -1;
      }
      *head = new_ptr;
      (*head)->alloc_values = new_alloc_values;
      (*head)->value.rs = (REC_SEL**)(*head + 1);	 
   }

   /* add the new selector value */
   /* suppress compiler complaint about discarding the const qualifier
      by casting */
   (*head)->value.rs[(*head)->num_values++] = (REC_SEL *)new_sel;
   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_validate_rec_sel_num_value - Check the given integer value
#cat:              against the restrictions in the given type
#cat:              parameters, and indicate whether the value is
#cat:              valid for this type.

   Input:
      type_params - points to a record selector type parameter structure
      value      - the value to check to see if it is valid for the given type
   Return Code:
      zero       - OK
      nonzero    - not valid
************************************************************************/
static int 
biomeval_nbis_validate_rec_sel_num_value(const REC_SEL_TYPE_PARAMS *const type_params,
			   const int value)
#define ERRHDR "ERROR : biomeval_nbis_validate_rec_sel_num_value : "
{
   /* The standard does not define numeric values for some things... */
   if (rsv_num != type_params->value_type)
      return FALSE;

   /* First check the extremes. */
   if (type_params->min < type_params->max) {
      if (value < type_params->min) {
	 fprintf(stderr, ERRHDR "%s (%s) value %d is below the minimum of %d\n",
		 type_params->description, type_params->name, 
		 value, type_params->min);
	 return -1;
      } else if (value > type_params->max) {
	 fprintf(stderr, ERRHDR "%s (%s) value %d is above the maximum of %d\n",
		 type_params->description, type_params->name, 
		 value, type_params->max);
	 return -2;
      }
   } 

   /* Even within the extremes, not all values are necessarily valid. */
   if (type_params->enum_values) {
      const REC_SEL_VALUE *valp;
      
      for (valp = type_params->enum_values;
	   valp - type_params->enum_values < type_params->value_count;
	   valp++) {
	 if ((*valp).num == value) {
	    return 0;
	 }
      }
      fprintf(stderr, ERRHDR "%s (%s) value %d is not valid.\n",
	      type_params->description, type_params->name, value);
      return -3;
   }

   /* If no constraints are defined, then anything goes. */
   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_rec_sel_usage - Print information about how to specify record
#cat:              selectors of a particular type

   Input:
      tp         - a pointer to a set of record selector type parameters
   Output:       
                 - prints to stderr
   Return Code:
                 - none
***********************************************************************/
static void biomeval_nbis_rec_sel_usage(const REC_SEL_TYPE_PARAMS *const tp)
{
   const int ind_inc = 3, line_len = 80;
   int i, pos, indent = ind_inc;

   fprintf(stderr, "%*sSpecification of %s (%s):\n",
	   indent, "", tp->description, tp->name);
   indent += ind_inc;
   if (tp->reference)
      fprintf(stderr, "%*sreference: %s\n", indent, "", tp->reference);
   if (tp->min != tp->max) 
      fprintf(stderr, "%*slimits: %d, %d\n", indent, "", tp->min, tp->max);
   else
      fprintf(stderr, "%*slimits: unspecified\n", indent, "");

   if (tp->value_count) {
      pos = fprintf(stderr, "%*svalid values: ", indent, "");
      for (i = 0; i < tp->value_count; i++) {
	 if (pos + 4 > line_len) 
	    /* spaces below to match:    'valid values: ' */
	    pos = fprintf(stderr,   "\n%*s              ", indent, "") - 1;
	 pos += fprintf(stderr, "%ld, ", tp->enum_values[i].num);
      }
      fprintf(stderr, "\b\b.\n");
   } else {
      fprintf(stderr, "%*svalid values: whole range, endpoints included\n",
	      indent, "");
   }

   if (tp->named_sets_count) {
      fprintf(stderr, "%*snamed sets of values"
	      " (unique abbreviations accepted):\n", indent, "");
      indent += ind_inc;
      for (i = 0; i < tp->named_sets_count; i++) {
	 const REC_SEL_NAMED_SETS *const ns = tp->named_sets + i;
	 const char *const *npp;
	 const REC_SEL_VALUE *vp;
	 
	 pos = fprintf(stderr, "%*s", indent, "");
	 for (npp = ns->names; *npp; npp++) {
	    if (pos + 2 + strlen(*npp) > line_len)
	       pos = fprintf(stderr,   "\n%*s", indent, "") - 1;
	    pos += fprintf(stderr, "%s, ", *npp);
	 }
	 indent += ind_inc;

	 pos += fprintf(stderr, "\b\b ") - 4;
	 if (pos + 8 > line_len)
	    pos = fprintf(stderr, "\n%*s", indent, "") - 1;
	 pos += fprintf(stderr, "=> {");
	 for (vp = ns->values; vp - ns->values < ns->num_values; vp++) {
	    if (pos + 4 > line_len)
	       pos = fprintf(stderr,   "\b\n%*s", indent, "") - 2;
	    if (rsv_num == tp->value_type)
	       pos += fprintf(stderr, "%ld, ", (*vp).num);
	    else if (rsv_str == tp->value_type)
	       pos += fprintf(stderr, "\"%s\", ", (*vp).str);
	 }
	 fprintf(stderr, "\b\b}\n");
	 indent -= ind_inc;
      }
      indent -= ind_inc;
   }
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_parse_rec_sel_option - Parse a command line option specifying
#cat:              record selection criteria, such as FGP or IMP.
#cat:              Values may be specified as integers from tables in
#cat:              the standard, lists or ranges of such integers, or
#cat:              strings that map to one or more integer values.
#cat:              Returns a record selector of type 'or' containing
#cat:              all the specified values.

   Input:
      type       - type of selector to parse, e.g., FGP, or IMP.
      optstr	 - option argument string
      rest       - NULL or address for returning a pointer into a string
      head       - top level record selector, NULL on the first call,
                   or the address returned previously
      verbose    - if true, print some information to stderr
   Output:
      rest       - address of remaining unparsed portion of option string
      head 	 - address of the top level criteria structure holding 
                   the results of the parsing

   Return Code:
      zero     	 - success
      nonzero    - error
************************************************************************/
int biomeval_nbis_parse_rec_sel_option(const REC_SEL_TYPE type,
			 const char *const optstr,
			 const char **remainder,
			 REC_SEL **head,
			 const int verbose)
#define ERRHDR "ERROR : biomeval_nbis_parse_rec_sel_option : "
{
   const char *const separators = ",-:"; /* meanings: list, range, stop */
   const REC_SEL_TYPE_PARAMS *type_params;
   const char *startp, *strendp;
   int range_start;
   
   /* the first time through, an 'rs_or' record needs to be created to
      hold the alternative acceptable values for this type */
   if (*head == NULL) {
      REC_SEL *orsel;

      if (biomeval_nbis_alloc_rec_sel(&orsel, rs_or, 5)) {
	 return -1;
      }
      *head = orsel;
   }
   
   /* Choose a table for interpreting names and validating values
      based on the selector type. */
   if (biomeval_nbis_get_type_params_by_type(&type_params, type)) {
      fprintf(stderr, ERRHDR "option parsing not implemented for type %d\n",
	      type);
      return -1;
   }

   /* print a notice about parameters about to be processed, if it
      looks like there are any */
   if (verbose && *optstr != '\0' && *optstr != ':')
      fprintf(stderr, "%s: using selector type %s (%s) parameters\n\t",
	      __func__, type_params->description, type_params->name);

   /* Parse each field of the option string value.  A field is a
      single token that specifies one or more values, i.e., a name or
      number.  Fields are combined using commas to form a list, or a
      dash to form a range of numerical values. */

   range_start = -1; /* Zero is typically a valid field value, so -1
			is used to indicate when no range start value
			has been identified. */

   /* loop over the fields in the option argument string */
   for (startp = strendp = optstr; 
	*strendp != '\0' && *strendp != ':';
	startp = strendp+1) {
      char *numendp;
      int new_value;

      /* skip any spaces preceeding the value */
      while (*startp == ' ') {
	 ++startp;
      }

      /* find an option field separator or the end of the option */
      for(strendp = startp; *strendp; strendp++) {
	 if (strchr(separators, *strendp)) {
	    break;		/* break small local for-loop */
	 }
      }
      /* strendp now points to a value separator or the end of the string */

      /* check for an empty string, which could happen after ignoring spaces */
      if (startp == strendp) {
	 fprintf(stderr, ERRHDR
		 "missing value near character %lu in argument : '%s'\n",
		 (unsigned long)(startp-optstr+1), optstr);
	 return -1;
      }
      
      if (!strncasecmp(startp, "help", 4)) {
 	 biomeval_nbis_rec_sel_usage(type_params);
	 if ( remainder && 
	      (*strendp != ':' || !strncasecmp(strendp, ":help", 5)) ) {
	    *remainder = startp;
	    return 0;
	 }
	 return -1;  /* not really an error, but should not proceed */
      }

      /* check for a number that takes up the whole string */
      new_value = strtol(startp, &numendp, 10);
      if (numendp == strendp) {                              /* number found */

	 /* Validate the number.  First check the extremes. */
	 if (biomeval_nbis_validate_rec_sel_num_value(type_params, new_value))
	    return -3;
	 
	 /* What is the use of the number? */
	 if (range_start != -1) {                 /* end of range */

	    if (type_params->enum_values) { /* allowed values are enumerated */
	       /* include only valid values within the range */
	       const REC_SEL_VALUE *valp;

	       for (valp = type_params->enum_values;
		    valp - type_params->enum_values < type_params->value_count;
		    valp++) {
		  if ((*valp).num >= range_start && (*valp).num <= new_value) {
		     if (verbose) 
			fprintf(stderr, "%ld ", (*valp).num);
		     if (biomeval_nbis_add_rec_sel_num(head, type, (*valp).num))
			return -5;
		  }
	       }
	    } else { /* no enumerated values */
	       /* all values within the range are valid */
	       int val;

	       for (val = range_start; val <= new_value; val++) {
		  if (verbose)
		     fprintf(stderr, "%d ", val);
		  if (biomeval_nbis_add_rec_sel_num(head, type, val))
		     return -5;
	       }
	    }
	    range_start = -1;

	 } else if (strendp && *strendp == '-') { /* beginning of range */
	    range_start = new_value;

	 } else {		                  /* just a number */
	    if (verbose)
	       fprintf(stderr, "%d ", new_value);
	    if (biomeval_nbis_add_rec_sel_num(head, type, new_value))
	       return -6;
	 }

      } else if (range_start != -1 || *strendp == '-') { /*invalid range found*/
	 fprintf(stderr, ERRHDR "unsupported range specifier : %s\n", startp);
	    return -7;

      } else {		                           /* maybe its a named value */
	 /* just looking, using pointers to constants to make sure */
	 const REC_SEL_NAMED_SETS *ntp, *found_ntp;
	 const char *const *namep;
	 const REC_SEL_NAMED_SETS *const name_table
	    = type_params->named_sets;
	 int num_dups = 0, num_problem_dups = 0, exact_match = 0;
	 const char *dups[20];

	 /* Check the relevant table of named value sets, allowing
	    unique abbreviations. */
	 found_ntp = NULL;
	 for (ntp = name_table;
	      ntp - name_table < type_params->named_sets_count;
	      ntp++) {
	    for (namep = ntp->names; *namep; namep++) {
	       if (!strncasecmp(startp, *namep, strendp-startp)) {
		  if (!strncasecmp(startp, *namep, strlen(*namep))) {/* exact */
		     found_ntp = ntp;
		     exact_match = 1;
		  }
		  if (num_dups < AR_SZ(dups)) {
		     dups[num_dups++] = *namep;
		  }
		  if (found_ntp  /* duplicate abbreviation */
		      && found_ntp != ntp) {  /* duplicates within a set
					       are not a problem */
			num_problem_dups++;
		  } else {
		     found_ntp = ntp;		    
		  }
	       }
	    }
	 }

	 if (found_ntp) {
	    const REC_SEL_VALUE *valp;
	    int dup_i;
	    
	    if (num_problem_dups && !exact_match) {
	       fprintf(stderr, ERRHDR 
		       "ambiguous abbreviation '%.*s', could match ",
		       (int)(strendp-startp), startp);
	       for (dup_i = 0; dup_i < num_dups; dup_i++) {
		  fprintf(stderr, "'%s', ", dups[dup_i]);
	       }
	       fprintf(stderr, "\b\b.\n");
	       return -8;
	    }
	    
	    /* Convert text items to canonical values, either numbers
	       or strings. */
	    for (valp = found_ntp->values;
		 valp - found_ntp->values < found_ntp->num_values; 
		 valp++) {
	       if (rsv_num == type_params->value_type) {
		  if (verbose)
		     fprintf(stderr, "%ld ", valp->num);
		  if (biomeval_nbis_add_rec_sel_num(head, type, valp->num) < 0)
		     return -9;
	       } else if (rsv_str == type_params->value_type) {
		  if (verbose)
		     fprintf(stderr, "\"%s\" ", valp->str);
		  if (biomeval_nbis_add_rec_sel_str(head, type, valp->str) < 0)
		     return -11;
	       }
	    }
	 } else {
	    fprintf(stderr, ERRHDR "unimplemented %s (%s) name : '%.*s'\n",
		    type_params->description, type_params->name,
		    (int)(strendp-startp), startp);
	    return -10;
	 }
      }
   }

   /* print a newline only if there were parameter values printed */
   if (verbose && *optstr != '\0' && *optstr != ':')
      fprintf(stderr, "\n");

   /* The remainder, if any, after the colon ':', can be parsed
      differently by the caller, if the call included a non-null
      pointer to a character pointer to recieve the location. */
   if (*strendp == ':') {	/* there is more that wasn't parsed */
      if (remainder) {
	 *remainder = ++strendp;
      } else {
	 fprintf(stderr, ERRHDR "part of option specifier not parsed : %s\n",
		 ++strendp);
      }
   } else if (remainder) {	/* end of string, but caller wants remainder */
      *remainder = strendp;
   }

   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_simplify_rec_sel - Simplify a set of record selectors,
#cat:              eliminating unnecessary or redundant elements, like
#cat:              an 'rs_and' or 'rs_or' with one or no arguments,
#cat:              combining nested 'rs_and's, etc.

   Input:
      rec_sel    - pointer to record selector to simplify
   Output:
      rec_sel    - pointer to simplified record selector
   Return Code:
      zero     	 - success
      negative   - error
************************************************************************/
int biomeval_nbis_simplify_rec_sel(REC_SEL **rs)
{
#if 0
   int i, j, k;		/* loop control is tricky here */

   /* atomic record selectors cannot be simplified,
      only 'rs_and' and 'rs_or' types */
   if ((*rs)->type == rs_and || (*rs)->type == rs_or) {
      REC_SEL **rsvn = (*rs)->value.rs;
      for (i = 0; i < (*rs)->num_values; i++) {
	 if (biomeval_nbis_simplify_rec_sel(&rsvn[i])) /* depth first recursion */
	    return -1;
	 
	 /* eliminate NULLs */
	 while ((*rs)->num_values > 1 && rsvn[i] == NULL) {
	    /* shift remaining values left to eliminate the NULL */
	    for(j = i + 1; j < (*rs)->num_values; j++) {
	       rsvn[j - 1] == rsvn[j];
	    }
	    (*rs)->num_values--; /* dynamic loop limit change */
	 }

	 /* eliminate duplicates */
	 j = i + 1;
	 while (j < (*rs)->num_values) {
	    if (rsvn[i]->type == rsvn[j]->type
		&& rsvn[i]->value.num == rsvn[j]->value.num) {
	       /* shift remaining values left to eliminate the redundant one */
	       for (k = j + 1; k < (*rs)->num_values; k++) {
		  rsvn[k - 1] = rsvn[k];
	       }
	       (*rs)->num_values--; /* dynamic loop limit change */
	    } else {
	       j++;   /* increment j only if we have not changed the value */
	    }
	 }
      }

      /* splice out unneeded boolean item */
      if ((*rs)->num_values == 0) {
	 /*	 biomeval_nbis_free_rec_sel(*rs); */
	 *rs = NULL;
      } else if ((*rs)->num_values == 1) {
	 REC_SEL *old_rs = *rs;
	 *rs = (*rs)->value.rs[0];
	 /*	 biomeval_nbis_free_rec_sel(old_rs); */
      }
   }
#endif
   return 0;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_rec_sel - Write a set of record selectors to an open I/O
#cat:              stream in a human readable format that can be
#cat:              examined, modified, and read back in with
#cat:              biomeval_nbis_read_rec_sel.

   Input:
      fpout      - pointer to a writeable file I/O stream
      sel        - the record selector structure to write
   Output:
                 - output is written to the specified stream
   Return Code:
      zero     	 - success
      nonzero    - error
************************************************************************/
int biomeval_nbis_write_rec_sel(FILE *fpout, const REC_SEL *const sel)
#define ERRHDR "ERROR : biomeval_nbis_write_rec_sel : "
{
   static int indent = 0;
   const REC_SEL_TYPE_PARAMS *type_params;
   int rs_i, indent_increment, num_values;

   if (!sel)
      return 0;

   /* look at how many values are assigned, and omit any that are NULL */
   num_values = sel->num_values;
   if (sel->type == rs_and || sel->type == rs_or) {
      for (rs_i = 0; rs_i < sel->num_values; rs_i++) {
	 if (!sel->value.rs)
	    --num_values;
      }
   }
   /* ... do nothing if there is nothing left */
   if (!num_values)
      return 0;

   /* lookup the record selector type parameters,
      which includes the type name string */
   if (biomeval_nbis_get_type_params_by_type(&type_params, sel->type)) {
      fprintf(stderr, ERRHDR "unimplemented type %d\n", sel->type);
      return -3;
   }

   /* print out the selectors, with indentation for easier reading */
   fprintf(fpout, "%*s", indent, "");
   indent_increment = 2 + fprintf(fpout, "%s ", type_params->name);
   switch (type_params->value_type) {
   case rsv_rs:
      fprintf(fpout, "{\n");
      indent += indent_increment;
      for (rs_i = 0; rs_i < sel->num_values; rs_i++) {
	 if (biomeval_nbis_write_rec_sel(fpout, sel->value.rs[rs_i]) < 0) {
	    return -4;
	 }
      }
      fprintf(fpout, "%*s}", indent-2, "");
      indent -= indent_increment;
      break;

   case rsv_num:
      fprintf(fpout, "%3ld", sel->value.num);
      break;

   case rsv_str:
      fprintf(fpout, "\"%s\"", sel->value.str);
      break;

   default:
      fprintf(stderr, ERRHDR "invalid record-selector value type: %d\n",
	      type_params->value_type);
      return -1;
   }
   fprintf(fpout, "\n");
   
   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_rec_sel_file - Write a set of record selectors to the
#cat:              named file in a human readable format that can be
#cat:              examined, modified, and read back in with
#cat:              biomeval_nbis_read_rec_sel_file.

   Input:
      file       - the name of a file to be written
      sel        - the record selector structure to write
   Output:
       	         - output is written to the specified file
   Return Code:
      zero     	 - success
      nonzero    - error
************************************************************************/
int biomeval_nbis_write_rec_sel_file(const char *const file, 
		       const REC_SEL *const sel)
#define ERRHDR "ERROR : biomeval_nbis_write_rec_sel_file : "
{
   FILE *fpout;
   int ret;

   fpout = fopen(file, "w");
   if (fpout == NULL) {
      fprintf(stderr, ERRHDR "fopen : %s : %s\n", file, strerror(errno));
      return -1;
   }
   
   if ( (ret = biomeval_nbis_write_rec_sel(fpout, sel)) ) {
      if (fclose(fpout)) {
	  fprintf(stderr, ERRHDR "fclose : %s : %s\n",
		  file, strerror(errno));
       }
       return ret;
   }

   if (fclose(fpout)) {
      fprintf(stderr, ERRHDR "fclose : %s : %s\n", file,strerror(errno));
      return -2;
   }

   return 0;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_rec_sel - Read a set or record selectors from an open file
#cat:              I/O stream into a set of record selector
#cat:              structures.

   Input
      fpin       - pointer to a readable file I/O stream
   Output
      sel        - Address used to return a pointer to the record selection
                   criteria read.  The space is dynamically allocated and can
                   be freed with biomeval_nbis_free_rec_sel.
   Return
      zero       - success
      nonzero    - error
************************************************************************/
int biomeval_nbis_read_rec_sel(FILE *fpin, REC_SEL **sel)
#define ERRHDR "ERROR : biomeval_nbis_read_rec_sel : "
{
   char buffer[64], *cp, *ep;
   const REC_SEL_TYPE_PARAMS *type_params = NULL;
   REC_SEL *new_sel, *current_sel = NULL;
   int ci, num;
   static int line_no = 1, col_no = 1;

   /* 'type_params' starts out NULL.  When a record selector type name
      is read, 'type_params' is set to point to the parameter
      structure for that type.  After the corresponding value is read,
      checked, and assigned, 'type_params' is again set to NULL,
      indicating readiness to read another type.

      'current_sel' starts out NULL.  After a record selector type is
      read, a record selector structure is created, and its address is
      assigned to 'current_sel'.  Then, one or more values are
      assigned to the structure, depending the type. */

   cp = buffer;
   while (!feof(fpin)) {
      ci = fgetc(fpin);

      /* track cursor position for error reporting */
      if (ci == '\n') {
	 line_no++;		/* count the line */
	 col_no = 1;		/* reset the column */
      } else {
	 col_no++;		/* count the column */
      }

      /* first accumulate the input into tokens */
      if (!isspace(ci) && ci != EOF) {
	 *cp++ = ci;	  /* append another character to the string */
	 if (cp - buffer > sizeof(buffer)) {
	    fprintf(stderr, ERRHDR
		    "buffer overflow, exceeded %lu bytes, reading '%.*s'\n",
		    (unsigned long)sizeof(buffer), (int)sizeof(buffer), buffer);
	    return -1;
	 }

      } else if (cp > buffer) {	/* the end of a token was found */
	 *cp = 0;		/* ... terminate it */

	 /* determine the the type of token and its value, check
	   whether it is valid in the current context, then create the
	   corresponding data structures */

	 if (!strcmp(buffer, "{")) {       /* nested criteria opening bracket */
	    if (!type_params
		|| (type_params->type != rs_and && type_params->type != rs_or)) {
	       fprintf(stderr, ERRHDR "unexpected '{' at line %d, column %d\n",
		       line_no, col_no);
	       return -2;
	    }
	    while (1) {		/* iterate over nested criteria */
	       if (biomeval_nbis_read_rec_sel(fpin, &new_sel))
		  return -3;
	       if (!new_sel) /* exit loop after closing bracket */
		  break;
	       if (biomeval_nbis_add_rec_sel(&current_sel, new_sel))
		  return -4;
	    }
	    *sel = current_sel;
	    return 0;
	    
	 } else if (!strcmp(buffer, "}")) { /* nested criteria closing bracket*/
	    *sel = NULL;
	    return 0;		/* empty success */

	 } else if (num = strtol(buffer, &ep, 0), *ep == '\0') { /* num value */
	    if (!type_params ||
		type_params->type == rs_and || type_params->type == rs_or) {
	       fprintf(stderr, ERRHDR
		       "unexpected number %d at line %d, column %d\n",
		       num, line_no, col_no);
	       return -5;
	    }
	    if (!current_sel) {
	       /* this should never happen, but it doesn't hurt to check */
	       fprintf(stderr, ERRHDR "no structure allocated to store"
		       " type %s with value %d at line %d, column %d\n",
		       type_params->name, num, line_no, col_no);
	       return -6;
	    }       
	    if (biomeval_nbis_validate_rec_sel_num_value(type_params, num))
	       return -7;

	    current_sel->value.num = num;
	    current_sel->num_values = 1;
	    *sel = current_sel;
	    return 0;	/* success reading simple type/value pair */
	    
	 } else if (!type_params &&              /* record selector type name */
		    !biomeval_nbis_get_type_params_by_name(&type_params, buffer)) {
	    if (biomeval_nbis_alloc_rec_sel(&current_sel, type_params->type, 1))
	       return -8;
	    /* proceed to read the next token */
	    
	 } else {	    /* unexpected record selector type name, or other */
	    fprintf(stderr, ERRHDR
		    "unexpected token '%s' at line %d, column %lu\n",
		    buffer, line_no, (unsigned long)(col_no - (cp - buffer)));
	    return -9;
	 }
	 
	 cp = buffer;		/* prepare for next token */
      }     
   }
   
   fprintf(stderr, ERRHDR "incomplete record selection specifier,"
	   " at line %d, column %d\n", line_no, col_no);
   *sel = NULL;
   return -10;
}
#undef ERRHDR

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_rec_sel_file - Read a set or record selectors from a file
#cat:              into a set of record selector structures.

   Input
      input_file - The name of a file containing a tree of record
                   selectors to be read and parsed to create the
                   output structure.
   Output
      sel -        Address used to return a pointer to the record
                   selection criteria structure created from the data
                   read from the input file.  The space is dynamically
                   allocated and can be freed with biomeval_nbis_free_rec_sel.
   Return
      zero       - success
      nonzero    - error
************************************************************************/
int biomeval_nbis_read_rec_sel_file(const char *const input_file, REC_SEL **sel)
{
   FILE *fpin;
   int ret;

   fpin = fopen(input_file, "r");
   if (fpin == NULL) {
      fprintf(stderr, "ERROR : biomeval_nbis_read_rec_sel_file : fopen : %s : %s\n",
	      input_file, strerror(errno));
      return -1;
   }

   if ( (ret = biomeval_nbis_read_rec_sel(fpin, sel)) ) {
      if (fclose(fpin)) {
	 fprintf(stderr, "ERROR : biomeval_nbis_read_rec_sel_file : fclose : %s : %s\n",
		 input_file, strerror(errno));
      }
      return ret;	  
   }

   if (fclose(fpin)) {
      fprintf(stderr, "ERROR : biomeval_nbis_read_rec_sel_file : fclose : %s : %s\n",
	      input_file, strerror(errno));
      return -2;
   }

   return 0;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_imp_is_live_scan - Indicates whether an impression of a given type
#cat:              is a live scan.

   Input
      imp -        Impression type number.

   Return
      TRUE or FALSE
************************************************************************/
int biomeval_nbis_imp_is_live_scan(const int imp)
{
   int i;

   for (i = 0; i < biomeval_nbis_imp_live_scan_set->num_values; i++) {
      if (imp == biomeval_nbis_imp_live_scan_set->values[i].num)
	 return TRUE;
   }
   return FALSE;
}


/***********************************************************************
************************************************************************
#cat: biomeval_nbis_imp_is_latent - Indicates whether an impression of a given type
#cat:              is a latent print.

   Input
      imp -        Impression type number.

   Return
      TRUE or FALSE
************************************************************************/
int biomeval_nbis_imp_is_latent(const int imp)
{
   int i;

   for (i = 0; i < biomeval_nbis_imp_latent_set->num_values; i++) {
      if (imp == biomeval_nbis_imp_latent_set->values[i].num)
	 return TRUE;
   }
   return FALSE;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_imp_is_rolled - Indicates whether an impression of a given type
#cat:              is a rolled print.

   Input
      imp -        Impression type number.

   Return
      TRUE or FALSE
************************************************************************/
int biomeval_nbis_imp_is_rolled(const int imp)
{
   int i;

   for (i = 0; i < biomeval_nbis_imp_rolled_set->num_values; i++) {
      if (imp == biomeval_nbis_imp_rolled_set->values[i].num)
	 return TRUE;
   }
   return FALSE;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_imp_is_flat - Indicates whether an impression of a given type
#cat:              is a flat print.

   Input
      imp -        Impression type number.

   Return
      TRUE or FALSE
************************************************************************/
int biomeval_nbis_imp_is_flat(const int imp)
{
   int i;

   for (i = 0; i < biomeval_nbis_imp_flat_set->num_values; i++) {
      if (imp == biomeval_nbis_imp_flat_set->values[i].num)
	 return TRUE;
   }
   return FALSE;
}
