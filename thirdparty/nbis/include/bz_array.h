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

#ifndef _BZ_ARRAY_H
#define _BZ_ARRAY_H

#define STATIC     static
/* #define BAD_BOUNDS 1 */

#define COLP_SIZE_1 20000
#define COLP_SIZE_2 5

#define COLS_SIZE_2 6
#define SCOLS_SIZE_1 20000
#define FCOLS_SIZE_1 20000

#define SCOLPT_SIZE 20000
#define FCOLPT_SIZE 20000

#define SC_SIZE 20000


#define RQ_SIZE 20000
#define TQ_SIZE 20000
#define ZZ_SIZE 20000



#define RX_SIZE 100
#define MM_SIZE 100
#define NN_SIZE 20



#define RK_SIZE 20000



#define RR_SIZE     100
#define AVN_SIZE      5
#define AVV_SIZE_1 2000
#define AVV_SIZE_2    5
#define CT_SIZE    2000
#define GCT_SIZE   2000
#define CTT_SIZE   2000


#ifdef BAD_BOUNDS
#define CTP_SIZE_1 2000
#define CTP_SIZE_2 1000
#else
#define CTP_SIZE_1 2000
#define CTP_SIZE_2 2500
#endif



/*
rp[x] == ctp[][x] :: sct[x][]
*/




#define RF_SIZE_1 100
#define RF_SIZE_2  10

#define CF_SIZE_1 100
#define CF_SIZE_2  10

#define Y_SIZE 20000






#define YL_SIZE_1    2
#define YL_SIZE_2 2000




#define YY_SIZE_1 1000
#define YY_SIZE_2    2
#define YY_SIZE_3 2000



#ifdef BAD_BOUNDS
#define SCT_SIZE_1 1000
#define SCT_SIZE_2 1000
#else
#define SCT_SIZE_1 2500
#define SCT_SIZE_2 1000
#endif

#define CP_SIZE 20000
#define RP_SIZE 20000

#endif /* !_BZ_ARRAY_H */
