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
      LIBRARY: AN2K - ANSI/NIST 2007 Implementation

      FILE:    FMTSTD.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
      UPDATE:  10/10/2007 by (Kenneth Ko)
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  12/02/2008 by Kenneth Ko - Fix to support 64-bit
      UPDATE:  01/26/2009 by Joseph C. Konczal - improve handling of bad input

      Contains routines responsible for reading and writing files
      formatted according to the ANSI/NIST standard.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_read_ANSI_NIST_file()
                        biomeval_nbis_read_ANSI_NIST()
                        biomeval_nbis_read_Type1_record()
                        biomeval_nbis_read_ANSI_NIST_remaining_records()
                        biomeval_nbis_read_ANSI_NIST_record()
                        biomeval_nbis_read_ANSI_NIST_tagged_record()
                        biomeval_nbis_read_ANSI_NIST_record_length()
                        biomeval_nbis_read_ANSI_NIST_version()
                        biomeval_nbis_read_ANSI_NIST_integer_field()
                        biomeval_nbis_read_ANSI_NIST_remaining_fields()
                        biomeval_nbis_read_ANSI_NIST_field()
                        biomeval_nbis_read_ANSI_NIST_image_field()
                        biomeval_nbis_read_ANSI_NIST_tagged_field()
                        biomeval_nbis_read_ANSI_NIST_field_ID()
                        biomeval_nbis_parse_ANSI_NIST_field_ID()
                        biomeval_nbis_read_ANSI_NIST_subfield()
                        biomeval_nbis_read_ANSI_NIST_item()
                        biomeval_nbis_read_ANSI_NIST_binary_image_record()
                        biomeval_nbis_read_ANSI_NIST_binary_signature_record()
                        biomeval_nbis_read_ANSI_NIST_binary_field()
                        biomeval_nbis_write_ANSI_NIST_file()
                        biomeval_nbis_write_ANSI_NIST()
                        biomeval_nbis_write_ANSI_NIST_record()
                        biomeval_nbis_write_ANSI_NIST_tagged_field()
                        biomeval_nbis_write_ANSI_NIST_tagged_subfield()
                        biomeval_nbis_write_ANSI_NIST_tagged_item()
                        biomeval_nbis_write_ANSI_NIST_separator()
                        biomeval_nbis_write_ANSI_NIST_binary_field()
                        biomeval_nbis_write_ANSI_NIST_binary_subfield()
                        biomeval_nbis_write_ANSI_NIST_binary_item()

***********************************************************************/

#include <stdio.h>
#include <errno.h>
#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#include <limits.h>
#endif

#include <swap.h>
#include <an2k.h>

/*
 * Local functions to do the actual reading or scanning work.
 */
static int biomeval_nbis_i_read_ANSI_NIST_binary_field(FILE *fpin, AN2KBDB *buf,
    FIELD **ofield, const int num_bytes)
{
   int ret, i;
   unsigned char *itemvalue;
   ITEM *item;
   SUBFIELD *subfield;
   FIELD *field;

   /* Allocate new field ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)) != 0)
      return(ret);

   if(num_bytes <= sizeof(int)){
      if (fpin != NULL)
          ret = biomeval_nbis_read_binary_item_data(fpin, &itemvalue, num_bytes);
      else
          ret = biomeval_nbis_scan_binary_item_data(buf, &itemvalue, num_bytes);
      if(ret != 0){
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)) != 0){
         free(itemvalue);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      free(item->value);
      item->value = itemvalue;
      item->alloc_chars = strlen((char *)itemvalue)+1;
      /* Set number of characters in value buffer. */
      item->num_chars = item->alloc_chars;
      /* Set number of binary bytes read. */
      item->num_bytes = num_bytes;

      if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0){
         biomeval_nbis_free_ANSI_NIST_item(item);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      /* Add item to subfield ... */
      if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)) != 0){
         biomeval_nbis_free_ANSI_NIST_item(item);
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      /* Add subfield to the field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }
   }
   /* Otherwise, assume we have field composed of multiple single-byte     */
   /* items such as Type-3, FGP, which has up to 6 finger positions in it. */
   else{
      /* Allocate a single subfield to hold the multiple items. */
      if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0){
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }
      /* Add subfield to the field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      for(i = 0; i < num_bytes; i++){
          if (fpin != NULL)
              ret = biomeval_nbis_read_binary_item_data(fpin, &itemvalue, 1);
          else
              ret = biomeval_nbis_scan_binary_item_data(buf, &itemvalue, 1);
          if(ret != 0){
            biomeval_nbis_free_ANSI_NIST_field(field);
            return(ret);
         }

         if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)) != 0){
            free(itemvalue);
            biomeval_nbis_free_ANSI_NIST_field(field);
            return(ret);
         }

         free(item->value);
         item->value = itemvalue;
         item->alloc_chars = strlen((char *)itemvalue);
         /* Set number of characters in value buffer. */
         item->num_chars = item->alloc_chars;
         /* Set number of binary bytes read. */
         item->num_bytes = 1;

         /* Add item to subfield ... */
         if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)) != 0){
            biomeval_nbis_free_ANSI_NIST_item(item);
            biomeval_nbis_free_ANSI_NIST_subfield(subfield);
            biomeval_nbis_free_ANSI_NIST_field(field);
            return(ret);
         }

         /* Update size of field by number of bytes in new item. */
         field->num_bytes += item->num_bytes;
      }
   }

   *ofield = field;

   /* Return normally. */
   return(0);
}

