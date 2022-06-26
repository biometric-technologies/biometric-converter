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
      LIBRARY: JPEGL - Lossless JPEG Image Compression

      FILE:    PPI.C
      AUTHORS: Craig Watson
               Michael Garris
      DATE:    01/17/2001

      Contains routines responsible for determining the scan resolution
      in units of pixels per inch from a JPEGL compressed datastream.

      ROUTINES:
#cat: biomeval_nbis_get_ppi_jpegl - Given a JFIF Header from a JPEGL compressed
#cat:                 datastream, extracts/derives the pixel scan
#cat:                 resolution in units of pixel per inch.

***********************************************************************/

#include <stdio.h>
#include <jpegl.h>

#define CM_PER_INCH   2.54

/************************************************************************/
int biomeval_nbis_get_ppi_jpegl(int *oppi, JFIF_HEADER *jfif_header)
{
   int ppi;

   /* Get and set scan density in pixels per inch. */
   switch(jfif_header->units){
      /* pixels per inch */
      case 1:
         /* take the horizontal pixel density, even if the vertical is */
         /* not the same */
         ppi = jfif_header->dx;
         break;
      /* pixels per cm */
      case 2:
         /* compute ppi from horizontal density even if not */
         /* equal to vertical */
         ppi = (int)((jfif_header->dx * CM_PER_INCH) + 0.5);
         break;
      /* unknown density */
      case 0:
         /* set ppi to -1 == UNKNOWN */
         ppi = -1;
         break;
      /* ERROR */
      default:
         fprintf(stderr, "ERROR : biomeval_nbis_get_ppi_jpegl : ");
         fprintf(stderr, "illegal density unit = %d\n", jfif_header->units);
         return(-2);
   }

   *oppi = ppi;

   return(0);
}
