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

      FILE:    IMGDAT.C
      AUTHORS: Craig Watson
               Michael Garris
      DATE:    11/28/2000
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for manipulating an IMG_DAT
      (image data) structure that hold an image's pixmap and
      related attributes.

      ROUTINES:
#cat: biomeval_nbis_get_IMG_DAT_image - Extracts the image pixmap and returns its
#cat:                     attributes stored in an IMG_DAT structure.
#cat: biomeval_nbis_setup_IMG_DAT_nonintrlv_encode - Initialize an IMG_DAT structure
#cat:                     for JPEGL compressing a non-interleaved pixmap.
#cat: biomeval_nbis_setup_IMG_DAT_decode - Initialize an IMG_DAT structure for
#cat:                     compressing a general pixmap.
#cat: biomeval_nbis_update_IMG_DAT_decode - Augments an IMG_DAT structure used for
#cat:                     decompression with attributes derived from a
#cat:                     JPEGL SCN Header, including the allocation for
#cat:                     the reconstructred pixmap.
#cat: biomeval_nbis_free_IMG_DAT - Deallocates an IMG_DAT structure.

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <jpegl.h>

/**************************************/
/* Extract image from image structure */
/**************************************/
int biomeval_nbis_get_IMG_DAT_image(unsigned char **odata, int *olen,
                      int *width, int *height, int *depth, int *ppi,
                      IMG_DAT *img_dat)
{
   int i, nsizes[MAX_CMPNTS], nlen;
   unsigned char *ndata, *nptr;

   nlen = 0;
   for(i = 0; i < img_dat->n_cmpnts; i++){
      nsizes[i] = img_dat->samp_width[i] * img_dat->samp_height[i];
      nlen += nsizes[i];
   }
 
   ndata = (unsigned char *)malloc(nlen * sizeof(unsigned char));
   if(ndata == (unsigned char *)NULL){
	   fprintf(stderr, "ERROR : biomeval_nbis_get_IMG_DAT_image : malloc : ndata\n");
      return(-2);
   }

   nptr = ndata;
   for(i = 0; i < img_dat->n_cmpnts; i++){
      memcpy(nptr, img_dat->image[i], nsizes[i] * sizeof(unsigned char));
      nptr += nsizes[i];
   }

   *odata = ndata;
   *olen = nlen;
   *width = img_dat->max_width;
   *height = img_dat->max_height;
   *depth = img_dat->pix_depth;
   *ppi = img_dat->ppi;

   return(0);
}

