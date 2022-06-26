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

      FILE:    UPDATE.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines responsible for appending data structures
      including logical records, fields, subfields, and information
      items into an ANSI/NIST file structure.  These routines are
      used primarily to support the input parsing of ANSI/NIST files.
      Routines are also provided to recompute and update the
      length/size of logical records.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_update_ANSI_NIST()
                        biomeval_nbis_update_ANSI_NIST_field()
                        biomeval_nbis_update_ANSI_NIST_subfield()
                        biomeval_nbis_update_ANSI_NIST_item()
                        biomeval_nbis_update_ANSI_NIST_record_LENs()
                        biomeval_nbis_update_ANSI_NIST_record_LEN()
                        biomeval_nbis_update_ANSI_NIST_binary_record_LEN()
                        biomeval_nbis_update_ANSI_NIST_tagged_record_LEN()
                        biomeval_nbis_update_ANSI_NIST_field_ID()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST - Takes a new logical record structure and
#cat:              appends it to the end of the specified ANSI/NIST
#cat:              file structure.  This routine does nothing to
#cat:              update the CNT field of the Type-1 record or to
#cat:              manage IDCs.  It primarily supports the input
#cat:              parsing of an ANSI/NIST file.

   Input:
      record     - record structure to be appended
      ansi_nist  - ANSI/NIST file structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST(ANSI_NIST *ansi_nist, RECORD *record)
{
   if(ansi_nist->num_records >= ansi_nist->alloc_records){
      size_t new_size = (ansi_nist->alloc_records
			   + ANSI_NIST_CHUNK) * sizeof(RECORD *);
      RECORD ** new_ptr = (RECORD **)realloc(ansi_nist->records, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST : "
		 "realloc : records (increase %lu bytes to %lu)\n",
		 (unsigned long)(ansi_nist->alloc_records * sizeof(RECORD *)),
		 (unsigned long)new_size);
         return(-2);
      }
      ansi_nist->records = new_ptr;
      ansi_nist->alloc_records += ANSI_NIST_CHUNK;
   }

   ansi_nist->records[ansi_nist->num_records++] = record;
   /* Accumulate record size. */
   ansi_nist->num_bytes += record->num_bytes;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_record - Takes a new field structure and
#cat:              appends it to the end of the specified logical record
#cat:              structure.  This routine primarily supports the input
#cat:              parsing of an ANSI/NIST file.

   Input:
      field      - field structure to be appended
      record     - record structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_record(RECORD *record, FIELD *field)
{
   if(record->num_fields >= record->alloc_fields){
      size_t new_size = (record->alloc_fields
			   + ANSI_NIST_CHUNK) * sizeof(FIELD *);
      FIELD ** new_ptr = (FIELD **)realloc(record->fields, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_record : "
		 "realloc : fields (increase %lu bytes to %lu)\n",
		 (unsigned long)(record->alloc_fields * sizeof(FIELD *)),
		 (unsigned long)new_size);
         return(-2);
      }
      record->fields = new_ptr;
      record->alloc_fields += ANSI_NIST_CHUNK;
   }

   record->fields[record->num_fields++] = field;
   /* Accumulate field size. */
   record->num_bytes += field->num_bytes;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_field - Takes a new subfield structure and
#cat:              appends it to the end of the specified field
#cat:              structure.  This routine primarily supports the input
#cat:              parsing of an ANSI/NIST file.

   Input:
      subfield   - subfield structure to be appended
      field      - field structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_field(FIELD *field, SUBFIELD *subfield)
{
   if(field->num_subfields >= field->alloc_subfields){
      size_t new_size = (field->alloc_subfields 
			   + ANSI_NIST_CHUNK) * sizeof(SUBFIELD *);
      SUBFIELD ** new_ptr = (SUBFIELD **)realloc(field->subfields, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_field : "
		 "realloc : subfields (increase %lu bytes to %lu)\n",
		 (unsigned long)(field->alloc_subfields * sizeof(SUBFIELD *)),
		 (unsigned long)new_size);
         return(-2);
      }
      field->subfields = new_ptr;
      field->alloc_subfields += ANSI_NIST_CHUNK;
   }

   field->subfields[field->num_subfields++] = subfield;
   /* Accumulate subfield size. */
   field->num_bytes += subfield->num_bytes;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_subfield - Takes a new information item and
