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


#ifndef _PCA_H
#define _PCA_H


#include <mlp.h>
#include <little.h>

/* Classifier type for pcasys */
#define	PNN_CLSFR	1
#define MLP_CLSFR	2

/* Standard Image Size */
#define WIDTH  512
#define HEIGHT 480
/* Feature detection window sizes */
#define WS  16
#define HWS 8

/* Typedefs of the structures each of which contains the several
parameters of a routine which has more than one parameter.  By
parameter here is meant, not just any argument of the routine, but a
controlling number which is left unchanged during a run.  Also there
are comments for single parameters. */

/* sgmnt (segmentor): */
typedef struct {
  int fac_n, min_fg, max_fg, nerode,
    rsblobs, fill, min_n, hist_thresh,
    origras_wmax, origras_hmax;
  float fac_min, fac_del, slope_thresh;
} SGMNT_PRS;

/* enhnc (enhancer): */
typedef struct {
  int rr1, rr2;
  float pow;
} ENHNC_PRS;

/* (rors has the single parm rors_slit_range_thresh) */

/* (r92a has the single parm r92a_discard_thresh) */

/* rgar (registering re-averager): */
typedef struct {
  int std_corepixel_x, std_corepixel_y;
} RGAR_PRS;

/* (trnsfrm has the single parm trnsfrm_nrows_use) */
/* (It also has data: the transform matrix) */

/* pnn: */
typedef struct {
  int nclasses;    /* how many classes there are (6 for pcasys) */
  int nprotos_use; /* how many prototypes feature vectors to use */
  int nfeats_use;  /* how may features to use */
  float osf;       /* overall smoothing factor */
  char cls_str[50];/* class string used to display activations */
  int trnsfrm_rws; /* # transform rows */
  int trnsfrm_cls; /* # transform cols */
} PNN_PRS;
/* And pnn has data: the protos and their classes */

/* pseudo (pseudoridge tracer): */
typedef struct {
  float slthresh0, slthresh1, smooth_cwt, stepsize;
  int max_tilt, min_side_turn, initi_s, initi_e,
    initj_s, initj_e, maxsteps_eachdir, nsmooth,
    maxturn;
} PSEUDO_PRS;

/* (combine has the single parm combine_clash_conf) */

/* defines used in enhnc */
#define sq(x) ((x)*(x))
#define slen(x,y) (sq(x)+sq(y))
#define sqpow(x,a) pow((double)sq(x),(double)a)
#define slpow(x,y,a) pow((double)slen((x),(y)),(double)a)


/* defines used in r92 and ridge */
#define D2R            .0174533 /* degrees to radians */
#define R2D            57.2958  /* radians to degrees */
#define PI             3.14159

#define BAD_PIXELROR 8

/* A structure containing the numbers of seconds to sleep at
various points in the graphical version */
  
typedef struct {
  int titlepage, sgmntwork, segras, enhnc, core_medcore, regbars,
    featvec, normacs, foundconup, noconup, lastdisp;
} SLEEPS;

/* Prototype Definitions */

/* combine.c */
extern void combine(const unsigned char, const float, const int, const float,
                 unsigned char *, float *, char *);

/* enhnc.c */
extern void enhnc(unsigned char **, ENHNC_PRS *, unsigned char ***,
                 const int, const int);

/* eigen.c */
extern void eigen(const int, int *, float **, float **, float *, const int);
extern void diag_mat_eigen(const int, float *, const int, float **, float **,
                 int **, int *);

/* inits.c */
extern void mkoas_init(char *, SGMNT_PRS *, ENHNC_PRS *, int *, float *,
                 RGAR_PRS *, int *, int *, int *, FILE **, int *, char *);
extern void mkoas_readparms(char *, SGMNT_PRS *, ENHNC_PRS *, int *, float *,
                 RGAR_PRS *, int *, int *, int *, char *, char *);
extern void mkoas_check_parms_allset(void);
extern void pcasys_init(char *, SGMNT_PRS *, ENHNC_PRS *, int *, float *,
                 RGAR_PRS *, int *, int *, PNN_PRS *, MLP_PARAM *,
                 PSEUDO_PRS *, float *, float **, unsigned char **, float **,
                 FILE **, FILE **);
