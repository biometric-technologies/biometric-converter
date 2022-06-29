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


#ifndef _MLP_H
#define _MLP_H

#include <usebsd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <defs.h>
#include <swap.h>
#include <util.h>
#include <memalloc.h>

/***********************************************************************/
/* Formerly in mlp/defs.h */
#ifndef FALSE
#define FALSE ((char)0)
#define TRUE  ((char)1)
#endif

/***********************************************************************/
/* Formerly in mlp/fmt_msgs.h */
/* For use by strm_fmt() and lgl_tbl(), which format the warning and
error messages that may be written as the result of scanning a
specfile.  Columns are numbered starting at 0. */

#define MESSAGE_FIRSTCOL_FIRSTLINE   6 /* for first line of a msg */
#define MESSAGE_FIRSTCOL_LATERLINES  8 /* later lines indented */
#define MESSAGE_LASTCOL             70
#define MESSAGE_FIRSTCOL_TABLE      12 /* table indented even more */

/***********************************************************************/
/* Formerly in mlp/get_phr.h */
/* Names of get_phr()'s return values: */
#define WORD_PAIR      ((char)0)
#define NEWRUN         ((char)1)
#define ILLEGAL_PHRASE ((char)2)
#define FINISHED       ((char)3)

/***********************************************************************/
/* Formerly in mlp/lbfgs_dr.h */
#define STPMIN 1.e-20
#define STPMAX 1.e+20

/***********************************************************************/
/* Formerly in mlp/lims.h */
#define MAXMED 100000
#define LONG_CLASSNAME_MAXSTRLEN 32

/***********************************************************************/
/* Formerly in mlp/macros.h */
#define mlp_min(x,y) ((x)<=(y)?(x):(y))
#define mlp_max(x,y) ((x)>=(y)?(x):(y))

/***********************************************************************/
/* Formerly in mlp/mtch_pnm.h */
/* Names of the values of the a_type parm of mtch_pnm. */
#define MP_FILENAME ((char)0)
#define MP_INT      ((char)1)
#define MP_FLOAT    ((char)2)
#define MP_SWITCH   ((char)3)

/* Bundles together some parms for mtch_pnm, to reduce the verbosity
of the (many) calls of it by st_nv_ok. */
typedef struct {
  char *namestr, *valstr, *errstr, ok;
  int linenum;
} NVEOL;

/***********************************************************************/
/* Formerly in mlp/rd_words.h */
#define RD_INT   ((char)0)
#define RD_FLOAT ((char)1)

/***********************************************************************/
/* Formerly in mlp/scg.h */
#define XLSTART 0.01 /* Starting value for xl. */
#define NF 3         /* Don't quit until NF * nfreq iters or... */
#define NITER 40     /* ...until NITER iters, whichever is larger... */
#define NBOLTZ 100   /* ...until NBOLTZ iters, if doing Boltzmann. */
#define NNOT 3       /* Quit if not improving NNOT times in row. */
#define NRESTART 100000 /* Restart after NRESTART iterations. */

/***********************************************************************/
/* Formerly in mlp/tda.h */
/* Two-dimensional arrays with dimensions that are variables. */

typedef struct {
  int dim2;
  char *buf;
} TDA_CHAR;

typedef struct {
  int dim2;
  int *buf;
} TDA_INT;

typedef struct {
  int dim2;
  float *buf;
} TDA_FLOAT;

/* "Element" macro: refers to the (i,j) element of atda, which can
be a TDA_CHAR, a TDA_INT, or a TDA_FLOAT. */
#define e(atda,i,j) (*((atda).buf+(i)*(atda).dim2+(j)))

/***********************************************************************/
/* Formerly in mlp/parms.h */
#define PARMTYPE_FILENAME ((char)0)
#define PARMTYPE_INT      ((char)1)
#define PARMTYPE_FLOAT    ((char)2)
#define PARMTYPE_SWITCH   ((char)3)

#define PARM_FILENAME_VAL_DIM 100

typedef struct {
  char set_tried, set;
  int linenum;
} SSL;

typedef struct {
  char val[PARM_FILENAME_VAL_DIM];
  SSL ssl;
} PARM_FILENAME;

typedef struct {
  int val;
  SSL ssl;
} PARM_INT;

