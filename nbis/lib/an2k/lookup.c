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

      FILE:    LOOKUP.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
               02/04/2008 by Joseph C. Konczal
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines for searching and locating specific field,
      subfield, and item structures in an ANSI/NIST file structure.
      Routines are also provided for search different types of records,
      such as image records.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_lookup_ANSI_NIST_field()
                        biomeval_nbis_lookup_ANSI_NIST_subfield()
                        biomeval_nbis_lookup_ANSI_NIST_item()
                        biomeval_nbis_lookup_ANSI_NIST_image()
                        biomeval_nbis_lookup_ANSI_NIST_image_ppmm()
                        biomeval_nbis_lookup_binary_field_image_ppmm()
                        biomeval_nbis_lookup_tagged_field_image_ppmm()
                        biomeval_nbis_lookup_ANSI_NIST_fingerprint()
                        biomeval_nbis_lookup_ANSI_NIST_grayprint()
                        biomeval_nbis_lookup_binary_field_fingerprint()
                        biomeval_nbis_lookup_tagged_field_fingerprint()
                        biomeval_nbis_lookup_fingerprint_with_IDC()
                        biomeval_nbis_lookup_FGP_field()
                        biomeval_nbis_lookup_IMP_field()
                        biomeval_nbis_lookup_minutiae_format()
                        biomeval_nbis_lookup_ANSI_NIST_record()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_field - Routine takes a field ID and searches