static int biomeval_nbis_i_read_ANSI_NIST_binary_signature_record(FILE *fpin, AN2KBDB *buf,
    RECORD **orecord, const unsigned int record_type)
{
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   int ret, nread, record_bytes;
   char *errmsg;

   /* Allocate new record ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)) != 0){
      return(ret);
   }
   /* Assign type to record. */
   record->type = record_type;

   /* Read the LEN field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_LEN_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "LEN field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-2);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Store total number of bytes in record. */
   record->total_bytes = atoi((char *)field->subfields[0]->items[0]->value);
   /* Keep track of bytes left to read in record. */
   record_bytes = record->total_bytes - BINARY_LEN_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the IDC field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_IDC_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "IDC field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-3);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_IDC_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the SIG field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_SIG_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "SIG field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-4);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_SIG_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the SRT field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
      BINARY_SRT_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "SRT field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-5);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_SRT_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the ISR field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
      BINARY_ISR_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "ISR field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-6);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_ISR_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the HLL field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
      BINARY_HLL_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "HLL field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-7);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_HLL_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the VLL field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
      BINARY_VLL_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "VLL field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-8);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_VLL_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Allocate new item ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* If item's value not large enough to hold entire record ... */
   if(item->alloc_chars < record_bytes){
      unsigned char * new_ptr =
	 (unsigned char *)realloc(item->value, record_bytes);

      if(new_ptr == NULL){
         biomeval_nbis_free_ANSI_NIST_item(item);
         biomeval_nbis_free_ANSI_NIST_record(record);
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
		 "realloc : item->value (increase %d bytes to %d), "
		 "in record [Type-%d], at %ld\n",
		 item->alloc_chars, record_bytes, record_type,
		 biomeval_nbis_fbtell(fpin, buf));
         return(-2);
      }
      item->value = new_ptr;
      item->alloc_chars = record_bytes;
   }

   /* Read entire binary image record into item's value. */
   nread = biomeval_nbis_fbread(item->value, 1, (size_t)record_bytes, fpin, buf);
   if(nread != record_bytes){
      errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "read : only %d bytes read of %d: %s, at %ld\n",
	      nread, record_bytes, errmsg, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-3);
   }

   /* Set item's size attributes (there is no trailing delimiter). */
   item->num_bytes = record_bytes;
   item->num_chars = record_bytes;

   /* Allocate new subfield ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Add item to subfield ... */
   if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Allocate new field ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)) != 0){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;

   /* Add subfield to the field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Add field to the record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   if(record->total_bytes != record->num_bytes){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_signature_record : "
	      "only %d of total %d bytes read in record [Type-%d], "
	      "at %ld\n", record->num_bytes, record->total_bytes,
	      record_type, biomeval_nbis_fbtell(fpin, buf));
      return(-4);
   }

   *orecord = record;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_binary_image_record(FILE *fpin, AN2KBDB *buf,
    RECORD **orecord, const unsigned int record_type)
{
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   int ret, nread, record_bytes;
   char *errmsg;

   /* Allocate new record ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)) != 0){
      return(ret);
   }
   /* Assign type to record. */
   record->type = record_type;

   /* Read the LEN field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
      BINARY_LEN_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "LEN field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-2);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Store total number of bytes in record. */
   record->total_bytes = atoi((char *)field->subfields[0]->items[0]->value);
   /* Keep track of bytes left to read in record. */
   record_bytes = record->total_bytes - BINARY_LEN_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the IDC field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_IDC_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "IDC field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-3);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_IDC_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the IMP field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_IMP_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "IMP field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-4);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_IMP_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the FGP field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_FGP_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "FGP field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-5);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_FGP_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the ISR field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_ISR_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "ISR field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-6);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_ISR_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the HLL field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_HLL_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "HLL field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-7);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_HLL_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the VLL field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_VLL_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "VLL field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-8);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_VLL_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read the CA field ... */
   if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, buf, &field,
       BINARY_CA_BYTES)) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "CA field index [%d] not read in record [Type-%d]\n",
              record->num_fields+1, record_type);
      return(-9);
   }
   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;
   /* Keep track of bytes left to read in record. */
   record_bytes -= BINARY_CA_BYTES;
   /* Add field to record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Allocate new item ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* If item's value not large enough to hold entire record ... */
   if(item->alloc_chars < record_bytes){
      unsigned char * new_ptr =
	 (unsigned char *)realloc(item->value, (size_t)record_bytes);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
		 "realloc : item->value (increase %d bytes to %d), "
		 "in record [Type-%d], at %ld\n",
		 item->alloc_chars, record_bytes, record_type,
		 biomeval_nbis_fbtell(fpin, buf));
         biomeval_nbis_free_ANSI_NIST_item(item);
         biomeval_nbis_free_ANSI_NIST_record(record);
         return(-2);
      }
      item->value = new_ptr;
      item->alloc_chars = record_bytes;
   }

   /* Read entire binary image record into item's value. */
   nread = biomeval_nbis_fbread(item->value, 1, (size_t)record_bytes, fpin, buf);
   if(nread != record_bytes){
      errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "read record : only %d bytes read of %d, in record [Type-%d]: "
	      "%s, at %ld\n", nread, record_bytes, record_type,
	      errmsg, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-3);
   }

   /* Set item's size attributes (there is no trailing delimiter). */
   item->num_bytes = record_bytes;
   item->num_chars = record_bytes;

   /* Allocate new subfield ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Add item to subfield ... */
   if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Allocate new field ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)) != 0){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Set field attributes. */
   /* Let field->id empty. */
   field->record_type = record_type;
   field->field_int = record->num_fields+1;

   /* Add subfield to the field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   /* Add field to the record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   if(record->total_bytes != record->num_bytes){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_binary_image_record : "
	      "only %d of total %d bytes read in record [Type-%d], "
	      "at %ld\n", record->num_bytes, record->total_bytes,
	      record_type, biomeval_nbis_fbtell(fpin, buf));
      return(-4);
   }

   *orecord = record;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_item(FILE *fpin, AN2KBDB *buf, ITEM **oitem)
{
   ITEM *item;
   int ret, nextchar;
   char *errmsg;

   /* Allocate an item. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)) != 0)
      return(ret);

   while(1){
      /* If EOF, then error ... */
      if((nextchar = biomeval_nbis_fbgetc(fpin, buf)) == EOF){
	 errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_item : "
		 "getc item: %s, at %ld\n",
		 errmsg, biomeval_nbis_fbtell(fpin, buf));
         biomeval_nbis_free_ANSI_NIST_item(item);
         return(-2);
      }
      /* If delimiter is found ... */
      if(biomeval_nbis_is_delimiter(nextchar) != 0){
         /* If the trailing delimiter is a US separator ... */
         if(nextchar == US_CHAR){
            /* Set item's US separator flag to TRUE. */
            item->us_char = TRUE;
            /* Include trailing separator in byte size of item. */
            item->num_bytes++;
         }
         /* Done reading this item. */
         break;
      }
      /* Otherwise, add character to the item. */
      if((ret = biomeval_nbis_update_ANSI_NIST_item(item, nextchar)) != 0){
         biomeval_nbis_free_ANSI_NIST_item(item);
         return(ret);
      }
   }

   /* Assign new item to output pointer. */
   *oitem = item;
   /* Return the last delimiter read. */
   return(nextchar);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_subfield(FILE *fpin, AN2KBDB *buf,
    SUBFIELD **osubfield)
{
   int ret, ret_delimiter;
   SUBFIELD *subfield;
   ITEM *item;

   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0)
      return(ret);

   /* Continue until no more items remain in subfield. */
   while(1){
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_item(fpin, buf, &item);
      /* If error ... */
      if(ret_delimiter < 0){
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         return(ret_delimiter);
      }

      /* Otherwise new item read ... */

      /* Add new item to the subfield. */
      if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)) != 0){
         biomeval_nbis_free_ANSI_NIST_item(item);
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         return(ret);
      }

      /* Check if no more items remain after this one ... */
      if(ret_delimiter != US_CHAR){
         /* If the item's trailing delimiter is an RS separator ... */
         if(ret_delimiter == RS_CHAR){
            /* Set subfield's RS separator flag to TRUE. */
            subfield->rs_char = TRUE;
            /* Include trailing separator in byte size of subfield. */
            subfield->num_bytes++;
         }
         /* If item does not end with US separator, then done */
         /* with this subfield ...                           */
         break;
      }

      /* Otherwise, there is another item to read in current subfield. */
   }

   *osubfield = subfield;

   /* Return last delimiter read. */
   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_field_ID(FILE *fpin, AN2KBDB *buf, char **ofield_id,
    unsigned int *orecord_type, unsigned int *ofield_int)
{
   char *field_id, *iptr, *rptr, *fptr, *errmsg;
   int nextchar;
   int record_type = 0, field_int = 0;
   int field_id_bytes;
   int i, sep_found;

   /* Allocate maximum size field ID as twice the max number of characters  */
   /* for the field_int + 2 characters for the '.' and ':' */
   /* + 1 for the NULL terminator. */
   field_id_bytes = (2 * FIELD_NUM_LEN) + 3;

   /* Use calloc so that ID buffer is set to all zeros (NULL). */
   field_id = (char *)calloc(field_id_bytes, 1);
   if(field_id == NULL){
      errmsg = strerror(errno);
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
	      "calloc : field_id (%d bytes): %s, at %ld\n",
	      field_id_bytes, errmsg, biomeval_nbis_fbtell(fpin, buf));
      return(-2);
   }

   i = 0;
   /* Set current character pointer to start of field_id string. */
   iptr = field_id;
   /* Set current record number pointer to start of field_id string. */
   rptr = field_id;
   sep_found = FALSE;
   /* Parse until '.' is read. */
   while(i < FIELD_NUM_LEN+1){
      /* If EOF, then error ... */
      if((nextchar = biomeval_nbis_fbgetc(fpin, buf)) == EOF){
	 errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
		 "getc record_type digit: %s, at %ld\n",
		 errmsg, biomeval_nbis_fbtell(fpin, buf));
	 free(field_id);
         return(-3);
      }
      /* Otherwise, bump characters read. */
      i++;
      /* If '.' found, then complete record_type read. */
      if(nextchar == '.'){
         /* Convert characters read so far to integer. */
         record_type = atoi(rptr);
         /* Store the '.' in the ID string. */
         *iptr++ = nextchar;
         /* Set flag to TRUE. */
         sep_found = TRUE;
         /* Exit this loop. */
         break;
      }
      /* Otherwise, store the next character if it is numeric. */
     if((nextchar >= '0') && (nextchar <= '9'))
         *iptr++ = nextchar;
      else{
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
		 "record_type character '%c' (0x%02x) not numeric, "
		 "at %ld\n", nextchar, (unsigned int)nextchar,
		 biomeval_nbis_fbtell(fpin, buf));
	 free(field_id);
         return(-4);
      }
   }

   if(sep_found == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
	      "record integer not found in field ID, at %ld\n",
	      biomeval_nbis_fbtell(fpin, buf));
      free(field_id);
      return(-5);
   }

   /* Now, parse until ':' is read. */
   i = 0;
   /* Set field number pointer to next character in field_id string. */
   fptr = iptr;
   sep_found = FALSE;
   while(i < FIELD_NUM_LEN+1){
      /* If EOF, then error ... */
      if((nextchar = biomeval_nbis_fbgetc(fpin, buf)) == EOF){
	 errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
		 "getc field_int digit: %s, at %ld\n",
		 errmsg, biomeval_nbis_fbtell(fpin, buf));
         free(field_id);
         return(-6);
      }
      /* Otherwise, bump characters read. */
      i++;
      /* If ':' found, then complete field_int read. */
      if(nextchar == ':'){
         /* Convert to integer the characters read so far from the */
         /* field_int pointer. */
         field_int = atoi(fptr);
         /* Store the ':' in the ID string. */
         *iptr++ = nextchar;
         /* Set flag to TRUE. */
         sep_found = TRUE;
         /* Exit this loop. */
         break;
      }
      /* Otherwise, store the next character if it is numeric. */
      if((nextchar >= '0') && (nextchar <= '9'))
         *iptr++ = nextchar;
      else{
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
		 "field_int character '%c' (0x%02x) not numeric, "
		 "at %ld\n", nextchar, (unsigned int)nextchar,
		 biomeval_nbis_fbtell(fpin, buf));
         free(field_id);
         return(-7);
      }
   }

   if(sep_found == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field_ID : "
	      "field integer not found in field ID, at %ld\n",
	      biomeval_nbis_fbtell(fpin, buf));
      free(field_id);
      return(-8);
   }

   /* Assign results to output pointer(s). */
   *ofield_id = field_id;
   *orecord_type = record_type;
   *ofield_int = field_int;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_tagged_field(FILE *fpin, AN2KBDB *buf,
    FIELD **ofield, char *field_id, const int record_type,
    const int field_int, int record_bytes)
{
   int ret, ret_delimiter;
   FIELD *field;
   SUBFIELD *subfield;
   /* const size_t start_loc = ftell(fpin);*/

   /* Otherwise, field is textual ... */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)) != 0)
      return(ret);

   /* Otherwise field ID read. */
   /* Add ID to structrue. */
   field->id = field_id;
   field->record_type = record_type;
   field->field_int = field_int;

   /* Accumulate bytes in field. */
   field->num_bytes += strlen(field_id);

   /* Continue until no more subfields remain in field. */
   while(1){
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_subfield(fpin, buf, &subfield);
      /* If error ... */
      if(ret_delimiter < 0){
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret_delimiter);
      }

      /* Otherwise new subfield read ... */
      /* Add new subfield to field (includes accumulating subfield's bytes). */
      if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
         biomeval_nbis_free_ANSI_NIST_subfield(subfield);
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      /* Check if no more subfields remain after this one ... */
      if(ret_delimiter != RS_CHAR){
         /* If the subfield's trailing delimiter is a GS separator ... */
         if(ret_delimiter == GS_CHAR){
            /* Set field's GS separator flag to TRUE. */
            field->gs_char = TRUE;
            /* Include trailing separator in byte size of field. */
            field->num_bytes++;
         }
         /* If subfield does not end with RS separator, then done */
         /* with this field ...                               */
         break;
      }

      /* Otherwise, there is another subfield to read in current field. */
   }

   *ofield = field;

   /* Return last delimiter read. */
   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_image_field(FILE *fpin, AN2KBDB *buf,
    FIELD **ofield, char *field_id, const int record_type, const int field_int,
    int record_bytes)
{
   int ret, nread;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *image_item;
   int image_size;
   int delimiter;
   char *errmsg;

   /* Must read this field based on remaining bytes in record.  If */
   /* record bytes is passed UNSET, then it is an error.           */
   if(record_bytes == UNSET){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
	      "remaining bytes in record unspecified "
	      "in Type-%d record, at %ld\n", 
	      record_type, biomeval_nbis_fbtell(fpin, buf));
      return(-2);
   }

   /* Set image data size to record_bytes - field ID bytes - trailing FS */
   image_size = record_bytes - strlen(field_id) - 1;

   /* Added by jck on 2009-01-22 to fix trouble when the Type-1 record
      is not terminated with FS, before the error was caught sooner. */
   if (image_size < 0) {
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
	      "too few bytes %d remaining in Type-%d record, "
	      "image size %d, at %ld\n",
	      record_bytes, record_type, image_size, biomeval_nbis_fbtell(fpin, buf));
      return(-21);
   }

   /* Allocate image item with proper length value buffer. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&image_item)) != 0){
      return(ret);
   }
   if(image_item->alloc_chars < image_size){
      unsigned char * new_ptr =
	 (unsigned char *)realloc(image_item->value, (size_t)image_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
		 "realloc : image_item->value (increase %d bytes to %d), "
		 "in Type-%d record, at %ld\n",
		 image_item->alloc_chars, image_size, record_type,
		  biomeval_nbis_fbtell(fpin, buf));
         biomeval_nbis_free_ANSI_NIST_item(image_item);
         return(-3);
      }
      image_item->value = new_ptr;
      image_item->alloc_chars = image_size;
   }

   /* Read image item from file. */
   nread = biomeval_nbis_fbread(image_item->value, 1, (size_t)image_size, fpin, buf);
   if (nread != image_size){
      errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
	      "read image_item->value: only %d bytes read of %d, "
	      "in Type-%d record: %s, at %ld\n",
	      nread, image_size, record_type,
	      errmsg, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_item(image_item);
      return(-4);
   }
   image_item->num_bytes = image_size;
   image_item->num_chars = image_size;
   
   /* Read and check for terminating FS character. */
   if ((delimiter = biomeval_nbis_fbgetc(fpin, buf)) == EOF){
      errmsg = SHORT_SCAN_READ_ERR_MSG(fpin, buf);
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
	      "getc delimiter: in Type-%d record: %s, at %ld\n",
	      record_type, errmsg, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_item(image_item);
      return(-5);
   }

   if(delimiter != FS_CHAR){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_image_field : "
	      "image record terminated with 0x%02x not FS_CHAR (0x%02x), "
	      "in Type-%d record, at %ld\n",
	      (unsigned int)delimiter, FS_CHAR, record_type, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_item(image_item);
      return(-6);
   }

   /* Allocate a new subfield. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(image_item);
      return(ret);
   }

   /* Update subfield with image item. */
   if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, image_item)) != 0){
      biomeval_nbis_free_ANSI_NIST_item(image_item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      return(ret);
   }

   /* Allocate a new field. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)) != 0){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      return(ret);
   }
   /* Set field attributes. */
   field->id = field_id;
   field->record_type = record_type;
   field->field_int = field_int;
   field->num_bytes += strlen(field_id);

   /* Update field with subfield. */
   if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)) != 0){
      biomeval_nbis_free_ANSI_NIST_field(field);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      return(ret);
   }

   /* Assign new field to output pointer. */
   *ofield = field;

   /* Return FS character. */
   return(delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_field(FILE *fpin, AN2KBDB *buf,
    FIELD **ofield, int record_bytes)
{
   int ret, ret_delimiter;
   char *field_id;
   FIELD *field;
   unsigned int record_type, field_int;

   if (record_bytes < UNSET) {	/* assuming UNSET remains equal to -1 */
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_field : "
	      "too few bytes %d remaining in record, at %ld\n",
	      record_bytes, biomeval_nbis_fbtell(fpin, buf));
      return -1;
   }

   ret = biomeval_nbis_i_read_ANSI_NIST_field_ID(fpin, buf, &field_id, &record_type,
      &field_int);
   /* If error ... */
   if(ret < 0){
      return(ret);
   }

   /* If record_type = {10,13,14,15,16,17,99} and field_int is "999", */
   /* then binary image field.  Must read this field based on   */
   /* remaining bytes in record.                                */
   if((tagged_image_record(record_type) != 0) && (field_int == IMAGE_FIELD)){
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_image_field(fpin, buf, &field,
          field_id, record_type, field_int, record_bytes);
      if(ret_delimiter < 0)
         return(ret_delimiter);
   }
   else{
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_tagged_field(fpin, buf, &field,
         field_id, record_type, field_int, record_bytes);
      if(ret_delimiter < 0)
         return(ret_delimiter);
   }

   *ofield = field;

   /* Return last delimiter read. */
   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_remaining_fields(FILE *fpin, AN2KBDB *buf,
    RECORD *record)
{
   FIELD *field;
   int ret, ret_delimiter;

   /* Continue until no more fields remain in record. */
   while(1){
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_field(fpin, buf, &field,
          record->total_bytes - record->num_bytes);
      /* If error ... */
      if(ret_delimiter < 0){
         return(ret_delimiter);
      }

      /* Otherwise new field read ... */

      /* Add field to the record. */
      if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
         biomeval_nbis_free_ANSI_NIST_field(field);
         return(ret);
      }

      /* Check if no more fields remain after this one ... */
      if(ret_delimiter != GS_CHAR){
         /* If the field's trailing delimiter is an FS separator ... */
         if(ret_delimiter == FS_CHAR){
            /* Set record's FS separator flag to TRUE. */
            record->fs_char = TRUE;
            /* Include trailing separator in byte size of record. */
            record->num_bytes++;
         }

         /* If field does not end with GS separator, then done */
         /* with this record ...                               */
         break;
      }

      /* Otherwise, there is another field to read in current record. */

   }

   /* Return the last delimiter read. */
   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_integer_field(FILE *fpin, AN2KBDB *buf,
    int *ofield_value, FIELD **ofield)
{
   int ret_delimiter;
   FIELD *field;

   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_field(fpin, buf, &field, UNSET);
   /* If error ... */
   if(ret_delimiter < 0){
      return(ret_delimiter);
   }

   /* Otherwise, field was read... */
   if(field->num_subfields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_integer_field : "
	      "bad integer field format, %d subfields, should be 1, near %ld\n",
	      field->num_subfields, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_field(field);
      return -1;
   }
   if (field->subfields[0]->num_items != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_integer_field : "
	      "bad integer field format, %d items, should be 1, near %ld\n",
	      field->subfields[0]->num_items, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(-2);
   }

   /* Convert item's value to numeric integer. */
   *ofield_value = atoi((char *)field->subfields[0]->items[0]->value);
   *ofield = field;

   /* Return last delimiter read. */
   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_version(FILE *fpin, AN2KBDB *buf,
    int *oversion, FIELD **ofield)
{
   int ret_delimiter, version;
   FIELD *field;

   /* Read length (VER) field. */
   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_integer_field(fpin, buf,
       &version, &field);
   /* If error ... */
   if(ret_delimiter < 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_version : "
	      "Type-1 VER Field (1.%03d) version not parsed\n", VER_ID);
      return(ret_delimiter);
   }

   /* If field is not the version (VER) field ... */
   if(field->field_int != VER_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_version : "
	      "field int %d not %d\n", field->field_int, VER_ID);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(-2);
   }

   /* Get integer length of record. */
   *oversion = version;
   *ofield = field;

   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_record_length(FILE *fpin, AN2KBDB *buf,
    int *orecord_bytes, FIELD **ofield)
{
   int ret_delimiter, record_bytes;
   FIELD *field;

   /* Read length (LEN) field. */
   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_integer_field(fpin, buf,
        &record_bytes, &field);
   /* If error ... */
   if(ret_delimiter < 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_record_length : "
	      "record length not parsed\n");
      return(ret_delimiter);
   }

   /* If field is not a length (LEN) field ... */
   if(field->field_int != LEN_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_record_length : "
	      "field num %d, not %d as required, at %ld\n",
	      field->field_int, LEN_ID, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(-2);
   }

   *orecord_bytes = record_bytes;
   *ofield = field;

   return(ret_delimiter);
}

