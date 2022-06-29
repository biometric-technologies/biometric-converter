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

 
#ifndef _LITTLE_H
#define _LITTLE_H

/* Declarations of those functions in src/lib/utils/little.c with
non-int return values (including, void return value).  Stdio.h is
needed for FILE. */

#define INSTALL_DIR "/mnt/c/Projects/Rel_5.0.0"
#define INSTALL_DATA_DIR "/mnt/c/Projects/Rel_5.0.0/.build/nbis"
#define INSTALL_NBIS_DIR "/mnt/c/Projects"

extern int creat_ch(char *);
extern void dptr2ptr_uchar(unsigned char **, unsigned char **, const int,
                 const int);
extern void erode(unsigned char *, const int, const int);
extern int exists(char *);
extern FILE *fopen_ch(char *, char *);
extern FILE * fopen_noclobber(char *filename);
extern char *get_datadir(void);
extern int isverbose(void);
extern char *lastcomp(char *);
extern int linecount(char *);
extern int linreg(int *, int *, const int, float *, float *);
extern char *malloc_ch(const int);
extern int open_read_ch(char *);
extern void rcfill(unsigned char *, const int, const int);
extern void rsblobs(unsigned char *, const int, const int);
extern void setverbose(const int verbose);
extern void sleepity(const int);
extern void summary(const int, const int, int *, FILE *, const int, char *);
extern char *tilde_filename(char [], const int);
extern void usage_func(char *, char *);
extern void Usage_func(const int, char *, char *);
extern void write_ihdr_std(unsigned char *, const int, const int, const int,
                 char *);

#endif /* !_LITTLE_H */