#cat:              the given record structure in order to locate the
#cat:              matching field.

   Input:
      field_int  - integer field ID to be matched
      record     - record structure to be searched
   Output:
      ofield     - points to matching field in record
      ofield_i   - index location of matching field in record
   Return Code:
      TRUE       - matching field located
      FALSE      - matching field NOT located
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_field(FIELD **ofield, int *const ofield_i,
			   const unsigned int field_int,
			   const RECORD *const record)
{
   int i;

   i = 0;
   while(i < record->num_fields){
      if(record->fields[i]->field_int == field_int){
         *ofield = record->fields[i];
         *ofield_i = i;
         return(TRUE);
      }
      i++;
   }

   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_subfield - Routine takes a subfield index and tests
#cat:              if the subfield structure exists in the given field.

   Input:
      subfield_index - integer subfield index to be found
      field          - field structure to be searched
   Output:
      osubfield      - points to matching subfield in field
   Return Code:
      TRUE       - matching subfield located
      FALSE      - matching subfield NOT located
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_subfield(SUBFIELD **osubfield,
			      const unsigned int subfield_index,
			      const FIELD *const field)
{
   if(subfield_index < field->num_subfields){
      *osubfield = field->subfields[subfield_index];
      return(TRUE);
   }

   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_item - Routine takes an information item index and
#cat:              tests if the item structure exists in the given subfield.

   Input:
      item_index - integer item index to be found
      subfield   - subfield structure to be searched
   Output:
      oitem      - points to matching item in subfield
   Return Code:
      TRUE       - matching item located
      FALSE      - matching item NOT located
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_item(ITEM **oitem, const unsigned int item_index,
			  const SUBFIELD *const subfield)
{
   if(item_index < subfield->num_items){
      *oitem = subfield->items[item_index];
      return(TRUE);
   }

   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_image - Routine takes an starting record index
#cat:              into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of an image record
#cat:              (Type-3,4,5,6,10,13,14,15,&16).  When found, it
#cat:              returns the record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_image(RECORD **oimgrecord, int *const oimgrecord_i,
                           const int strt_record,
			   const ANSI_NIST *const ansi_nist)
{
   int i, found;
   RECORD *record = NULL;

   /* Initiate search from specified record index. */
   i = strt_record;
   /* Ininitalize found flag to FALSE. */
   found = FALSE;
   /* While records remain ... */
   while(i < ansi_nist->num_records){
      /* Set temporary record pointer. */
      record = ansi_nist->records[i];
      /* If Type-3,4,5,6 ... */
      if(biomeval_nbis_binary_image_record(record->type)){
         /* Binary field image record found ... */
         found = TRUE;
         break;
      }
      /* If Type-10,13,14,15,16 ... */
      else if(tagged_image_record(ansi_nist->records[i]->type)){
         /* Tagged field image record found ... */
         found = TRUE;
         break;
      }
      else if(record->type == TYPE_8_ID){
         /* Otherwise post warning and ignore record. */
         fprintf(stderr, "WARNING : biomeval_nbis_lookup_ANSI_NIST_image : "
		 "Type-8 record [%d] not supported\n"
		 "Image record ignored.\n", i+1);
         /* Skip to next record ... */
      }
      /* Otherwise, go to next record ... */
      i++;
   }

   if(found){
      *oimgrecord = record;
      *oimgrecord_i = i;
      return(TRUE);
   }

   /* Otherwise, we did not find a suitable image record. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_image_ppmm - Routine takes an ANSI/NIST file
#cat:              structure and derives the scan resolution in units of
#cat:              pixels/mm for the specified image record.

   Input:
      ansi_nist   - ANSI/NIST file structure to be searched
      imgrecord_i - index of ANSI/NIST image record
   Output:
      oppmm      - points to derived scan resolution
   Return Code:
      Zero       - scan resolution sucessfully derived and returned
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_image_ppmm(double *const oppmm,
				const ANSI_NIST *const ansi_nist,
                                const int imgrecord_i)
{
   int ret;
   RECORD *imgrecord;
   double ppmm;

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_ANSI_NIST_image_ppmm : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   /* If binary field image record ... */
   if(biomeval_nbis_binary_image_record(imgrecord->type)){
      /* Lookup image record's pixel/mm scan resolution. */
      if((ret = biomeval_nbis_lookup_binary_field_image_ppmm(&ppmm, ansi_nist, imgrecord_i)))
         return(ret);
   }
   /* If tagged field image record ... */
   else if(tagged_image_record(imgrecord->type)){
      /* Lookup image record's pixel/mm scan resolution. */
      ret = biomeval_nbis_lookup_tagged_field_image_ppmm(&ppmm, imgrecord);
      /* If ERROR ... */
      if(ret < 0)
         return(ret);
      /* If IGNORE ... then ERROR. */
      else if(ret == FALSE){
         fprintf(stderr, "ERROR : biomeval_nbis_lookup_ANSI_NIST_image_ppmm : "
		 "biomeval_nbis_lookup_tagged_field_image_ppmm returned IGNORE : "
		 "treated by the caller as an ERROR\n");
         return(-3);
      }
      /* Otherwise, derived ppmm successfully. */
   }
   /* Otherwise, not an image record, so ERROR. */
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_ANSI_NIST_image_ppmm : "
	      "Record index [%d] [Type-%d] not an image record\n",
              imgrecord_i+1, imgrecord->type);
      return(-4);
   }

   /* Set output pointer. */
   *oppmm = ppmm;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_binary_field_image_ppmm - Routine takes an ANSI/NIST file
#cat:              structure and derives the scan resolution in units of
#cat:              pixels/mm for the specified binary field image record.

   Input:
      ansi_nist   - ANSI/NIST file structure to be searched
      imgrecord_i - index of binary field image record
   Output:
      oppmm      - points to derived scan resolution
   Return Code:
      Zero       - scan resolution sucessfully derived and returned
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_binary_field_image_ppmm(double *const oppmm,
				   const ANSI_NIST *const ansi_nist,
                                   const int imgrecord_i)
{
   double ppmm;
   float tfloat;
   RECORD *imgrecord;
   FIELD *field;
   int field_i;

   /* If image record index is out of range ... */
   if((imgrecord_i < 1) || (imgrecord_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_binary_field_image_ppmm : "
	      "record index [%d] out of range [1..%d]\n",
              imgrecord_i+1, ansi_nist->num_records+1);
      return(-2);
   }

   /* Set image record pointer. */
   imgrecord = ansi_nist->records[imgrecord_i];

   if(!biomeval_nbis_binary_image_record(imgrecord->type)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_binary_field_image_ppmm : "
	      "record index [%d] [Type-%d] \n"
	      "not a binary field image record\n",
	      imgrecord_i+1, imgrecord->type);
      return(-3);
   }

   /* Lookup NTR (Field 1.012). */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, NTR_ID,
                              ansi_nist->records[0])){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_binary_field_image_ppmm : "
	      "NTR field not found in record index [1] [Type-%d.%03d]\n",
              imgrecord->type, NTR_ID);
      return(-4);
   }

   /* Convert to double ppmm. */
   sscanf((char *)field->subfields[0]->items[0]->value, "%f", &tfloat);
   ppmm = tfloat;

   /* If low-resolution image record ... */
   if((imgrecord->type == TYPE_3_ID) ||
      (imgrecord->type == TYPE_5_ID))
      /* Then ppmm must be cut in half. */
      ppmm *= (double)0.5;

   /* Set output pointer. */
   *oppmm = ppmm;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_tagged_field_image_ppmm - Routine takes a tagged field
