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

      FILE:    DECODE.C
      AUTHOR:  Michael D. Garris
      DATE:    02/21/2001
      UPDATE:  03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  10/02/2008 by Joseph C. Konczal - support 8-bit jp2
      UPDATE:  01/06/2009 by Kenneth Ko - add support for HPUX compile
               02/25/2015 (Kenneth Ko) - Updated everything related to
                                         OPENJPEG to OPENJP2

      Contains routines responsible for decoding image data contained
      in image records according to the ANSI/NIST 2007 standard.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_decode_ANSI_NIST_image()
                        biomeval_nbis_decode_binary_field_image()
                        biomeval_nbis_decode_tagged_field_image()

***********************************************************************/

#include <stdio.h>

#include <an2k.h>
#include <wsq.h>
#include <jpegb.h>
#include <jpegl.h>
#include <intrlv.h>
#include <defs.h>
#ifdef __NBIS_JASPER__
	#include <jpeg2k.h>
#endif
#ifdef __NBIS_OPENJP2__
	#include <jpeg2k.h>
#endif
#ifdef __NBIS_PNG__
	#include <png_dec.h>
#endif

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_decode_ANSI_NIST_image() - Takes an ANSI/NIST image record and
#cat:                decodes its image data (if necessary) and returns
#cat:                the reconstructed pixmap and image attributes.

   Input:
      ansi_nist   - ANSI/NIST file structure
      imgrecord_i - index of record to be decoded
      intrlvflag  - if image data is RGB, then this flagged designates
                    whether the returned pixmap should be interleaved or not
   Output:
      odata  - points to reconstructed pixmap
      ow     - pixel width of pixmap
      oh     - pixel height of pixmap
      od     - pixel depth of pixmap
      oppmm  - scan resolution of pixmap in pixels/mm
   Return Code:
      TRUE     - successful image reconstruction
      FALSE    - image record ignored
      Negative - system error
**************************************************************************/
int biomeval_nbis_decode_ANSI_NIST_image(unsigned char **odata,
                     int *ow, int *oh, int *od, double *oppmm,
                     const ANSI_NIST *ansi_nist, const int imgrecord_i,
                     const int intrlvflag)
{
   int ret;
   RECORD *imgrecord;

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_ANSI_NIST_image : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   /* If Type-3,4,5,6 ... */
   if(biomeval_nbis_binary_image_record(imgrecord->type) != 0){
      ret = biomeval_nbis_decode_binary_field_image(odata, ow, oh, od, oppmm, ansi_nist,
                                      imgrecord_i);
      /* if ERROR, IGNORE, or successfull ... return code. */
      return(ret);
   }
   /* If Type-10,13,14,15,16 ... */
   else if(tagged_image_record(imgrecord->type) != 0){
      ret = biomeval_nbis_decode_tagged_field_image(odata, ow, oh, od, oppmm, ansi_nist,
                                      imgrecord_i, intrlvflag);
      /* if ERROR, IGNORE, or successfull ... return code. */
      return(ret);
   }
   /* Otherwise, not an image record, so ERROR. */
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_decode_ANSI_NIST_image : "
	      "Record index [%d] [Type-%d] not an image record\n",
              imgrecord_i+1, imgrecord->type);
      return(-2);
   }

   /* Can't reach this point. */
}

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_decode_binary_field_image() - Takes an ANSI/NIST binary field
#cat:                image record and decodes its image data (if necessary)
#cat:                and returns the reconstructed pixmap and its attributes.

   Input:
      ansi_nist   - ANSI/NIST file structure
      imgrecord_i - index of record to be decoded
   Output:
      odata  - points to reconstructed pixmap
      ow     - pixel width of pixmap
      oh     - pixel height of pixmap
      od     - pixel depth of pixmap
      oppmm  - scan resolution of pixmap in pixels/mm
   Return Code:
      TRUE     - successful image reconstruction
      FALSE    - image record ignored
      Negative - system error
