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
      LIBRARY: AN2K - ANSI/NIST Reference Implementation

      FILE:    TYPE1314.C
      AUTHOR:  Michael D. Garris
      DATE:    10/23/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines responsible for converting fingerprint pixmaps
      into Type-13 and Type-14 tagged field image records according to
      the ANSI/NIST 2007 standard.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_fingerprint2tagged_field_image()
                        biomeval_nbis_image2type_13()
                        biomeval_nbis_image2type_14()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>
#include <defs.h>

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_fingerprint2tagged_field_image - Takes a fingerprint image pixmap
#cat:                (possibly compressed) and converts it into a Type-13
#cat:                or Type-14 tagged field image record determined by
#cat:                the impression type passed.

   Input:
      idata    - fingerprint image data (possibly compressed)
      ilen     - length in bytes of the image data buffer
      iw       - width (in pixels) of the fingerprint image
      ih       - height (in pixels) of the fingerprint image
      id       - pixel depth (in bits) of the fingerprint image
      ppmm     - the scan resolution (in pixels/mm) of the fingerprint image
      img_comp - compression type string
      img_idc  - the image record's IDC
      img_imp  - the fingerprin image's impression type (IMP)
      source_str - string identifying the originating source of the image
   Output:
      orecord  - points to resulting tagged field image record
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int biomeval_nbis_fingerprint2tagged_field_image(RECORD **orecord, unsigned char *idata,
                      const int ilen, const int iw, const int ih, const int id,
                      const double ppmm, char *img_comp, const int img_idc,
                      const int img_imp, char *source_str)
{
   int ret;
   RECORD *record;

   switch(img_imp){
   case 0:
   case 1:
   case 2:
   case 3:
      /* Convert image pixmap to a Type-14 tagged image record.  */
      if((ret = biomeval_nbis_image2type_14(&record, idata, ilen, iw, ih, id,
                             ppmm, img_comp, img_idc, img_imp, source_str))){
         return(ret);
      }
      break;
   case 4:
   case 5:
   case 6:
   case 7:
      /* Convert image pixmap to a Type-13 tagged image record.  */
      if((ret = biomeval_nbis_image2type_13(&record, idata, ilen, iw, ih, id,
                             ppmm, img_comp, img_idc, img_imp, source_str))){
         return(ret);
      }
      break;
   default:
      fprintf(stderr, "ERROR : fingerprint2tagged_image_record : "
	      "illegal Impression Type = %d\n", img_imp);
      return(-2);
   }

   /* Set output pointer. */
   *orecord = record;

   /* Return normally. */
   return(0);
}

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_image2type_13 - Takes a fingerprint image pixmap (possibly
#cat:               compressed) and converts it into a Type-13 tagged
#cat:               field image record.

   Input:
      idata    - fingerprint image data
      ilen     - length in bytes of the image data buffer
      iw       - width (in pixels) of the fingerprint image
      ih       - height (in pixels) of the fingerprint image
      id       - pixel depth (in bits) of the fingerprint image
      ppmm     - the scan resolution (in pixels/mm) of the fingerprint image
      img_comp - compression type string
      img_idc  - the image record's IDC
      img_imp  - the fingerprin image's impression type (IMP)
      source_str - string identifying the originating source of the image
   Output:
      orecord  - points to resulting Type-13 tagged field image record
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int biomeval_nbis_image2type_13(RECORD **orecord, unsigned char *idata, const int ilen,
                  const int iw, const int ih, const int id,
                  const double ppmm, char *img_comp, const int img_idc,
                  const int img_imp, char *source_str)
{
   int ret, ppcm, record_type;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   char *date_str;
   char uint_str[MAX_UINT_CHARS+1];
   unsigned char *new_val;

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* Only support pixel depth of 8. */
   if(id != 8){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_13 : image pixel depth = %d "
	      "not currently supported in Type-13 record\n", id);
      return(-2);
   }

   /* Permit only uncompressed pixmaps for Type-13 records. */
   if(strcmp(img_comp, COMP_NONE) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_13 : image compression \"%s\""
	      "not permitted in Type-13 record\n"
	      "Currently no compression is allowed\n", img_comp);
      return(-3);
   }

   /* Allocate new record. */
   record_type = TYPE_13_ID;
   if((ret = biomeval_nbis_new_ANSI_NIST_record(&record, record_type)))
      return(ret);

   /* 13.001: LEN Field */
   /* Create LEN field with value == "0" (place holder for now). */
   if((ret = biomeval_nbis_value2field(&field, record_type, LEN_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with LEN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.002: IDC Field */
   /* Create IDC field with value passed. */
   /* Remember that IDC is 2 bytes according to EFTS, so stay */
   /* consistent here. */
   sprintf(uint_str, "%02d", img_idc);
   if((ret = biomeval_nbis_value2field(&field, record_type, IDC_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with IDC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.003: IMP Field */
   /* Create IMP field with passed value. */
   sprintf(uint_str, "%02d", img_imp);
   if((ret = biomeval_nbis_value2field(&field, record_type, IMP_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with IMP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.004: SRC Field */
   /* Create SRC field with source origination string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, SRC_ID, source_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with SRC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.005: LCD Field */
   /* Create ANSI/NIST date string from current date. */
   if((ret = biomeval_nbis_get_ANSI_NIST_date(&date_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Create LCD field from date string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, CD_ID, date_str))){
      free(date_str);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Deallocate date string. */
   free(date_str);
   /* Append record with LCD field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.006: HLL Field */
   /* Create string from image pixel width. */
   sprintf(uint_str, "%d", iw);
   /* Create HLL field from width string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, HLL_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.007: VLL Field */
   /* Create string from image pixel width. */
   sprintf(uint_str, "%d", ih);
   /* Create VLL field from width string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, VLL_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with VLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.008: SLC Field */
   /* Create SLC field with value set to "2" (pp/cm). */
   if((ret = biomeval_nbis_value2field(&field, record_type, SLC_ID, PP_CM))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with SLC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.009: HPS Field */
   /* Compute pixel/cm from pixel/mm. */
   ppcm = sround(ppmm * 10.0);
   /* Create string from ppcm. */
   sprintf(uint_str, "%d", ppcm);
   /* Create HPS field with ppcm string value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, HPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.010: VPS Field */
   /* Create VPS field with HPS value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, VPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.011: CGA Field */
   /* Create CGA field from compression string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, TAG_CA_ID, img_comp))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with CGA field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.012: BPX Field */
   /* Create string from image pixel depth. */
   sprintf(uint_str, "%d", id);
   /* Create BPX field with pixel depth string value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, BPX_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with BPX field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.013: FGP Tagged Field */
   /* Create FGP tagged field with default value = "0" (Unknown Finger). */
   if((ret = biomeval_nbis_value2field(&field, record_type, FGP3_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with FGP tagged field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.999: DAT Field */
   /* Create new item to hold image data.  */
   /* Remember width must be byte-aligned. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   
   /* Reallocate the item's value. */
   new_val = (unsigned char *)realloc(item->value, ilen);
   if(new_val == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_13 : "
	      "realloc : DAT item value (increase %d bytes to %d)\n",
	      item->alloc_chars, ilen);
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-4);
   }
   item->value = new_val;
   item->alloc_chars = ilen;

   /* Copy image data into item's value. */
   memcpy(item->value, idata, item->alloc_chars);
   /* Set item's size. */
   item->num_bytes = item->alloc_chars;
   item->num_chars = item->alloc_chars;
   /* Create subfield to hold new item. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append subfield with new item. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(subfield, item))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Create new DAT field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&field, record_type, DAT2_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append DAT field with with new subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(field, subfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with DAT field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Update the LEN field with length of record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(record))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Set output pointer. */
   *orecord = record;

   /* Return normally. */
   return(0);
}

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_image2type_14 - Takes a fingerprint image pixmap (possibly
#cat:               compressed) and converts it into a Type-14 tagged
#cat:               field image record.

   Input:
      idata    - fingerprint image data
      ilen     - length in bytes of the image data buffer
      iw       - width (in pixels) of the fingerprint image
      ih       - height (in pixels) of the fingerprint image
      id       - pixel depth (in bits) of the fingerprint image
      ppmm     - the scan resolution (in pixels/mm) of the fingerprint image
      img_comp - compression type string
      img_idc  - the image record's IDC
      img_imp  - the fingerprin image's impression type (IMP)
      source_str - string identifying the originating source of the image
   Output:
      orecord  - points to resulting Type-14 tagged field image record
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int biomeval_nbis_image2type_14(RECORD **orecord, unsigned char *idata, const int ilen,
                  const int iw, const int ih, const int id,
                  const double ppmm,  char *img_comp, const int img_idc,
                  const int img_imp, char *source_str)
{
   int ret, ppcm, record_type;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   char *date_str;
   char uint_str[MAX_UINT_CHARS+1];
   unsigned char *new_val;

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* Only support pixel depth of 8. */
   if(id != 8){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_14 : image pixel depth = %d "
	      "not currently supported in Type-14 record\n", id);
      return(-2);
   }

   /* Permit only uncompressed or WSQ compressed pixmaps. */
   if((strcmp(img_comp, COMP_NONE) != 0) &&
      (strcmp(img_comp, COMP_WSQ) != 0)){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_14 : image compression \"%s\""
	      "not permitted in Type-14 record\n"
	      "Only WSQ compression is allowed\n", img_comp);
      return(-3);
   }

   /* Allocate new record. */
   record_type = TYPE_14_ID;
   if((ret = biomeval_nbis_new_ANSI_NIST_record(&record, record_type)))
      return(ret);

   /* 14.001: LEN Field */
   /* Create LEN field with value == "0" (place holder for now). */
   if((ret = biomeval_nbis_value2field(&field, record_type, LEN_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with LEN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.002: IDC Field */
   /* Create IDC field with value passed. */
   /* Remember that IDC is 2 bytes according to EFTS, so stay */
   /* consistent here. */
   sprintf(uint_str, "%02d", img_idc);
   if((ret = biomeval_nbis_value2field(&field, record_type, IDC_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with IDC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.003: IMP Field */
   /* Create IMP field with passed value. */
   sprintf(uint_str, "%02d", img_imp);
   if((ret = biomeval_nbis_value2field(&field, record_type, IMP_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with IMP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.004: SRC Field */
   /* Create SRC field with source origination string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, SRC_ID, source_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with SRC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.005: LCD Field */
   /* Create ANSI/NIST date string from current date. */
   if((ret = biomeval_nbis_get_ANSI_NIST_date(&date_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Create LCD field from date string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, CD_ID, date_str))){
      free(date_str);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Deallocate date string. */
   free(date_str);
   /* Append record with LCD field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.006: HLL Field */
   /* Create string from image pixel width. */
   sprintf(uint_str, "%d", iw);
   /* Create HLL field from width string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, HLL_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.007: VLL Field */
   /* Create string from image pixel width. */
   sprintf(uint_str, "%d", ih);
   /* Create VLL field from width string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, VLL_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with VLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.008: SLC Field */
   /* Create SLC field with value set to "2" (pp/cm). */
   if((ret = biomeval_nbis_value2field(&field, record_type, SLC_ID, PP_CM))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with SLC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.009: HPS Field */
   /* Compute pixel/cm from pixel/mm. */
   ppcm = sround(ppmm * 10.0);
   /* Create string from ppcm. */
   sprintf(uint_str, "%d", ppcm);
   /* Create HPS field with ppcm string value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, HPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.010: VPS Field */
   /* Create VPS field with HPS value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, VPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with HPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.011: CGA Field */
   /* Create CGA field from compression string. */
   if((ret = biomeval_nbis_value2field(&field, record_type, TAG_CA_ID, img_comp))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with CGA field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.012: BPX Field */
   /* Create string from image pixel depth. */
   sprintf(uint_str, "%d", id);
   /* Create BPX field with pixel depth string value. */
   if((ret = biomeval_nbis_value2field(&field, record_type, BPX_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with BPX field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 14.013: FGP Tagged Field */
   /* Create FGP tagged field with default value = "0" (Unknown Finger). */
   if((ret = biomeval_nbis_value2field(&field, record_type, FGP3_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with FGP tagged field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* 13.999: DAT Field */
   /* Create new item to hold image data.  */
   /* Remember width must be byte-aligned. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Create new item to hold image data. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Reallocate the item's value. */
   new_val = (unsigned char *)realloc(item->value, ilen);
   if(new_val == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_image2type_14 : "
	      "realloc : DAT item value (increase %d bytes to %d)\n",
	      item->alloc_chars, ilen);
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-4);
   }
   item->value = new_val;
   item->alloc_chars = ilen;

   /* Copy image data into item's value. */
   memcpy(item->value, idata, item->alloc_chars);
   /* Set item's size. */
   item->num_bytes = item->alloc_chars;
   item->num_chars = item->alloc_chars;
   /* Create subfield to hold new item. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append subfield with new item. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(subfield, item))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Create new DAT field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&field, record_type, DAT2_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append DAT field with with new subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(field, subfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }
   /* Append record with DAT field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(record, field))){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Update the LEN field with length of record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(record))){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Set output pointer. */
   *orecord = record;

   /* Return normally. */
   return(0);
}