/*******************************************/
/* Setup image structure to compress image */
/*******************************************/
int biomeval_nbis_setup_IMG_DAT_nonintrlv_encode(IMG_DAT **oimg_dat, unsigned char *idata,
          const int w, const int h, const int d, const int ppi,
          int *hor_sampfctr, int *vrt_sampfctr, const int n_cmpnts,
          const unsigned char pt_val, const unsigned char pred_val)
{
   int i, j, max_hor, max_vrt, plane_size;
   IMG_DAT *img_dat;
   unsigned char *iptr;

   if((d != 8) && (d != 24)){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_nonintrlv_encode : ");
      fprintf(stderr, "image pixel depth %d != 8 or 24\n", d);
      return(-2);
   }

   if(n_cmpnts > MAX_CMPNTS){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_nonintrlv_encode : ");
      fprintf(stderr, "number of components = %d > %d\n",
              n_cmpnts, MAX_CMPNTS);
      return(-3);
   }

   if(((d == 8) && (n_cmpnts != 1)) ||
      ((d == 24) && (n_cmpnts != 3))){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_nonintrlv_encode : ");
      fprintf(stderr, "depth = %d mismatched with n_cmpnts = %d\n",
              d, n_cmpnts);
      return(-4);
   }

   if((img_dat = (IMG_DAT *)calloc(1, sizeof(IMG_DAT))) == (IMG_DAT *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_nonintrlv_encode : ");
      fprintf(stderr, "calloc : img_dat\n");
      return(-5);
   }

   img_dat->max_width = w;
   img_dat->max_height = h;
   img_dat->pix_depth = d;
   img_dat->ppi = ppi;
   img_dat->intrlv = NO_INTRLV;
   img_dat->n_cmpnts = n_cmpnts;
   img_dat->cmpnt_depth = 8;  /* data units must be unsigned char */

   /* Determine max tile dimensions across all components ... */
   max_hor = -1;
   max_vrt = -1;
   for(i = 0; i < n_cmpnts; i++){
      if(hor_sampfctr[i] > max_hor)
         max_hor = hor_sampfctr[i];
      if(vrt_sampfctr[i] > max_vrt)
         max_vrt = vrt_sampfctr[i];
   }

   iptr = idata;
   for(i = 0; i < n_cmpnts; i++){
      img_dat->hor_sampfctr[i] = hor_sampfctr[i];
      img_dat->vrt_sampfctr[i] = vrt_sampfctr[i];
      /* Compute the pixel width & height of the component's plane.  */
      img_dat->samp_width[i] = (int)ceil(img_dat->max_width *
                                         (hor_sampfctr[i] / (double)max_hor));
      img_dat->samp_height[i] = (int)ceil(img_dat->max_height *
                                          (vrt_sampfctr[i] / (double)max_vrt));
      img_dat->point_trans[i] = pt_val;
      img_dat->predict[i] = pred_val;

      plane_size = img_dat->samp_width[i] * img_dat->samp_height[i];
      img_dat->image[i] =
               (unsigned char *)malloc(plane_size * sizeof(unsigned char));
      if(img_dat->image[i] == (unsigned char *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_nonintrlv_encode : ");
         fprintf(stderr, "malloc : img_dat->image[%d]\n", i);
         for(j = 0; j < i; j++)
            free(img_dat->image[j]);
         free(img_dat);
         return(-6);
      }
      memcpy(img_dat->image[i], iptr, plane_size);

      /* Bump to start of next component plane. */
      iptr += plane_size;
   }

   *oimg_dat = img_dat;
   return(0);
}

/*********************************************/
int biomeval_nbis_setup_IMG_DAT_decode(IMG_DAT **oimg_dat, const int ppi,
                         FRM_HEADER_JPEGL *frm_header)
{
   int i, max_hor, max_vrt;
   IMG_DAT *img_dat;

   img_dat = (IMG_DAT *)calloc(1, sizeof(IMG_DAT));
   if(img_dat == (IMG_DAT *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : calloc : img_dat\n");
      return(-2);
   }

   img_dat->max_width = frm_header->x;
   img_dat->max_height = frm_header->y;
   img_dat->pix_depth = frm_header->Nf * 8;
   img_dat->ppi = ppi;
   img_dat->intrlv = -1;
   img_dat->n_cmpnts = frm_header->Nf;
   img_dat->cmpnt_depth = frm_header->prec;

   max_hor = -1;   
   max_vrt = -1;   
   for(i = 0; i < img_dat->n_cmpnts; i++){
      img_dat->hor_sampfctr[i] = frm_header->HV[i]>>4;
      img_dat->vrt_sampfctr[i] = frm_header->HV[i] & 0x0F;
      if(max_hor < img_dat->hor_sampfctr[i])
         max_hor = img_dat->hor_sampfctr[i];
      if(max_vrt < img_dat->vrt_sampfctr[i])
         max_vrt = img_dat->vrt_sampfctr[i];
   }

   for(i = 0; i < img_dat->n_cmpnts; i++){
      img_dat->samp_width[i] = (int)ceil(img_dat->max_width *
                               (img_dat->hor_sampfctr[i]/(double)max_hor));
      img_dat->samp_height[i] = (int)ceil(img_dat->max_height *
                               (img_dat->vrt_sampfctr[i]/(double)max_vrt));
   }

   *oimg_dat = img_dat;
   return(0);
}

/*********************************************/
int biomeval_nbis_update_IMG_DAT_decode(IMG_DAT *img_dat, SCN_HEADER *scn_header,
                          HUF_TABLE **huf_table)
{
   int i, cmpnt_i;

   if(scn_header->Ns > 1)
      img_dat->intrlv = 1;
   else
      img_dat->intrlv = 0;

   /* NOTE: scn_header->Ns == 1 if encoded data is NOT interleaved. */
   for(i = 0; i < scn_header->Ns; i++) {
      cmpnt_i = scn_header->Cs[i];
      if((huf_table[cmpnt_i] == (HUF_TABLE *)NULL) ||
         (huf_table[cmpnt_i]->def != 1)){
         fprintf(stderr, "ERROR : biomeval_nbis_update_IMG_DAT_decode : ");
         fprintf(stderr, "huffman table %d not defined\n", cmpnt_i);
         return(-2);
      }
      img_dat->point_trans[cmpnt_i] = scn_header->Ahl;
      img_dat->predict[cmpnt_i] = scn_header->Ss;
      img_dat->image[cmpnt_i] =
               (unsigned char *)malloc(img_dat->samp_width[cmpnt_i] *
                                       img_dat->samp_height[cmpnt_i]);
      if(img_dat->image[cmpnt_i] == (unsigned char *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_IMG_DAT_decode : ");
         fprintf(stderr, "malloc : img_dat->image[%d]\n", cmpnt_i);
         return(-3);
      }
   }

   return(0);
}

/*********************************************/
int biomeval_nbis_setup_IMG_DAT_decode_old(IMG_DAT **oimg_dat, const int ppi,
                         FRM_HEADER_JPEGL *frm_header,
                         SCN_HEADER *scn_header, HUF_TABLE **huf_table)
{
   int i, cmpnt_i;
   IMG_DAT *img_dat;

   img_dat = (IMG_DAT *)calloc(1, sizeof(IMG_DAT));
   if(img_dat == (IMG_DAT *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : calloc : img_dat\n");
      return(-2);
   }

   img_dat->n_cmpnts = frm_header->Nf;
   if(scn_header->Ns > 1)
      img_dat->intrlv = 1;
   else
      img_dat->intrlv = 0;

   if(!(img_dat->intrlv)) {
      cmpnt_i = scn_header->Cs[0];
      if((huf_table[cmpnt_i] == (HUF_TABLE *)NULL) ||
         (huf_table[cmpnt_i]->def != 1)){
         fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : ");
         fprintf(stderr, "huffman table %d not defined %d\n", cmpnt_i,
                    scn_header->Ns);
         biomeval_nbis_free_IMG_DAT(img_dat, NO_FREE_IMAGE);
	 return(-3);
      }
      img_dat->point_trans[cmpnt_i] = scn_header->Ahl;
      img_dat->predict[cmpnt_i] = scn_header->Ss;
      img_dat->max_width = frm_header->x;
      img_dat->max_height = frm_header->y;
      img_dat->pix_depth = frm_header->prec;
      img_dat->ppi = ppi;

      img_dat->image[cmpnt_i] = (unsigned char *)malloc(img_dat->max_width *
                                          img_dat->max_height);
      if(img_dat->image[cmpnt_i] == (unsigned char *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : ");
         fprintf(stderr, "malloc : img_dat->image[%d]\n", cmpnt_i);
         biomeval_nbis_free_IMG_DAT(img_dat, NO_FREE_IMAGE);
	 return(-4);
      }
   }
   else {
      img_dat->max_width = frm_header->x;
      img_dat->max_height = frm_header->y;
      img_dat->pix_depth = frm_header->prec;
      img_dat->ppi = ppi;

      for(i = 0; i < scn_header->Ns; i++) {
	 cmpnt_i = scn_header->Cs[i];
         if((huf_table[cmpnt_i] == (HUF_TABLE *)NULL) ||
            (huf_table[cmpnt_i]->def != 1)){
            fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : ");
            fprintf(stderr, "huffman table %d not defined\n", cmpnt_i);
            biomeval_nbis_free_IMG_DAT(img_dat, NO_FREE_IMAGE);
	    return(-5);
         }
	 img_dat->point_trans[cmpnt_i] = scn_header->Ahl;
	 img_dat->predict[cmpnt_i] = scn_header->Ss;

         img_dat->image[cmpnt_i] = (unsigned char *)malloc(img_dat->max_width *
                                             img_dat->max_height);
         if(img_dat->image[cmpnt_i] == (unsigned char *)NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_setup_IMG_DAT_decode : ");
            fprintf(stderr, "malloc : img_dat->image[%d]\n", cmpnt_i);
            biomeval_nbis_free_IMG_DAT(img_dat, NO_FREE_IMAGE);
	    return(-6);
         }
      }
   }

   *oimg_dat = img_dat;
   return(0);
}


/******************************/
/* Deallocate image structure */
/******************************/
void biomeval_nbis_free_IMG_DAT(IMG_DAT *img_dat, const int img_flag)
{
   int i;

   for(i = 0; i < img_dat->n_cmpnts; i++){
      if(img_dat->diff[i] != (short *)NULL)
         free(img_dat->diff[i]);
   }

   if(img_flag){
      for(i = 0; i < img_dat->n_cmpnts; i++){
	 if(img_dat->image[i] != (unsigned char *)NULL)
            free(img_dat->image[i]);
      }
   }

   free(img_dat);
}