#cat:              image record and derives the scan resolution of the
#cat:              image in units of pixels/mm.

   Input:
      record     - structure containing tagged image record
   Output:
      oppmm      - points to derived scan resolution
   Return Code:
      TRUE       - scan resolution sucessfully derived and returned
      FALSE      - image record ignored
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_tagged_field_image_ppmm(double *const oppmm,
				   const RECORD *const record)
{
   FIELD *field;
   int field_i;
   int hps, vps, slc;
   double ppmm;

   /* Initialize return ppmm to UNSET. */
   *oppmm = (double)UNSET;

   /* Lookup HPS & VPS. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, HPS_ID, record)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_tagged_field_image_ppmm : "
	      "HPS field not found in image record [Type-%d.%03d]\n",
	      record->type, HPS_ID);
      return(-2);
   }
   hps = atoi((char *)field->subfields[0]->items[0]->value);

   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, VPS_ID, record)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_tagged_field_image_ppmm : "
	      "VPS field not found in imagerecord [Type-%d.%03d]\n",
	      record->type, VPS_ID);
      return(-3);
   }
   vps = atoi((char *)field->subfields[0]->items[0]->value);

   /* Code change by MDG on 07/02/04 to post warning with pixel aspect    */
   /* ratio != 1, but do not ignore, rather treat as if aspect ration = 1 */

   /* If pixel aspect ratio not 1, then ignore ... */
   if(hps != vps){
      /* Unsupported pixel aspect ratio, */
      /* so post warning, but continue with ratio = 1 assumption. */
      fprintf(stderr, "WARNING : biomeval_nbis_lookup_tagged_field_image_ppmm : "
	      "pixel aspect ratio != 1 not directly supported "
	      "in image record [Type-%d]\n"
	      "Will continue with operating assumpiton that "
	      "aspect ratio = 1.\n", record->type);
   }

   /* Lookup SLC. */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, SLC_ID, record)){
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_tagged_field_image_ppmm : "
	      "SLC field not found in image record [Type-%d.%03d]\n",
	      record->type, SLC_ID);
      return(-4);
   }
   slc = atoi((char *)field->subfields[0]->items[0]->value);

   switch(slc){
      /* If SLC not available (as typical of Type-10) ... */
      case 0:
         /* Set ppmm to UNSET. */
         ppmm = (double)UNSET;
         break;
      case 1:
         /* Then VPS in ppi, so convert to ppmm. */
         ppmm = vps / (double)MM_PER_INCH;
         break;
      case 2:
         /* Then VPS in ppcm, so convert to ppmm. */
         ppmm = vps / (double)10.0;
         break;
      default:
         fprintf(stderr, "ERROR : biomeval_nbis_lookup_tagged_field_image_ppmm : "
		 "illegal SLC code = %d in image record [Type-%d.%03d]\n",
		 slc, record->type, SLC_ID);
         return(-5);
   }

   /* Set output pointer. */
   *oppmm = ppmm;

   /* Return with ppmm successfully derived. */
   return(TRUE);

}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_fingerprint - Routine takes an starting record index
#cat:              into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of a fingerprint image record
#cat:              (Type-3,4,5,6,13,&14).  When found, it returns the
#cat:              record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_fingerprint(RECORD **oimgrecord, int *const oimgrecord_i,
                     const int strt_record, const ANSI_NIST *const ansi_nist)
{
   int i, ret;
   RECORD *imgrecord;
   int imgrecord_i;

   i = strt_record;
   while(i < ansi_nist->num_records){
      /* Look up next image record in ANSI/NIST file. */
      ret = biomeval_nbis_lookup_ANSI_NIST_image(&imgrecord, &imgrecord_i, i, ansi_nist);
      /* If error ... */
      if(ret < 0)
         return(ret);

       /* If image record not found ... */
      if(!ret)
         /* Then done and search failed. */
         return(FALSE);

      /* Otherwise, image record found ... */
      /* Is the image record a fingerprint? */
      switch(imgrecord->type){
      case TYPE_3_ID:
      case TYPE_4_ID:
      case TYPE_5_ID:
      case TYPE_6_ID:
      case TYPE_13_ID:
      case TYPE_14_ID:
         /* Then success. */
         *oimgrecord = imgrecord;
         *oimgrecord_i = imgrecord_i;
         return(TRUE);
      }

      /* Otherwise, image record found, but not a fingerprint, */
      /* so resume search one passed current record. */
      i = imgrecord_i + 1;
   }
   
   /* If exhaust all records, then grayscale record not found. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_grayprint - Routine takes an starting record index
#cat:              into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of an 8-bit grayscale fingerprint
#cat:              image record (Type-4,13,&14).  When found, it returns
#cat:              the record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_grayprint(RECORD **oimgrecord, int *const oimgrecord_i,
                     const int strt_record, const ANSI_NIST *const ansi_nist)
{
   int i, id, ret;
   RECORD *imgrecord;
   int imgrecord_i;
   FIELD *field;
   int field_i;

   i = strt_record;
   while(i < ansi_nist->num_records){
      /* Look up next image record in ANSI/NIST file. */
      ret = biomeval_nbis_lookup_ANSI_NIST_image(&imgrecord, &imgrecord_i, i, ansi_nist);
      /* If error ... */
      if(ret < 0)
         return(ret);

       /* If image record not found ... */
      if(!ret)
         /* Then done and search failed. */
         return(FALSE);

      /* Otherwise, image record found ... */
      /* Is the image record a grayscale fingerprint? */
      switch(imgrecord->type){
      /* Type-4 records are 8-bit grayscale. */
      case TYPE_4_ID:
         /* Then success. */
         *oimgrecord = imgrecord;
         *oimgrecord_i = imgrecord_i;
         return(TRUE);
      /* Type-13 & 14 records may have pixel depth other than 8, */
      /* so test and make sure 8-bit grayscale. */
      case TYPE_13_ID:
      case TYPE_14_ID:
         /* Lookup bits per pixel. */
         if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, BPX_ID, imgrecord)){
            fprintf(stderr, "ERROR : biomeval_nbis_lookup_ANSI_NIST_grayprint : "
		    "BPX field not found in record index [%d] [Type-%d.%03d]\n",
                    imgrecord_i+1, imgrecord->type, BPX_ID);
            return(-2);
         }
         /* Set image pixel depth = BPX_ID. */
         id = atoi((char *)field->subfields[0]->items[0]->value);

         /* If Type-13 or 14 record is 8-bit grayscale ... */
         if(id == 8){
            /* Then success. */
            *oimgrecord = imgrecord;
            *oimgrecord_i = imgrecord_i;
            return(TRUE);
         }
         /* Otherwise, not 8-bit gray so ignore record. */
         break;
      }
      /* Otherwise, image record found, but not grayscale fingerprint, */
      /* so resume search one passed current record. */
      i = imgrecord_i + 1;
   }
   
   /* If exhaust all records, then grayscale record not found. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_binary_field_fingerprint - Routine takes a starting record 
#cat:              index into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of a binary field fingerprint
#cat:              image record (Type-3,4,5,6).  When found, it returns
#cat:              the record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_binary_field_fingerprint(RECORD **oimgrecord,
				    int *const oimgrecord_i,
				    const int strt_record,
				    const ANSI_NIST *const ansi_nist)
{
   int i, ret;
   RECORD *imgrecord;
   int imgrecord_i;

   i = strt_record;
   while(i < ansi_nist->num_records){
      /* Look up next image record in ANSI/NIST file. */
      ret = biomeval_nbis_lookup_ANSI_NIST_image(&imgrecord, &imgrecord_i, i, ansi_nist);
      /* If error ... */
      if(ret < 0)
         return(ret);

       /* If image record not found ... */
      if(!ret)
         /* Then done and search failed. */
         return(FALSE);

      /* Otherwise, image record found ... */
      /* Is the image record a binary field fingerprint record? */
      switch(imgrecord->type){
      case TYPE_4_ID:
      case TYPE_6_ID:
         /* Then success. */
         *oimgrecord = imgrecord;
         *oimgrecord_i = imgrecord_i;
         return(TRUE);
      case TYPE_3_ID:
      case TYPE_5_ID:
         fprintf(stderr, "WARNING : biomeval_nbis_lookup_binary_field_fingerprint : "
		 "low resolution image record [%d] [Type-%d] not supported\n"
		 "Image record ignored.\n", imgrecord_i+1, imgrecord->type);
         break;
      }

      /* Otherwise, image record found, but not a supported binary field */
      /* fingerprint, so resume search one passed current record.        */
      i = imgrecord_i + 1;
   }
   
   /* If exhaust all records, then binary field fingerprint record */
   /* not found. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_tagged_field_fingerprint - Routine takes a starting record 
#cat:              index into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of a tagged field fingerprint
#cat:              image record (Type-13&14).  When found, it returns
#cat:              the record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_tagged_field_fingerprint(RECORD **oimgrecord,
				    int *const oimgrecord_i,
				    const int strt_record,
				    const ANSI_NIST *const ansi_nist)
{
   int i, ret;
   RECORD *imgrecord;
   int imgrecord_i;


   i = strt_record;
   while(i < ansi_nist->num_records){
      /* Look up next image record in ANSI/NIST file. */
      ret = biomeval_nbis_lookup_ANSI_NIST_image(&imgrecord, &imgrecord_i, i, ansi_nist);
      /* If error ... */
      if(ret < 0)
         return(ret);

       /* If image record not found ... */
      if(!ret)
         /* Then done and search failed. */
         return(FALSE);

      /* Otherwise, image record found ... */
      /* Is the image record a tagged field fingerprint record? */
      switch(imgrecord->type){
      case TYPE_13_ID:
      case TYPE_14_ID:
         /* Then success. */
         *oimgrecord = imgrecord;
         *oimgrecord_i = imgrecord_i;
         return(TRUE);
      case TYPE_16_ID:
         fprintf(stderr, "WARNING : biomeval_nbis_lookup_tagged_field_fingerprint : "
		 "Type-16 record found but currently unsupported\n"
		 "Image record ignored.\n");
         break;
      }

      /* Otherwise, image record found, but not a supported tagged field */
      /* fingerprint, so resume search one passed current record.        */
      i = imgrecord_i + 1;
   }
   
   /* If exhaust all records, then tagged field fingerprint record */
   /* not found. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_fingerprint_with_IDC - Routine takes an starting record index
#cat:              into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of a fingerprint image record
#cat:              with specified IDC value.  (Type-3,4,5,6,13,&14).
#cat:              When found, it returns the record structure.

   Input:
      idc         - IDC value to be matched
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_fingerprint_with_IDC(RECORD **orecord, int *const orecord_i, 
				const int idc, const int strt_record, 
				const ANSI_NIST *const ansi_nist)
{
   int i, ret, imgidc;
   RECORD *imgrecord;
   int imgrecord_i;
   FIELD *field;
   int field_i;
   

   i = strt_record;
   while(i < ansi_nist->num_records){
      ret = biomeval_nbis_lookup_ANSI_NIST_fingerprint(&imgrecord, &imgrecord_i,
                                         i, ansi_nist);
      /* If error ... */
      if(ret < 0)
         return(ret);
      /* If fingerprint image not found ... */
      if(!ret)
         return(FALSE);

      /* Otherwise, fingerprint image found, so lookup IDC field. */
      if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, IDC_ID, imgrecord)){
         fprintf(stderr, "ERROR : biomeval_nbis_lookup_fingerprint_with_IDC : "
		 "No IDC found in image record [Type-%d.%03d]\n",
		 imgrecord->type, IDC_ID);
         return(-2);
      }
      imgidc = atoi((char *)field->subfields[0]->items[0]->value);

      if(idc == imgidc){
         *orecord = imgrecord;
         *orecord_i = imgrecord_i;
         return(TRUE);
      }

      /* Otherwise, continue searching for next image record. */
      i = imgrecord_i + 1;
   }

   /* If exhaust all records, then matching fingerprint record not found. */
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_FGP_field - Examines the record type in order to
#cat:              determine whether it contains a FGP (Finger
#cat:              Position) or PMP (Palmprint Position) field, and
#cat:              where that field is located.

   Input:
      record     - record structure to be searched
   Output:
      ofield     - points to FGP field in record
      ofield_i   - index location of matching field in record
   Return Code:
      TRUE       - matching field located
      FALSE      - matching field NOT located
