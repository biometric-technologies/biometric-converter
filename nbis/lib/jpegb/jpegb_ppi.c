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
      LIBRARY: JPEGB - Baseline (Lossy) JPEG Utilities

      FILE:    PPI.C
      AUTHORS: Michael Garris
               Craig Watson
      DATE:    01/09/2001

      Contains routines responsible for determining a compressed image's
      scan resolution in units of pixels per inch.

      ROUTINES:
#cat: biomeval_nbis_get_ppi_jpegb - If possible, computes and returns the scan resolution
#cat:                 in pixels per inch of a JPEGB datastream given a
#cat:                 JPEGB decompess structure.

***********************************************************************/

#include <stdio.h>
#include <jpegb.h>

#define CM_PER_INCH   2.54

/************************************************************************/
int biomeval_nbis_get_ppi_jpegb(int *oppi, j_decompress_ptr cinfo)
{
   int ppi;

   /* Get and set scan density in pixels per inch. */
   switch(cinfo->density_unit){
      /* pixels per inch */
      case 1:
         /* take the horizontal pixel density, even if the vertical is */
         /* not the same */
         ppi = cinfo->X_density;
         break;
      /* pixels per cm */
      case 2:
         /* compute ppi from horizontal density even if not */
         /* equal to vertical */
         ppi = (int)((cinfo->X_density * CM_PER_INCH) + 0.5);
         break;
      /* unknown density */
      case 0:
         /* set ppi to -1 == UNKNOWN */
         ppi = -1;
         break;
      /* ERROR */
      default:
         fprintf(stderr, "ERROR : biomeval_nbis_get_ppi_jpegb : ");
         fprintf(stderr, "illegal density unit = %d\n", cinfo->density_unit);
         return(-2);
   }

   *oppi = ppi;

   return(0);
}
