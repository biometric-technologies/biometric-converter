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

      FILE:    TO_IAFIS.C
      AUTHOR:  Michael D. Garris
      DATE:    10/20/2000
      UPDATED: 02/28/2007 by Kenneth Ko
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines responsible for converting an ANSI/NIST 2007
      formatted file structure into an EFTS Version 7 FBI/IAFIS file
      structure.  The results of this program may be viewed by ULW.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_nist2iafis_fingerprints()
                        biomeval_nbis_nist2iafis_fingerprint()
                        biomeval_nbis_nist2iafis_type_9s()
                        biomeval_nbis_nist2iafis_needed()
                        biomeval_nbis_nist2iafis_type_9()
                        biomeval_nbis_nist2iafis_method()
                        biomeval_nbis_nist2iafis_minutia_type()
                        biomeval_nbis_nist2iafis_pattern_class()
                        biomeval_nbis_nist2iafis_ridgecount()

***********************************************************************/

#include <usebsd.h>
#include <an2k.h>
#include <string.h>

#include <nbis_sysdeps.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_nist2iafis_fingerprints - Searches an ANSI/NIST file structure for
#cat:              tagged field fingerprint records, and if possible,
#cat:              converts them to binary field image records.  For
#cat:              example, a 500 dpi 8-bit grayscale Type-14 record
#cat:              will be converted to a Type-4 record.

   Input:
      ansi_nist  - ANSI/NIST structure to be searched and modified
   Output:
      ansi_nist  - ANSI/NIST structure with modified records
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_fingerprints(ANSI_NIST *ansi_nist)
{
   int i, ret;
   RECORD *imgrecord, *nimgrecord;
   int imgrecord_i;

   i = 1;
   /* While records remain to be searched... */
   while(i < ansi_nist->num_records){
      /* Lookup next tagged field fingerprint record ... */
      ret = biomeval_nbis_lookup_tagged_field_fingerprint(&imgrecord, &imgrecord_i,
                                            i, ansi_nist);
      /* If error. */
      if(ret < 0)
         return(ret);
      /* If image record not found. */
      if(!ret)
         /* Then done, so return normally. */
         return(0);
      /* Otherwise, tagged field fingerprint record found, so convert it */
      /* if possible. */
      ret = biomeval_nbis_nist2iafis_fingerprint(&nimgrecord, imgrecord);
      /* If error ... */
      if(ret < 0)
         return(ret);
      /* If new image record created ... */
      if(ret){
         /* Insert new binary field fingerprint record. */
         if((ret = biomeval_nbis_insert_ANSI_NIST_record_frmem(imgrecord_i, nimgrecord,
                                                ansi_nist))){
            biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
            return(ret);
         }
         /* Delete old tagged field fingerprint record. */
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
#cat: biomeval_nbis_nist2iafis_fingerprint - Takes an ANSI/NIST tagged field image
#cat:              fingerprint record, and if possible, converts it to
#cat:              a binary field image record.  For example, a 500 dpi
#cat:              8-bit grayscale Type-14 record will be converted to
#cat:              a Type-4 record.

   Input:
      imgrecord  - Record structure to be converted
   Output:
      oimgrecord - ANSI/NIST structure with modified records
   Return Code:
      TRUE       - record successfully converted
      FALSE      - record ignored
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_fingerprint(RECORD **oimgrecord, RECORD *imgrecord)
{
   int i, ret, id, newtype;
   int record_bytes;
   char *img_comp, *isr;
   double ppmm;
   RECORD *nimgrecord;
   FIELD *field, *nfield, *cmpfield, *bpxfield;
   SUBFIELD *nsubfield;
   ITEM *nitem;
   int field_i, cmpfield_i, bpxfield_i;
   char uint_str[MAX_UINT_CHARS+1];

   /* If record is not a tagged field fingerprint record ... */
   if((imgrecord->type != TYPE_13_ID) && (imgrecord->type != TYPE_14_ID)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "image record Type-%d not a "
	      "tagged field fingerprint record\n", imgrecord->type);
      return(-3);
   }

   /* Determine validity and type of new record to be created. */

   /* Look up the compression field (TAG_CA_ID). */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&cmpfield, &cmpfield_i, TAG_CA_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "TAG_CA field not found in image record [Type-%d.%03d]\n",
	      imgrecord->type, TAG_CA_ID);
      return(-4);
   }

   /* If the image data is uncompressed ... */
   if(strcmp((char *)cmpfield->subfields[0]->items[0]->value, COMP_NONE) == 0)
      /* Set binary field image compression code. */
      img_comp = BIN_COMP_NONE;
   /* If the image data is WSQ compressed ... */
   else if(strcmp((char *)cmpfield->subfields[0]->items[0]->value,
            COMP_WSQ) == 0)
      /* Set binary field image compression code. */
      img_comp = BIN_COMP_WSQ;
   /* Otherwise, unsupported compression type so ignore ... */
   else{
      /* Post warning and ignore image record. */
      fprintf(stderr, "WARNING : biomeval_nbis_nist2iafis_fingerprint : "
	      "image compression \"%s\" not supported in "
	      "image record [Type-%d.%03d]\n"
	      "Image record ignored.\n",
	      cmpfield->subfields[0]->items[0]->value,
	      imgrecord->type, TAG_CA_ID);
      /* Ignore image record ... */
      return(FALSE);
   }

   /* Lookup the image record's pixels/mm scan resolution. */
   ret = biomeval_nbis_lookup_tagged_field_image_ppmm(&ppmm, imgrecord);
   /* If error ... */
   if(ret < 0)
      return(ret);
   /* If image record ignored ... */
   if(!ret)
      return(FALSE);
   /* Otherwise ppmm sucessfully derived. */

   /* If scan resolution is not within tolerance of */
   /* Minimum Scanning Resolution ... */
   if((ppmm < MIN_RESOLUTION - MM_TOLERANCE) ||
      (ppmm > MIN_RESOLUTION + MM_TOLERANCE)){
      fprintf(stderr, "WARNING : biomeval_nbis_nist2iafis_fingerprint : "
	      "scanning resolution = %f not within tolerance "
	      "of Minimum Scanning Resolution = %f in "
	      "image record [Type-%d]\n"
	      "Image record ignored.\n",
	      ppmm, MIN_RESOLUTION, imgrecord->type);
      /* Ignore image record. */
      return(FALSE);
   }

   /* Lookup pixel depth field (BPX_ID). */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&bpxfield, &bpxfield_i, BPX_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "BPX field not found in image record [Type-%d.%03d]\n",
	      imgrecord->type, BPX_ID);
      return(-5);
   }
   /* Set image pixel depth = BPX_ID. */
   id = atoi((char *)bpxfield->subfields[0]->items[0]->value);
   /* If not 1-bit or 8-bit grayscale ... */
   switch(id){
      case 1:
         newtype = TYPE_6_ID;
         break;
      case 8:
         newtype = TYPE_4_ID;
         break;
      default:
         /* Unsupported pixel depth, so post warning and ignore. */
         fprintf(stderr, "WARNING : biomeval_nbis_nist2iafis_fingerprint : "
		 "pixel depth \"%s\" not supported in "
		 "in image record [Type-%d.%03d]\n"
		 "Image record ignored.\n",
		 bpxfield->subfields[0]->items[0]->value,
		 imgrecord->type, BPX_ID);
         /* Ignore image record. */
         return(FALSE);
   }

   /* Allocate new image record. */
   if((ret = biomeval_nbis_new_ANSI_NIST_record(&nimgrecord, newtype)))
      return(ret);

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* {4,6}.001: LEN Field */
   /* Locate LEN field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, LEN_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "LEN field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, LEN_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-6);
   }
   /* Copy LEN Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, LEN_ID);
   /* Set binary byte length of LEN field. */
   nfield->num_bytes = BINARY_LEN_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_LEN_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_LEN_BYTES;
   /* Increment binary record's bytes. */
   record_bytes = BINARY_LEN_BYTES;
   /* Append record with new LEN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {4,6}.002: IDC Field */
   /* Locate IDC field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IDC_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "IDC field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, IDC_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-7);
   }
   /* Copy IDC Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, IDC_ID);
   /* Set binary byte length of IDC field. */
   nfield->num_bytes = BINARY_IDC_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_IDC_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_IDC_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_IDC_BYTES;
   /* Append record with new IDC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   
   /* {4,6}.003: IMP Field */
   /* Locate IMP field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IMP_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "IMP field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, IMP_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-8);
   }
   /* Copy IMP Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, IMP_ID);
   /* Set binary byte length of IMP field. */
   nfield->num_bytes = BINARY_IMP_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_IMP_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_IMP_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_IMP_BYTES;
   /* Append record with new IMP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   
   /* {4,6}.004: FGP Field */
   /* Locate FGP3 field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FGP3_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "FGP field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, FGP3_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-9);
   }
   /* Create new FGP subfield from first finger position. */
   if((ret = biomeval_nbis_value2subfield(&nsubfield,
                           (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Foreach remaining subfield (reference finger position) ... */
   for(i = 1; i < field->num_subfields; i++){
      /* If reference positions exceed length of binary FGP field ... */
      if(i >= BINARY_FGP_BYTES){
         /* Then post warning and ignore remaining references. */
         fprintf(stderr, "WARNING : biomeval_nbis_nist2iafis_fingerprint : "
		 "number of reference finger positions = %d > %d: "
		 "extra references ignored\n",
		 field->num_subfields, BINARY_FGP_BYTES);
         break;
      }
      /* Create new FGP item from next reference position subfield. */
      if((ret = biomeval_nbis_value2item(&nitem,
                           (char *)field->subfields[i]->items[0]->value))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
         return(ret);
      }
      /* Append subfield with new item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
         return(ret);
      }
   }
   /* Foreach reference position not assigned up to fixed number ... */
   for(;i < BINARY_FGP_BYTES; i++){
      /* Create new FGP item set to UNUSED. */
      if((ret = biomeval_nbis_value2item(&nitem, UNUSED_STR))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
         return(ret);
      }
      /* Append subfield with new item. */
      if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
         return(ret);
      }
   }

   /* Create new FGP field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, newtype, FGP_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append new FGP field with subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Append record with new FGP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Set all binary items in new subfield to size of 1 byte. */
   for(i = 0; i < nsubfield->num_items; i++)
      nsubfield->items[i]->num_bytes = 1;
   /* Set binary byte length of FGP subfield and field. */
   nsubfield->num_bytes = BINARY_FGP_BYTES;
   nfield->num_bytes = BINARY_FGP_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_FGP_BYTES;

   /* {4,6}.005: ISR Field */
   /* If ppmm is exactly equal to the Minimum Scanning Resolution ...*/
   if(ppmm == MIN_TAGGED_RESOLUTION)
      /* Then ISR = 0. */
      isr = "0";
   else
      /* Otherwise, ISR = 1. */
      isr = "1";
   /* Create new ISR field. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, ISR_ID, isr))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Set binary byte length of ISR field. */
   nfield->num_bytes = BINARY_ISR_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_ISR_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_ISR_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_ISR_BYTES;
   /* Append record with new ISR field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {4,6}.006: HLL Field */
   /* Locate HLL field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, HLL_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "HLL field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, HLL_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-10);
   }
   /* Copy HLL Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, HLL_ID);
   /* Set binary byte length of HLL field. */
   nfield->num_bytes = BINARY_HLL_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_HLL_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_HLL_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_HLL_BYTES;
   /* Append record with new HLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {4,6}.007: VLL Field */
   /* Locate VLL field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VLL_ID, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "VLL field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, VLL_ID);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-11);
   }
   /* Copy VLL Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, VLL_ID);
   /* Set binary byte length of VLL field. */
   nfield->num_bytes = BINARY_VLL_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_VLL_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_VLL_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_VLL_BYTES;
   /* Append record with new VLL field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {4,6}.008: GCA/BCA Field */
   /* Create new GCA/BCA field. */
   if((ret = biomeval_nbis_value2field(&nfield, newtype, BIN_CA_ID, img_comp))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Set binary byte length of GCA/BCAL field. */
   nfield->num_bytes = BINARY_CA_BYTES;
   nfield->subfields[0]->num_bytes = BINARY_CA_BYTES;
   nfield->subfields[0]->items[0]->num_bytes = BINARY_CA_BYTES;
   /* Increment binary record's bytes. */
   record_bytes += BINARY_CA_BYTES;
   /* Append record with new GCA/BCA field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* {4,6}.009: Image Data Field */
   /* Locate Image Data field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IMAGE_FIELD, imgrecord)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_fingerprint : "
	      "Image Data field not found in input record [Type-%d.%03d]\n",
	      imgrecord->type, IMAGE_FIELD);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(-12);
   }
   /* Copy Image Data Field. */
   if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, field))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   /* Change Image Data Field's ID. */
   biomeval_nbis_update_ANSI_NIST_field_ID(nfield, newtype, BIN_IMAGE_ID);
   /* Set binary byte length of Image Data field. */
   nfield->num_bytes = field->subfields[0]->items[0]->num_bytes;
   nfield->subfields[0]->num_bytes = field->subfields[0]->items[0]->num_bytes;
   nfield->subfields[0]->items[0]->num_bytes =
                                     field->subfields[0]->items[0]->num_bytes;
   /* Increment binary record's bytes. */
   record_bytes += field->subfields[0]->items[0]->num_bytes;
   /* Append record with new Image Data field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(nimgrecord, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }

   /* Create new LEN item with correct binary record length. */
   sprintf(uint_str, "%d", record_bytes);
   if((ret = biomeval_nbis_value2item(&nitem, uint_str))){
      biomeval_nbis_free_ANSI_NIST_record(nimgrecord);
      return(ret);
   }
   nitem->num_bytes = BINARY_LEN_BYTES;
   /* Deallocate current LEN item. */
   biomeval_nbis_free_ANSI_NIST_item(nimgrecord->fields[0]->subfields[0]->items[0]);
   /* Assign new LEN item. */
   nimgrecord->fields[0]->subfields[0]->items[0] = nitem;

   /* Set new binary record's length. */
   imgrecord->num_bytes = record_bytes;
   imgrecord->total_bytes = record_bytes;

   /* Set output pointer. */
   *oimgrecord = nimgrecord;

   /* Return new image record successfully created. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_nist2iafis_type_9s - Searches an ANSI/NIST file structure for
#cat:              Type-9 records with NIST fields populated and converts
#cat:              the fields, populating FBI/IAFIS Type-9 fields. 

   Input:
      ansi_nist  - ANSI/NIST structure to be searched and modified
   Output:
      ansi_nist  - ANSI/NIST structure with modified records
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_type_9s(ANSI_NIST *ansi_nist)
{
   int i, ret;
   RECORD *ntype_9;

   for(i = 1; i < ansi_nist->num_records; i++){
      /* Is next record a Type-9 ... */
      if(ansi_nist->records[i]->type == TYPE_9_ID){
         /* Check to see if conversion necessary ... */
         if(biomeval_nbis_nist2iafis_needed(ansi_nist->records[i])){
            /* If so, then convert NIST Type-9 to IAFIS Type-9. */
            if((ret = biomeval_nbis_nist2iafis_type_9(&ntype_9, ansi_nist, i)))
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
#cat: biomeval_nbis_nist2iafis_needed - Determines if the specified Type-9 record
#cat:              contains populated FBI/IAFIS fields.  If not, the
#cat:              routine returns TRUE, otherwise it returns FALSE.

   Input:
      record     - Type-9 record structure to be searched
   Return Code:
      TRUE       - FBI/IAFIS fields NOT populated
      FALSE      - FBI/IAFIS fields ARE populated
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_needed(RECORD *record)
{
   FIELD *fgnfield;
   int fgnfield_i;

   if(record->type != TYPE_9_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_needed : "
	      "record type = %d : not Type-9", record->type);
      exit(-2);
   }

   if(!biomeval_nbis_lookup_ANSI_NIST_field(&fgnfield, &fgnfield_i, FGN_ID, record))
      return(TRUE);

   /* Otherwise, IAFIS fields already exist. */
   fprintf(stderr, "WARNING: biomeval_nbis_nist2iafis_needed : "
	   "IAFIS Type-9 fields already exist so ignoring record.\n");
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_nist2iafis_type_9 - Takes a Type-9 record with NIST fields populated
#cat:              and converts the fields, populating FBI/IAFIS fields
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
      otype_9    - new Type-9 record with FBI/IAFIS fields populated
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_type_9(RECORD **otype_9, ANSI_NIST *ansi_nist,
                      const int type_9_i)
{
   int i, j, ret, field_i, rcflag, afis_rcflag;
   RECORD *type_9, *ntype_9;
   FIELD *field, *nfield;
   SUBFIELD *subfield, *nsubfield;
   ITEM *nitem;
   char *sysname, *method, *nmethod;
   char *class, *nclass, *type, *ntype, *afis_rc;
   int idc, ih;
   double ppmm;
   RECORD *imgrecord;
   int imgrecord_i, finger_pos, num_min, imp_code, quality;

   /* Make sure to build ANSI/NIST structures bottom up, so that */
   /* byte sizes (including separator characters) are computed   */
   /* correctly when appending structures.                       */

   /* If record index out of range ... */
   if((type_9_i < 1) || (type_9_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "Type 9 record index = %d out of range [2..%d]\n",
	      type_9_i+1, ansi_nist->num_records);
      return(-2);
   }
   /* Assign Type 9 record pointer. */
   type_9 = ansi_nist->records[type_9_i];
   /* If record is not a Type-9 ... */
   if(type_9->type != TYPE_9_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
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
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
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
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
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
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
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
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
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
   /* Create new FMT field with value = "U" for User-Defined fields. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, FMT_ID, "U"))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new FMT field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.014: FGN Field */
   /* Locate FGP2 (9.006) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FGP2_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "FGP2 field not found in input record [Type-9.%03d]\n", FGP2_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-10);
   }
   /* Check if position code is valid ... palm or finger. */
   finger_pos = atoi((char *)field->subfields[0]->items[0]->value);
   if(!(((finger_pos >= MIN_TABLE_6_CODE) &&
         (finger_pos <= MAX_TABLE_6_CODE)) ||
        ((finger_pos >= MIN_TABLE_19_CODE) &&
         (finger_pos <= MAX_TABLE_19_CODE)))){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "FGP = %d not a valid Table 6 or Table 19 code in "
	      "input record [Type-9.%03d]\n", finger_pos, FGP2_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-11);
   }
   /* Create new FGN field from FGP2 value. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, FGN_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new FGN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.015: NMN Field */
   /* Locate MIN (9.010) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, MIN_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "MIN field not found in input record [Type-9.%03d]\n", MIN_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-12);
   }
   /* If number of minutiae exceeds FBI/IAFIS limit ... */
   num_min = atoi((char *)field->subfields[0]->items[0]->value);
   if(num_min > MAX_IAFIS_MINUTIAE){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "MIN field [Type-9.%03d] Number of Minutiae "
	      "= %d > %d, exceeds FBI/IAFIS limit\n",
	      MIN_ID, num_min, MAX_IAFIS_MINUTIAE);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-13);
   }
   /* Create new NMN field from MIN value. */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, NMN_ID,
                        (char *)field->subfields[0]->items[0]->value))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new NMN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.016: FCP Field */
   /* Locate OFR (9.005) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, OFR_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "OFR field not found in input record [Type-9.%03d]\n", OFR_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-14);
   }
   if(field->subfields[0]->num_items != 2){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "OFR field does not contain two items in [Type-9.%03d]\n", OFR_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-15);
   }
   sysname = (char *)field->subfields[0]->items[0]->value;
   method = (char *)field->subfields[0]->items[1]->value;
   /* Create FCP subfield with first item == OFR System Name. */
   if((ret = biomeval_nbis_value2subfield(&nsubfield, sysname))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Create default second item (version) == "00". */
   if((ret = biomeval_nbis_value2item(&nitem, "00"))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append Version item to FCP subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
      biomeval_nbis_free_ANSI_NIST_item(nitem);
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Convert ANSI/NIST system method to FBI/IAFIS method. */
   if((ret = biomeval_nbis_nist2iafis_method(&nmethod, method))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   if((ret = biomeval_nbis_value2item(&nitem, nmethod))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append Method item to FCP subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
      biomeval_nbis_free_ANSI_NIST_item(nitem);
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* Allocate new FCP field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, FCP_ID))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append FCP field with new subfield. */
   if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new FCP field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.017: APC Field */
   /* Note: This is a manditory NIST field and an optional IAFIS field. */
   /* Locate FPC (9.007) field in input record. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FPC_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "FPC field not found in input record [Type-9.%03d]\n", FPC_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-16);
   }
   /* First subfield is different from the rest and should have 2 items. */
   if(field->subfields[0]->num_items != 2){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "primary pattern classification not found in "
	      "FPC field [Type-9.%03d]\n", FPC_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-17);
   }
   /* Only deal with Standard Table 8 class codes. */
   if(strcmp((char *)field->subfields[0]->items[0]->value, TBL_STR) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "Non-standard pattern classes not supported in "
	      "FPC field [Type-9.%03d]\n", FPC_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-18);
   }
   /* If number of pattern classes exceeds FBI/IAFIS limit ... */
   if(field->num_subfields > MAX_IAFIS_PATTERN_CLASSES){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "FPC field [Type-9.%03d] number of pattern classes "
	      "= %d > %d, exceeds FBI/IAFIS limit\n",
	      FPC_ID, field->num_subfields, MAX_IAFIS_PATTERN_CLASSES);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-19);
   }

   /* Foreach pattern class in FPC field ... */
   for(i = 0; i < field->num_subfields; i++){
      /* If primary classificaiton (1st time through loop) ... */
      if(i == 0){
         /* Primary pattern class is in 2nd item. */
         class = (char *)field->subfields[i]->items[1]->value;
         /* Allocate new APC field. */
         if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, APC_ID))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* All others classifications are in 1st item. */
      else
         class = (char *)field->subfields[i]->items[0]->value;
      /* Translate Standard class codes to IAFIS class codes. */
      if((ret = biomeval_nbis_nist2iafis_pattern_class(&nclass, class, finger_pos))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Create new APC subfield with next pattern classification. */
      if((ret = biomeval_nbis_value2subfield(&nsubfield, nclass))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* Create 2 default Ridge Count items and add to current */
      /* subfield. */
      for(j = 0; j < 2; j++){
         if((ret = biomeval_nbis_value2item(&nitem, "31"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append subfield with new Ridge Count item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append APC field with new subfield. */
      if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
   } /* Next pattern class in FPC. */
   /* Append record with new APC field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.018: ROV Field */
   /* Optional and not populated in this application. */

   /* 9.019: COF Field */
   /* Optional and not populated in this application. */

   /* 9.020: ORN Field */
   /* Create ORN field with default value == "0" */
   if((ret = biomeval_nbis_value2field(&nfield, TYPE_9_ID, ORN_ID, "0"))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Append record with new ORN field. */
   if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
      biomeval_nbis_free_ANSI_NIST_field(nfield);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }

   /* 9.021: CRA Field */
   /* Optional, so populate only if Core(s) reported in NIST fields. */
   /* Locate CRP field in input record. */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, CRP_ID, type_9)){
      /* If number of cores in CRP field within range of CRA field ... */
      if((field->num_subfields < 1) ||
         (field->num_subfields > MAX_IAFIS_CORES)) {
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Number of cores = %d out of range [1..%d] in "
		 "CRP field [Type-9.%03d], exceed FBI/IAFIS limit\n",
		 field->num_subfields, MAX_IAFIS_CORES, CRP_ID);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-20);
      }
      /* Allocate new CRA field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, CRA_ID))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* For each core reported in CRP field ... */
      for(i = 0; i < field->num_subfields; i++){
         /* Create new CRA subfield from next core's location. */
         if((ret = biomeval_nbis_value2subfield(&nsubfield, 
                             (char *)field->subfields[i]->items[0]->value))){
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* If core location string not 8 characters long ... */
         if(strlen((char *)nsubfield->items[0]->value) != 8){
            fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		    "Core location = %s is not format XXXXYYYY in"
		    "CRP field [Type-9.%03d]\n",
                    nsubfield->items[0]->value, CRP_ID);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(-21);
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
         /* Create Direction item with default value "000". */
         if((ret = biomeval_nbis_value2item(&nitem, "000"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append subfield with Direction item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Create Position Uncertainty item with default value "0000". */
         if((ret = biomeval_nbis_value2item(&nitem, "0000"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append subfield with Position Uncertainty item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append CRA field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append record with new CRA field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
        return(ret);
      }
   }

   /* 9.022: DLA Field */
   /* Optional, so populate only if Delta(s) reported in NIST fields. */
   /* Locate DLT field in input record. */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, DLT_ID, type_9)){
      /* If number of deltas in DLT field within range ... */
      if((field->num_subfields <= 0) ||
         (field->num_subfields > MAX_IAFIS_DELTAS)) {
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Number of deltas = %d out of range [1..%d] in "
		 "DLT field [Type-9.%03d], exceeds FBI/IAFIS limit\n",
		 field->num_subfields, MAX_IAFIS_DELTAS, DLT_ID);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-22);
      }
      /* Allocate new DLA field. */
      if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, DLA_ID))){
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* For each delta reported in DLT field ... */
      for(i = 0; i < field->num_subfields; i++){
         /* Create new DLA subfield from next delta's location. */
         if((ret = biomeval_nbis_value2subfield(&nsubfield, 
                            (char *)field->subfields[i]->items[0]->value))){
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* If delta location string not 8 characters long ... */
         if(strlen((char *)nsubfield->items[0]->value) != 8){
            fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		    "Delta location = %s is not format XXXXYYYY in"
		    "DLT field [Type-9.%03d]\n",
                    nsubfield->items[0]->value, DLT_ID);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(-23);
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
         /* Create default directions for Upward, Leftward, and Rightward. */
         for(j = 0; j < 3; j++){
            /* Create new Direction item with default value "000". */
            if((ret = biomeval_nbis_value2item(&nitem, "000"))){
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Append subfield with new Direction item. */
            if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
               biomeval_nbis_free_ANSI_NIST_item(nitem);
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
         }
         /* Create Position Uncertainty item with default value "0000". */
         if((ret = biomeval_nbis_value2item(&nitem, "0000"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append subfield with Position Uncertainty item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append DLA field with new subfield. */
         if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }
      /* Append record with new DLA field. */
      if((ret = biomeval_nbis_append_ANSI_NIST_record(ntype_9, nfield))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
        return(ret);
      }
   }

   /* 9.023: MAT Field */
   /* First determine if we have AFIS/FBI ridge counts stored in */
   /* MRC (9.012) field.                                         */
   /* Look up RDG (9.011) field. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, RDG_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "FDG field not found in input record [Type-9.%03d]\n", RDG_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-24);
   }
   rcflag = atoi((char *)field->subfields[0]->items[0]->value);
   /* If rcflag set and system name (which was already looked up */
   /* in OFR (9.005) field) is set to "AFIS/FBI" ... */
   if(rcflag && (strcmp(sysname, "AFIS/FBI") == 0))
      /* We have AFIS/FBI ridge counts to be converted and stored. */
      afis_rcflag = TRUE;
   else
      /* We don't have AFIS/FBI ridge counts. */
      afis_rcflag = FALSE;
   /* Look up manditory MRC (9.012) field. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, MRC_ID, type_9)){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
	      "MRC field not found in input record [Type-9.%03d]\n", MRC_ID);
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(-25);
   }
   /* Allocate new MAT field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&nfield, TYPE_9_ID, MAT_ID))){
      biomeval_nbis_free_ANSI_NIST_record(ntype_9);
      return(ret);
   }
   /* Foreach minutiae reported in MRC field ... */
   for(i = 0; i < field->num_subfields; i++){
      /* Set subfield pointer. */
      subfield = field->subfields[i];
      /* MRC minutiae subfields have 2 manditory items. */
      /* 1. Index  2. Location/Direction                */
      if(subfield->num_items < 2){
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Minutia %s has %d items < 2 in record [Type-9.%03d]\n",
                 subfield->items[0]->value, subfield->num_items, MRC_ID);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-26);
      }
      /* If ridge counts included, then subfield has at least 4 */
      /* manditory items.  1. Index  2. Location/Direction      */
      /* 3. Quality  4. Type  5. 0 or more Ridge Counts         */
      if(rcflag && (subfield->num_items < 4)){
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Minutia %s has %d items < 4 in record [Type-9.%03d]\n",
                 subfield->items[0]->value, subfield->num_items, MRC_ID);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-27);
      }
      /* If AFIS/FBI ridge counts, then MRC must have 4 manditory fields */
      /* plus 8 neighboring ridge counts. */
      if(afis_rcflag &&
         (subfield->num_items < (MAX_IAFIS_MINUTIA_ITEMS-1))){
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Minutia %s has %d items < %d in record [Type-9.%03d]\n",
                          subfield->items[0]->value, subfield->num_items,
                          MAX_IAFIS_MINUTIA_ITEMS-1, MRC_ID);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-28);
      }

      /* 1. Index Item */
      /* Create MAT subfield with first item == Index item. */
      j = 0;
      if((ret = biomeval_nbis_value2subfield(&nsubfield,
                               (char *)subfield->items[j++]->value))){
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }

      /* 2. Location/Direction Item */
      /* Create MAT subfield's second item from MRC XXXXYYYYTTT value. */
      if((ret = biomeval_nbis_value2item(&nitem, (char *)subfield->items[j++]->value))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
      /* If minutia's location string not 11 characters long ... */
      if(strlen((char *)nitem->value) != 11){
         fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		 "Minutia %s location = %s is not "
		 "format XXXXYYYYTTT in MRC field [Type-9.%03d]\n",
                 subfield->items[0]->value, nitem->value, MRC_ID);
         biomeval_nbis_free_ANSI_NIST_item(nitem);
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(-29);
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

      /* 3. Quality Item (conditionally optional) */
      /* If another item exists ... */
      if(j < subfield->num_items){
         /* Check if minutia quality is valid. */
         quality = atoi((char *)subfield->items[j]->value);
         if((quality < MIN_QUALITY_VALUE) ||
            (quality > MAX_QUALITY_VALUE)){
            fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_type_9 : "
		    "Minutia %s quality = %d out of range [%d..%d] "
		    "in [Type-9.%03d]\n",
		    subfield->items[0]->value, quality,
		    MIN_QUALITY_VALUE, MAX_QUALITY_VALUE, MRC_ID);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(-30);
         }
         /* Create MAT subfield's 3rd item from MRC Quality value. */
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
      }
      /* Otherwise, assign quality value = "01" for Quality Not Available. */
      else{
         /* Create MAT subfield's 3rd item with "unavailable" quality. */
         if((ret = biomeval_nbis_value2item(&nitem, "01"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append minutia subfield with default Quality item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }

      /* 4. Type Item (conditionally optional) */
      /* If another item exists ... */
      if(j < subfield->num_items){
         /* Translate Standard minutia type codes to IAFIS type codes. */
         type = (char *)subfield->items[j++]->value;
         if((ret = biomeval_nbis_nist2iafis_minutia_type(&ntype, type))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Create MAT subfield's 4th item from AFIS Type value. */
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
      }
      /* Otherwise, minutia type value = "C" for No Distinction Provided. */
      else {
         /* Create MAT subfield's 4th item with default minutia type = "C". */
         if((ret = biomeval_nbis_value2item(&nitem, "C"))){
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
         /* Append minutia subfield with default Minutia Type item. */
         if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
            biomeval_nbis_free_ANSI_NIST_item(nitem);
            biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
            biomeval_nbis_free_ANSI_NIST_field(nfield);
            biomeval_nbis_free_ANSI_NIST_record(ntype_9);
            return(ret);
         }
      }

      /* 5-12. Ridge Counts. */
      /* If AFIS/FBI ridge counts ... */
      if(afis_rcflag){
         while(j < subfield->num_items){
            if((ret = biomeval_nbis_nist2iafis_ridgecount(&afis_rc,
                                     (char *)subfield->items[j++]->value))){
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Create item from new AFIS ridge count. */
            if((ret = biomeval_nbis_value2item(&nitem, afis_rc))){
               free(afis_rc);
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Deallocate AFIS ridge count string. */
            free(afis_rc);
            /* Append minutia subfield with new Ridge Count item. */
            if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
               biomeval_nbis_free_ANSI_NIST_item(nitem);
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
         }
      }
      /* Otherwise, skip MRC ridge counts (if they even exist) and insert  */
      /* default value "25515" for each of the FBI/IAFIS octant neighbors. */
      else {
         /* Foreach octant neighbor... */
         for(j = 0; j < 8; j++){
            /* Create default item with value = "25515". */
            if((ret = biomeval_nbis_value2item(&nitem, "25515"))){
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
            /* Append minutia subfield with defalut Ridge Count item. */
            if((ret = biomeval_nbis_append_ANSI_NIST_subfield(nsubfield, nitem))){
               biomeval_nbis_free_ANSI_NIST_item(nitem);
               biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
               biomeval_nbis_free_ANSI_NIST_field(nfield);
               biomeval_nbis_free_ANSI_NIST_record(ntype_9);
               return(ret);
            }
         }
      }

      /* 13. Octant Residuals. */
      /* These are not recorded in the NIST Type-9 fields, so skip */
      /* as they are an optional information item to FBI/IAFIS.    */

      /* Append MAT field with new minutia subfield. */
      if((ret = biomeval_nbis_append_ANSI_NIST_field(nfield, nsubfield))){
         biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         biomeval_nbis_free_ANSI_NIST_field(nfield);
         biomeval_nbis_free_ANSI_NIST_record(ntype_9);
         return(ret);
      }
   } /* Next minutia */

   /* Append record with new MAT field. */
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
#cat: biomeval_nbis_nist2iafis_method - Takes an ANSI/NIST system method string
#cat:              and returns an FBI/IAFIS method following an
#cat:              "imperfect" mapping.

      ANSI/NIST METHODS        IAFIS METHODS
         "A"              ==>      "CAT"
         "E"              ==>      "NEV"
         "M"              ==>      "NMV"

   Input:
      method    - ANSI/NIST system method string
   Output:
      omethod   - points to FBI/IAFIS method
   Return Code:
      Zero      - successful completion
      Negative  - system error
************************************************************************/
int biomeval_nbis_nist2iafis_method(char **omethod, char *method)
{
   char *nmethod;

   if(strlen(method) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_method : "
	      "invalid method %s found : "
	      "ANSI/NIST method must be one 1 character\n", method);
      return(-2);
   }

   switch(*method){
      case 'A':
           nmethod = "CAT";
           break;
      case 'E':
           nmethod = "NEV";
           break;
      case 'M':
           nmethod = "NMV";
           break;
      default:
           fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_method : "
		   "invalid ANSI/NIST method %s found\n", method);
           return(-3);
   }

   /* Assign output pointer. */
   *omethod = nmethod;
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_nist2iafis_minutia_type - Takes an ANSI/NIST Table 7 minutia type
#cat:              and returns an FBI/IAFIS minutia type.
   
   Input:
      type      - ANSI/NIST minutia type from Table 7
   Output:
      otype     - points to FBI/IAFIS minutia type
   Return Code:
      Zero      - successful completion
      Negative  - system error
************************************************************************/
int biomeval_nbis_nist2iafis_minutia_type(char **otype, char *type)
{
   char *ntype;

   /* Ridge Ending */
   if(strcmp(type, "A") == 0){
      ntype = "A";
   /* Bifurcation */
   } else if(strcmp(type, "B") == 0){
      ntype = "B";
   /* Compound */
   } else if(strcmp(type, "C") == 0){
      ntype = "D";
   /* Type Undefined */
   } else if(strcmp(type, "D") == 0){
      ntype = "C";
   /* Illegal type */
   } else{
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_minutia_type : "
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
#cat: biomeval_nbis_nist2iafis_pattern_class - Takes an ANSI/NIST Table 8 pattern
#cat:              classification and a finger position (if known) and
#cat:              if possible returns an FBI/IAFIS pattern classification.
   
   Input:
      class      - ANSI/NIST pattern classification from Table 8
      finger_pos - ANSI/NIST finger position from Table 6
   Output:
      oclass     - points to FBI/IAFIS pattern classification
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_pattern_class(char **oclass, char *class, const int finger_pos)
{
   char *nclass = NULL;
   int ret, hand;

   /* Determine which hand the finger is on. */
   ret = biomeval_nbis_which_hand(finger_pos);
   /* If error ... */
   if(ret < 0)
      return(ret);
   /* Otherwise, hand is either specified or "unknown". */
   hand = ret;

   /* Plain Arch */
   if(strcmp(class, "PA") == 0){
      nclass = "AU";
   /* Tented Arch */
   } else if(strcmp(class, "TA") == 0){
      nclass = "AU";
   /* Radial Loop - points to thumb */
   } else if(strcmp(class, "RL") == 0){
      switch(hand){
         case UNKNOWN_HAND:
            /* If we don't know which hand, then we don't know */
            /* loop's direction, so set "unclassifiable". */
            nclass = "UC";
            break;
         case RIGHT_HAND:
            /* With right hand face down, pointing to thumb */
            /* is pointing to the left. */
            nclass = "LS";
           break;
         case LEFT_HAND:
            /* With left hand face down, pointing to thumb */
            /* is pointing to the right. */
            nclass = "RS";
            break;
      }
   /* Ulnar Loop - points to little finger */
   } else if(strcmp(class, "UL") == 0){
      switch(hand){
         case UNKNOWN_HAND:
            /* If we don't know which hand, then we don't know */
            /* loop's direction, so set "unclassifiable". */
            nclass = "UC";
            break;
         case RIGHT_HAND:
            /* With right hand face down, pointing to little finger */
            /* is pointing to the right. */
            nclass = "RS";
           break;
         case LEFT_HAND:
            /* With left hand face down, pointing to little finger */
            /* is pointing to the left. */
            nclass = "LS";
            break;
      }
   /* Plain Whorl */
   } else if(strcmp(class, "PW") == 0){
         nclass = "WU";
   /* Central Pocket Loop - subtype of whorl */
   } else if(strcmp(class, "CP") == 0){
         nclass = "WU";
   /* Double Loop - subtype of whorl */
   } else if(strcmp(class, "DL") == 0){
         nclass = "WU";
   /* Accidental Whorl */
   } else if(strcmp(class, "AW") == 0){
         nclass = "WU";
   /* Whorl, type not designated */
   } else if(strcmp(class, "WN") == 0){
      nclass = "WU";
   /* Right Slant Loop */
   } else if(strcmp(class, "RS") == 0){
      nclass = "RS";
   /* Left Slant Loop */
   } else if(strcmp(class, "LS") == 0){
      nclass = "LS";
   /* Scar */
   } else if(strcmp(class, "SR") == 0){
      nclass = "SR";
   /* Amputation */
   } else if(strcmp(class, "XX") == 0){
      nclass = "XX";
   /* Unknown or Unclassifiable */
   } else if(strcmp(class, "UN") == 0){
      nclass = "UC";
   /* Illegal pattern class */
   } else{
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_pattern_class : "
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
#cat: biomeval_nbis_nist2iafis_ridgecount - Takes a NIST formatted neighbor/ridge count
#cat:              string containing the neighbor index and ridge count
#cat:              separated by a comma, and generates an FBI/IAFIS
#cat:              formatted string that is fixed length with 3 characters
#cat:              containing the neighbor index immediately followed by
#cat:              2 characters containing the ridge count.
   
   Input:
      nist_rc    - NIST formatted neighbor/ridgecount string
   Output:
      oiafis_rc  - points to new FBI/IAFIS formatted string
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_nist2iafis_ridgecount(char **oiafis_rc, char *nist_rc)
{
   char *iafis_rc, *cptr, c;
   int nbr_i, rc;
   char uint_str1[MAX_UINT_CHARS+1], uint_str2[MAX_UINT_CHARS+1];

   /* Parse neighbor index. */
   if((cptr = index(nist_rc, ',')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_ridgecount : "
	      "parse error in string = %s, no comma found.\n", nist_rc);
      return(-2);
   }
   c = *cptr;
   *cptr = '\0';
   nbr_i = atoi(nist_rc);
   *cptr = c;
   cptr++;
   if(*cptr == '\0'){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_ridgecount : "
	      "parse error in string = %s, ridge count not found.\n", nist_rc);
      return(-3);
   }
   rc = atoi(cptr);

   /* Allocate IAFIS ridge count string "IIICC". */
   iafis_rc = (char *)calloc(6, sizeof(char));
   if(iafis_rc == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_ridgecount : "
	      "calloc : iafis_rc (%lu bytes)\n",
	      (unsigned long)(6 * sizeof(char)));
      return(-4);
   }
   /* Check format of neighbor index. */
   sprintf(uint_str1, "%03d", nbr_i);
   if(strlen(uint_str1) != 3){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_ridgecount : "
	      "neighbor index = %s longer than 3 characters\n", uint_str1);
      free(iafis_rc);
      return(-5);
   }
   /* Check format of ridge count. */
   sprintf(uint_str2, "%02d", rc);
   if(strlen(uint_str2) != 2){
      fprintf(stderr, "ERROR : biomeval_nbis_nist2iafis_ridgecount : "
	      "ridge count = %s longer than 2 characters\n", uint_str2);
      free(iafis_rc);
      return(-6);
   }
   /* Create FBI/IAFIS ridge count string. */
   sprintf(iafis_rc, "%03d%02d", nbr_i, rc);

   /* Set output pointer. */
   *oiafis_rc = iafis_rc;
   /* Return normally. */
   return(0);
}

