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

      FILE:    UTIL.C
      AUTHORS: Craig Watson
               Michael Garris
      DATE:    11/28/2000

      Contains general routines responsible for supporting JPEGL
      (lossless) image compression.

      ROUTINES:
#cat: biomeval_nbis_predict - Used to predict the pixel values in an image.
#cat:
#cat: biomeval_nbis_categorize - Determines the category for a given difference value.
#cat:

***********************************************************************/

#include <stdio.h>
#include <jpegl.h>

/**************************************************************/
/*This routine is used to predict the pixel values in an image*/
/**************************************************************/
int biomeval_nbis_predict(short *odata_pred, unsigned char *indata, const int width,
            const int pixel_num, const int cmpnt_depth, const int pred_type,
            const int Pt)
{
   short data_pred = 0;                  /*pixel prediction*/

   if(pixel_num == 0) {                 /*determine predictor for first pixel*/
      data_pred = (1 << (cmpnt_depth-Pt-1));
      *odata_pred = data_pred;
      return(0);
   }

   if(pixel_num > 0 && pixel_num < width) {
      data_pred = *(indata - 1);        /*determine predictor for first
                                          line in the non-interleaved
                                          component*/
      *odata_pred = data_pred;
      return(0);
   }
                                        /*determine predictor for the
                                          rest of the pixels in the 
                                          component*/
   if(pixel_num > (width - 1))  {
      if((pixel_num % width) == 0)      /*predictor if pixel is at the
                                          beginning of a line*/
	 data_pred = *(indata - width);
      else{ 
         switch(pred_type) {            /*various predictor types defined
                                          in the standard*/
            case PRED1:
               data_pred = *(indata - 1);
               break;
            case PRED2:
               data_pred = *(indata - width);
               break;
            case PRED3:
               data_pred = *(indata -(width + 1));
               break;
            case PRED4:
               data_pred = *(indata - 1) + 
                           *(indata - width) -
                           *(indata -(width + 1));
               break;
            case PRED5:
               data_pred = *(indata - 1) + 
                           ((*(indata - width) >> 1) - 
                           (*(indata -(width + 1)) >> 1));
               break;
            case PRED6:
               data_pred = *(indata - width) + 
                           ((*(indata - 1) >> 1) - 
                           (*(indata -(width + 1)) >> 1));
               break;
            case PRED7:
               data_pred = (*(indata -1) + *(indata - width)) / 2;
               break;
            default:
               fprintf(stderr, "ERORR : biomeval_nbis_predict : invalid prediction type ");
               fprintf(stderr, "%d not in range [%d..%d]\n",
                                pred_type, PRED1, PRED7);
               return(-2);
         }
      }
   }

   *odata_pred = data_pred;
   return(0);
}

/********************************************************************/
/*This function determines the category for a given difference value*/
/********************************************************************/
short biomeval_nbis_categorize(const short idiff)
{
   int bit;                     /*bit pointer to difference value*/
   short diff;

   diff = idiff;

   if(diff == 0)
      return (0);               /*difference category zero*/

   if (diff < 0)
      diff *= -1;
   
   for (bit = 0; bit < MAX_HUFFBITS; bit++) {
      if ((diff & CATMASK) != 0)
	 return ((short)(MAX_HUFFBITS - bit));
      diff <<= 1;
   }
   return(-1);
}
