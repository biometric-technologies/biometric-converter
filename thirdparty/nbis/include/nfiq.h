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


#ifndef _NFIQ_H
#define _NFIQ_H

/* UPDATED: 11/21/2006 by KKO */

#include <defs.h>
#include <lfs.h>
#include <mlp.h>

#ifndef DEFAULT_PPI
#define DEFAULT_PPI       500
#endif

#define NFIQ_VCTRLEN      11
#define NFIQ_NUM_CLASSES  5
#define EMPTY_IMG         1
#define EMPTY_IMG_QUAL    5
#define TOO_FEW_MINUTIAE  2
#define MIN_MINUTIAE      5
#define MIN_MINUTIAE_QUAL 5

/***********************************************************************/
/* NFIQ.C : NFIQ supporting routines */
extern int comp_nfiq_featvctr(float *, const int, MINUTIAE *,
                              int *, const int, const int, int *);
int comp_nfiq(int *, float *, unsigned char *,
              const int, const int, const int, const int, int *);
int comp_nfiq_flex(int *, float *, unsigned char *,
              const int, const int, const int, const int,
              float *, float *, const int, const int, const int,
              const char, const char, float *, int *);

/***********************************************************************/
/* ZNORM.C : Routines supporting Z-Normalization */
extern void znorm_fniq_featvctr(float *, float *, float *, const int);
extern int comp_znorm_stats(float **, float **, float *,
                            const int, const int);

/***********************************************************************/
/* NFIQGBLS.C : Global variables supporting NFIQ */
extern float dflt_znorm_means[];
extern float dflt_znorm_stds[];
extern char  dflt_purpose;
extern int   dflt_nInps;
extern int   dflt_nHids;
extern int   dflt_nOuts;
extern char  dflt_acfunc_hids;
extern char  dflt_acfunc_outs;
extern float dflt_wts[];

/***********************************************************************/
/* NFIQREAD.C */
extern int read_imgcls_file(char *, char ***, char ***, int *);
extern int read_znorm_file(char *, float *, float *, const int);

#endif /* !_NFIQ_H */