#cat:              appends it to the end of the specified subfield
#cat:              structure.  This routine primarily supports the input
#cat:              parsing of an ANSI/NIST file.

   Input:
      item       - information item structure to be appended
      subfield   - subfield structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_subfield(SUBFIELD *subfield, ITEM *item)
{
   if(subfield->num_items >= subfield->alloc_items){
      size_t new_size = (subfield->alloc_items
			   + ANSI_NIST_CHUNK) * sizeof(ITEM *);
      ITEM ** new_ptr = (ITEM **)realloc(subfield->items, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_subfield : "
		 "realloc : items (increase %lu bytes to %lu)\n",
		 (unsigned long)(subfield->alloc_items * sizeof(ITEM *)),
		 (unsigned long)new_size);
         return(-2);
      }
      subfield->items = new_ptr;
      subfield->alloc_items += ANSI_NIST_CHUNK;
   }

   subfield->items[subfield->num_items++] = item;
   /* Accumulate item size. */
   subfield->num_bytes += item->num_bytes;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_item - Takes a newly read character and
#cat:              appends it to the end of the specified information
#cat:              item's string value.  This routine primarily supports
#cat:              the input parsing of an ANSI/NIST file.

   Input:
      nextchar   - character to be appended
      item       - information item structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_item(ITEM *item, const int nextchar)
{
   int alloc_chars;

   /* Num_chars does NOT include NULL terminator. */
   alloc_chars = item->num_chars + 1;

   if(alloc_chars >= item->alloc_chars){
      int new_size = item->alloc_chars + ANSI_NIST_CHUNK;
      unsigned char * new_ptr = (unsigned char *)realloc(item->value, new_size);

      if(new_ptr == NULL){
         fprintf(stderr,"ERROR : biomeval_nbis_update_ANSI_NIST_item : "
		 "realloc : item->value (increase %d bytes to %d)\n",
		 item->alloc_chars, new_size);
         return(-2);
      }
      item->value = new_ptr;
      item->alloc_chars += ANSI_NIST_CHUNK;
   }

   /* Assign character to next byte in item. */
   item->value[item->num_chars++] = nextchar;
   item->value[item->num_chars] = '\0';
   item->num_bytes++;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_record_LENs - Takes an ANSI/NIST file structure
#cat:              and record by record recomputes and updates the
#cat:              length/size of each record.

   Input:
      ansi_nist  - ANSI/NIST file structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_record_LENs(ANSI_NIST *ansi_nist)
{
   int record_i, ret;

   for(record_i = 0; record_i < ansi_nist->num_records; record_i++){
     if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
        return(ret);
   }

   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_record_LEN - Takes a specified logical record in
#cat:              an ANSI/NIST file structure and recomputes and updates
#cat:              the length/size of the record and parent ANSI/NIST
#cat:              structure.

   Input:
      record_i   - integer index of desired record structure
      ansi_nist  - ANSI/NIST file structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_record_LEN(ANSI_NIST *ansi_nist, const int record_i)
{
   int ret;
   RECORD *record;
   FIELD *lenfield;
   int lenfield_i;
   ITEM *lenitem;
   int orig_recordlen;

   /* If record index out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_record_LEN : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* If LEN "01" field not found ... */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&lenfield, &lenfield_i, LEN_ID, record)){
      /* Assume we have a partial tagged field record. For example, an   */
      /* fmttext file may have been parsed that specifies only a single  */
      /* field or subfield.  This happens in applications when inserting */
      /* or substituting single fields or subfields.                     */
      /* Ignore the update request. */
      return(0);
   }

   /* If LEN subfield or item not found ... */
   if((lenfield->num_subfields != 1) ||
      (lenfield->subfields[0]->num_items != 1)){
      fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_record_LEN : "
	      "LEN field index [%d.%d] format error in record [Type-%d.%03d]\n",
              record_i+1, lenfield_i+1, record->type, lenfield->field_int);
      return(-4);
   }

   /* Convert original LEN to numeric integer. */
   lenitem = lenfield->subfields[0]->items[0];
   orig_recordlen = atoi((char *)lenitem->value);

   /* If original LEN != byte size of record ... */
   if(orig_recordlen != record->num_bytes){
      /* If a binary record ... */
      if(biomeval_nbis_binary_record(record->type)){
         /* Update binary field record with fixed LEN field ... */
         if((ret = biomeval_nbis_update_ANSI_NIST_binary_record_LEN(record)))
            return(ret);

         fprintf(stderr, "LEN field index [%d.%d] [Type-%d.%03d] updated "
		 "(%d now %d)\n",
                 record_i+1, lenfield_i+1, record->type, lenfield->field_int,
		 orig_recordlen, record->num_bytes);

         /* Return normally. */
         return(0);
      }

      /* Otherwise, update tagged field record with variable LEN field ... */
      if((ret = biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(record)))
         return(ret);

      ansi_nist->num_bytes += (record->num_bytes - orig_recordlen);

      fprintf(stderr, "LEN field index [%d.%d] [Type-%d.%03d] updated "
	      "(%d now %d)\n",
              record_i+1, lenfield_i+1, record->type, lenfield->field_int,
	      orig_recordlen, record->num_bytes);

   }
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_binary_record_LEN - Takes a specified binary field
#cat:              record and recomputes and updates the length/size of
#cat:              the record.

   Input:
      record     - ANSI/NIST structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_binary_record_LEN(RECORD *record)
{
   ITEM *item;
   FIELD *field;
   char value[MAX_UINT_CHARS+1];