/*
 */
static int biomeval_nbis_i_read_Type1_record(FILE *fpin, AN2KBDB *buf, RECORD **orecord,
    unsigned int *oversion)
{
   int ret, ret_delimiter;
   int record_bytes, version;
   RECORD *record;
   FIELD *field;

   if((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)) != 0)
      return(ret);

   /* Read length (LEN) field. */
   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_record_length(fpin, buf,
        &record_bytes, &field);
   /* If error ... */
   if(ret_delimiter < 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret_delimiter);
   }

   /* If not desired record type ... */
   if(field->record_type != TYPE_1_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_read_Type1_record : "
	      "first record type %d, must be 1\n", field->record_type);
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(-2);
   }

   /* Assign type to record. */
   record->type = TYPE_1_ID;
   /* Store total number of bytes in record. */
   record->total_bytes = record_bytes;

   /* If length field does not end with GS then error. */
   if(ret_delimiter != GS_CHAR){
      fprintf(stderr, "ERROR : biomeval_nbis_read_Type1_record : length field (1.001) "
	      "terminated with 0x%02x, not GS_CHAR (0x%02x), at %ld\n",
	      (unsigned int)ret_delimiter, (unsigned int)GS_CHAR,
		 biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-3);
   }

   /* Add field to the record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Read version (VER) field. */
   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_version(fpin, buf, &version, &field);
   /* If error ... */
   if(ret_delimiter < 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret_delimiter);
   }

   /* If version field does not end with GS then error. */
   if(ret_delimiter != GS_CHAR){
      fprintf(stderr, "ERROR : biomeval_nbis_read_Type1_record : version field (1.002) "
	      "terminated with 0x%02x, not GS_CHAR (0x%02x), at %ld\n",
	      (unsigned int)ret_delimiter, (unsigned int)GS_CHAR,
	      biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-4);
   }

   /* Add field to the record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }
   
   switch(version){
      case VERSION_0200:
      case VERSION_0201:
      case VERSION_0300:
      case VERSION_0400:
      case VERSION_0500:
      case VERSION_0501:
      case VERSION_0502:
         break;

      default: 
         fprintf(stderr,
         "WARNING : biomeval_nbis_read_Type1_record : ANSI/NIST Version = %d Unsupported\n",
                 version);
         fprintf(stderr, "          Attempting to read ...\n");
         break;
    }
    ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_remaining_fields(fpin, buf, record);
    if(ret_delimiter < 0){
       biomeval_nbis_free_ANSI_NIST_record(record);
       return(ret_delimiter);
    }

   /* Last delimiter in record must be an FS character. */
   /* Note delimiter already added to record by call to */
   /* biomeval_nbis_read_ANSI_NIST_remaining_fields() above.         */
   if(ret_delimiter != FS_CHAR){
      fprintf(stderr, "ERROR : biomeval_nbis_read_Type1_record : "
	      "record terminated by 0x%02x not FS_CHAR (0x%02x), "
	      "byte %d of a %d byte record, at %ld\n",
	      (unsigned int)ret_delimiter, FS_CHAR, 
	      record->num_bytes, record->total_bytes, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-6);
   }

   if(record->total_bytes != record->num_bytes){
      fprintf(stderr, "ERROR : biomeval_nbis_read_Type1_record : "
	      "read %d bytes of a %d byte record, at %ld\n",
	      record->num_bytes, record->total_bytes, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-7);
   }

   if(record->num_fields < TYPE_1_NUM_MANDATORY_FIELDS){
      fprintf(stderr,"ERROR : biomeval_nbis_read_Type1_record : "
	      "record missing %d mandatory fields, at %ld\n",
	      TYPE_1_NUM_MANDATORY_FIELDS - record->num_fields,
	      biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-8);
   }

   /* Assign results to output pointer(s). */
   *orecord = record;
   *oversion = version;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_tagged_record(FILE *fpin, AN2KBDB *buf,
    RECORD **orecord, const unsigned int record_type)
{
   FIELD *field;
   RECORD *record;
   int ret, ret_delimiter, record_bytes;

   if((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)) != 0)
      return(ret);

   /* Read length (LEN) field. */
   ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_record_length(fpin, buf,
       &record_bytes, &field);
   /* If error ... */
   if(ret_delimiter < 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret_delimiter);
   }

   /* If not desired record type ... */
   if((record_type != UNSET) && (field->record_type != record_type)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_tagged_record : "
	      "Type-%d record found, not Type-%d as expected, at %ld\n",
	      field->record_type, record_type, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(-2);
   }

   /* Assign type to record. */
   record->type = record_type;
   /* Store total number of bytes in record. */
   record->total_bytes = record_bytes;

   /* If LEN is only field in record ... */
   if(ret_delimiter == FS_CHAR){
      /* Then FS will trail. */
      record->fs_char = TRUE;
      /* Need to bump number of record bytes read. */
      record->num_bytes++;
   }

   /* Add field to the record. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* If length field ends with GS ... */
   if(ret_delimiter == GS_CHAR){
      /* Parse remaining fields in the record. */
      ret_delimiter = biomeval_nbis_i_read_ANSI_NIST_remaining_fields(fpin, buf, record);
      /* If error ... */
      if(ret_delimiter < 0){
         biomeval_nbis_free_ANSI_NIST_record(record);
         return(ret_delimiter);
      }
   }

   /* We get here when the final field in the record has been read.       */
   /* Last delimiter read must be an FS character terminating the record. */
   /* Note delimiter already added to record by call to */
   /* biomeval_nbis_read_ANSI_NIST_remaining_fields() above.         */
   if(ret_delimiter != FS_CHAR){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_tagged_record : "
	      "record terminated by 0x%02x not FS_CHAR (0x%02x), "
	      "at byte %d of a %d byte record, at %ld\n",
	      (unsigned int)ret_delimiter, FS_CHAR,
	      record->num_bytes, record->total_bytes, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-3);
   }

   if((record->total_bytes != UNSET) &&
      (record->total_bytes != record->num_bytes)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_tagged_record : "
	      "only %d of total %d bytes read in Type-%d record, at %ld\n",
	      record->num_bytes, record->total_bytes,
	      record->type, biomeval_nbis_fbtell(fpin, buf));
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(-4);
   }

   /* Could add mandatory field checks here. */

   *orecord = record;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_record(FILE *fpin, AN2KBDB *buf, RECORD **orecord,
    const unsigned int record_type)
{
   RECORD *record;
   int ret;

   /* If tagged field record ... */
   if(biomeval_nbis_tagged_record(record_type) != 0){
      /* Read in tagged field record ... */
      if((ret = biomeval_nbis_i_read_ANSI_NIST_tagged_record(fpin, buf,
            &record, record_type)) != 0)
         return(ret);
   }
   /* If binary image record ... */
   else if(biomeval_nbis_binary_image_record(record_type) != 0){
      /* Read binary image fields and data. */
      if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_image_record(fpin, buf,
            &record, record_type)) != 0)
         return(ret);
   }
   /* If binary signature record ... */
   else if(biomeval_nbis_binary_signature_record(record_type) != 0){
      /* Read signature image fields and data. */
      if((ret = biomeval_nbis_i_read_ANSI_NIST_binary_signature_record(fpin, buf,
           &record, record_type)) != 0)
         return(ret);
   }
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_record : "
	      "unsupported record type %d, at %ld\n",
	      record_type, biomeval_nbis_fbtell(fpin, buf));
      return(-2);
   }

   *orecord = record;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST_remaining_records(FILE *fpin, AN2KBDB *buf,
    ANSI_NIST *ansi_nist)
{
   int ret, i;
   FIELD *cntfield;
   int num_records, cntfield_i;
   int record_type;
   RECORD *record;

   /* Need to have at least the Type-1 record already parsed. */
   if(ansi_nist->num_records < 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_remaining_records : "
	      "no records found\n");
      return(-2);
   }
   if(ansi_nist->records[0]->type != TYPE_1_ID){
      fprintf(stderr,"ERROR : biomeval_nbis_read_ANSI_NIST_remaining_records : "
	      "first record type %d, must be 1\n",
	      ansi_nist->records[0]->type);
      return(-3);
   }

   /* Lookup CNT field within Type-1 record. */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&cntfield, &cntfield_i, CNT_ID,
                              ansi_nist->records[0]) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_remaining_records : "
	      "Type-1 CNT Field (1.003) not found, at %ld\n",
	       biomeval_nbis_fbtell(fpin, buf));
      return(-4);
   }

   /* The number of subfields is the number of records in the file. */
   num_records = cntfield->num_subfields;

   /* Already read Type-1 record (perhaps more), so skip any already */
   /* read and begin parsing remaining records.                      */
   for(i = ansi_nist->num_records; i < num_records; i++){
      if(cntfield->subfields[i]->num_items != 2){
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_remaining_records : "
		 "Type-1 CNT Field (1.003) Subfield %d bad format: "
		 "number of items %d, not 2 as required\n",
		 i, cntfield->subfields[i]->num_items);
         return(-5);
      }
      /* Get integer type of current record. */
      record_type = atoi((char *)cntfield->subfields[i]->items[0]->value);

      /* Parse in the next record from the file with this type. */
      if((ret = biomeval_nbis_i_read_ANSI_NIST_record(fpin, buf, &record, record_type)) != 0)
         return(ret);

      /* Add the new record to the ANSI/NIST structure. */
      if((ret = biomeval_nbis_update_ANSI_NIST(ansi_nist, record)) != 0){
         biomeval_nbis_free_ANSI_NIST_record(record);
         return(ret);
      }
   }

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_ANSI_NIST(FILE *fpin, AN2KBDB *buf, ANSI_NIST *ansi_nist)
{
   int ret;
   unsigned int version;
   RECORD *record;
   FIELD *dcsfield;
   int dcsfield_i;
   int subfield;
   char dcsASCII_ID[4];

   /* Parse Type-1 record. */
   if((ret = biomeval_nbis_i_read_Type1_record(fpin, buf, &record, &version)) != 0) {
      return(ret);
   }
   ansi_nist->version = version;

   /* Check if DCS field is included in the record.  If it is, */
   /* then flag an error when not ASCII/7-bit English, because */
   /* Base-64 encoding of text is not currently supported.     */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&dcsfield, &dcsfield_i, DCS_ID, record) != 0){
      if (snprintf(dcsASCII_ID, 4, "%03d", ASCII_CSID) != 3) {
         biomeval_nbis_free_ANSI_NIST_record(record);
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST : DCS field (1.015) found: "
                         "error creating ASCII DCS ID\n");
         return(-2);
      }
      for (subfield = 0; subfield < dcsfield->num_subfields; ++subfield) {
         if (dcsfield->subfields[subfield]->num_items < 2) {
            biomeval_nbis_free_ANSI_NIST_record(record);
            fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST : DCS field (1.015) found: "
                            "invalid number of items in subfield\n");
            return(-2);
         }
         if (strncmp((char *)dcsfield->subfields[subfield]->items[0]->value,
           dcsASCII_ID, 3) != 0) {
            biomeval_nbis_free_ANSI_NIST_record(record);
            fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST : DCS field (1.015) found: "
                            "alternate character sets not supported\n");
            return(-2);
         }
      }
   }

   /* Add Type-1 record to ANSI/NIST structure. */
   if((ret = biomeval_nbis_update_ANSI_NIST(ansi_nist, record)) != 0){
      biomeval_nbis_free_ANSI_NIST_record(record);
      return(ret);
   }

   return biomeval_nbis_i_read_ANSI_NIST_remaining_records(fpin, buf, ansi_nist);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_file - Routine reads the contents of the specified
