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

      FILE:    COPY.C
      AUTHOR:  Michael D. Garris
      DATE:    09/22/2000
      UPDATED: 03/03/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  04/01/2008 by Joseph Konczal - fixed memory overrun error in
                          biomeval_nbis_copy_ANSI_NIST, caused by the wrong letter case

      Contains routines responsible for creating memory copies
      of ANSI/NIST file structures plus individual logical records,
      fields, subfields, and information items.  These routines are
      used primarily to support the construction of ANSI/NIST structures.

***********************************************************************
               ROUTINES:
                        copyANSI_NIST()
                        copyANSI_NIST_field()
                        copyANSI_NIST_subfield()
                        copyANSI_NIST_item()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_copy_ANSI_NIST - Takes an ANSI/NIST structure and creates a
#cat:              memory copy of its contents.

   Input:
      ansi_nist  - ANSI/NIST file structure to be copied
   Output:
      oansi_nist - points to new memory copy
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_copy_ANSI_NIST(ANSI_NIST **oansi_nist, ANSI_NIST *ansi_nist)
{
   int i, ret;
   ANSI_NIST *nansi_nist;
   RECORD *nrecord;

   /* Allocate new ANSI/NIST structure. */
   nansi_nist = (ANSI_NIST *)malloc(sizeof(ANSI_NIST)); /* upcased - jck */
   if(nansi_nist == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST : "
	      "malloc : nansi_nist (%lu bytes)\n", 
	      (unsigned long)sizeof(ANSI_NIST));
      return(-2);
   }

   /* Copy ANSI/NIST structure. */
   memcpy(nansi_nist, ansi_nist, sizeof(ANSI_NIST));

   /* Allocate same size list of RECORD pointers. */
   nansi_nist->records = (RECORD **)malloc(ansi_nist->alloc_records *
                                           sizeof(RECORD *));
   if(nansi_nist->records == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST : "
	      "malloc : %d records (%lu bytes)\n", ansi_nist->alloc_records,
	      (unsigned long)(ansi_nist->alloc_records * sizeof(RECORD *)));
      free(nansi_nist);
      return(-3);
   }

   /* Foreach record in input ANSI/NIST ... */
   for(i = 0; i < ansi_nist->num_records; i++){
      if((ret = biomeval_nbis_copy_ANSI_NIST_record(&nrecord, ansi_nist->records[i])) != 0){
	 /* error - clean up and return */
	 nansi_nist->num_records = i;
	 biomeval_nbis_free_ANSI_NIST(nansi_nist);
         return(ret);
      }
      nansi_nist->records[i] = nrecord;
   }

   /* Assign output pointer. */
   *oansi_nist = nansi_nist;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_copy_ANSI_NIST_record - Takes a logical record structure and creates a
#cat:              memory copy of its contents.

   Input:
      record     - logical record to be copied
   Output:
      orecord    - points to new memory copy
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_copy_ANSI_NIST_record(RECORD **orecord, RECORD *record)
{
   int i, ret;
   RECORD *nrecord;
   FIELD *nfield;

   /* Allocate new RECORD structure. */
   nrecord = (RECORD *)malloc(sizeof(RECORD));
   if(nrecord == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_record : "
	      "malloc : nrecord (%lu bytes)\n", (unsigned long)sizeof(RECORD));
      return(-2);
   }

   /* Copy RECORD structure. */
   memcpy(nrecord, record, sizeof(RECORD));

   /* Allocate same size list of FIELD pointers. */
   nrecord->fields = (FIELD **)malloc(record->alloc_fields * 
				      sizeof(FIELD *));
   if(nrecord->fields == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_record : "
	      "malloc : %d fields (%lu bytes)\n", record->alloc_fields,
	      (unsigned long)(record->alloc_fields * sizeof(FIELD *)));
      free(nrecord);
      return(-3);
   }

   /* Foreach field in input RECORD ... */
   for(i = 0; i < record->num_fields; i++){
      if((ret = biomeval_nbis_copy_ANSI_NIST_field(&nfield, record->fields[i])) != 0){
	 nrecord->num_fields = i;
	 biomeval_nbis_free_ANSI_NIST_record(nrecord);
         return(ret);
      }
      nrecord->fields[i] = nfield;
   }

   /* Assign output pointer. */
   *orecord = nrecord;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_copy_ANSI_NIST_field - Takes a field structure and creates a