extern void pcasys_readparms(char *, SGMNT_PRS *, ENHNC_PRS *, int *, float *,
                 RGAR_PRS *, int *, char [], int *, PNN_PRS *, char [], char [],
                 char [], PSEUDO_PRS *, float *, SLEEPS *, int *, char [],
                 char [], int *, int *, MLP_PARAM *);
extern void check_cls_str(char *, const int);

/* mlp_single.c */
extern void mlp_single(MLP_PARAM, float *, char *, float *,
                 void (*)(float *), void (*)(float *));

/* pnn.c */
extern void pnn(float *, PNN_PRS *, float *, unsigned char *, float *,
                 unsigned char *, float *);

/* pseudo.c */
extern int pseudo(unsigned char **, const int, const int, float **, float **,
                 const int, const int, PSEUDO_PRS *);
extern int print_has_conup(float **, float **, const int, const int,
                 const int, const int, const int, const int, const int,
                 const int, char **, float **, const int, const int
#ifdef GRPHCS
                 , const int, const int
#endif
         );
extern int path_has_conup(
#ifdef GRPHCS
                 const int, const int, float *, float *,
#endif
                 float *, float *, const int, const int, const int);
extern int lobe_is_conup(
#ifdef GRPHCS
                 const int, const int, float *, float *,
#endif
                 float *, float *, const int, const int, const int, const int,
                 const int, const int);
extern void pseudo_avrors2_smooth_iter(float **, float **, const int,
                 const int, const float, const float);
extern void pseudo_avrors2_xys2has(float **, float **, float **, const int,
                 const int);

/* r92.c */
extern int r92(float *, int *, int *, int *);

/* r92a.c */
extern void r92a(float **, float **, const int, const int, const float,
                 int *, int *);

/* readfing.c */
extern int readfing(char *, unsigned char **, int *, int *, int *,
                 char *, char *, unsigned char *, FILE *);

/* results.c */
extern void results(const int, const int, const float, const int, const int,
                 const float, FILE *, char *, char *, const int);

/* ridge.c */
extern void rors(unsigned char **, const int, const int, const int,
                 char ***, float ***, float ***, int *, int *);
extern void rgar(char **, const int, const int, const int, const int,
                 RGAR_PRS *, float ***, float ***, int *, int *);
extern void ar2(char **, const int, const int, float ***, float ***,
                 int *, int *);
extern void make_cs(float **, float **);

/* sgmnt.c */
extern void sgmnt(unsigned char *, const int, const int, SGMNT_PRS *,
                 unsigned char ***, const int, const int, unsigned char ***,
                 int *, int *);
extern int sgmnt_make_fg(unsigned char *, const int, const int, unsigned char *,
                 const int, const int, const int, const int, const int,
                 const int, const float, const float, const int);
extern int sgmnt_ebfc(unsigned char *, const int, const int, const int,
                 const int, int *, int *, const int, const int, const int);
extern int sgmnt_edges(unsigned char *, const int, const int, const int,
                 const int, const int, const int, int *, int *, float *,
                 const int, const float, const int, const int, const int);
extern int scan_row_from_left_foundtrue(unsigned char *, const int, const int,
                 int *);
extern int scan_row_from_right_foundtrue(unsigned char *, const int, const int,
                 int *);
extern int scan_col_from_top_foundtrue(unsigned char *, const int, const int,
                 const int, int *);
extern int sgmnt_decide_location(unsigned char *, const int, const int,
                 const int, const int, const float, int *, int *, const int,
                 const int, const int);
extern void sgmnt_snip_interp(unsigned char *, const int, const int,
                 const int, const int, const float, unsigned char ***,
                 const int, const int, unsigned char *, const int,
                 const int, unsigned char ***, int *, int *);
extern void sgmnt_snip(unsigned char *, const int, const int, const int,
                 const int, const float, unsigned char ***, const int,
                 const int, unsigned char *, const int, const int,
                 unsigned char ***, int *, int *);

/* trnsfrm.c */
extern void trnsfrm(float **, float **, const int, const int, const int,
                 float *, const int, const int, float *);

#endif /* !_PCA_H */
