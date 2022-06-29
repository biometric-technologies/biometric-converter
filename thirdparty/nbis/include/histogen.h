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

/******************************************************************************
      PACKAGE: ANSI/NIST 2007 Standard Reference Implementation

      FILE:    HISTOGEN.H

      AUTHORS: Bruce Bandini
      DATE:    05/18/2010

*******************************************************************************/
#ifndef _HISTOGEN_H
#define _HISTOGEN_H

#define str_eq(s1,s2)  (!strcmp ((s1),(s2)))

/* If filemask = *, then getopt adds files in current dir to non-options list.
   For a correctly formed command line, argc is always <= 4. */
#define MAX_ARGC    4
#define NUM_OPTIONS 4

#define CMD_LEN 512
#define FILESYS_PATH_LEN 256
#define READ_LINE_BUFFER 256
#define MAX_FIELD_NUM_CHARS 12
#define MAX_FIELD_NUMS 30
#define ALLOC_BLOCK_SIZE 10
#define HISTOGEN_LOG_FNAME "histogen.log"

typedef struct histo HISTO;
struct histo {
  char  field_num[12];
  int   count;
  HISTO *next;
};

HISTO *histo_head;

enum {
  INCLUDE_INVALID_FILES=5,
  INCLUDE_FIELD_SEPARATORS,
  INCLUDE_NEWLINE_CHARS,
  INCLUDE_SPACE_CHARS
};

enum {
  FALSE=0,
  TRUE=1
};

/******************************************************************************/
/* histogen.c */
extern int process_file(const char *);
extern int initialize_linked_list();
extern int output_linked_list(FILE *);

#endif /* !_HISTOGEN_H */

