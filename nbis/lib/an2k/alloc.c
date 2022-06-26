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

      FILE:    ALLOC.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2008 by Joseph C. Konczal
                          - report more details when things go wrong

      Contains routines responsible for allocating and deallocating
      data structures used to represent and manipulate logical records,
      fields, subfields, and information items in an ANSI/NIST file.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_alloc_ANSI_NIST()
			biomeval_nbis_new_ANSI_NIST_record()
                        biomeval_nbis_alloc_ANSI_NIST_record()
			biomeval_nbis_new_ANSI_NIST_field()
                        biomeval_nbis_alloc_ANSI_NIST_field()
                        biomeval_nbis_alloc_ANSI_NIST_subfield()
                        biomeval_nbis_alloc_ANSI_NIST_item()
                        biomeval_nbis_free_ANSI_NIST()
                        biomeval_nbis_free_ANSI_NIST_record()
                        biomeval_nbis_free_ANSI_NIST_field()
                        biomeval_nbis_free_ANSI_NIST_subfield()
                        biomeval_nbis_free_ANSI_NIST_item()

***********************************************************************/


#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_ANSI_NIST - Allocates an empty initialized structure
#cat:              designed to respresent and manipulate the contents
#cat:              of an ANSI/NIST file.

   Output:
      oansi_nist - points to allocated empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_alloc_ANSI_NIST(ANSI_NIST **oansi_nist)
{
   ANSI_NIST *ansi_nist;

   ansi_nist = (ANSI_NIST *)malloc(sizeof(ANSI_NIST));
   if(ansi_nist == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST : "
	      "malloc : ansi_nist (%lu bytes)\n",
	      (unsigned long)sizeof(ANSI_NIST));
      return(-2);
   }

   ansi_nist->version = UNSET;
   ansi_nist->num_bytes = 0;
   ansi_nist->num_records = 0;
   ansi_nist->alloc_records = ANSI_NIST_CHUNK;
   ansi_nist->records = (RECORD **)malloc(ANSI_NIST_CHUNK * sizeof(RECORD *));
   if(ansi_nist->records == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST : "
	      "malloc : %d records (%lu bytes)\n", ANSI_NIST_CHUNK,
	      (unsigned long)(ANSI_NIST_CHUNK * sizeof(RECORD *)));
      free(ansi_nist);
      return(-3);
   }

   /* Assign results to output pointer(s). */
   *oansi_nist = ansi_nist;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_new_ANSI_NIST_record - Allocates and initializes an empty record
#cat:              structure.  Note - This routine sets the record type
#cat:              in addition to calling biomeval_nbis_alloc_ANSI_NIST_record.

   Input:
      record_type - the type of record to be created
   Output:
      orecord    - points to allocated and initialized empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_new_ANSI_NIST_record(RECORD **orecord, const int record_type)
{
   int ret;
   RECORD *record;

   /* Allocate record. */
   if ((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)) != 0)
      return(ret);

   /* Assign record type. */
   record->type = record_type;

   /* Set FS_CHAR of record. */
   record->fs_char = TRUE;
   record->num_bytes++;

   /* Set output pointer. */
   *orecord = record;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_ANSI_NIST_record - Allocates an empty initialized structure
#cat:              designed to respresent and manipulate the contents
#cat:              of an ANSI/NIST logical record.

   Output:
      orecord    - points to allocated empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_alloc_ANSI_NIST_record(RECORD **orecord)
{
   RECORD *record;

   record = (RECORD *)malloc(sizeof(RECORD));
   if(record == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_record : "
	      "malloc : record (%lu bytes)\n", (unsigned long)sizeof(RECORD));
      return(-2);
   }

   record->fields = (FIELD **)malloc(ANSI_NIST_CHUNK * sizeof(FIELD *));
   if(record->fields == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_record : "
	      "malloc : %d fields (%lu bytes)\n", ANSI_NIST_CHUNK,
	      (unsigned long)(ANSI_NIST_CHUNK * sizeof(FIELD *)));
      free(record);
      return(-3);
   }
   record->type = UNSET;
   /* Total number of bytes to be read. */
   record->total_bytes = UNSET;
   /* Total number of bytes read so far. */
   record->num_bytes = 0;
   record->num_fields = 0;
   record->alloc_fields = ANSI_NIST_CHUNK;
   /* Initialize record without a trailing FS separator. */
   record->fs_char = FALSE;

   /* Assign results to output pointer(s). */
   *orecord = record;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_new_ANSI_NIST_field - Allocates and initializes an empty field