#cat:              ANSI/NIST filename into memory.

   Input:
      ifile      - name of file to be read
   Output:
      oansi_nist - points to resulting structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_file(const char *ifile, ANSI_NIST **oansi_nist)
{
   FILE *fpin;
   int ret;
   ANSI_NIST *ansi_nist;

   if((fpin = fopen(ifile, "rb")) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_file : fopen '%s': %s\n",
	      ifile, strerror(errno));
      return(-2);
   }

   if((ret = biomeval_nbis_alloc_ANSI_NIST(&ansi_nist)) != 0){
      if(fclose(fpin) != 0){
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_file : fclose '%s': %s\n",
		 ifile, strerror(errno));
      }      
      return(ret);
   }

   if((ret = biomeval_nbis_read_ANSI_NIST(fpin, ansi_nist)) != 0){
      /* biomeval_nbis_read_ANSI_NIST() has already output an appropriate error msgessage */
      biomeval_nbis_free_ANSI_NIST(ansi_nist);
      if(fclose(fpin) != 0){
         fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_file : fclose '%s': %s\n",
		 ifile, strerror(errno));
         return(-4);
      }
      return(ret);
   }

   if (fgetc(fpin) != EOF) {
      fprintf(stderr,
	      "ERROR : biomeval_nbis_read_ANSI_NIST_file : extra data starting at %s:%lu\n",
	      ifile, (long unsigned)ftell(fpin));
   }

   if(fclose(fpin) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_ANSI_NIST_file : fclose '%s': %s\n",
	      ifile, strerror(errno));
      return(-5);
   }

   /* Assign results to output pointer(s). */
   *oansi_nist = ansi_nist;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST - Routine reads the contents from an open file pointer
