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

      FILE:    IS_AN2K.C
      AUTHOR:  Michael D. Garris
      DATE:    09/10/2004
      UPDATE:  01/04/2008 Joseph C. Konczal -- added const to ifile param
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2008 jck - report more details when things go wrong

      Contains routines responsible for determining if a file or
      byte stream is AN2K formatted.

************************************************************************

               ROUTINES:
                        biomeval_nbis_is_ANSI_NIST_file()
                        biomeval_nbis_is_ANSI_NIST()

************************************************************************/

#include <stdio.h>
#include <an2k.h>

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_is_ANSI_NIST_file - Takes an input file, opens it, and scans the
#cat:                       start of the file for field id ("1.001:").
#cat:                       If field id found, then assumed AN2K format.
   Input:
      ifile    - input file
   Return Code:
      TRUE     - is ANSI/NIST format
      FALSE    - not ANSI/NIST format
      Negative - system error
**************************************************************************/
int biomeval_nbis_is_ANSI_NIST_file(const char *const ifile)
{
   FILE *fp;
   int ret, n;
   unsigned char buffer[(2 * FIELD_NUM_LEN) + 3], *cbufptr, *ebufptr;
   char *field_id;
   unsigned int record_type, field_int;

   if((fp = fopen(ifile, "rb")) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_is_ANSI_NIST_file : fopen '%s': %s\n",
	      ifile, strerror(errno));
      return(-2);
   }

   n = fread(buffer, sizeof(unsigned char), (2 * FIELD_NUM_LEN) + 2, fp);
   if(ferror(fp)){
      fprintf(stderr, "ERROR : biomeval_nbis_is_ANSI_NIST_file : fread '%s': %s\n",
	      ifile, SHORT_READ_ERR_MSG(fp));
      fclose(fp);
      return(-4);
   }

   fclose(fp);
   cbufptr = buffer;
   ebufptr = buffer+n;

   ret = biomeval_nbis_parse_ANSI_NIST_field_ID(&cbufptr, ebufptr, &field_id,
                                     &record_type, &field_int);
   /* if system error */
   if(ret < 0){
      return(ret);
   }
   /* if field ID successfully parsed ...*/
   if(ret == TRUE){
      if((record_type == TYPE_1_ID) && (field_int == LEN_ID)){
         free(field_id);
         return(TRUE);
      }
   }

   /* Otherwise, not an ANSI/NIST file */
   return(FALSE);

}

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_is_ANSI_NIST - Takes a byte stream, and scans the start of
#cat:                  stream for field id ("1.001:").
#cat:                  If field id found, then assumed AN2K format.
   Input:
      idata    - input byte stream
      ilen     - length of stream
   Return Code:
      TRUE     - is ANSI/NIST format
      FALSE    - not ANSI/NIST format
      Negative - system error
**************************************************************************/
int biomeval_nbis_is_ANSI_NIST(unsigned char *idata, const int ilen)
{
   int ret;
   unsigned char *cbufptr, *ebufptr;
   char *field_id;
   unsigned int record_type, field_int;

   cbufptr = idata;
   ebufptr = idata + ilen;

   ret = biomeval_nbis_parse_ANSI_NIST_field_ID(&cbufptr, ebufptr, &field_id,
                                     &record_type, &field_int);
   /* if system error */
   if(ret < 0)
      return(ret);
   /* if field ID successfully parsed ...*/
   if(ret == TRUE){
      if((record_type == TYPE_1_ID) && (field_int == LEN_ID)){
         free(field_id);
         return(TRUE);
      }
   }

   return(FALSE);
}