************************************************************************/
int biomeval_nbis_lookup_FGP_field(FIELD **ofield, int *const ofield_i,
		     const RECORD *const record)
{
   int fgp_field_id;
   FIELD *tfield;
   int tfield_i;
   char minfmt;

   switch (record->type) {
      /* these record types do not contain finger positions */
   case TYPE_1_ID:		/* transaction information */
   case TYPE_2_ID:		/* user-defined descriptive text */
   case TYPE_7_ID:		/* user-defined image */
   case TYPE_8_ID:		/* signature image */
   case TYPE_10_ID:		/* facial & SMT image record */
   case TYPE_11_ID:		/* reserved */
   case TYPE_12_ID:		/* reserved */
   case TYPE_16_ID:		/* user-defined variable-resolution image */
   case TYPE_17_ID:		/* iris image */
   case TYPE_99_ID:		/* CBEFF biometric data */
      return FALSE;
      
      /* specific-resolution fingerprint images */
   case TYPE_3_ID:		/* low-resolution grayscale image */
   case TYPE_4_ID:		/* high-resolution grayscale image */
   case TYPE_5_ID:		/* low-resolution binary image */
   case TYPE_6_ID:		/* high-resolution binary image */
      fgp_field_id = FGP_ID;
      break;
      
   case TYPE_9_ID:		/* minutiae */
      if (!biomeval_nbis_lookup_minutiae_format(&minfmt, record)) {
	 return FALSE;
      }
      if (minfmt != 'S') {
	 fprintf(stderr, "WARNING : biomeval_nbis_lookup_FGP_field : "
		 "minutiae type %c found but currently unsupported\n",
		 minfmt);
	 return FALSE;
      }

      /* only the standard format contains FGP in a known location */
      fgp_field_id = FGP2_ID;
      break;

      /* variable-resolution fingerprint images */
   case TYPE_13_ID:		/* variable-resolution latent image */
   case TYPE_14_ID:		/* variable-resolution fingerprint image */
   case TYPE_15_ID:		/* variable-resolution palmprint image */
      fgp_field_id = FGP3_ID;
		 break;
      
	 default:
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_FGP_field : "
	      "unsuported record type id : %d\n", record->type);
      return FALSE;
   }
   
   if (!biomeval_nbis_lookup_ANSI_NIST_field(&tfield, &tfield_i, fgp_field_id, record)) {
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_FGP_field : "
	      "cannot retrieve field %d\n", fgp_field_id);
      return FALSE;
   }
   *ofield = tfield;
   *ofield_i = tfield_i;
   return TRUE;
}