#cat:              memory copy of its contents.

   Input:
      field      - field structure to be copied
   Output:
      ofield     - points to new memory copy
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_copy_ANSI_NIST_field(FIELD **ofield, FIELD *field)
{
   int i, ret;
   FIELD *nfield;
   SUBFIELD *nsubfield;
   int field_id_bytes;

   /* Allocate new FIELD structure. */
   nfield = (FIELD *)malloc(sizeof(FIELD));
   if(nfield == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_field : "
	      "malloc : nfield (%lu bytes)\n", (unsigned long)sizeof(FIELD));
      return(-2);
   }

   /* Copy FIELD structure. */
   memcpy(nfield, field, sizeof(FIELD));

   /* Condition statement added by MDG on 03/03/2005 */
   if(field->id != NULL){

      /* Create copy of field ID string. */
      /* Allocate maximum size field ID as twice the max number of       */
      /* characters for the field_int + 2 characters for the '.' and ':' */
      /* + 1 for the NULL terminator.                                    */
      field_id_bytes = (2 * FIELD_NUM_LEN) + 3;
      /* Use calloc so that ID buffer is set to all zeros (NULL). */
      nfield->id = (char *)calloc((size_t)field_id_bytes, 1);
      if(nfield->id == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_field : "
		 "calloc : nfield->id (%d bytes)\n", field_id_bytes);
	 free(nfield);
         return(-3);
      }
      /* Copy ID string. */
      strcpy(nfield->id, field->id);
   }
   /* Otherwise, new field ID already set to NULL */

   /* Allocate same size list of SUBFIELD pointers. */
   nfield->subfields = (SUBFIELD **)malloc(field->alloc_subfields *
                                           sizeof(SUBFIELD *));
   if(nfield->subfields == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_field : "
	      "malloc : %d subfields (%lu bytes)\n", field->alloc_subfields,
	      (unsigned long)(field->alloc_subfields * sizeof(SUBFIELD *)));
      free(nfield->id);
      free(nfield);
      return(-4);
   }

   /* Foreach subfield in input FIELD ... */
   for(i = 0; i < field->num_subfields; i++){
      if((ret = biomeval_nbis_copy_ANSI_NIST_subfield(&nsubfield, field->subfields[i])) != 0){
	 nfield->num_subfields = i;
	 biomeval_nbis_free_ANSI_NIST_field(nfield);
         return(ret);
      }
      nfield->subfields[i] = nsubfield;
   }

   /* Assign output pointer. */
   *ofield = nfield;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_copy_ANSI_NIST_subfield - Takes a subfield structure and creates a
#cat:              memory copy of its contents.

   Input:
      subfield   - subfield structure to be copied
   Output:
      osubfield  - points to new memory copy
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_copy_ANSI_NIST_subfield(SUBFIELD **osubfield, SUBFIELD *subfield)
{
   int i, ret;
   SUBFIELD *nsubfield;
   ITEM *nitem;

   /* Allocate new SUBFIELD structure. */
   nsubfield = (SUBFIELD *)malloc(sizeof(SUBFIELD));
   if(nsubfield == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_subfield : "
	      "malloc : nsubfield (%lu bytes)\n",
	      (unsigned long)sizeof(SUBFIELD));
      return(-2);
   }

   /* Copy SUBFIELD structure. */
   memcpy(nsubfield, subfield, sizeof(SUBFIELD));

   /* Allocate same size list of ITEM pointers. */
   nsubfield->items = (ITEM **)malloc(subfield->alloc_items *
                                      sizeof(ITEM *));
   if(nsubfield->items == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_subfield : "
	      "malloc : %d items (%lu bytes)\n", subfield->alloc_items,
	      (unsigned long)(subfield->alloc_items * sizeof(ITEM *)));
      free(nsubfield);
      return(-3);
   }

   /* Foreach item in input SUBFIELD ... */
   for(i = 0; i < subfield->num_items; i++){
      if((ret = biomeval_nbis_copy_ANSI_NIST_item(&nitem, subfield->items[i])) != 0){
	 nsubfield->num_items = i;
	 biomeval_nbis_free_ANSI_NIST_subfield(nsubfield);
         return(ret);
      }
      nsubfield->items[i] = nitem;
   }

   /* Assign output pointer. */
   *osubfield = nsubfield;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_copy_ANSI_NIST_item - Takes an information item and creates a
#cat:              memory copy of its contents.

   Input:
      item       - information item to be copied
   Output:
      oitem      - points to new memory copy
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_copy_ANSI_NIST_item(ITEM **oitem, ITEM *item)
{
   ITEM *nitem;

   /* Allocate new ITEM structure. */
   nitem = (ITEM *)malloc(sizeof(ITEM));
   if(nitem == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_item : "
	      "malloc : nitem (%lu bytes)\n", (unsigned long)sizeof(ITEM));
      return(-2);
   }

   /* Copy ITEM structure. */
   memcpy(nitem, item, sizeof(ITEM));

   /* Allocate same size value string. */
   nitem->value = (unsigned char *)calloc((size_t)item->alloc_chars,
					  sizeof(unsigned char));
   if(nitem->value == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_copy_ANSI_NIST_item : "
	      "calloc : value (%lu bytes)\n",
	      (unsigned long)(item->alloc_chars * sizeof(unsigned char)));
      free(nitem);
      return(-3);
   }

   /* Copy input ITEM's value. */
   /* NOTE: Image Data field items will NOT be NULL terminated. */
   /*       Can't use strcpy(nitem->value, item->value);        */
   memcpy(nitem->value, item->value, (size_t)item->alloc_chars);

   /* Assign output pointer. */
   *oitem = nitem;

   /* Return normally. */
   return(0);
}
