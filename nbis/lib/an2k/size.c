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
      LIBRARY: AN2K - ANSI/NIST 2007 Reference Implementation

      FILE:    SIZE.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines that return the number of binary bytes used
      to represent specific fields in binary records of an ANSI/NIST file.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_binary_image_field_bytes()
                        biomeval_nbis_binary_signature_field_bytes()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_binary_image_field_bytes - Takes an integer field ID associated
#cat:              with a specific field in a binary image record
#cat:              (Type-3,4,5,6) and returns the number of bytes used
#cat:              to represent that field.

   Input:
      field_int  - the integer ID of the field in question
   Return Code:
      Positive   - byte size of binary field
      Negative   - unrecognized field ID
************************************************************************/
int biomeval_nbis_binary_image_field_bytes(const int field_int)
{
   switch(field_int){
      case LEN_ID:
           return(BINARY_LEN_BYTES);
      case IDC_ID:
           return(BINARY_IDC_BYTES);
      case IMP_ID:
           return(BINARY_IMP_BYTES);
      case FGP_ID:
           return(BINARY_FGP_BYTES);
      case ISR_ID:
           return(BINARY_ISR_BYTES);
      case HLL_ID:
           return(BINARY_HLL_BYTES);
      case VLL_ID:
           return(BINARY_VLL_BYTES);
      case BIN_CA_ID:
           return(BINARY_CA_BYTES);
      default:
           fprintf(stderr, "ERROR : biomeval_nbis_binary_image_field_bytes : "
		   "illegal binary image field ID [%d]\n",
                   field_int);
           return(-1);
   }
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_binary_signature_field_bytes - Takes an integer field ID associated
#cat:              with a specific field in a signature record (Type-8)
#cat:              and returns the number of bytes used to represent
#cat:              that field.

   Input:
      field_int  - the integer ID of the field in question
   Return Code:
      Positive   - byte size of binary field
      Negative   - unrecognized field ID
************************************************************************/
int biomeval_nbis_binary_signature_field_bytes(const int field_int)
{
   switch(field_int){
      case LEN_ID:
           return(BINARY_LEN_BYTES);
      case IDC_ID:
           return(BINARY_IDC_BYTES);
      case SIG_ID:
           return(BINARY_SIG_BYTES);
      case SRT_ID:
           return(BINARY_SRT_BYTES);
      case ISR_ID:
           return(BINARY_ISR_BYTES);
      case HLL_ID:
           return(BINARY_HLL_BYTES);
      case VLL_ID:
           return(BINARY_VLL_BYTES);
      default:
           fprintf(stderr, "ERROR : biomeval_nbis_binary_signature_field_bytes : "
		   "illegal binary signature field ID [%d]\n",
                   field_int);
           return(-1);
   }
}