**************************************************************************/
int biomeval_nbis_decode_binary_field_image(unsigned char **odata,
                     int *ow, int *oh, int *od, double *oppmm,
                     const ANSI_NIST *ansi_nist, const int imgrecord_i)
{
   int ret;
   RECORD *imgrecord;
   FIELD *field;
   int field_i;
   char *img_comp;
   unsigned char *idata1, *idata2;
   int ilen, iw1, ih1, id1;
   double ppmm;
   int iw2, ih2, id2, ppi, lossyflag;

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   /* If for some reason someone's passes a Type-8 record ... */
   if(imgrecord->type == TYPE_8_ID){
      /* Post warning and ignore record Type-8. */
      fprintf(stderr, "WARNING : biomeval_nbis_decode_binary_field_image : "
	      "Type-8 record [%d] not supported.\n"
	      "Image record ignored.\n", imgrecord_i+1);
      return(FALSE);
   }

   /* 1. Determine Compression */
   /* Lookup binary compression algorithm (BIN_CA_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, BIN_CA_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "BIN_CA field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, BIN_CA_ID);
      return(-3);
   }
   img_comp = (char *)field->subfields[0]->items[0]->value;

   /* If Type-5,6 ... ignore if compressed. */
   if((imgrecord->type == TYPE_5_ID) ||
      (imgrecord->type == TYPE_6_ID)){
      /* If the image data is compressed ... */
      if(strcmp(img_comp, BIN_COMP_NONE) != 0){
         fprintf(stderr, "WARNING : biomeval_nbis_decode_binary_field_image : "
		   "binary image compression of "
		   "record index [%d] [Type-%d] is unsupported.\n"
		   "Image record ignored.\n",
                 imgrecord_i+1, imgrecord->type);
         /* Ignore image record ... */
         return(FALSE);
      }
   }
         
   /* 2. Determine image W */
   /* Lookup horizontal line length (HLL_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, HLL_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "HLL field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, HLL_ID);
      return(-4);
   }
   iw1 = atoi((char *)field->subfields[0]->items[0]->value);

   /* 3. Determine image H */
   /* Lookup vertical line length (VLL_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VLL_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "VLL field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, VLL_ID);
      return(-5);
   }
   ih1 = atoi((char *)field->subfields[0]->items[0]->value);

   /* 4. Determine pixel depth */
   switch(imgrecord->type){
      case TYPE_3_ID:
      case TYPE_4_ID:
           /* Image record is grayscale, so set pixel depth = 8. */
           id1 = 8;
           break;
      case TYPE_5_ID:
      case TYPE_6_ID:
           /* Image record is bi-level, so set pixel depth = 1. */
           id1 = 1;
           break;
      default:
           fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
		   "illegal binary image record type = %d\n", imgrecord->type);
           return(-6);
   }

   /* 5. Determine ppmm */
   /* Lookup image record's pixel/mm scan resolution. */
   if((ret = biomeval_nbis_lookup_binary_field_image_ppmm(&ppmm, ansi_nist, imgrecord_i)) !=0)
      return(ret);

   /* Set image pointer to last field value in record. */
   field = imgrecord->fields[imgrecord->num_fields-1];
   idata1 = field->subfields[0]->items[0]->value;
   ilen = field->subfields[0]->items[0]->num_bytes;

   /* If the image data is uncompressed ... */
   if(strcmp(img_comp, BIN_COMP_NONE) == 0){
      idata2 = (unsigned char *)malloc((size_t)ilen);
      if(idata2 == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
		 "malloc : idata2 (%d bytes)\n", ilen);
         return(-7);
      }
      memcpy(idata2, idata1, (size_t)ilen);

      *odata = idata2;
      *ow = iw1;
      *oh = ih1;
      *od = id1;
      *oppmm = ppmm;

      return(TRUE);
   }
   /* Otherwise, binary field image data is compressed ... */
   /* If WSQ compressed ... */
   else if(strcmp(img_comp, BIN_COMP_WSQ) == 0){
      if((ret = biomeval_nbis_wsq_decode_mem(&idata2, &iw2, &ih2, &id2, &ppi, &lossyflag,
			       idata1, ilen)) != 0)
         return(ret);
   }
   /* Otherwise, unsupported compression algorithm. */
   else{
      fprintf(stderr, "WARNING : biomeval_nbis_decode_binary_field_image : "
	      "unsupported compression algorithm %.10s%s in "
	      "image record index [%d] [Type-%d].\n"
	      "Image record ignored.\n",
              img_comp, strlen(img_comp) > 10 ? "..." : "",
	      imgrecord_i+1, imgrecord->type);
      /* Ignore image record ... */
      return(FALSE);
   }

   if(iw1 != iw2){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "[HLL field (from file) = %d] != "
	      "[image width (from decoder) = %d]\n",
	      iw1, iw2);
      free(idata2);
      return(-8);
   }
   if(ih1 != ih2){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "[VLL field (from file) = %d] != "
	      "[image height (from decoder) = %d]\n", ih1, ih2);
      free(idata2);
      return(-9);
   }
   if(id1 != id2){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_binary_field_image : "
	      "[pixel depth (from record Type-%d) = %d] != "
	      "[pixel depth (from decoder) = %d]\n",
	      id2, imgrecord->type, id1);
      free(idata2);
      return(-10);
   }

   *odata = idata2;
   *ow = iw1;
   *oh = ih1;
   *od = id1;
   *oppmm = ppmm;

   return(TRUE);
}

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_decode_tagged_field_image() - Takes an ANSI/NIST tagged field
#cat:                image record and decodes its image data (if necessary)
#cat:                and returns the reconstructed pixmap and its attributes.

   Input:
      ansi_nist   - ANSI/NIST file structure
      imgrecord_i - index of record to be decoded
      intrlvflag  - if image data is RGB, then this flagged designates
                    whether the returned pixmap should be interleaved or not
   Output:
      odata  - points to reconstructed pixmap
      ow     - pixel width of pixmap
      oh     - pixel height of pixmap
      od     - pixel depth of pixmap
      oppmm  - scan resolution of pixmap in pixels/mm
   Return Code:
      TRUE     - successful image reconstruction
      FALSE    - image record ignored
      Negative - system error