   /* Set pointer to LEN field (must be first field in record). */
   field = record->fields[0];
   item = field->subfields[0]->items[0];

   /* Convert new record length to string. */
   sprintf(value, "%d", record->num_bytes);

   if(strlen(value) >= item->alloc_chars){
      unsigned char *new_ptr = (unsigned char *)realloc(item->value,
							MAX_UINT_CHARS+1);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_binary_record_LEN : "
		 "realloc : LEN item value (increase %d bytes to %d)\n",
		 item->alloc_chars, MAX_UINT_CHARS+1);
         return(-2);
      }
      item->value = new_ptr;
      item->alloc_chars = MAX_UINT_CHARS+1;
   }

   /* Then, simply replace the value in fixed length LEN field. */
   strcpy((char *)item->value, value);
   item->num_chars = strlen(value);
   /* num_bytes fixed in binary field. */

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_tagged_record_LEN - Takes a specified tagged field
#cat:              record and recomputes and updates the length/size of
#cat:              the record.

   Input:
      record     - ANSI/NIST structure to be modified
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_update_ANSI_NIST_tagged_record_LEN(RECORD *record)
{
   int byte_adjust;
   int recordlen, recordchars;
   int old_recordchars;
   ITEM *item;
   FIELD *field;
   char value[MAX_UINT_CHARS+1];

   /* Set pointer to LEN field (must be first field in record). */
   field = record->fields[0];
   item = field->subfields[0]->items[0];

   /* Assign original length to current record length. */
   recordlen = atoi((char *)item->value);
   /* Convert original record length to string. */
   sprintf(value, "%d", recordlen);
   /* Count characters in original record length. */
   recordchars = strlen(value);

   /* While the current record length is not equal to bytes in */
   /* record structure ...                                     */
   while(recordlen != record->num_bytes){

      /* Set old record length character count. */
      old_recordchars = recordchars;

      /* Set current record length to number of bytes in */
      /* record structure.                               */
      recordlen = record->num_bytes;
      /* Convert current record length to string. */
      sprintf(value, "%d", recordlen);
      /* Count characters in current record length. */
      recordchars = strlen(value);

      /* If current and old record lengths have different number */
      /* of characters ...                                       */
      if(old_recordchars != recordchars){
         /* Compute byte adjustment from difference. */
         byte_adjust = recordchars - old_recordchars;
         /* Adjust parent structure byte counts to account for       */
         /* difference between current and old record length values. */
         field->subfields[0]->num_bytes += byte_adjust;
         field->num_bytes += byte_adjust;
         record->num_bytes += byte_adjust;
      }

      if(strlen(value) >= item->alloc_chars){
         unsigned char * new_ptr = (unsigned char *)realloc(item->value,
							    MAX_UINT_CHARS+1);

         if(new_ptr == NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_update_ANSI_NIST_tagged_record_LEN : "
		    "realloc : LEN item value (increase %d bytes to %d)\n",
		    item->alloc_chars, MAX_UINT_CHARS+1);
            return(-2);
         }
	 item->value = new_ptr;
         item->alloc_chars = MAX_UINT_CHARS+1;
      }

      /* Copy last record length value into LEN field's item. */
      strcpy((char *)item->value, value);
      item->num_chars = strlen(value);
      item->num_bytes = item->num_chars;
      /* If item has a trailing US separator ... */
      if(item->us_char)
         /* Include separator in byte size of item. */
         item->num_bytes++;
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_update_ANSI_NIST_field_ID - Takes a field structure and assigns it a 
#cat:              new record type and field identifier and udpates the
#cat:              field ID string if it has been previously allocated
#cat:              and assigned.

   Input:
      field       - field structure to be updated
      record_type - type of new record field is a part of
      field_int   - integer identifier for field
   Output:
      field       - structure's members are udpated
************************************************************************/
void biomeval_nbis_update_ANSI_NIST_field_ID(FIELD *field, const int record_type,
                               const int field_int)
{
   /* Set new record type. */
   field->record_type = record_type;

   /* Set new field ID. */
   field->field_int = field_int;

   /* If Field ID string has been previously allocated and assigned ... */
   if(field->id != NULL)
      sprintf(field->id, FLD_FMT, record_type, field_int);
}