#cat:              structure.  Note - This routine sets the field ID
#cat:              in addition to calling biomeval_nbis_alloc_ANSI_NIST_field.

   Input:
      record_type - the type of record to be created
      field_int   - the type of field to be created
   Output:
      orield     - points to allocated and initialized empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_new_ANSI_NIST_field(FIELD **ofield, const int record_type,
                        const int field_int)
{
   int ret, id_strlen;
   FIELD *field;
   int field_id_bytes;

   /* Allocate field. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)))
      return(ret);

   /* Assign record type and field ID. */
   field->record_type = record_type;
   field->field_int = field_int;

   /* Create field ID string. */
   /* Allocate maximum size field ID as twice the max number of       */
   /* characters for the field_int + 2 characters for the '.' and ':' */
   /* + 1 for the NULL terminator.                                    */
   field_id_bytes = (2 * FIELD_NUM_LEN) + 3;
   /* Use calloc so that ID buffer is set to all zeros. */
   field->id = (char *)calloc((size_t)field_id_bytes, 1);
   if(field->id == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_new_ANSI_NIST_field : "
	      "calloc : field->id [%d.%03d] (%d bytes)\n",
	      record_type, field_int, field_id_bytes);
      free(field);
      return(-2);
   }
   /* Construct ID string. */

   id_strlen =  sprintf(field->id, FLD_FMT, record_type, field_int);

   /* This should never happen. */
   if (id_strlen >= field_id_bytes) {
      fprintf(stderr, "ERROR : biomeval_nbis_new_ANSI_NIST_field : "
	      "snprintf %d byte string overflows %d byte buffer\n",
	      id_strlen, field_id_bytes);
      free(field->id);
      free(field);
      return -3;
   }

   /* Update field size. */
   field->num_bytes += id_strlen;

   /* Set output pointer. */
   *ofield = field;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_ANSI_NIST_field - Allocates an empty initialized structure
#cat:              designed to respresent and manipulate the contents
#cat:              of an ANSI/NIST field.

   Output:
      ofield     - points to allocated empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_alloc_ANSI_NIST_field(FIELD **ofield)
{
   FIELD *field;

   field = (FIELD *)malloc(sizeof(FIELD));
   if(field == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_field : "
	      "malloc : field (%lu bytes)\n", (unsigned long)sizeof(FIELD));
      return(-2);
   }

   field->subfields = (SUBFIELD **)malloc(ANSI_NIST_CHUNK * sizeof(SUBFIELD *));
   if(field->subfields == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_field : "
	      "malloc : %d subfields (%lu bytes)\n", ANSI_NIST_CHUNK,
	      (unsigned long)(ANSI_NIST_CHUNK * sizeof(SUBFIELD *)));
      free(field);
      return(-3);
   }
   field->id = NULL;
   field->record_type = UNSET;
   field->field_int = UNSET;
   field->num_bytes = 0;
   field->num_subfields = 0;
   field->alloc_subfields = ANSI_NIST_CHUNK;
   /* Initialize field without a trailing GS separator. */
   field->gs_char = FALSE;

   /* Assign results to output pointer(s). */
   *ofield = field;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_ANSI_NIST_subfield - Allocates an empty initialized structure