**************************************************************************/
int biomeval_nbis_decode_tagged_field_image(unsigned char **odata,
                     int *ow, int *oh, int *od, double *oppmm,
                     const ANSI_NIST *ansi_nist, const int imgrecord_i,
                     const int intrlvflag)
{
   int i, ret;
   RECORD *imgrecord;
   FIELD *field;
   int field_i;
   char *img_comp, *img_csp;
   unsigned char *idata1, *idata2;
   int ilen1, iw1, ih1, id1;
   double ppmm;
   int ilen2, iw2, ih2, id2, ppi, lossyflag;
   IMG_DAT *img_dat;
   int hor_sampfctr[MAX_CMPNTS], vrt_sampfctr[MAX_CMPNTS];
   int n_cmpnts;

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   /* If NOT Type-10,13,14,15,16 ... */
   if(tagged_image_record(ansi_nist->records[imgrecord_i]->type) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "record index [%d] [Type-%d] is not a tagged file image record\n",
              imgrecord_i+1, imgrecord->type);
      return(-3);
   }

   /* 1. Determine Compression */
   /* Lookup grayscale compression algorithm (TAG_CA_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, TAG_CA_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "TAG_CA field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, TAG_CA_ID);
      return(-4);
   }
   img_comp = (char *)field->subfields[0]->items[0]->value;

   /* 2. Determine image W */
   /* Lookup horizontal line length (HLL_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, HLL_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "HLL field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, HLL_ID);
      return(-5);
   }
   iw1 = atoi((char *)field->subfields[0]->items[0]->value);

   /* 3. Determine image H */
   /* Lookup vertical line length (VLL_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VLL_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "VLL field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, VLL_ID);
      return(-6);
   }
   ih1 = atoi((char *)field->subfields[0]->items[0]->value);

   /* 4. Determine pixel depth */
   /* Lookup bits per pixel or colorspace (BPX_ID==CSP_ID). */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, BPX_ID, imgrecord) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
	      "BPX field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, BPX_ID);
      return(-7);
   }
   /* if Type-10 & 17... */
   if((imgrecord->type == TYPE_10_ID) || (imgrecord->type == TYPE_17_ID)){
      if (imgrecord->type == TYPE_10_ID){
      /* Grayscale? CSP_ID = 12, "GRAY" */
         if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, CSP_ID, imgrecord) == 0){
            fprintf(stderr, "ERROR : dpyan2k_tagged_record : "
		    "CSP field in record index [%d] [Type-%d.%03d] not found\n",
                    imgrecord_i+1, imgrecord->type, CSP_ID);
            return(-2);  
         }               
      }     
      if (imgrecord->type == TYPE_17_ID){
      /* Grayscale? CSP_ID_Type_17 = 13, "GRAY" */
         if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, CSP_ID_Type_17, imgrecord)
	    == 0){
            fprintf(stderr, "ERROR : dpyan2k_tagged_record : "
		    "CSP field in record index [%d] [Type-%d.%03d] not found\n",
                    imgrecord_i+1, imgrecord->type, CSP_ID_Type_17);
            return(-2);
         } 
      }    

      img_csp = (char *)field->subfields[0]->items[0]->value;
      if(strcmp(img_csp, CSP_GRAY) == 0)
         /* Set grayscale image attributes. */
         id1 = 8;
      else if((strcmp(img_csp, CSP_RGB) == 0) || 
              (strcmp(img_csp, CSP_YCC) == 0) ||
              (strcmp(img_csp, CSP_SRGB) == 0) ||
              (strcmp(img_csp, CSP_SYCC) == 0))
         /* Set color image attributes. */
         id1 = 24;
      /* Otherwise, unsupported color space, so ignore image record. */
      else{
         fprintf(stderr, "WARNING : biomeval_nbis_decode_tagged_field_image : "
		 "colorspace \"%.10s%s\" not supported in "
		 "in record [%d] [Type-%d.%03d].\n"
		 "Image record ignored.\n", 
		 img_csp, strlen(img_csp) > 10 ? "..." : "",
		 imgrecord_i+1, imgrecord->type, CSP_ID);
         return(FALSE);
      }
   }
   /* Otherwise, Type-13,14,15,16 ... so current value == BPX_ID. */
   else{
      /* Set image pixel depth = BPX_ID. */
      id1 = atoi((char *)field->subfields[0]->items[0]->value);
   }

   /* 5. Determine ppmm */
   /* Lookup image record's pixel/mm scan resolution. */
   ret = biomeval_nbis_lookup_tagged_field_image_ppmm(&ppmm, imgrecord);
   /* If error or IGNORE ... */
   if(ret <= 0)
      return(ret);

   /* Set image pointer to last field value in record. */
   field = imgrecord->fields[imgrecord->num_fields-1];
   idata1 = field->subfields[0]->items[0]->value;
   ilen1 = field->subfields[0]->items[0]->num_bytes;

   /* If the image data is uncompressed ... */
   if(strcmp(img_comp, COMP_NONE) == 0){
      idata2 = (unsigned char *)malloc((size_t)ilen1);
      if(idata2 == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_decode_tagged_field_image : "
		 "malloc : idata2 (%d bytes)\n", ilen1);
         return(-8);
      }
      memcpy(idata2, idata1, (size_t)ilen1);

      /* For uncompressed RGB, the ANSI/NIST Standard dictates that */
      /* the pixels will be stored noninterleaved in 3 separate     */
      /* component planes. */
      /* So, if flag set to interleaved ... */
      if((id1 == 24) && (intrlvflag != 0)){
         /* There is no downsampling of RGB planes. */
         n_cmpnts = 3;
         for(i = 0; i < n_cmpnts; i++){
            hor_sampfctr[i] = 1;
            vrt_sampfctr[i] = 1;
         }
         if((ret = biomeval_nbis_not2intrlv_mem(&idata1, &ilen1, idata2, iw1, ih1, id1,
				  hor_sampfctr, vrt_sampfctr, n_cmpnts)) != 0){
            free(idata2);
            return(ret);
         }
         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }

      *odata = idata2;
      *ow = iw1;
      *oh = ih1;
      *od = id1;
      *oppmm = ppmm;

      return(TRUE);
   }
   /* If WSQ compressed ... */
   else if(strcmp(img_comp, COMP_WSQ) == 0){
      if((ret = biomeval_nbis_wsq_decode_mem(&idata2, &iw2, &ih2, &id2, &ppi, &lossyflag,
                              idata1, ilen1)) != 0)
         return(ret);
   }
   /* If JPEGB compressed ... */
   else if(strcmp(img_comp, COMP_JPEGB) == 0){
      if((ret = biomeval_nbis_jpegb_decode_mem(&idata2, &iw2, &ih2, &id2, &ppi, &lossyflag,
                                 idata1, ilen1)) != 0)
         return(ret);
      /* For 3 component color, JPEGB's decoder returns interleaved RGB. */
      /* So, if flag set to NOT interleaved ... */
      if((id2 == 24) && (intrlvflag == 0)){
         /* There is no downsampling of RGB planes. */
         n_cmpnts = 3;
         for(i = 0; i < n_cmpnts; i++){
            hor_sampfctr[i] = 1;
            vrt_sampfctr[i] = 1;
         }
         if((ret = biomeval_nbis_intrlv2not_mem(&idata1, &ilen1, idata2, iw2, ih2, id2,
                                 hor_sampfctr, vrt_sampfctr, n_cmpnts)) != 0){
            free(idata2);
            return(ret);
         }
         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }
   }
   /* If JPEGL compressed ... */
   else if(strcmp(img_comp, COMP_JPEGL) == 0){
      if((ret = biomeval_nbis_jpegl_decode_mem(&img_dat, &lossyflag, idata1, ilen1)) != 0)
         return(ret);
      if((ret = biomeval_nbis_get_IMG_DAT_image(&idata2, &ilen2, &iw2, &ih2, &id2, &ppi,
                                 img_dat)) != 0){
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }
      /* For 3 component color, JPEGL's decoder returns non-interleaved  */
      /* components planes.  So, if flag set to interleaved ...          */
      if((id2 == 24) && (intrlvflag != 0)){
         /* Note that this is set up to handle downsampled Cb and Cr planes */
         /* if the units encoded were in the YCbCr color space.  However    */
         /* the color space would need to be converted to RGB upon return   */
         /* of this routine. */
         /* The operational assumption is that the encoded color space */
         /* was RGB. */
         if((ret = biomeval_nbis_not2intrlv_mem(&idata1, &ilen1, idata2,
                                 img_dat->max_width, img_dat->max_height,
                                 img_dat->pix_depth, img_dat->hor_sampfctr,
                                 img_dat->vrt_sampfctr, img_dat->n_cmpnts)) !=0){
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
            free(idata2);
            return(ret);
         }

         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }
      biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
   }
