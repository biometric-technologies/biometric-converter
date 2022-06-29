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


#ifndef _MEMALLOC_H
#define _MEMALLOC_H

/* UPDATED: 03/15/2005 by MDG */

extern int malloc_char_ret(char **, const int, char *);
extern int malloc_uchar_ret(unsigned char **, const int, char *);
extern int malloc_int_ret(int **, const int, char *);
extern int calloc_int_ret(int **, const int, char *);
extern int realloc_int_ret(int **, const int, char *);

extern void *datadup(void *, int, char *);
extern void malloc_char(char **, int, char *);
extern void malloc_uchar(unsigned char **, int, char *);
extern void malloc_shrt(short **, int, char *);
extern void malloc_int(int **, int, char *);
extern void malloc_flt(float **, int, char *);
extern void calloc_char(char **, int, char *);
extern void calloc_uchar(unsigned char **, int, char *);
extern void calloc_shrt(short **, int, char *);
extern void calloc_int(int **, int, char *);
extern void calloc_flt(float **, int, char *);
extern void malloc_dbl_char_l1(char ***, int, char *);
extern void malloc_dbl_uchar_l1(unsigned char ***, int, char *);
extern void malloc_dbl_shrt_l1(short ***, int, char *);
extern void malloc_dbl_int_l1(int ***, int, char *);
extern void malloc_dbl_flt_l1(float ***, int, char *);
extern void realloc_char(char **, int, char *);
extern void realloc_uchar(unsigned char **, int, char *);
extern void realloc_shrt(short **, int, char *);
extern void realloc_int(int **, int, char *);
extern void realloc_flt(float **, int, char *);
extern void realloc_dbl_int_l1(int ***, int, char *);
extern void realloc_dbl_char_l1(char ***, int, char *);
extern void realloc_dbl_uchar_l1(unsigned char ***, int, char *);
extern void realloc_dbl_flt_l1(float ***, int, char *);
extern void free_dbl_char(char **, const int);
extern void free_dbl_uchar(unsigned char **, const int);
extern void free_dbl_flt(float **, const int);
extern void malloc_dbl_char(char ***, const int, const int, char *);
extern void malloc_dbl_uchar(unsigned char ***, const int, const int, char *);
extern void malloc_dbl_flt(float ***, const int, const int, char *);

#endif /* !_MEMALLOC_H */
