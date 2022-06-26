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

      FILE:    TO_NIST.C
      AUTHOR:  Michael D. Garris
      DATE:    10/20/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for converting an EFTS Version 7
      formatted FBI/IAFIS file structure into an ANSI/NIST 2007 file
      structure.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_iafis2nist_fingerprints()
                        biomeval_nbis_iafis2nist_fingerprint()
                        biomeval_nbis_iafis2nist_type_9s()
                        biomeval_nbis_iafis2nist_needed()
                        biomeval_nbis_iafis2nist_type_9()
                        biomeval_nbis_iafis2nist_method()
                        biomeval_nbis_iafis2nist_minutia_type()
                        biomeval_nbis_iafis2nist_pattern_class()
                        biomeval_nbis_iafis2nist_ridgecount()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>
#include <defs.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_fingerprints - Searches an ANSI/NIST file structure for
#cat:              binary field fingerprint records and converts them
#cat:              to tagged field image records.  For example, a 500 dpi
#cat:              8-bit grayscale tenprint Type-4 record will be converted
#cat:              to a Type-14 record.

   Input:
      ansi_nist  - ANSI/NIST structure to be searched and modified
   Output:
      ansi_nist  - ANSI/NIST structure with modified records
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_fingerprints(ANSI_NIST *ansi_nist)
{
   int i, ret;
   RECORD *imgrecord, *nimgrecord;
   int imgrecord_i;

   i = 1;
   /* While records remain to be searched... */
   while(i < ansi_nist->num_records){
      /* Lookup next binary field fingerprint record ... */
      ret = biomeval_nbis_lookup_binary_field_fingerprint(&imgrecord, &imgrecord_i,
                                            i, ansi_nist);
      /* If error. */
      if(ret < 0)
         return(ret);
      /* If image record not found. */
      if(!ret)
         /* Then done, so return normally. */
         return(0);
      /* Otherwise, binary field fingerprint record found, so convert it */
      /* if possible. */
      ret = biomeval_nbis_iafis2nist_fingerprint(&nimgrecord, ansi_nist, imgrecord_i);
      /* If error ... */
      if(ret < 0)
         return(ret);
      /* If new image record created ... */
      if(ret){

         /* Update standard version in Type-1 record, if necessary,    */
         /* since version 200 does not support record types 13 and 14. */
         if(ansi_nist->version < VERSION_0300) {
            if (biomeval_nbis_substitute_ANSI_NIST_item(0, VER_ID-1, 0, 0, "0300",
                                          ansi_nist) < 0) {
	      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprints : "
		 "cannot change VER to 0300 to support added image record.\n");
              biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
	      return (-65);
            }
            ansi_nist->version = VERSION_0300;
         }

         /* Insert new tagged field fingerprint record. */
         if((ret = biomeval_nbis_insert_ANSI_NIST_record_frmem(imgrecord_i, nimgrecord,
                                                ansi_nist))){
            biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
            return(ret);
         }
         /* Delete old binary field fingerprint record. */
         if((ret = biomeval_nbis_delete_ANSI_NIST_record(imgrecord_i+1, ansi_nist))){
            return(ret);
         }
      }
      /* Otherwise, image record ignored. */

      /* Increment i and continue search. */
      i = imgrecord_i+1;
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_fingerprint - Takes an ANSI/NIST binary field image
#cat:              fingerprint record, and converts it to a tagged
#cat:              field image record.  For example, a 500 dpi 8-bit
#cat:              grayscale tenprint Type-4 record will be converted
#cat:              to a Type-14 record.

   Input:
      ansi_nist   - ANSI/NIST file structure to be searched
      imgrecord_i - index of binary field image record
   Output:
      oimgrecord - ANSI/NIST structure with modified records
   Return Code:
      TRUE       - record successfully converted
      FALSE      - record ignored
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_fingerprint(RECORD **oimgrecord, ANSI_NIST *ansi_nist,
                           const int imgrecord_i)
{
   int i, ret, img_bpx, img_imp, img_idc, newtype;
   char *img_comp;
   double ppmm;
   int ppcm;
   RECORD *imgrecord, *nimgrecord;
   FIELD *field, *nfield, *cmpfield, *impfield;
   SUBFIELD *nsubfield;
   ITEM *nitem;
   int field_i, cmpfield_i, impfield_i;
   char uint_str[MAX_UINT_CHARS+1];

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   /* If record is a low resolution binary field fingerprint record ... */
   if((imgrecord->type == TYPE_3_ID) || (imgrecord->type == TYPE_5_ID)){
      fprintf(stderr, "WARNING : biomeval_nbis_iafis2nist_fingerprint : "
	      "low resolution image record [%d] [Type-%d] not supported\n"
	      "Image record ignored.\n", imgrecord_i+1, imgrecord->type);
      /* Ignore low resolution image record. */
      return(FALSE);
   }

   /* If record is not a binary field fingerprint record ... */
   if((imgrecord->type != TYPE_4_ID) && (imgrecord->type != TYPE_6_ID)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "image record Type-%d not a "
	      "binary field fingerprint record\n", imgrecord->type);
      return(-3);
   }

   /* Set image depth (BPX). */
   /* If input image record is Type-4, then 8 bits per pixel. */
   if(imgrecord->type == TYPE_4_ID)
      img_bpx = 8;
   /* Otherwise, image record is Type-6, so 1 bits per pxiel. */
   else
      img_bpx = 1;

   /* Determine validity and type of new record to be created. */

   /* Look up the compression field (BIN_CA_ID). */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&cmpfield, &cmpfield_i, BIN_CA_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "BIN_CA field not found in image record [Type-%d.%03d]\n",
	      imgrecord->type, BIN_CA_ID);
      return(-4);
   }

   /* If the image data is uncompressed ... */
   if(strcmp((char *)cmpfield->subfields[0]->items[0]->value,
             BIN_COMP_NONE) == 0)
      /* Set tagged field image compression code. */
      img_comp = COMP_NONE;
   /* If the image data is WSQ compressed ... */
   else if(strcmp((char *)cmpfield->subfields[0]->items[0]->value,
                  BIN_COMP_WSQ) == 0)
      /* Set tagged field image compression code. */
      img_comp = COMP_WSQ;
   /* Otherwise, unsupported compression type so ignore ... */
   else{
      /* Post warning and ignore image record. */
      fprintf(stderr, "WARNING : biomeval_nbis_iafis2nist_fingerprint : "
	      "image compression \"%s\" not supported in "
	      "image record [Type-%d.%03d]\n"
	      "Image record ignored.\n",
	      cmpfield->subfields[0]->items[0]->value,
	      imgrecord->type, BIN_CA_ID);
      /* Ignore image record ... */
      return(FALSE);
   }

   /* Lookup the image record's pixels/mm scan resolution. */
   if((ret = biomeval_nbis_lookup_binary_field_image_ppmm(&ppmm, ansi_nist, imgrecord_i)))
      return(ret);

   /* If image's scanning resolution is less than the */
   /* Minimum Scanning Resolution tolerance ... */
   if(ppmm < MIN_RESOLUTION - MM_TOLERANCE){
      fprintf(stderr, "WARNING : biomeval_nbis_iafis2nist_fingerprint : "
	      "scanning resolution = %f below tolerance "
	      "of Minimum Scanning Resolution = %f in "
	      "image record [%d] [Type-%d]\n"
	      "Image record ignored.\n", 
	      ppmm, MIN_RESOLUTION - MM_TOLERANCE,
	      imgrecord_i+1, imgrecord->type);
      /* Ignore image record. */
      return(FALSE);
   }

   /* Lookup Fingerprint Impression (IMP) field. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&impfield, &impfield_i, IMP_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "IMP field not found in image record [%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, IMP_ID);
      return(-5);
   }
   img_imp = atoi((char *)impfield->subfields[0]->items[0]->value);
   /* Determine new image record type based on IMP value. */
   switch(img_imp){
   /* If tenprint image... */
   case 0:
   case 1:
   case 2:
   case 3:
      /* Convert image pixmap to a Type-14 tagged image record.  */
      newtype = TYPE_14_ID;
      break;
   /* If latent image... */
   case 4:
   case 5:
   case 6:
   case 7:
      newtype = TYPE_13_ID;
      break;
   /* Otherwise illegal impression type... */
   default:
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "illegal Impression Type = %d\n", img_imp);
      return(-6);
   }

   /* Allocate new image record. */
   if((ret = biomeval_nbis_new_ANSI_NIST_record(&nimgrecord, newtype)))
      return(ret);

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* {13,14}.001: LEN Field */
   /* Create LEN field with value == "0" (place holder for now). */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, LEN_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with LEN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.002: IDC Field */
   /* Locate IDC field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IDC_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "IDC field not found in input record [%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, IDC_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-7);
   }

   /* FBI/IAFIS requires IDC's to be 2 chars, so make sure here. */
   img_idc = atoi((char *)field->subfields[0]->items[0]->value);
   sprintf(uint_str, "%02d", img_idc);

   /* Create new field from IDC value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, IDC_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with IDC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   
   /* {13,14}.003: IMP Field */
   /* IMP field already found during validation above. */
   /* Create new field from IMP value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, IMP_ID,
                        (char *)impfield->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with IMP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   
   /* {13,14}.004: SRC Field */
   /* Binary field image records do not contain an ORI SRC, so */
   /* assume the ORI in the Type-1 record is the organization  */
   /* that created the current image. */
   /* Locate ORI field in the input Type-1 record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, ORI_ID,
                              ansi_nist->records[0])){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "ORI field not found in input record [1] [Type-%d.%03d]\n",
	      ansi_nist->records[0]->type, ORI_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-8);
   }
   /* Create new field from ORI value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, SRC_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with SRC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.005: LCD/TCD Field */
   /* Binary field image records do not contain an image Creation  */
   /* Date, so assume the Date (DAT) field in the Type-1 record is */
   /* the correct date. */
   /* Locate the Type-1 Date (DAT) field */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, DAT_ID,
                              ansi_nist->records[0])){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "DAT field not found in input record [1] [Type-%d.%03d]\n",
	      ansi_nist->records[0]->type, DAT_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-9);
   }
   /* Create new field from DAT value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, CD_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with LCD/TCD field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.006: HLL Field */
   /* Locate HLL field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, HLL_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "HLL field not found in input record [%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, HLL_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-10);
   }
   /* Create new field from HLL value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, HLL_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with HLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.007: VLL Field */
   /* Locate VLL field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VLL_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "VLL field not found in input record [%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, VLL_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-11);
   }
   /* Create new field from VLL value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, VLL_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with VLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.008: SLC Field */
   /* Create new SLC field with value set to "2" (pp/cm). */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, SLC_ID, "2"))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with SLC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.009: HPS Field */
   /* Compute pixel/cm from pixel/mm. */
   ppcm = sround(ppmm * 10.0);
   /* Create string from ppcm. */
   sprintf(uint_str, "%d", ppcm);
   /* Create new field from pp/cm value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, HPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with HPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.010: VPS Field */
   /* Compute VPS field with HPS value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, VPS_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with VPS field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.011: Compression Algorithm (TAG_CA) Field */
   /* Compute TAG_CA field with compression value "NONE". */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, TAG_CA_ID, img_comp))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with TAG_CA field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.012: BPX Field */
   /* Create new field from BPX value. */
   sprintf(uint_str, "%d", img_bpx);
   if((ret = biomeval_nbis_value2field(&nfield, newtype, BPX_ID, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with BPX field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.013: FGP Field */
   /* Locate FGP field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FGP_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "FGP field not found in input record [%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, FGP_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-12);
   }
   /* Create new field from first FGP value. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, FGP3_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Foreach additional finger position (item) in input FGP field ... */
   /* NOTE: that I chose to represent multiple finger position         */
   /* references in the binary field image records as multiple         */
   /* information items of the same subfield, where as they are        */
   /* represented by multiple subfields in the tagged field records.   */
   for(i = 1; i < field->subfields[0]->num_items; i++){
      /* If another reference value exists ... */
      if(strcmp((char *)field->subfields[0]->items[i]->value, "255") != 0){
         /* Create new subfield from reference positon value. */
         if((ret = biomeval_nbis_value2subfield(&nsubfield,
                          (char *)field->subfields[0]->items[i]->value))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
            return(ret);
         }
         /* Append field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
            return(ret);
         }
      }
      /* Otherwise, no more finger positions ... */
      else
         break;
   }
   /* Append record with FGP3 field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {13,14}.999: DAT Field */
   /* Locate BIN_IMAGE field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, BIN_IMAGE_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_fingerprint : "
	      "Image Data field not found in input record "
	      "[%d] [Type-%d.%03d]\n",
	      imgrecord_i+1, imgrecord->type, BIN_IMAGE_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-13);
   }
   /* Copy image data item. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_item(&nitem, field->subfields[0]->items[0]))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Allocate new subfield. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&nsubfield))){
      biomeval_nbis_free_ANSI_NIST_item(nitem);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append subfield with new image data item. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
      biomeval_nbis_free_ANSI_NIST_item(nitem);
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Create new DAT field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, newtype, DAT2_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append DAT field with with new subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with DAT field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* Update the LEN field with length of record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(nimgrecord))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* Set output pointer. */
   *oimgrecord = nimgrecord;

   /* Return new image record successfully created. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_type_9s - Searches an ANSI/NIST file structure for
#cat:              Type-9 records with FBI/IAFIS fields populated and
#cat:               converts the fields, populating NIST Type-9 fields. 

   Input:
      ansi_nist  - ANSI/NIST structure to be searched and modified
   Output:
      ansi_nist  - ANSI/NIST structure with modified records
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_type_9s(ANSI_NIST *ansi_nist)
{
   int i, ret;
   RECORD *ntype_9;

   for(i = 1; i < ansi_nist->num_records; i++){
      /* Is next record a Type-9 ... */
      if(ansi_nist->records[i]->type == TYPE_9_ID){
         /* Check to see if conversion necessary ... */
         if(biomeval_nbis_iafis2nist_needed(ansi_nist->records[i])){
            /* If so, then convert FBI/IAFIS Type-9 to NIST Type-9. */
            if((ret = biomeval_nbis_iafis2nist_type_9(&ntype_9, ansi_nist, i)))
               return(ret);
            /* Insert new Type-9. */
            if((ret = biomeval_nbis_insert_ANSI_NIST_record_frmem(i, ntype_9, ansi_nist))){
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Delete old Type-9. */
            if((ret = biomeval_nbis_delete_ANSI_NIST_record(i+1, ansi_nist))){
               return(ret);
            }
         }
         /* Otherwise, conversion not needed. */
      }
      /* Otherwise, not Type-9. */
   }
   
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_needed - Determines if the specified Type-9 record
#cat:              contains populated NIST fields.  If not, the
#cat:              routine returns TRUE, otherwise it returns FALSE.

   Input:
      record     - Type-9 record structure to be searched
   Return Code:
      TRUE       - NIST fields NOT populated
      FALSE      - NIST fields ARE populated
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_needed(RECORD *record)
{
   FIELD *mrcfield;
   int mrcfield_i;

   if(record->type != TYPE_9_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_needed : "
	      "record type = %d : not Type-9", record->type);
      exit(-2);
   }

   if(!biomeval_nbis_lookup_ANSI_NIST_field(&mrcfield, &mrcfield_i, MRC_ID, record))
      return(TRUE);

   /* Otherwise, NIST fields already exist. */
   fprintf(stderr, "WARNING: biomeval_nbis_iafis2nist_needed : "
	   "NIST Type-9 fields already exist so ignoring record.\n");
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_type_9 - Takes a Type-9 record with FBI/IAFIS fields
#cat:              populated and converts the fields, populating NIST fields
#cat:              in the record.

   This routine requires the parent ANSI/NIST structure to be passed,
   rather than just the Type-9 record itself, because y-coordinate
   inversion requires knowledge of the corresponding image's pixel
   height, which is not stored in the Type-9, but must come from an
   image record.
   
   Input:
      ansi_nist  - ANSI/NIST file structure conntaining record to
                   be processed
      type_9_i   - index of Type-9 record to be processed
   Output:
      otype_9    - new Type-9 record with NIST fields populated
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_type_9(RECORD **otype_9, ANSI_NIST *ansi_nist,
                      const int type_9_i)
{
   int i, j, ret, field_i;
   RECORD *type_9, *ntype_9;
   FIELD *field, *nfield;
   SUBFIELD *subfield, *nsubfield;
   ITEM *nitem;
   char *sysname, *method, *nmethod;
   char *class, *nclass, *type, *ntype, *nist_rc;
   int idc, ih;
   double ppmm;
   RECORD *imgrecord;
   int imgrecord_i, finger_pos, imp_code, quality;

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* If record index out of range ... */
   if((type_9_i < 1) || (type_9_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "Type 9 record index = %d out of range [2..%d]\n",
	      type_9_i+1, ansi_nist->num_records);
      return(-2);
   }
   /* Assign Type 9 record pointer. */
   type_9 = ansi_nist->records[type_9_i];
   /* If record is not a Type-9 ... */
   if(type_9->type != TYPE_9_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "Record index [%d] Type-%d not Type-9\n",
	      type_9_i+1, type_9->type);
      return(-3);
   }

   /* Allocate new Type-9 record. */
   if((ret = biomeval_nbis_new_ANSI_NIST_record(&ntype_9, TYPE_9_ID)))
      return(ret);

   /* 9.001: LEN Field */
   /* Locate LEN field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, LEN_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "LEN field not found in input record [Type-9.%03d]\n", LEN_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-4);
   }
   /* Copy LEN Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new LEN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.002: IDC Field */
   /* Locate IDC field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IDC_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "IDC field not found in input record [Type-9.%03d]\n", IDC_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-5);
   }
   /* Copy IDC Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new IDC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* Lookup image record with matching IDC.  Will need image height */
   /* and ppmm to flip y-coords later.                               */
   /* NOTE: This assumes that the scan resolution of the image with  */
   /* the same IDC is the same scan resolution of the minutiae       */
   /* coordinates in the Type-9 record with the same IDC.  This is   */
   /* a reasonable operational assumption that might not always hold!*/
   idc = atoi((char *)field->subfields[0]->items[0]->value);
   ret = biomeval_nbis_lookup_fingerprint_with_IDC(&imgrecord, &imgrecord_i,
                                     idc, 1, ansi_nist);
   /* If error ... */
   if(ret < 0){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* If matching fingerprint image record not found ... */
   if(!ret){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "Fingerprint image record with IDC = %d not found\n", idc);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-6);
   }
   /* Otherwise, fingerprint record with matching IDC found, */
   /* so get image height and ppmm. */
   /* Lookup horizontal line length (VLL_ID). */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VLL_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_ANSI_NIST_image : "
	      "VLL field not found in record index [%d] [Type-%d.%03d]\n",
              imgrecord_i+1, imgrecord->type, VLL_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-7);
   }
   ih = atoi((char *)field->subfields[0]->items[0]->value);

   /* Lookup image record's pixel/mm scan resolution. */
   if((ret = biomeval_nbis_lookup_ANSI_NIST_image_ppmm(&ppmm, ansi_nist, imgrecord_i))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.003: IMP Field */
   /* Locate IMP field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IMP_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "IMP field not found in input record [Type-9.%03d]\n", IMP_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-8);
   }
   /* Check to make sure IMP is in range of ANSI/NIST Table 5. */
   imp_code = atoi((char *)field->subfields[0]->items[0]->value);
   if((imp_code < MIN_TABLE_5_CODE) ||
      (imp_code > MAX_TABLE_5_CODE)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "IMP = %d out of Table 5 range [%d..%d] in [Type-9.%03d]\n",
              imp_code, MIN_TABLE_5_CODE, MAX_TABLE_5_CODE, IMP_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-9);
   }
   /* Copy IMP Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new IMP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.004: FMT Field */
   /* Create new FMT field with value = "S" for Standard fields. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, FMT_ID, "S"))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new FMT field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.005: OFR Field */
   /* Locate FCP (9.016) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FCP_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "FCP field not found in input record [Type-9.%03d]\n", FCP_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-10);
   }
   if(field->subfields[0]->num_items != 3){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "FCP field does not contain 3 items in [Type-9.%03d]\n", FCP_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-11);
   }
   sysname = (char *)field->subfields[0]->items[0]->value;
   method = (char *)field->subfields[0]->items[2]->value;
   /* Create OFR subfield with first item == FCP Equipment Name. */
   if((ret = biomeval_nbis_value2subfield(&nsubfield, sysname))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Convert FBI/IAFIS system method to ANSI/NIST method. */
   if((ret = biomeval_nbis_iafis2nist_method(&nmethod, method))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   if((ret = biomeval_nbis_value2item(&nitem, nmethod))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append Method item to OFR subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
      biomeval_nbis_free_ANSI_NIST_item(nitem);
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Create new OFR field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, OFR_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append OFR field with new subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new OFR field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.006: FGP2 Field */
   /* Locate FGN (9.014) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FGN_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "FGN field not found in input record [Type-9.%03d]\n", FGN_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-12);
   }
   /* Check if position code is valid ... palm or finger. */
   finger_pos = atoi((char *)field->subfields[0]->items[0]->value);
   if(!(((finger_pos >= MIN_TABLE_6_CODE) &&
         (finger_pos <= MAX_TABLE_6_CODE)) ||
        ((finger_pos >= MIN_TABLE_19_CODE) &&
         (finger_pos <= MAX_TABLE_19_CODE)))){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "FGN = %d not a valid Table 6 or Table 19 code in "
	      "input record [Type-9.%03d]\n",
	      finger_pos, FGN_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-13);
   }
   /* Create new FGP2 field from FGN value. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, FGP2_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new FGP2 field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.007: FPC Field */
   /* Note: This is a manditory NIST field and an optional IAFIS field. */
   /* Locate APC (9.017) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, APC_ID, type_9)){
      /* Create a new subfield with first item set to "T" for Table 8. */
      if((ret = biomeval_nbis_value2subfield(&nsubfield, "T"))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Then set primary pattern class to "UN" for Unknown. */
      if((ret = biomeval_nbis_value2item(&nitem, "UN"))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append subfield with new item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Create new FPC field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, FPC_ID))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append FPC field with default subfield. */
      if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append record with new FPC field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
   }
   /* Otherwise, use pattern classes in APC field. */
   else{
      /* Create new FPC field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, FPC_ID))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Foreach pattern class in APC field ... */
      for(i = 0; i < field->num_subfields; i++){
         /* Get next APC pattern class. */
         class = (char *)field->subfields[i]->items[0]->value;
         /* Translate IAFIS class codes to NIST class codes.         */
         if((ret = biomeval_nbis_iafis2nist_pattern_class(&nclass, class, finger_pos))){
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* If primary classificaiton (1st time through loop) ... */
         if(i == 0){
            /* Create a new subfield with first item set to "T" for Table 8. */
            if((ret = biomeval_nbis_value2subfield(&nsubfield, "T"))){
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Create new item from current pattern classification. */
            if((ret = biomeval_nbis_value2item(&nitem, nclass))){
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Append subfield with new item. */
            if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
               biomeval_nbis_free_ANSI_NIST_item(nitem);
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
         }
         /* Otherwise, secondary reference classes which have one */
         /* item per subfield. */
         else{
            /* Create a new subfield with current classification value. */
            if((ret = biomeval_nbis_value2subfield(&nsubfield, nclass))){
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
         }
         /* Append FPC field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      } /* Next pattern classification. */
      /* Append record with new FPC field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
   }

   /* 9.008: CRP Field */
   /* Optional, so populate only if Core(s) reported in FBI/IAFIS fields. */
   /* Locate CRA field in input record. */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, CRA_ID, type_9)){
      /* Allocate new CRP field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, CRP_ID))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* For each core reported in CRA field ... */
      for(i = 0; i < field->num_subfields; i++){
         /* Create new CRP subfield from next core's location. */
         if((ret = biomeval_nbis_value2subfield(&nsubfield, 
                          (char *)field->subfields[i]->items[0]->value))){
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* If core location string not 8 characters long ... */
         if(strlen((char *)nsubfield->items[0]->value) != 8){
            fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
		    "Core location = %s is not format XXXXYYYY in "
		    "CRA field [Type-9.%03d]\n",
                    nsubfield->items[0]->value, CRA_ID);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(-14);
         }
         /* Flip core's y-coord. */
         /* Start 5 chars in on XXXXYYYY and proceed to work on 4 */
         /* chars from that point. */
         if((ret = biomeval_nbis_flip_y_coord((char *)&(nsubfield->items[0]->value[4]), 4,
                               ih, ppmm))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append CRP field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append record with new CRP field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
        return(ret);
      }
   }

   /* 9.009: DLT Field */
   /* Optional, so populate only if Delta(s) reported in FBI/IAFIS fields. */
   /* Locate DLA field in input record. */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, DLA_ID, type_9)){
      /* Allocate new DLT field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, DLT_ID))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* For each delta reported in DLA field ... */
      for(i = 0; i < field->num_subfields; i++){
         /* Create new DLT subfield from next delta's location. */
         if((ret = biomeval_nbis_value2subfield(&nsubfield, 
                          (char *)field->subfields[i]->items[0]->value))){
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* If delta location string not 8 characters long ... */
         if(strlen((char *)nsubfield->items[0]->value) != 8){
            fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
		    "Delta location = %s is not format XXXXYYYY in "
		    "DLA field [Type-9.%03d]\n",
                    nsubfield->items[0]->value, DLA_ID);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(-15);
         }
         /* Flip first delta's y-coord. */
         /* Start 5 chars in on XXXXYYYY and proceed to work on 4 */
         /* chars from that point. */
         if((ret = biomeval_nbis_flip_y_coord((char *)&(nsubfield->items[0]->value[4]), 4,
                               ih, ppmm))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append DLT field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append record with new DLT field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
        return(ret);
      }
   }

   /* 9.010: MIN Field */
   /* Locate NMN (9.015) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, NMN_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "NMN field not found in input record [Type-9.%03d]\n", NMN_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-16);
   }
   /* Create new MIN field from NMN value. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, MIN_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new MIN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.011: RDG Field */
   /* FBI/IAFIS minutiae record includes manditory ridge counts. */
   /* Create new RDG field with flag = "1" for Ridge Counts Available. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, RDG_ID, "1"))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new RDG field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.012: MRC Field */
   /* Look up MAT (9.023) field. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, MAT_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
	      "MAT field not found in input record [Type-9.%03d]\n", MAT_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-17);
   }
   /* Allocate new MRC field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, MRC_ID))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Foreach minutiae reported in MAT field ... */
   for(i = 0; i < field->num_subfields; i++){
      /* Set subfield pointer. */
      subfield = field->subfields[i];
      /* MAT minutiae subfields have 12 manditory items. By testing for */
      /* this condition, we avoid needing to test for conditionally     */
      /* optional minutiae attributes individually.                     */
      if(subfield->num_items < 12){
         fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
		 "Minutia %s has %d items < 12 in record [Type-9.%03d]\n",
                 subfield->items[0]->value, subfield->num_items, MAT_ID);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-18);
      }

      /* 1. Index Item */
      /* Create MRC subfield with first item == Index item. */
      j = 0;
      if((ret = biomeval_nbis_value2subfield(&nsubfield,
                               (char *)subfield->items[j++]->value))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }

      /* 2. Location/Direction Item */
      /* Create MRC subfield's second item from MAT XXXXYYYYTTT value. */
      if((ret = biomeval_nbis_value2item(&nitem, (char *)subfield->items[j++]->value))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* If minutia's location string not 11 characters long ... */
      if(strlen((char *)nitem->value) != 11){
         fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
		 "Minutia %s location = %s is not "
		 "format XXXXYYYYTTT in MAT field [Type-9.%03d]\n",
                 subfield->items[0]->value, nitem->value, MAT_ID);
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-19);
      }
      /* Flip minutia's y-coord. */
      /* Start 5 chars in on XXXXYYYYTTT and proceed to work on 4 */
      /* chars from that point. */
      if((ret = biomeval_nbis_flip_y_coord((char *)&(nitem->value[4]), 4, ih, ppmm))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Flip minutia's direction. */
      /* Start 9 chars in on XXXXYYYYTTT and proceed to work on 3 */
      /* chars from that point. */
      if((ret = biomeval_nbis_flip_direction((char *)&(nitem->value[8]), 3))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append minutia subfield with new Location/Theta item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }

      /* 3. Quality Item */
      /* Check if minutia quality is valid. */
      quality = atoi((char *)subfield->items[j]->value);
      if((quality < MIN_QUALITY_VALUE) ||
         (quality > MAX_QUALITY_VALUE)){
         fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_type_9 : "
		 "Minutia %s quality = %d out of range [%d..%d] in "
		 "[Type-9.%03d]\n",
		 subfield->items[0]->value, quality,
		 MIN_QUALITY_VALUE, MAX_QUALITY_VALUE, MAT_ID);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-20);
      }
      /* Create MRC subfield's 3rd item from MAT Quality value. */
      if((ret = biomeval_nbis_value2item(&nitem, (char *)subfield->items[j++]->value))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append minutia subfield with new Quality item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }

      /* 4. Type Item (conditionally optional) */
      /* Translate FBI/IAFIS minutia type codes to ANSI/NIST type codes. */
      type = (char *)subfield->items[j++]->value;
      if((ret = biomeval_nbis_iafis2nist_minutia_type(&ntype, type))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Create MRC subfield's 4th item from Minutia Type value. */
      if((ret = biomeval_nbis_value2item(&nitem, ntype))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Append minutia subfield with new Type item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }

      /* 5-12. Ridge Counts. */
      while(j < 12){
         if((ret = biomeval_nbis_iafis2nist_ridgecount(&nist_rc,
                                        (char *)subfield->items[j++]->value))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Create item from new ANSI/NIST ridge count string. */
         if((ret = biomeval_nbis_value2item(&nitem, nist_rc))){
            free(nist_rc);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Deallocate NIST ridge count string. */
         free(nist_rc);
         /* Append minutia subfield with new Ridge Count item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append MRC field with new minutia subfield. */
      if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
   } /* Next minutia */
   /* Append record with new MRC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* Update the LEN field with length of new Type-9 record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(ntype_9))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* Assign output pointer. */
   *otype_9 = ntype_9;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_method - Takes an FBI/IAFIS system method string
#cat:              and returns an ANSI/NIST method.

    IAFIS METHODS            ANSI/NIST METHODS
        "*A*"          ==>          "A"
        "*E*"          ==>          "E"
        "*M*"          ==>          "M"

   Input:
      method    - FBI/IAFIS system method string
   Output:
      omethod   - points to ANSI/NIST method
   Return Code:
      Zero      - successful completion
      Negative  - system error
************************************************************************/
int biomeval_nbis_iafis2nist_method(char **omethod, char *method)
{
   char *nmethod;

   if(strlen(method) != 3){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_method : "
	      "invalid method %s found : "
	      "FBI/IAFIS method must be %d characters\n", 
	      method, IAFIS_METHOD_STRLEN);
      return(-2);
   }

   /* Assign NIST method based on second character of FBI/IAFIS method. */
   switch(method[1]){
      case 'A':
           nmethod = "A";
           break;
      case 'E':
           nmethod = "E";
           break;
      case 'M':
           nmethod = "M";
           break;
      default:
           fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_method : "
		   "invalid FBI/IAFIS method %s found\n", method);
           return(-3);
   }

   /* Assign output pointer. */
   *omethod = nmethod;
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_minutia_type - Takes an FBI/IAFIS minutia type
#cat:              and returns an ANSI/NIST minutia type from Table 7.
   
   Input:
      type      - FBI/IAFIS minutia type
   Output:
      otype     - points to ANSI/NIST minutia type from Table 7
   Return Code:
      Zero      - successful completion
      Negative  - system error
************************************************************************/
int biomeval_nbis_iafis2nist_minutia_type(char **otype, char *type)
{
   char *ntype;

   /* Ridge Ending */
   if(strcmp(type, "A") == 0){
      ntype = "A";
   /* Bifurcation */
   } else if(strcmp(type, "B") == 0){
      ntype = "B";
   /* Ridge Ending or Bifurcation - no distinction provided */
   } else if(strcmp(type, "C") == 0){
      ntype = "D";
   /* Other type */
   } else if(strcmp(type, "D") == 0){
      ntype = "C";
   /* Illegal type */
   } else{
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_minutia_type : "
	      "invalid ANSI/NIST minutia type %s found\n", type);
      return(-2);
   }

   /* Set output pointer. */
   *otype = ntype;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_pattern_class - Takes an FBI/IAFIS pattern classification
#cat:              and a finger position (if known) and if possible returns
#cat:              an ANSI/NIST Table 8 pattern classification.
   
   Input:
      class      - FBI/IAFIS pattern classification
      finger_pos - ANSI/NIST finger position from Table 6
   Output:
      oclass     - points to ANSI/NIST Table 8 pattern classification
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_pattern_class(char **oclass, char *class, const int finger_pos)
{
   char *nclass;
   int ret;

   /* Determine which hand the finger is on. */
   ret = biomeval_nbis_which_hand(finger_pos);
   /* If error ... */
   if(ret < 0)
      return(ret);

   /* Arch - type not designated */
   if(strcmp(class, "AU") == 0){
      /* Not a perfect assignement to most prevelant Plain Arch, but */
      /* ANSI/NIST Table 8 does not provide for an Arch without      */
      /* designated type. */
      nclass = "PA";
   /* Left Slant Loop */
   } else if(strcmp(class, "LS") == 0){
      nclass = "LS";
   /* Right Slant Loop */
   } else if(strcmp(class, "RS") == 0){
      nclass = "RS";
   /* Whorl - type not designated */
   } else if(strcmp(class, "WU") == 0){
      nclass = "WN";
   /* Complete Scar */
   } else if(strcmp(class, "SR") == 0){
      nclass = "SR";
   /* Amputation */
   } else if(strcmp(class, "XX") == 0){
      nclass = "XX";
   /* Unknown or Unclassifiable */
   } else if(strcmp(class, "UC") == 0){
      nclass = "UN";
   /* Illegal pattern class */
   } else{
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_pattern_class : "
	      "invalid pattern class = %s\n", class);
      return(-2);
   }

   /* Set output pointer. */
   *oclass = nclass;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_iafis2nist_ridgecount - Takes an FBI/IAFIS formatted neighbor/ridge
#cat:              count fixed-length string containing a concatenated
#cat:              neighbor index and ridge count, and generates an ANSI/NIST
#cat:              formatted string with the index and count separated
#cat:              by a comma.
   
   Input:
      iafis_rc   - FBI/IAFIS formatted neighbor/ridgecount string
   Output:
      onist_rc   - points to new ANSI/NIST formatted string
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_iafis2nist_ridgecount(char **onist_rc, char *iafis_rc)
{
   char *nist_rc, c;
   int nbr_i, rc;

   /* FBI/IAFIS ridge count string must be 5 characters long: IIICC */
   if(strlen(iafis_rc) != 5){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_ridgecount : "
	      "FBI/IAFIS ridge count string = %s is not 5 chars long\n",
	      iafis_rc);
      return(-2);
   }

   /* Parse neighbor index. */
   c = iafis_rc[3];
   iafis_rc[3] = '\0';
   if(sscanf(iafis_rc, "%d", &nbr_i) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_ridgecount : "
	      "neighbor index parse error from FBI/IAFIS "
	      "ridge count string = %s\n", iafis_rc);
      return(-3);
   }
   iafis_rc[3] = c;
   if(sscanf(&(iafis_rc[3]), "%d", &rc) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_ridgecount : "
	      "ridge count parse error from FBI/IAFIS "
	      "ridge count string = %s\n", iafis_rc);
      return(-4);
   }
   /* Allocate NIST ridge count string "III,CC". */
   nist_rc = (char *)calloc(7, sizeof(char));
   if(nist_rc == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_iafis2nist_ridgecount : "
	      "calloc : nist_rc (%lu bytes)\n",
	      (unsigned long)(7 * sizeof(char)));
      return(-5);
   }
   /* Construct new NIST formateed ridge count string. */
   sprintf(nist_rc, "%03d,%02d", nbr_i, rc);

   /* Set output pointer. */
   *onist_rc = nist_rc;
   /* Return normally. */
   return(0);
}