typedef struct {
  float val;
  SSL ssl;
} PARM_FLOAT;

typedef struct {
  char val;
  SSL ssl;
} PARM_SWITCH;

typedef struct {
  PARM_FILENAME long_outfile, short_outfile, patterns_infile,
    wts_infile, wts_outfile, class_wts_infile, pattern_wts_infile,
    lcn_scn_infile;
  PARM_INT npats, ninps, nhids, nouts, seed, niter_max, nfreq, nokdel,
    lbfgs_mem;
  PARM_FLOAT regfac, alpha, temperature, egoal, gwgoal, errdel, oklvl,
    trgoff, scg_earlystop_pct, lbfgs_gtol;
  PARM_SWITCH errfunc, purpose, boltzmann, train_or_test, acfunc_hids,
    acfunc_outs, priors, patsfile_ascii_or_binary, do_confuse,
    show_acs_times_1000, do_cvr;
} PARMS;


/* Symbolic names of values of "switch" parms.  The corresponding
value strings (expected in the spec file) are these names but in lower
case; the numerical values are also ok in the spec file.  For example,
to set errfunc to MSE, use either of the following in the spec file:
  errfunc mse
  errfunc 0
Note that the names and corresponding code-numbers here must match the
contents of the legal_names_codes_str parms in the calls of mtch_pnm()
by st_nv_ok(), but with the names in lower case in those calls. */

/* For errfunc: */
#define MSE     ((char)0)
#define TYPE_1  ((char)1)
#define POS_SUM ((char)2)

/* For purpose: */
#define CLASSIFIER ((char)0)
#define FITTER     ((char)1)

/* For boltzmann: */
#define NO_PRUNE     ((char)0)
#define ABS_PRUNE    ((char)2)
#define SQUARE_PRUNE ((char)3)

/* For train_or_test: */
#define TRAIN ((char)0)
#define TEST  ((char)1)

/* For acfunc_hids and acfunc_outs: */
#define SINUSOID    ((char)0)
#define SIGMOID     ((char)1)
#define LINEAR      ((char)2)
#define BAD_AC_CODE ((char)127)

/* For priors: */
#define ALLSAME ((char)0)
#define CLASS   ((char)1)
#define PATTERN ((char)2)
#define BOTH    ((char)3)

/* For patsfile_ascii_or_binary: */
#define ASCII  ((char)0)
#define BINARY ((char)1)

/* The allowed values for the following "logical" switch parms are
TRUE and FALSE (defined in defs.h), which should be represented in the
spec file as true and false: do_confuse, show_acts_times_1000,
do_cvr. */


typedef struct mlpparamstruct{
   char **class_map;
   int  ninps;
   int  nhids;
   int  nouts;
   char acfunc_hids;
   char acfunc_outs;
   float *weights;
   char cls_str[50];
   int trnsfrm_rws;
   int trnsfrm_cls;
} MLP_PARAM;

#define MAX_NHIDS 1000 /* Maximum number of hidden nodes */
#define TREEPATSFILE 5151
#define JUSTPATSFILE    0
#define FMT_ITEMS       8

/***********************************************************************/
/* ACCUM.C : */
extern void accum_init(int, char, float);
extern void accum_zero(char);
extern void accum_cpat(char, char, float *, short, float *, float);
extern void accum_print(char, char, int, int, float, float, float, char,
                        float [], char **, char **, int *, int *, float *);
extern void accum_free(void);
extern void accum_printer(char **, char **, int, int, int *, TDA_INT *);
extern void accum_sumout(char, int, int, char, float [], float, float, float,
                         int *, int *, float *);
extern void accum_yow(FILE *, char []);


/***********************************************************************/
/* ACS.C : */
extern void ac_sinusoid(float, float *, float *);
extern void ac_v_sinusoid(float *);
extern void ac_sigmoid(float, float *, float *);
extern void ac_v_sigmoid(float *);
extern void ac_linear(float, float *, float *);
extern void ac_v_linear(float *);

/***********************************************************************/
/* ACSMAPS.C : */
extern void (*acsmaps_code_to_fn(char))(float, float *, float *);
extern char *acsmaps_code_to_str(char);
extern char acsmaps_str_to_code(char []);
extern void (*acsmaps_code_to_fn2(char))(float *);