#cat:              designed to respresent and manipulate the contents
#cat:              of an ANSI/NIST subfield.

   Output:
      osubfield     - points to allocated empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_alloc_ANSI_NIST_subfield(SUBFIELD **osubfield)
{
   SUBFIELD *subfield;

   subfield = (SUBFIELD *)malloc(sizeof(SUBFIELD));
   if(subfield == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_subfield : "
	      "malloc : subfield (%lu bytes)\n", 
	      (unsigned long)sizeof(SUBFIELD));
      return(-2);
   }

   subfield->alloc_items = ANSI_NIST_CHUNK;
   subfield->items = (ITEM **)malloc(ANSI_NIST_CHUNK * sizeof(ITEM *));
   if(subfield->items == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_subfield : "
	      "malloc : %d items (%lu bytes)\n", ANSI_NIST_CHUNK,
	      (unsigned long)(ANSI_NIST_CHUNK * sizeof(ITEM *)));
      free(subfield);
      return(-3);
   }
   subfield->num_bytes = 0;
   subfield->num_items = 0;
   /* Initialize subfield without a trailing RS separator. */
   subfield->rs_char = FALSE;

   /* Assign results to output pointer(s). */
   *osubfield = subfield;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_alloc_ANSI_NIST_item - Allocates an empty initialized structure
#cat:              designed to respresent and manipulate the contents
#cat:              of an ANSI/NIST information item.

   Output:
      oitem      - points to allocated empty structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_alloc_ANSI_NIST_item(ITEM **oitem)
{
   ITEM *item;

   item = (ITEM *)malloc(sizeof(ITEM));
   if(item == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_item : "
	      "malloc : item (%lu bytes)\n", (unsigned long)sizeof(ITEM));
      return(-2);
   }

   item->alloc_chars = ANSI_NIST_CHUNK;
   item->value = (unsigned char *)malloc(ANSI_NIST_CHUNK);
   if(item->value == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_alloc_ANSI_NIST_item : "
	      "malloc : item->value (%d bytes)\n", ANSI_NIST_CHUNK);
      free(item);
      return(-3);
   }
   item->num_bytes = 0;
   item->num_chars = 0;
   /* Automatically keep item's value NULL terminated. */
   *(item->value) = '\0';
   /* Initialize item without a trailing US separator. */
   item->us_char = FALSE;

   /* Assign results to output pointer(s). */
   *oitem = item;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_ANSI_NIST - Deallocates an ANSI/NIST file structure and
#cat:              all of its allocated substructures.

   Input:
      ansi_nist  - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_ANSI_NIST(ANSI_NIST *ansi_nist)
{
   int i;

   for(i = 0; i < ansi_nist->num_records; i++)
      biomeval_nbis_free_ANSI_NIST_record(ansi_nist->records[i]);
   free(ansi_nist->records);	/* It's safe to free NULL in C89 and C99. */
   free(ansi_nist);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_ANSI_NIST_record - Deallocates an ANSI/NIST logical record
#cat:              structure and all of its allocated substructures.

   Input:
      record  - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_ANSI_NIST_record(RECORD *record)
{
   int i;

   for(i = 0; i < record->num_fields; i++)
      biomeval_nbis_free_ANSI_NIST_field(record->fields[i]);
   free(record->fields);	/* It's safe to free NULL in C89 and C99. */
   free(record);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_ANSI_NIST_field - Deallocates an ANSI/NIST field
#cat:              structure and all of its allocated substructures.

   Input:
      field - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_ANSI_NIST_field(FIELD *field)
{
   int i;

   for(i = 0; i < field->num_subfields; i++)
      biomeval_nbis_free_ANSI_NIST_subfield(field->subfields[i]);
   free(field->subfields);
   free(field->id);
   free(field);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_ANSI_NIST_subfield - Deallocates an ANSI/NIST subfield
#cat:              structure and all of its allocated substructures.

   Input:
      subfield - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_ANSI_NIST_subfield(SUBFIELD *subfield)
{
   int i;

   for(i = 0; i < subfield->num_items; i++)
      biomeval_nbis_free_ANSI_NIST_item(subfield->items[i]);
   free(subfield->items);
   free(subfield);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_free_ANSI_NIST_subfield_item - Deallocates an ANSI/NIST information
#cat:              item structure and all of its allocated substructures.

   Input:
      item - points to the structure to be deallocated
************************************************************************/
void biomeval_nbis_free_ANSI_NIST_item(ITEM *item)
{
   free(item->value);
   free(item);
}