#cat:              into an ANSI/NIST file structure in memory.
#cat: biomeval_nbis_scan_ANSI_NIST - Routine reads the contents from an wrapped memory
#cat:              buffer into an ANSI/NIST file structure in memory.

   Input:
      fpin       - open ANSI/NIST file pointer
      buf        - a memory buffer wrapped in a basic_data_buffer
      ansi_nist  - empty initialized ANSI/NIST file structure
   Output:
      ansi_nist  - contains the read contents
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST(FILE *fpin, ANSI_NIST *ansi_nist)
{
	return (biomeval_nbis_i_read_ANSI_NIST(fpin, NULL, ansi_nist));
}

int biomeval_nbis_scan_ANSI_NIST(AN2KBDB *buf, ANSI_NIST *ansi_nist)
{
	return (biomeval_nbis_i_read_ANSI_NIST(NULL, buf, ansi_nist));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_Type1_record - Routine reads the contents of a Type-1 record
#cat:              from an open file pointer into memory.
#cat: biomeval_nbis_scan_Type1_record - Routine reads the contents of a Type-1 record
#cat:              from a wrapped buffer into memory.

   Input:
      fpin       - open ANSI/NIST file pointer
      buf        - a memory buffer wrapped in a basic_data_buffer
   Output:
      orecord    - points to created and filled record structure
      oversion   - integer contents of the VER field in the Type-1 record
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_Type1_record(FILE *fpin, RECORD **orecord, unsigned int *oversion)
{
	return (biomeval_nbis_i_read_Type1_record(fpin, NULL, orecord, oversion));
}

int biomeval_nbis_scan_Type1_record(AN2KBDB *buf, RECORD **orecord, unsigned int *oversion)
{
	return (biomeval_nbis_i_read_Type1_record(NULL, buf, orecord, oversion));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_remaining_records - Routine reads the contents of
#cat:              the logical records following the Type-1 record from
#cat:              an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_remaining_records - Routine reads the contents of
#cat:              the logical records following the Type-1 record from
#cat:              a wrapped buffer.

   Input:
      fpin       - open ANSI/NIST file pointer
      buf        - a memory buffer wrapped in a basic_data_buffer
      ansi_nist  - ANSI/NIST file structure containg file's contents
                   to this point.
   Output:
      ansi_nist  - updated to contain the complete contents of the file
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_remaining_records(FILE *fpin, ANSI_NIST *ansi_nist)
{
	return (biomeval_nbis_i_read_ANSI_NIST_remaining_records(fpin, NULL, ansi_nist));
}

int biomeval_nbis_scan_ANSI_NIST_remaining_records(AN2KBDB *buf, ANSI_NIST *ansi_nist)
{
	return (biomeval_nbis_i_read_ANSI_NIST_remaining_records(NULL, buf, ansi_nist));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_record - Routine reads the contents of a logical
#cat:              record from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_record - Routine reads the contents of a logical
#cat:              record from a wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
      record_type - integer type of record to be read
   Output:
      orecord    - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_record(FILE *fpin, RECORD **orecord,
    const unsigned int record_type)
{
	return (biomeval_nbis_i_read_ANSI_NIST_record(fpin, NULL, orecord, record_type));
}

int biomeval_nbis_scan_ANSI_NIST_record(AN2KBDB *buf, RECORD **orecord,
    const unsigned int record_type)
{
	return (biomeval_nbis_i_read_ANSI_NIST_record(NULL, buf, orecord, record_type));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_tagged_record - Routine reads the contents of a logical
#cat:              tagged record from an open ANSI/NIST file pointer.
#cat:              A tagged record is a record containing tagged fields.
#cat: biomeval_nbis_scan_ANSI_NIST_tagged_record - Routine reads the contents of a logical
#cat:              tagged record from a wrapped buffer.
#cat:              A tagged record is a record containing tagged fields.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
      record_type - integer type of record to be read
   Output:
      orecord    - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_tagged_record(FILE *fpin, RECORD **orecord,
                                const unsigned int record_type)
{
    return (biomeval_nbis_i_read_ANSI_NIST_tagged_record(fpin, NULL, orecord, record_type));
}

int biomeval_nbis_scan_ANSI_NIST_tagged_record(AN2KBDB *buf, RECORD **orecord,
                                const unsigned int record_type)
{
    return (biomeval_nbis_i_read_ANSI_NIST_tagged_record(NULL, buf, orecord, record_type));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_record_length - Routine reads the integer contents of
#cat:              a record's tagged LEN field from an open ANSI/NIST
#cat:              file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_record_length - Routine reads the integer contents of
#cat:              a record's tagged LEN field from a wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
   Output:
      orecord_bytes - points to LEN field's integer value
      ofield        - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_record_length(FILE *fpin, int *orecord_bytes, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_record_length(fpin, NULL, orecord_bytes, ofield));
}

int biomeval_nbis_scan_ANSI_NIST_record_length(AN2KBDB *buf, int *orecord_bytes, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_record_length(NULL, buf, orecord_bytes, ofield));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_version - Routine reads the integer contents of
#cat:              a record's tagged VER field from an open ANSI/NIST
#cat:              file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_version - Routine reads the integer contents of
#cat:              a record's tagged VER field from a wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
   Output:
      oversion    - points to VER field's integer value
      ofield       - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_version(FILE *fpin, int *oversion, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_version(fpin, NULL, oversion, ofield));
}

int biomeval_nbis_scan_ANSI_NIST_version(AN2KBDB *buf, int *oversion, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_version(NULL, buf, oversion, ofield));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_integer_field - Routine reads the integer contents of
#cat:              a tagged field from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_integer_field - Routine reads the integer contents of
#cat:              a tagged field from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
   Output:
      ofield_value - points to the field's integer value
      ofield       - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_integer_field(FILE *fpin, int *ofield_value, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_integer_field(fpin, NULL, ofield_value, ofield));
}

int biomeval_nbis_scan_ANSI_NIST_integer_field(AN2KBDB *buf, int *ofield_value, FIELD **ofield)
{
    return (biomeval_nbis_i_read_ANSI_NIST_integer_field(NULL, buf, ofield_value, ofield));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_remaining_fields - Routine reads the remaining tagged
#cat:               fields from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_remaining_fields - Routine reads the remaining tagged
#cat:               fields from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
      record       - contents of record read to this point
   Output:
      record       - record structure updated with remaining fields read
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_remaining_fields(FILE *fpin, RECORD *record)
{
    return (biomeval_nbis_i_read_ANSI_NIST_remaining_fields(fpin, NULL, record));
}

int biomeval_nbis_scan_ANSI_NIST_remaining_fields(AN2KBDB *buf, RECORD *record)
{
    return (biomeval_nbis_i_read_ANSI_NIST_remaining_fields(NULL, buf, record));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_field - Routine reads the contents of a tagged
#cat:               field from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_field - Routine reads the contents of a tagged
#cat:               field from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
      record_bytes - number of bytes remaining to be read in record
                     the current field is in
   Output:
      ofield       - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_field(FILE *fpin, FIELD **ofield, int record_bytes)
{
    return (biomeval_nbis_i_read_ANSI_NIST_field(fpin, NULL, ofield, record_bytes));
}

int biomeval_nbis_scan_ANSI_NIST_field(AN2KBDB *buf, FIELD **ofield, int record_bytes)
{
    return (biomeval_nbis_i_read_ANSI_NIST_field(NULL, buf, ofield, record_bytes));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_image_field - Routine reads the binary image data
#cat:               stored in a tagged image field from an open ANSI/NIST
#cat:               file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_image_field - Routine reads the binary image data
#cat:               stored in a tagged image field from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
      record_type  - integer type of record of field to be read
      field_int    - ID number of field to be read
      record_bytes - number of bytes remaining to be read in record
                     the current field is in
   Output:
      ofield       - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_image_field(FILE *fpin, FIELD **ofield,
    char *field_id, const int record_type, const int field_int,
    int record_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_image_field(fpin, NULL, ofield, field_id,
        record_type, field_int, record_bytes));
}

int biomeval_nbis_scan_ANSI_NIST_image_field(AN2KBDB *buf, FIELD **ofield,
    char *field_id, const int record_type, const int field_int,
    int record_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_image_field(NULL, buf, ofield, field_id,
        record_type, field_int, record_bytes));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_tagged_field - Routine reads the ASCII data
#cat:               stored in a tagged field from an open ANSI/NIST
#cat:               file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_tagged_field - Routine reads the ASCII data
#cat:               stored in a tagged field from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
      record_type  - integer type of record of field to be read
      field_int    - ID number of field to be read
      record_bytes - number of bytes remaining to be read in record
                     the current field is in
   Output:
      ofield       - points to created and filled field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_tagged_field(FILE *fpin, FIELD **ofield,
    char *field_id, const int record_type,
    const int field_int, int record_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_tagged_field(fpin, NULL, ofield,
        field_id, record_type, field_int, record_bytes));
}

int biomeval_nbis_scan_ANSI_NIST_tagged_field(AN2KBDB *buf, FIELD **ofield,
    char *field_id, const int record_type,
    const int field_int, int record_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_tagged_field(NULL, buf, ofield,
        field_id, record_type, field_int, record_bytes));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_field_ID - Routine reads the tag for a tagged field
#cat:               from an open ANSI/NIST file pointer.  A tag is
#cat:               comprised of the "<record type>.<field ID>:".  This
#cat:               routine parses and the record type separate from
#cat:               the field ID.
#cat: biomeval_nbis_scan_ANSI_NIST_field_ID - Routine reads the tag for a tagged field
#cat:               from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
   Output:
      ofield_id    - the entire tag string read
      orecord_type - integer type of record parsed from the tag
      ofield_int   - integer ID of field parsed from the tag
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_field_ID(FILE *fpin, char **ofield_id,
    unsigned int *orecord_type, unsigned int *ofield_int)
{
    return(biomeval_nbis_i_read_ANSI_NIST_field_ID(fpin, NULL, ofield_id,
        orecord_type, ofield_int));
}

int biomeval_nbis_scan_ANSI_NIST_field_ID(AN2KBDB *buf, char **ofield_id,
    unsigned int *orecord_type, unsigned int *ofield_int)
{
    return(biomeval_nbis_i_read_ANSI_NIST_field_ID(NULL, buf, ofield_id,
        orecord_type, ofield_int));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_parse_ANSI_NIST_field_ID - Routine parses the tag for a tagged field
#cat:               from a character buffer allegedly filled with bytes
#cat:               from read from an ANSI/NIST file.  A tag is
#cat:               comprised of the "<record type>.<field ID>:".  This
#cat:               routine parses and the record type separate from
#cat:               the field ID.  If the expected field ID is not found
#cat:               during the parse, this routine returns "unsuccessful".

   Input:
      oibufptr     - current pointer to start parse in input buffer
      ebufptr      - end of input buffer
   Output:
      oibufptr     - current pointer after parse to input buffer
      ofield_id    - the entire tag string read
      orecord_type - integer type of record parsed from the tag
      ofield_int   - integer ID of field parsed from the tag
   Return Code:
      TRUE       - successful completion
      FALSE      - unsuccessful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_parse_ANSI_NIST_field_ID(unsigned char **oibufptr, unsigned char *ebufptr,
       char **ofield_id, unsigned int *orecord_type, unsigned int *ofield_int)
{
   char *field_id, *iptr, *rptr, *fptr;
   unsigned char *cptr;
   int nextchar;
   unsigned int record_type = 0, field_int = 0;
   int field_id_bytes;
   int i, sep_found;

   /* Allocate maximum size field ID as twice the max number of characters  */
   /* for the field_int + 2 characters for the '.' and ':' */
   /* + 1 for the NULL terminator. */
   field_id_bytes = (2 * FIELD_NUM_LEN) + 3;

   /* Use calloc so that ID buffer is set to all zeros (NULL). */
   field_id = (char *)calloc((size_t)field_id_bytes, 1);
   if(field_id == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_parse_ANSI_NIST_field_ID : "
	      "calloc field_id (%d bytes)\n", field_id_bytes);
      return(-2);
   }

   i = 0;
   cptr = *oibufptr;
   /* Set current character pointer to start of field_id string. */
   iptr = field_id;
   /* Set current record number pointer to start of field_id string. */
   rptr = field_id;
   sep_found = FALSE;
   /* Parse until '.' is read. */
   while(i < FIELD_NUM_LEN+1){

      /* If EOB, then done & unsuccessful ... */
      if(cptr >= ebufptr){
         free(field_id);
         return(FALSE);
      }
      nextchar = *cptr;
      cptr++;

      /* Otherwise, bump characters read. */
      i++;
      /* If '.' found, then complete record_type read. */
      if(nextchar == '.'){
         /* Convert characters read so far to integer. */
         record_type = (unsigned int)atoi(rptr);
         /* Store the '.' in the ID string. */
         *iptr++ = nextchar;
         /* Set flag to TRUE. */
         sep_found = TRUE;
         /* Exit this loop. */
         break;
      }
      /* Otherwise, store the next character if it is numeric. */
      if((nextchar >= '0') && (nextchar <= '9'))
         *iptr++ = nextchar;
      else{
         /* Not numeric text, so not a field_id, so done & unsuccessful */
         free(field_id);
         return(FALSE);
      }
   }

   if(sep_found == 0){
      /* then not a field_id, so done & unsuccessful */
      free(field_id);
      return(FALSE);
   }

   /* Now, parse until ':' is read. */
   i = 0;
   /* Set field number pointer to next character in field_id string. */
   fptr = iptr;
   sep_found = FALSE;
   while(i < FIELD_NUM_LEN+1){
      /* If EOB, then done & unsuccessful ... */
      if(cptr >= ebufptr){
         free(field_id);
         return(FALSE);
      }
      nextchar = *cptr++;

      /* Otherwise, bump characters read. */
      i++;
      /* If ':' found, then complete field_int read. */
      if(nextchar == ':'){
         /* Convert to integer the characters read so far from the */
         /* field_int pointer. */
         field_int = (unsigned int)atoi(fptr);
         /* Store the ':' in the ID string. */
         *iptr++ = nextchar;
         /* Set flag to TRUE. */
         sep_found = TRUE;
         /* Exit this loop. */
         break;
      }
      /* Otherwise, store the next character if it is numeric. */
      if((nextchar >= '0') && (nextchar <= '9'))
         *iptr++ = nextchar;
      else{
         /* Not numeric text, so not a field_id, so done & unsuccessful */
         free(field_id);
         return(FALSE);
      }
   }

   if(sep_found == 0){
      /* then not a field_id, so done & unsuccessful */
      free(field_id);
      return(FALSE);
   }

   /* Assign results to output pointer(s). */
   *oibufptr = cptr;
   *ofield_id = field_id;
   *orecord_type = record_type;
   *ofield_int = field_int;

   /* Return successfully. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_subfield - Routine reads the contents of a subfield
#cat:               from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_subfield - Routine reads the contents of a subfield
#cat:               from a wrapped buffer.

   Input:
      fpin         - open ANSI/NIST file pointer
      buf          - a memory buffer wrapped in a basic_data_buffer
   Output:
      osubfield    - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_subfield(FILE *fpin, SUBFIELD **osubfield)
{
    return(biomeval_nbis_i_read_ANSI_NIST_subfield(fpin, NULL, osubfield));
}

int biomeval_nbis_scan_ANSI_NIST_subfield(AN2KBDB *buf, SUBFIELD **osubfield)
{
    return(biomeval_nbis_i_read_ANSI_NIST_subfield(NULL, buf, osubfield));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_item - Routine reads the contents of an information
#cat:               item from an open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_item - Routine reads the contents of an information
#cat:               item from a wrapped buffer.

   Input:
      fpin       - open ANSI/NIST file pointer
      buf        - a memory buffer wrapped in a basic_data_buffer
   Output:
      oitem      - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_item(FILE *fpin, ITEM **oitem)
{
    return(biomeval_nbis_i_read_ANSI_NIST_item(fpin, NULL, oitem));
}

int biomeval_nbis_scan_ANSI_NIST_item(AN2KBDB *buf, ITEM **oitem)
{
    return(biomeval_nbis_i_read_ANSI_NIST_item(NULL, buf, oitem));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_binary_image_record - Routine reads the contents
#cat:               of a binary image record (Type-3,4,5,6) from an
#cat:               open ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_binary_image_record - Routine reads the contents
#cat:               of a binary image record (Type-3,4,5,6) from a
#cat                wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
      record_type - integer type of record to be read
   Output:
      orecord     - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_binary_image_record(FILE *fpin, RECORD **orecord,
    const unsigned int record_type)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_image_record(fpin, NULL,
        orecord, record_type));
}

int biomeval_nbis_scan_ANSI_NIST_binary_image_record(AN2KBDB *buf, RECORD **orecord,
    const unsigned int record_type)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_image_record(NULL, buf,
        orecord, record_type));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_binary_signature_record - Routine reads the contents
#cat:               of a binary signature record (Type-8) from an open
#cat:               ANSI/NIST file pointer.
#cat: biomeval_nbis_scan_ANSI_NIST_binary_signature_record - Routine reads the contents
#cat:               of a binary signature record from a wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
      record_type - integer type of record to be read
   Output:
      orecord     - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_binary_signature_record(FILE *fpin,
    RECORD **orecord, const unsigned int record_type)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_signature_record(fpin, NULL,
        orecord, record_type));
}

int biomeval_nbis_scan_ANSI_NIST_binary_signature_record(AN2KBDB *buf,
    RECORD **orecord, const unsigned int record_type)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_signature_record(NULL, buf,
        orecord, record_type));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_ANSI_NIST_binary_field - Routine reads the contents of a
#cat:               binary record's field from an open ANSI/NIST file
#cat:               pointer.  These binary fields do not have tags and
#cat:               are in a fixed format in terms of size and order
#cat:               within the record.
#cat: biomeval_nbis_scan_ANSI_NIST_binary_field - Routine reads the contents of a
#cat:               binary record's field from a wrapped buffer.

   Input:
      fpin        - open ANSI/NIST file pointer
      buf         - a memory buffer wrapped in a basic_data_buffer
      num_bytes   - size of the field to be read in bytes
   Output:
      ofield      - points to created and filled structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_ANSI_NIST_binary_field(FILE *fpin, FIELD **ofield, const int num_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_field(fpin, NULL, ofield, num_bytes));
}

int biomeval_nbis_scan_ANSI_NIST_binary_field(AN2KBDB *buf, FIELD **ofield, const int num_bytes)
{
    return(biomeval_nbis_i_read_ANSI_NIST_binary_field(NULL, buf, ofield, num_bytes));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_file - Routine writes the contents of an 
#cat:               ANSI/NIST file structure to a specified file.

   Input:
      ansi_nist   - allocated and filled ANSI/NIST file structure
   Output:
      ofile       - name of file to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_file(const char *ofile, const ANSI_NIST *ansi_nist)
{
   FILE *fpout;
   int ret;

   /* Open the output file pointer. */
   fpout = fopen(ofile, "wb");
   if(fpout == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST : fopen : %s\n", ofile);
      return(-2);
   }

   if((ret = biomeval_nbis_write_ANSI_NIST(fpout, ansi_nist)) != 0){
      if(fclose(fpout) != 0){
         fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST : fclose : %s\n", ofile);
         return(-3);
      }
      return(ret);
   }

   /* Close the output file pointer. */
   if(fclose(fpout) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST : fclose : %s\n", ofile);
      return(-4);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST - Routine writes the contents of an ANSI/NIST file
#cat:               structure to an open file pointer.

   Input:
      ansi_nist   - allocated and filled ANSI/NIST file structure
   Output:
      fpout       - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST(FILE *fpout, const ANSI_NIST *ansi_nist)
{
   int ret, record_i;

   /* For each record in the ANSI_NIST structure ... */
   for(record_i = 0; record_i < ansi_nist->num_records; record_i++){
      /* Write the current record to the output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_record(fpout, ansi_nist->records[record_i])) != 0)
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_record - Routine writes the contents of a logical
#cat:               record structure to an open file pointer.

   Input:
      record      - allocated and filled record structure
   Output:
      fpout       - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_record(FILE *fpout, RECORD *record)
{
   int ret, field_i;

   /* If tagged field record ... */
   if(biomeval_nbis_tagged_record(record->type) != 0){
      /* Foreach field in record ... */
      for(field_i = 0; field_i < record->num_fields; field_i++){
         /* Write the field to the output file pointer. */
         if((ret = biomeval_nbis_write_ANSI_NIST_tagged_field(fpout,
                   record->fields[field_i])) != 0)
            return(ret);
      }

      /* If record's FS separator flag set ... */
      if(record->fs_char != 0){
         /* Write FS separator to output file pointer. */
         if((ret = biomeval_nbis_write_ANSI_NIST_separator(fpout, FS_CHAR)) != 0)
           return(ret);
      }
   }
   /* If binary record ... */
   else if(biomeval_nbis_binary_record(record->type) != 0){
      /* Foreach field in binary record ... */
      for(field_i = 0; field_i < record->num_fields; field_i++){
         /* Write the field to the output file pointer. */
         if((ret = biomeval_nbis_write_ANSI_NIST_binary_field(fpout,
                                                record->fields[field_i])) != 0)
            return(ret);
      }

      /* No FS separator at end of binary record. */
   }
   /* Otherwise, unkown record type ... */
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_record :"
	      "unkown record [Type-%d]\n", record->type);
      return(-2);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_tagged_field - Routine writes the contents of a
#cat:               tagged field structure to an open file pointer.

   Input:
      field      - allocated and filled field structure
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_tagged_field(FILE *fpout, const FIELD *field)
{
   int ret, subfield_i;

   /* Make sure this routine sent only tagged fields because */
   /* a binary image record's field ID string is empty.      */

   if(field->id == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_tagged_field :"
	      "field ID empty in tagged record [Type-%d.%03d]\n",
              field->record_type, field->field_int);
      return(-2);
   }

   /* Write the field ID string "r.f:" to the ouput file pointer. */
   if(fwrite(field->id, 1, strlen(field->id), fpout) != strlen(field->id)){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_field : fwrite : id = %s, %s\n",
              field->id, strerror(errno));
      return(-2);
   }

   /* For each subfield in field ... */
   for(subfield_i = 0; subfield_i < field->num_subfields; subfield_i++){
      /* Write subfield to ouput file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_tagged_subfield(fpout,
                                           field->subfields[subfield_i])) != 0)
         return(ret);
   }

   /* If field's GS separator flag set ... */
   if(field->gs_char != 0){
      /* Write GS separator to output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_separator(fpout, GS_CHAR)) != 0)
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_tagged_subfield - Routine writes the contents of a
#cat:               tagged field's subfield structure to an open file pointer.

   Input:
      subfield   - allocated and filled subfield structure
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_tagged_subfield(FILE *fpout, const SUBFIELD *subfield)
{
   int ret, item_i;

   /* For each item in subfield ... */
   for(item_i = 0; item_i < subfield->num_items; item_i++){
      /* Write item to output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_tagged_item(fpout, subfield->items[item_i])) != 0)
         return(ret);
   }

   /* If subfield's RS separator flag set ... */
   if(subfield->rs_char != 0){
      /* Write RS separator to output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_separator(fpout, RS_CHAR)) != 0)
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_tagged_item - Routine writes the contents of a
#cat:               tagged field's information item to an open file pointer.

   Input:
      item       - allocated and filled information item structure
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_tagged_item(FILE *fpout, const ITEM *item)
{
   int n, ret;

   /* Write item's value to output file pointer. */
   n = fwrite(item->value, 1, (size_t)item->num_chars, fpout);

   if(n != item->num_chars){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_tagged_item : "
	      "fwrite : wrote only %d of %d item bytes, %s\n",
	      n, item->num_chars, strerror(errno));
      return(-2);
   }

   /* If item's US separator flag set ... */
   if(item->us_char != 0){
      /* Write US separator to output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_separator(fpout, US_CHAR)) != 0)
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_separator - Routine writes a specified separator
#cat:               character to an open file pointer.  The FS (0x1C)
#cat:               terminates every record, GS (0x1D) separates fields,
#cat:               RS (0x1E) separates subfields, and US (0x1F)
#cat;               separates information items.

   Input:
      sep_char   - separator character byte to be written
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_separator(FILE *fpout, const char sep_char)
{
   /* If the specified separator byte is not valid ... */
   if(biomeval_nbis_is_delimiter(sep_char) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_separator : "
	      "illegal separator = 0x%02x\n", (unsigned int)sep_char);
      return(-2);
   }

   /* Write the separtor byte to the output file pointer. */
   if(fwrite(&sep_char, 1, 1, fpout) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_separator : "
	      "fwrite : 0x%02x, %s\n", (unsigned int)sep_char, strerror(errno));
      return(-3);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_binary_field - Routine writes the contents of a
#cat:               binary field to an open file pointer.

   Input:
      field      - allocated and filled field structure
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_binary_field(FILE *fpout, const FIELD *field)
{
   int ret, subfield_i;

   /* For each subfield in field ... */
   for(subfield_i = 0; subfield_i < field->num_subfields; subfield_i++){
      /* Write subfield to ouput file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_binary_subfield(fpout,
                                           field->subfields[subfield_i])) != 0)
         return(ret);
   }

   /* No GS separator between fields of binary record. */

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_binary_subfield - Routine writes the contents of a
#cat:               binary record's subfield to an open file pointer.

   Input:
      subfield   - allocated and filled subfield structure
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_binary_subfield(FILE *fpout, const SUBFIELD *subfield)
{
   int ret, item_i;

   /* For each item in subfield ... */
   for(item_i = 0; item_i < subfield->num_items; item_i++){
      /* Write item to output file pointer. */
      if((ret = biomeval_nbis_write_ANSI_NIST_binary_item(fpout, subfield->items[item_i])) != 0)
         return(ret);
   }

   /* No RS separator between subfields of binary record. */

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_ANSI_NIST_binary_item - Routine writes the contents of a
#cat:               binary record's information item to an open file pointer.

   Input:
      item       - allocated and filled information item
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_ANSI_NIST_binary_item(FILE *fpout, const ITEM *item)
{
   int n;
   unsigned int uint_val;
   unsigned short ushort_val;
   unsigned char uchar_val;

   if(item->num_bytes <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
	      "no bytes in item of size %d\n", item->num_bytes);
      return(-2);
   }

   switch(item->num_bytes){
      case 4:
         if (sscanf((char *)item->value, "%u", &uint_val) != 1) {
	    fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "sscanf : failed to parse uint item %4s\n",
		    (char *)item->value);
	    return -21;
	 }
#ifdef __NBISLE__
         swap_uint_bytes(uint_val);
#endif
         if(fwrite(&uint_val, sizeof(unsigned int), 1, fpout) != 1){
            fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "fwrite : failed to write uint item %u, %s\n",
		    uint_val, strerror(errno));
            return(-3);
         }
         break;
      case 2:
         if (sscanf((char *)item->value, "%hu", &ushort_val) != 1) {
	    fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "sscanf : failed to parse ushort item %2s\n",
		    (char *)item->value);
	    return -31;
	 }

#ifdef __NBISLE__
         swap_ushort_bytes(ushort_val);
#endif
         if(fwrite(&ushort_val, sizeof(unsigned short), 1, fpout) != 1){
            fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "fwrite : failed to write ushort item %hu, %s\n", 
		    ushort_val, strerror(errno));
            return(-4);
         }
         break;
      case 1:
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
         if (sscanf((char *)item->value, "%hhu", &uchar_val) != 1) {
	    fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "sscanf : failed to parse uchar item %1s\n",
		    (char *)item->value);
	    return -41;
	 }
#else
	 if (sscanf((char *)item->value, "%hu", &ushort_val) != 1) {
	    fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "sscanf : failed to parse uchar item %1s\n",
		    (char *)item->value);
	    return -41;
	 }
	 if (ushort_val > UCHAR_MAX) {
	    fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "unsigned character value %hu exceeds %hu (UCHAR_MAX)\n",
		    ushort_val, UCHAR_MAX);
	 }
	 uchar_val = (unsigned char)ushort_val;
#endif	 
         if(fwrite(&uchar_val, sizeof(unsigned char), 1, fpout) != 1){
            fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "fwrite : failed to write uchar item %c (0x%02x), %s\n",
		    uchar_val, (unsigned int)uchar_val, strerror(errno));
            return(-5);
         }
         break;
      /* Assume item's value contains block of binary data bytes, */
      /* for example, image data. */
      default:
         /* Write item's value to output file pointer as buffer of bytes. */
         n = fwrite(item->value, 1, (size_t)item->num_chars, fpout);
         if(n != item->num_chars){
            fprintf(stderr, "ERROR : biomeval_nbis_write_ANSI_NIST_binary_item : "
		    "fwrite : wrote only %d of %d item bytes, %s\n",
                    n, item->num_chars, strerror(errno));
            return(-6);
         }
         break;
   }

   /* Return normally. */
   return(0);
}