#ifdef __NBIS_JASPER__
   /* If JPEG2K compress ... */
   else if((strcmp(img_comp, COMP_JPEG2K) == 0) || 
           (strcmp(img_comp, COMP_JPEG2KL) == 0)){
      /* JPEG2K decoder always return 3 components planes. */
      /* id1 = 24; -- What's in the record has already been copied
	              into id1, and it isn't necessarily 24 - jck */
 
      if((ret = jpeg2k_decode_mem(&img_dat, &lossyflag, idata1, ilen1)) != 0)
         return(ret);
  
      if((ret = biomeval_nbis_get_IMG_DAT_image(&idata2, &ilen2, &iw2, &ih2, &id2, &ppi,
                                 img_dat)) != 0){
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }
      /* For 3 component color, JPEG2K's decoder returns non-interleaved  */
      /* components planes.  So, if flag set to interleaved ...          */
      if((id2 == 24) && (intrlvflag != 0)){
         if((ret = biomeval_nbis_not2intrlv_mem(&idata1, &ilen1, idata2,
                                  img_dat->max_width, img_dat->max_height,
                                  img_dat->pix_depth, img_dat->hor_sampfctr,
                                  img_dat->vrt_sampfctr, img_dat->n_cmpnts)) != 0){
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
            free(idata2);
            return(ret);

         }
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }
   }