/***********************************************************************
************************************************************************
#cat: int biomeval_nbis_lookup_IMP_field - Routine examines the record type in order
#cat:              to determine whether it contains an IMP (Impression
#cat:              Type) field.

   Input:
      record     - record structure to be searched
   Output:
      ofield     - points to IMP field in record
      ofield_i   - index location of matching field in record
   Return Code:
      TRUE       - matching field located
      FALSE      - matching field NOT located
************************************************************************/
int biomeval_nbis_lookup_IMP_field(FIELD **ofield, int *const ofield_i,
		     const RECORD *const record)
{
   FIELD *tfield;
   int tfield_i;

   switch(record->type) {
            /* these record types do not necessarily contain fingers */
   case TYPE_1_ID:		/* transaction information */
   case TYPE_2_ID:		/* user-defined descriptive text */
   case TYPE_7_ID:		/* user-defined image */
   case TYPE_8_ID:		/* signature image */
   case TYPE_10_ID:		/* facial & SMT image record */
   case TYPE_11_ID:		/* reserved */
   case TYPE_12_ID:		/* reserved */
   case TYPE_16_ID:		/* user-defined variable-resolution image */
   case TYPE_17_ID:		/* iris image */
   case TYPE_99_ID:		/* CBEFF biometric data */
      return FALSE;
   
      /* specific-resolution fingerprint images */
   case TYPE_3_ID:		/* low-resolution grayscale image */
   case TYPE_4_ID:		/* high-resolution grayscale image */
   case TYPE_5_ID:		/* low-resolution binary image */
   case TYPE_6_ID:		/* high-resolution binary image */
      /* minutiae */
   case TYPE_9_ID:
      /* variable-resolution fingerprint images */
   case TYPE_13_ID:		/* variable-resolution latent image */
   case TYPE_14_ID:		/* variable-resolution fingerprint image */
   case TYPE_15_ID:		/* variable-resolution palmprint image */
      break;

   default:
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_IMP_field : "
	      "unknown record type id : %d\n", record->type);
      return FALSE;
   }
   
   if (!biomeval_nbis_lookup_ANSI_NIST_field(&tfield, &tfield_i, IMP_ID, record)) {
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_IMP_field : "
	      "cannot retrieve field %d\n", IMP_ID);
      return FALSE;
   }
   *ofield = tfield;
   *ofield_i = tfield_i;
   return TRUE;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_minutiae_format - Routine takes a pointer to a record and