/***********************************************************************/
/* BOLTZ.C : */
extern void boltz(int, int, int, char, float, float *);
extern void boltz_work(float *, char, float, int, int *, int *,
                       float *, float *, float *, float *, float *);

/***********************************************************************/
/* CH_BPRMS.C : */
extern void ch_bprms(PARMS *, char *, char *);

/***********************************************************************/
/* CSOPIWH.C : */
extern void csopiwh(PARMS *parms);

/***********************************************************************/
/* CVR.C : */
extern void cvr_init(void);
extern void cvr_zero(void);
extern void cvr_cpat(float, short, short, float);
extern void cvr_print(char, int);

/***********************************************************************/
/* CWRITE.C : */
void cwrite(int iter);

/***********************************************************************/
/* E_AND_G.C : */
extern void e_and_g(char, char, char, char, char [], char, char, int,
                    int, int, float *, int, float *, char, float *, short *,
                    void (*)(float, float *, float *),
                    void (*)(float, float *, float *),
                    char, float, float *, float,
                    float, float *, float *, float *, float *);

/***********************************************************************/
/* EB.C : */
extern void eb_cat(char []);
extern char *eb_get(void);
extern void eb_clr(void);

/***********************************************************************/
/* EB_CAT_E.C : */
extern void eb_cat_e(char []);

/***********************************************************************/
/* EB_CAT_W.C : */
extern void eb_cat_w(char []);

/***********************************************************************/
/* EF.C : */
extern void ef_mse_t(int, float *, float *, float *, float *);
extern void ef_mse_c(int, float *, short, float *, float *);
extern void ef_t1_c(int, float *, short, float, float *, float *);
extern void ef_ps_c(int, float *, short, float *, float *);

/***********************************************************************/
/* ENDOPT.C : */
extern void endopt(int, int, int, float, float);

/***********************************************************************/
/* FSASO.C : */
extern void fsaso_init(char []);
extern void fsaso(char []);

/***********************************************************************/
/* GET_PHR.C : */
extern char get_phr(FILE *, char [], char [], char [], int *);

/***********************************************************************/
/* GETPAT.C : */
extern void getpat(char [], char, int, int, int, char, float, char ***,
            float **, float **, short **, int *);
extern char got_mmm(char [], char, int *, int *, int *, char []);

/***********************************************************************/
/* GOT_BLK.C : */
extern int got_blk(FILE *, PARMS *, char *, int *);

/***********************************************************************/
/* GOT_C.C : */
extern char got_c(FILE *, char *, int *);

/***********************************************************************/
/* GOT_NC_C.C : */
extern char got_nc_c(FILE *, char *, int *);

/***********************************************************************/
/* IS_WE.C : */
extern void is_w_set(void);
extern void is_w_clr(void);
extern char is_w_get(void);
extern void is_e_set(void);
extern void is_e_clr(void);
extern char is_e_get(void);

/***********************************************************************/
/* LBFGS.C : */
extern void lbfgs(int, int, float *, float, float *, int, float *, int *,
           float, float *, int *, int *, FILE *, FILE *, float, float,
           float, int *, int *, int *, float *);
extern void lb1(int *, int, int, float, int, int, float *, float, float *,
           float, int, FILE *);
extern void mcsrch(int, float *, float, float *, float *, float *, float,
           float, float, float, float, int, int *, int *, float *, FILE *);
extern void mcstep(float *, float *, float *, float *, float *, float *,
           float *, double *, float *, int *, float, float, int *);

/***********************************************************************/
/* LBFGS_DR.C : */
extern void lbfgs_dr(char, char, char [], char, char, int, int, int, int,
              int, float *, char, float *, short *,
              void(*)(float, float *, float *),
              void(*)(float, float *, float *),
              char, float, float *, float, float, int, float, float, float,
              char, char **, char **, float, int, float *, float *,
              float *, float *, int *, int *, int *);
extern void survey(int, float *, float *, float);

/***********************************************************************/
/* LGL_PNM.C : */
extern char lgl_pnm(char []);

/***********************************************************************/
/* LGL_TBL.C : */
extern void lgl_tbl(int, char *[], char [][2], char []);