#endif
#ifdef __NBIS_OPENJP2__
   /* If JPEG2K compress ... */
   else if((strcmp(img_comp, COMP_JPEG2K) == 0) ||
           (strcmp(img_comp, COMP_JPEG2KL) == 0)){
      /* JPEG2K decoder always return 3 components planes. */
      /* id1 = 24; -- What's in the record has already been copied
                      into id1, and it isn't necessarily 24 - jck */

      if((ret = openjpeg2k_decode_mem(&img_dat, &lossyflag, idata1, ilen1)) != 0)
         return(ret);
 
      if((ret = biomeval_nbis_get_IMG_DAT_image(&idata2, &ilen2, &iw2, &ih2, &id2, &ppi,
                                 img_dat)) != 0){
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }
      /* For 3 component color, JPEG2K's decoder returns non-interleaved  */
      /* components planes.  So, if flag set to interleaved ...          */
      if((id2 == 24) && (intrlvflag != 0)){
         if((ret = biomeval_nbis_not2intrlv_mem(&idata1, &ilen1, idata2,
                                  img_dat->max_width, img_dat->max_height,
                                  img_dat->pix_depth, img_dat->hor_sampfctr,
                                  img_dat->vrt_sampfctr, img_dat->n_cmpnts)) != 0){
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
            free(idata2);
            return(ret);

         }
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }
   }