#cat:              finds the minutia format, if it exists.

   Input:
      irecord    - the record to look in for the minutiae format
   Output:
      ofmt       - the single character minutiae format code, e.g., 'S'.
   Return Code:
      TRUE       - minutiae type found
      FALSE      - no minutiae type found, see error message for details
************************************************************************/
int biomeval_nbis_lookup_minutiae_format(char *const ofmt, const RECORD *const irecord) {
   FIELD *field;
   int field_i;

   if (irecord->type != TYPE_9_ID) {
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_minutiae_format"
	      " : type-%d records do not contain minutiae",
	      irecord->type);
      return FALSE;
   }
   if (!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, FMT_ID, irecord)) {
      fprintf(stderr, "ERROR : biomeval_nbis_lookup_minutiae_format"
	      " : cannot find FMT field");
      return FALSE;
   } 
   *ofmt = (char)*field->subfields[0]->items[0]->value;
   return TRUE;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_lookup_ANSI_NIST_record - Routine takes an starting record index
#cat:              into an ANSI/NIST structure and proceeds to search
#cat:              for the next occurrence of a record that satisfies
#cat:              the selection function.  When found, it returns the
#cat:              record structure.

   Input:
      strt_record - index from where to start record search
      ansi_nist   - ANSI/NIST structure to be searched
      sel_func    - function used to select the records returned
      sel_parm    - parameters for the selection function
   Output:
      oimgrecord - points to the image record structure
      oimgrecord_i - points to the index of the image record
   Return Code:
      TRUE       - image record found
      FALSE      - image record NOT found
      Negative   - system error
************************************************************************/
int biomeval_nbis_lookup_ANSI_NIST_record(RECORD **oimgrecord, int *const oimgrecord_i,
			    const int strt_record, 
			    const ANSI_NIST *const ansi_nist,
			    const REC_SEL *const sel_parm) {
   int rec_i;
   
   for (rec_i = strt_record; rec_i < ansi_nist->num_records; ++rec_i) {
      if (biomeval_nbis_select_ANSI_NIST_record(ansi_nist->records[rec_i], sel_parm)) {
	 /* return first match found */
	 *oimgrecord = ansi_nist->records[rec_i];
	 *oimgrecord_i = rec_i;
	 return TRUE;
      }
   }
   /* records exhausted, no match found */
   return FALSE;
}
