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

      FILE:    GETIMG.C
      AUTHOR:  Michael D. Garris
      DATE:    09/10/2004
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for searcing an ANSI/NIST structure
      locating, decoding, and returning image data.

************************************************************************

               ROUTINES:
                        biomeval_nbis_get_first_grayprint()

************************************************************************/
#include <stdio.h>
#include <an2k.h>

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_get_first_grayprint - Takes and ANSI/NIST structure and locates and
#cat:                         returns the first grayscale image record found.
   Input:
      ansi_nist - ANSI/NIST structure to be searched
   Output:
      odata    - points to image data within the record
      ow       - points to the width (in pixels) of the image
      oh       - points to the height (in pixels) of the image
      od       - points to the pixel depth (in bits) of the image
      oppmm    - points to the scan resolution (in pixels/mm) of the image
      oimg_idc - points to the image record's IDC
      oimg_imp - points to the image record's impression type (IMP)
      oimgrecord   - points to located image record
      oimgrecord_i - location of located image record in structure sequence
   Return Code:
      TRUE     - grayscale image record found
      FALSE    - grayscale image record NOT found
      Negative - system error
**************************************************************************/
int biomeval_nbis_get_first_grayprint(unsigned char **odata, int *ow, int *oh, int *od,
                        double *oppmm, int *oimg_idc, int *oimg_imp,
                        RECORD **oimgrecord, int *oimgrecord_i,
                        const ANSI_NIST *ansi_nist)
{
   int ret;
   unsigned char *idata;
   int iw, ih, id, img_idc, img_imp;
   double ppmm;
   RECORD *imgrecord;
   FIELD *idcfield, *impfield;
   int imgrecord_i, idcfield_i, impfield_i;

   /* Look up first grayscale fingerprint in ANSI/NIST file. */
   ret = biomeval_nbis_lookup_ANSI_NIST_grayprint(&imgrecord, &imgrecord_i, 1, ansi_nist);
   /* If error ... */
   if(ret < 0)
      return(ret);
   /* If grayscale fingerprint not found ... */
   if(!ret)
      return(FALSE);

   ret = biomeval_nbis_decode_ANSI_NIST_image(&idata, &iw, &ih, &id, &ppmm,
                                ansi_nist, imgrecord_i, 1 /*intrlvflag*/);
   /* If ERROR or IGNORE ... */
   if(ret <= 0)
      return(ret);

   /* Get IDC of selected image record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&idcfield, &idcfield_i, IDC_ID,
                              imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_get_first_grayprint : IDC field not found "
	      "in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, IDC_ID);
      return(-2);
   }
   /* Convert IDC value to numeric integer. */
   img_idc = atoi((char *)idcfield->subfields[0]->items[0]->value);

   /* Get IMP of selected image record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&impfield, &impfield_i, IMP_ID,
                              imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_get_first_grayprint : IMP field not found "
	      "in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, IMP_ID);
      return(-3);
   }
   /* Convert IMP value to numeric integer. */
   img_imp = atoi((char *)impfield->subfields[0]->items[0]->value);

   /* Set output pointers. */
   *odata = idata;
   *ow = iw;
   *oh = ih;
   *od = id;
   *oppmm = ppmm;
   *oimg_idc = img_idc;
   *oimg_imp = img_imp;

   /* Return successfully. */
   return(TRUE);
}  