#endif
#ifdef __NBIS_PNG__
   /* If PNG compressed ... */
   else if(strcmp(img_comp, COMP_PNG) == 0){
      if((ret = png_decode_mem(&img_dat, &lossyflag, idata1, ilen1)) != 0)
         return(ret);
      if((ret = biomeval_nbis_get_IMG_DAT_image(&idata2, &ilen2, &iw2, &ih2, &id2, &ppi,
                                 img_dat)) != 0){
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }
      /* For 3 component color, PNG's decoder returns non-interleaved  */
      /* components planes.  So, if flag set to interleaved ...          */
      if((id2 == 24) && (intrlvflag != 0)){
         if((ret = biomeval_nbis_not2intrlv_mem(&idata1, &ilen1, idata2,
                                  img_dat->max_width, img_dat->max_height,
                                  img_dat->pix_depth, img_dat->hor_sampfctr,
                                  img_dat->vrt_sampfctr, img_dat->n_cmpnts)) !=  0){
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE); 
            free(idata2);
            return(ret);
         }
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         free(idata2);
         idata2 = idata1;
         ilen2 = ilen1;
      }
   }
#endif
   /* Otherwise, unsupported compression algorithm. */
   else{
      fprintf(stderr, "WARNING : biomeval_nbis_decode_tagged_field_image : "
	      "unsupported compression algorithm %.10s%s in "
	      "image record index [%d] [Type-%d].\n"
	      "Image record ignored.\n",
              img_comp, strlen(img_comp) > 10 ? "..." : "",
              imgrecord_i+1, imgrecord->type);
      /* Ignore image record ... */
      return(FALSE);
   }

   /* Code change by MDG on 07/02/04 to handle discrepancies in   */
   /* image records where the values in fields HLL and VLL differ */
   /* from what was actually decoded from the compressed image    */
   /* block.  In these cases, we post a warning and then set      */
   /* image attributes to be consistent with what was decoded.    */
   if(iw1 != iw2){
      fprintf(stderr, "WARNING : biomeval_nbis_decode_tagged_field_image : "
	      "[HLL field (from file) = %d] != "
	      "[image width (from decoder) = %d]\n"
	      "Will continue with operating assumption "
	      "that image width is %d\n", iw1, iw2, iw2);
      iw1 = iw2;
   }
   if(ih1 != ih2){
      fprintf(stderr, "WARNING : biomeval_nbis_decode_tagged_field_image : "
	      "[VLL field (from file) = %d] != "
	      "[image height (from decoder) = %d]\n"
	      "Will continue with operating assumption "
	      "that image height is %d\n", ih1, ih2, ih2);
      ih1 = ih2;
   }
   if(id1 != id2){
      fprintf(stderr, "WARNING : biomeval_nbis_decode_tagged_field_image : "
	      "[pixel depth (from field value) = %d] != "
	      "[pixel depth (from decoder) = %d]\n"
	      "Will continue with operating assumption "
	      "that image depth is %d\n", id1, id2, id2);
      id1 = id2;
   }

   *odata = idata2;
   *ow = iw1;
   *oh = ih1;
   *od = id1;
   *oppmm = ppmm;

   return(TRUE);
}