/***********************************************************************/
/* LITTLE.C : */
extern void free_notnull(char *);
extern int intcomp(int *, int *);
extern char *s2hms(float);
extern float ups_secs(void);

/***********************************************************************/
/* LNG2SHRT.C : */
extern void lng2shrt(int, char **, char *, char ***);

/***********************************************************************/
/* MTCH_PNM.C : */
extern char mtch_pnm(NVEOL *, char *, void *, char, char *);

/***********************************************************************/
/* NEVERSET.C : */
extern void neverset(char []);

/***********************************************************************/
/* OPTCHK.C : */
extern void optchk_store_unchanging_vals(int, char **, char **, int,
                   char, float, int, float, int, float);
extern void optchk_store_e1_e2(float, float);
extern void optchk(char, int, float [], float, int *, float *);

/***********************************************************************/
/* OPTWTS.C : */
extern void optwts(char, float, char, char, char [], char, char, int,
            int, int, int, int, float *, char, float *, short *,
            void (*)(float, float *, float *),
            void (*)(float, float *, float *),
            char, float, float *, float, char, float, int, float, float,
            float, char **, char **, char, float, int, float *, float *,
            float *, int *, int *, int *);

/***********************************************************************/
/* PAT_IO.C : Patterns file I/O */
extern int read_bin_nnpats(char *, float **, float **, int **, char ***,
                           int *, int *, int *);
extern int read_num_pats(char *);
extern int read_text_nnpats(char *, float **, float **, char ***,
                            int *, int *, int *);
extern int write_bin_nnpats(char *, float *, float *, char **,
                            const int, const int, const int);
extern int write_text_nnpats(char *, float *, float *, char **,
                            const int, const int, const int);

/***********************************************************************/
/* RD_CWTS.C : */
extern void rd_cwts(int, char **, char *, float **);

/***********************************************************************/
/* RD_WORDS.C : */
extern void rd_words(char, FILE *, int, int, char, void *);

/***********************************************************************/
/* RPRT_PRS.C : */
extern void rprt_prs(PARMS *, int);

/***********************************************************************/
/* RUNMLP.C : Feedforward MLP Utilities */
extern void mlphypscons(int, int, int, char, char, float *, float *,
                        int, int *, float *);
extern void runmlp(int, int, int, char, char, float *, float *, float *,
                   int *, float *);
extern int runmlp2(const int, const int, const int,
                   const char, const char, float *, float *,
                   float *, int *, float *);

/***********************************************************************/
/* SCANSPEC.C : */
extern void scanspec(char [], int *, char *, char *);

/***********************************************************************/
/* SCG.C : */
extern void scg(char, char, char [], char, char, int, int, int, int, int,
                float *, char, float *, short *,
                void (*)(float, float *, float *),
                void (*)(float, float *, float *),
                char, float, float *, float, float, char, float, int,
                float, float, float, char, char **, char **, float *,
                float *, float *, float *, int *, int *, int *);

/***********************************************************************/
/* SET_FPW.C : */
extern void set_fpw(char, char *, int, char **, char *, int,
                    short *, float **);
extern void compute_new_priors(const int, char **, short *,
                    const int, float *);

/***********************************************************************/
/* ST_NV_OK.C : */
extern char st_nv_ok(char *, char *, int, PARMS *, char *);

/***********************************************************************/
/* STRM_FMT.C : */
extern void strm_fmt(char [], char []);

/***********************************************************************/
/* TARGET.C : Target vector utilities */
extern void comp_targvctr(float *, char *, char **, const int);

/***********************************************************************/
/* TSP_W.C : */
extern void tsp_w(char [], int);

/***********************************************************************/
/* UNI.C : */
extern float uni(int);

/***********************************************************************/
/* WTS.C : MLP Weights utilities */
extern void randwts(int, int, int, int, float **);
extern void randwts_oldorder(int, int, int, int, float **);
extern void readwts(PARMS *, float **);
extern void readwts_np(char [], char *, int *, int *, int *, char *,
                       char *, float **);
extern int readwts_np2(char *, char *, int *, int *, int *,  char *,
                       char *, float **);
extern void putwts(char [], float *, char, int, int, int, char, char);

#endif /* !_MLP_H */
