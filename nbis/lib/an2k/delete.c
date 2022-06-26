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

      FILE:    DELETE.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2008 by Joseph C. Konczal
                          - report more details when things go wrong

      Contains routines responsible for removing specified
      data structures including logical records, fields, subfields,
      and information items from an ANSI/NIST file structure.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_do_delete()
                        biomeval_nbis_delete_ANSI_NIST_select()
                        biomeval_nbis_delete_ANSI_NIST_record()
                        biomeval_nbis_adjust_delrec_CNT_IDCs()
                        biomeval_nbis_delete_ANSI_NIST_field()
                        biomeval_nbis_delete_ANSI_NIST_subfield()
                        biomeval_nbis_delete_ANSI_NIST_item()

***********************************************************************/

#include <stdio.h>
#include <errno.h>

#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_do_delete - Master routine used to carry out a requested structure
#cat:              deletion and write the results to file.  The structure
#cat:              to be deleted is represented by a 4-tuple of indices.

   Input:
      record_i   - index of logical record to be deleted
      field_i    - index of field if to be deleted
      subfield_i - index of subfield if to be deleted
      item_i     - index of information item if to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ofile      - name of file results are to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_do_delete(const char *ofile,
              const int record_i, const int field_i,
              const int subfield_i, const int item_i,
              ANSI_NIST *ansi_nist)
{
   int ret;
   FILE *fpout;

   /* Conduct structure deletion. */
   if((ret = biomeval_nbis_delete_ANSI_NIST_select(record_i, field_i, subfield_i, item_i,
                                    ansi_nist)) != 0){
      return(ret);
   }

   /* If an output file is specified, then open it for writing. */
   if(ofile != NULL){
      fpout = fopen(ofile, "wb");
      if(fpout == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_do_delete : fopen '%s': %s\n", 
		 ofile, strerror(errno));
         return(-2);
      }
   }
   /* Otherwise, set the output file pointer to a default (ex. stdout). */
   else
      fpout = DEFAULT_FPOUT;

   if((ret = biomeval_nbis_write_ANSI_NIST(fpout, ansi_nist)) != 0)
      return(ret);

   /* Close the file pointer if necessary. */
   if(ofile != NULL){
      if(fclose(fpout) != 0){
         fprintf(stderr, "ERROR : biomeval_nbis_do_delete : fclose '%s': %s\n",
		 ofile, strerror(errno));
         return(-3);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_delete_ANSI_NIST_select - Routine used to carry out a requested
#cat:              structure deletion based on a 4-tuple of structure
#cat:              indices. These indices represent the physical order
#cat:              of subsequent logical records, fields, subfields, and
#cat:              information items in the ANSI/NIST file structure.
#cat:              In order to specify the deletion of higher-level
#cat:              structures, subordinate structures are assigned the
#cat:              value of "UNDEFINED_INT".

   Input:
      record_i   - index of logical record to be deleted
      field_i    - index of field if to be deleted
      subfield_i - index of subfield if to be deleted
      item_i     - index of information item if to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_delete_ANSI_NIST_select(const int record_i, const int field_i,
                            const int subfield_i, const int item_i,
                            ANSI_NIST *ansi_nist)
{
   int ret;

   /* If record index defined ... */
   if(record_i != UNDEFINED_INT){
      /* If field ID defined ... */
      if(field_i != UNDEFINED_INT){
         /* If subfield index defined ... */
         if(subfield_i != UNDEFINED_INT){
            /* If item index defined ... */
            if(item_i != UNDEFINED_INT){
               /* Delete the item. */
               if((ret = biomeval_nbis_delete_ANSI_NIST_item(record_i, field_i,
					       subfield_i, item_i, 
					       ansi_nist)) != 0)
                  return(ret);
            }
            /* Otherwise, item index is UNDEFINED ... */
            else{
               /* Delete the subfield. */
               if((ret = biomeval_nbis_delete_ANSI_NIST_subfield(record_i, field_i,
						   subfield_i, ansi_nist)) != 0)
                  return(ret);
            }
         }
         /* Otherwise, subfield index is UNDEFINED ... */
         else{
            /* Delete the field. */
            if((ret = biomeval_nbis_delete_ANSI_NIST_field(record_i, field_i, ansi_nist)) != 0)
               return(ret);
         }
      }
      /* Otherwise, field ID is UNDEFINED ... */
      else{
         /* Delete the record. */
         if((ret = biomeval_nbis_delete_ANSI_NIST_record(record_i, ansi_nist)) != 0)
            return(ret);
      }
   }
   /* Otherwise, record type is UNDEFINED ... */
   else{
      /* So ignore request. */
      fprintf(stderr, "WARNING : biomeval_nbis_delete_ANSI_NIST_select : "
	      "record index not specified so request ignored\n");
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_delete_ANSI_NIST_record - Routine used to delete the specified
#cat:              logical record from an ANSI/NIST file structure.
#cat:              This routine also updates the CNT field of the Type-1
#cat:              record to accurately reflect the deletion.

   Input:
      record_i   - index of logical record to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_delete_ANSI_NIST_record(const int record_i, ANSI_NIST *ansi_nist)
{
   int ret, j, k; 
   RECORD *record;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_record : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* Remove record by sliding subsequent records down in the list. */
   for(j = record_i, k = record_i+1; k < ansi_nist->num_records; j++, k++){
       ansi_nist->records[j] = ansi_nist->records[k];
   }

   /* Decrement number of records in ANSI_NIST structure. */
   /* Empty list could result. */
   ansi_nist->num_records--;

   /* Subtract deleted record's size from the byte size of the */
   /* ANSI_NIST structure. */
   ansi_nist->num_bytes -= record->num_bytes;

   /* By deleting a record, the Type-1 CNT field must be updated, and */
   /* it is possible that the deleted record's IDC is orphaned, and   */
   /* subsequent IDCs may need to be adjusted.                        */
   if((ret = biomeval_nbis_adjust_delrec_CNT_IDCs(record_i, ansi_nist)) != 0)
      return(ret);

   /* Leave ANSI_NIST allocated even if empty after deletion. */
   /* Should never happen, because an ERROR will throw if the */
   /* Type-1 record is deleted, because it will then be       */
   /* impossible to update the CNT field.                     */

   fprintf(stderr, "Deleted record index [%d] [Type-%d]\n",
           record_i+1, record->type);

   /* Deallocate record's memory. */
   biomeval_nbis_free_ANSI_NIST_record(record);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_adjust_delrec_CNT_IDCs - Routine used to update an ANSI/NIST file
#cat:              structure's Type-1 record, CNT 1.3 field, due to a
#cat:              deleted logical record.  This routine also manages
#cat:              the remaining records' IDC values in an attempt to
#cat:              maintain contiguous IDCs.

   Input:
      record_i   - index of deleted logical record
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_adjust_delrec_CNT_IDCs(const int record_i, ANSI_NIST *ansi_nist)
{
   int ret, j, found; 
   FIELD *cntfield, *idcfield;
   SUBFIELD *subfield;
   ITEM *item;
   int cntfield_i, idcfield_i;
   int itemint, delidc;

   /* Need to adjust Type-1 CNT 1.3 field, so look up Type-1 record. */
   fprintf(stderr, "Updating CNT field [Type-1.%03d]\n", CNT_ID);

   /* If Type-1 record not found ... */
   /* Type-1 record must be first in record list ... */
   if((ansi_nist->num_records <= 0) ||
      (ansi_nist->records[0]->type != TYPE_1_ID)){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "Type-1 record not found\n");
      return(-2);
   }

   /* If CNT field within Type-1 record not found ... */
   if(biomeval_nbis_lookup_ANSI_NIST_field(&cntfield, &cntfield_i, CNT_ID,
			     ansi_nist->records[0]) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "CNT field not found in record index [1] [Type-1.%03d]\n",
	      CNT_ID);
      return(-3);
   }

   /* If CNT subfield for deleted record not found ... */
   if(biomeval_nbis_lookup_ANSI_NIST_subfield(&subfield, record_i, cntfield) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "subfield index [1.%d.%d] not found in "
	      "CNT field [Type-1.%d]\n", cntfield_i+1, record_i+1, CNT_ID);
      return(-4);
   }

   /* If the IDC item for deleted record in CNT not found ... */
   if(biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, subfield) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "IDC item index [1.%d.%d.2] not found in "
	      "CNT field [Type-1.%03d]\n", cntfield_i+1, record_i+1, CNT_ID);
      return(-5);
   }

   /* Convert deleted record's IDC to numeric integer. */
   delidc = atoi((char *)item->value);

   /* Delete subfield from CNT corresponding to deleted record. */
   if((ret = biomeval_nbis_delete_ANSI_NIST_subfield(0, cntfield_i, record_i, ansi_nist)) != 0)
      return(ret);

   /* If first subfield in CNT field not found ... */
   if(biomeval_nbis_lookup_ANSI_NIST_subfield(&subfield, 0, cntfield) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "subfield index [1.%d.1] not found "
	      "in CNT field [Type-1.%d]\n", cntfield_i+1, CNT_ID);
      return(-6);
   }

   /* If record count item in first subfield in CNT not found ... */
   if(biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, subfield) == 0){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
	      "Record count item index [1.%d.1.2] not found "
	      "in CNT field [Type-1.%d]\n", cntfield_i+1, CNT_ID);
      return(-7);
   }

   /* Decrement number of records contained in the CNT field. */
   if((ret = biomeval_nbis_decrement_numeric_item(0, cntfield_i, 0, 1,
                                   ansi_nist, (char *)NULL)) != 0)
      return(ret);

   /* If deleted record was the last to reference its IDC, then need    */
   /* to decrement all IDCs that are larger than the deleted IDC.  This */
   /* includes changing IDCs stored in corresponding records as well.   */
   fprintf(stderr, "Updating IDCs\n");

   found = FALSE;
   /* For each subfield in CNT field (skipping the first which has */
   /* special meaning) ... */
   for(j = 1; j < cntfield->num_subfields; j++){
      /* If next record's IDC not found in CNT subfield ... */
      if(biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, cntfield->subfields[j]) == 0){
         fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
		 "IDC index [1.%d.%d.2] not found "
		 "in CNT field [Type-1.%d]\n", cntfield_i+1, j+1, CNT_ID);
         return(-8);
      }
      /* Convert item's value to numeric integer. */
      itemint = atoi((char *)item->value);
      /* If deleted record's IDC found ... */
      if(itemint == delidc){
         found = TRUE;
         break;
      }
   }

   /* If deleted record's IDC not referenced in CNT field ... */
   if(found == 0){

      /* Decrement all IDCs larger than the deleted one. */

      /* For each subfield in CNT field (skipping the first which has */
      /* special meaning) ... */
      for(j = 1; j < cntfield->num_subfields; j++){
         /* If next record's IDC not found in CNT subfield ... */
         if(biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, cntfield->subfields[j]) == 0){
            fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
		    "IDC index [1.%d.%d.2] not found "
		    "in CNT field [Type-1.%d]\n", cntfield_i+1, j+1, CNT_ID);
            return(-9);
         }
         /* Convert item's value to numeric integer. */
         itemint = atoi((char *)item->value);

         /* If item's value is > the deleted record's IDC ... */
         if(itemint > delidc){

            /* Decrement the current record's IDC in the CNT field. */
            if((ret = biomeval_nbis_decrement_numeric_item(0, cntfield_i, j, 1,
                                            ansi_nist, IDC_FMT)) != 0)
               return(ret);

            /* If IDC field in corresponding record not found ... */
            if(biomeval_nbis_lookup_ANSI_NIST_field(&idcfield, &idcfield_i,
				      IDC_ID, ansi_nist->records[j]) == 0){
               fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
		       "IDC field not found "
		       "in record index [%d] [Type-%d.%03d]\n",
                       j+1, ansi_nist->records[j]->type, IDC_ID);
               return(-10);
            }

            if((idcfield->num_subfields != 1) ||
               (idcfield->subfields[0]->num_items != 1)){
               fprintf(stderr, "ERROR : biomeval_nbis_adjust_delrec_CNT_IDCs : "
		       "bad format of IDC field "
		       "in record index [%d] [Type-%d.%03d]\n",
                       j+1, ansi_nist->records[j]->type, IDC_ID);
               return(-11);
            }

            /* Decrement the IDC stored in the corresponing record's */
            /* IDC field.                                            */
            if((ret = biomeval_nbis_decrement_numeric_item(j, idcfield_i, 0, 0,
					     ansi_nist, IDC_FMT)) != 0)
               return(ret);
         }
         /* Otherwise, the current record's IDC is OK. */
      }/* END for j */
   }
   /* Otherwise, the deleted record's IDC is not "orphaned". */

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_delete_ANSI_NIST_field - Routine used to delete the specified
#cat:              field from an ANSI/NIST file structure.

   Input:
      record_i   - index of logical record of field to be deleted
      field_i    - index of field to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_delete_ANSI_NIST_field(const int record_i, const int field_i,
                           ANSI_NIST *ansi_nist)
{
   int ret, j, k; 
   RECORD *record;
   FIELD *field;
   int byte_adjust;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_field : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_field : "
	      "field index [%d.%d] out of range [1..%d] "
	      "in record [Type-%d]\n",
	      record_i+1, field_i+1, record->num_fields, record->type);
      return(-3);
   }
   field = record->fields[field_i];

   /* Do not permit deletion of binary fields ... */
   if(biomeval_nbis_binary_record(record->type) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_field : "
	      "field index [%d.%d] is fixed "
	      "in binary record [Type-%d.%03d]\n",
              record_i+1, field_i+1, record->type, field->field_int);
      return(-4);
   }

   /* If field to be deleted is the only one left in the record ... */
   if(record->num_fields == 1){
      fprintf(stderr, "Field index [%d.%d] last in record [Type-%d]\n",
              record_i+1, field_i+1, record->type);
      /* Then go and remove entire record. */
      if((ret = biomeval_nbis_delete_ANSI_NIST_record(record_i, ansi_nist)) != 0)
         return(ret);
   }
   /* Otherwise, more than one field currently in the record. */
   else{
      /* Remove field by by sliding subsequent fields down in the list. */
      for(j = field_i, k = field_i+1; k < record->num_fields; j++, k++){
         record->fields[j] = record->fields[k];
      }

      /* Decrement number of fields in record. */
      record->num_fields--;

      /* Set byte adjustment to size of deleted field. */
      byte_adjust = field->num_bytes;

      /* If the deleted field was at the end of the list ... */
      if(field_i == record->num_fields){
         /* Then the GS separator trailing the previous field */
         /* should be removed.                                */
         record->fields[field_i-1]->gs_char = FALSE;
         record->fields[field_i-1]->num_bytes--;

         /* Bump byte adjustment. */
         byte_adjust++;
      }
         
      /* Adjust size of parent structures. */
      record->num_bytes -= byte_adjust;
      ansi_nist->num_bytes -= byte_adjust;

      /* Update the current record's length field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)) != 0)
         return(ret);

      fprintf(stderr, "Deleted field index [%d.%d] in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, record->type, field->field_int);

      /* Deallocate field's memory */
      biomeval_nbis_free_ANSI_NIST_field(field);

   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_delete_ANSI_NIST_subfield - Routine used to delete the specified
#cat:              subfield from an ANSI/NIST file structure.

   Input:
      record_i   - index of logical record to be deleted
      field_i    - index of field to be deleted
      subfield_i - index of subfield to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_delete_ANSI_NIST_subfield(const int record_i, const int field_i,
                              const int subfield_i, ANSI_NIST *ansi_nist)
{
   int ret, j, k; 
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   int byte_adjust;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_subfield : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_subfield : "
	      "field index [%d.%d] out of range [1..%d] "
	      "in record [Type-%d]\n",
              record_i+1, field_i+1, record->num_fields, record->type);
      return(-3);
   }
   field = record->fields[field_i];

   /* Do not permit deletion of binary subfield ... */
   if(biomeval_nbis_binary_record(record->type) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_subfield : "
	      "subfield index [%d.%d.%d] is fixed "
	      "in binary record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1,
	      record->type, field->field_int);
      return(-4);
   }

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i >= field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_subfield : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, field->num_subfields+1,
              record->type, field->field_int);
      return(-5);
   }
   subfield = field->subfields[subfield_i];

   /* If subfield to be deleted is the only one left in the field ... */
   if(field->num_subfields == 1){
      fprintf(stderr, "Subfield index [%d.%d.%d] last in "
	      "field [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1,
              record->type, field->field_int);
      /* Then go and remove entire field. */
      if((ret = biomeval_nbis_delete_ANSI_NIST_field(record_i, field_i, ansi_nist)) != 0)
         return(ret);
   }
   /* Otherwise, more than one subfield currently in the field. */
   else{
      /* Remove subfield by by sliding subsequent subfields down the list. */
      for(j = subfield_i, k = subfield_i+1;
          k < field->num_subfields;
          j++, k++){
         field->subfields[j] = field->subfields[k];
      }

      /* Decrement number of subfields in field. */
      field->num_subfields--;

      /* Set byte adjustment to size of deleted subfield. */
      byte_adjust = subfield->num_bytes;

      /* If the deleted subfield was at the end of the list ... */
      if(subfield_i == field->num_subfields){
         /* Then the RS separator trailing the previous subfield */
         /* should be removed.                                   */
         field->subfields[subfield_i-1]->rs_char = FALSE;
         field->subfields[subfield_i-1]->num_bytes--;

         /* Bump byte adjustment. */
         byte_adjust++;
      }

      /* Adjust size of field, record and ansi_nist structures. */
      field->num_bytes -= byte_adjust;
      record->num_bytes -= byte_adjust;
      ansi_nist->num_bytes -= byte_adjust;

      /* Update the current record's length field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)) != 0)
         return(ret);

      fprintf(stderr, "Deleted subfield index [%d.%d.%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1,
              record->type, field->field_int);

      /* Deallocate subfield's memory */
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_delete_ANSI_NIST_item - Routine used to delete the specified
#cat:              information item from an ANSI/NIST file structure.

   Input:
      record_i   - index of logical record to be deleted
      field_i    - index of field to be deleted
      subfield_i - index of subfield to be deleted
      item_i     - index of information item to be deleted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_delete_ANSI_NIST_item(const int record_i, const int field_i,
                          const int subfield_i, const int item_i,
                          ANSI_NIST *ansi_nist)
{
   int ret, j, k;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   int byte_adjust;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_item : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_item : "
	      "field index [%d.%d] out of range [1..%d] "
	      "in record [Type-%d]\n",
              record_i+1, field_i+1, record->num_fields, record->type);
      return(-3);
   }
   field = record->fields[field_i];

   /* Do not permit deletion of binary field's item ... */
   if(biomeval_nbis_binary_record(record->type) != 0){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_item : "
	      "item index [%d.%d.%d.%d] is fixed "
	      "in binary record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int);
      return(-4);
   }

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i >= field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_item : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, field->num_subfields,
              record->type, field->field_int);
      return(-5);
   }
   subfield = field->subfields[subfield_i];

   /* If item index is out of range ... */
   if((item_i < 0) || (item_i >= subfield->num_items)){
      fprintf(stderr, "ERROR : biomeval_nbis_delete_ANSI_NIST_item : "
	      "item index [%d.%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              subfield->num_items, record->type, field->field_int);
      return(-6);
   }
   item = subfield->items[item_i];

   /* If item to be deleted is the only one left in the subfield ... */
   if(subfield->num_items == 1){
      fprintf(stderr, "Item index [%d.%d.%d.%d] last in subfield "
	      "of record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int);
      /* Then go and remove entire subfield. */
      if((ret = biomeval_nbis_delete_ANSI_NIST_subfield(record_i, field_i,
                                         subfield_i, ansi_nist)) != 0)
         return(ret);
   }
   /* Otherwise, more than one item currently in the subfield. */
   else{
      /* Remove item by by sliding subsequent items down the list. */
      for(j = item_i, k = item_i+1; k < subfield->num_items; j++, k++){
         subfield->items[j] = subfield->items[k];
      }

      /* Decrement number of items in subfield. */
      subfield->num_items--;

      /* Set byte adjustment to size of deleted item. */
      byte_adjust = item->num_bytes;

      /* If the deleted item was at the end of the list ... */
      if(item_i == subfield->num_items){
         /* Then the US separator trailing the previous item */
         /* should be removed.                               */
         subfield->items[item_i-1]->us_char = FALSE;
         subfield->items[item_i-1]->num_bytes--;

         /* Bump byte adjustment. */
         byte_adjust++;
      }

      /* Adjust size of subfield, field, record and ansi_nist structures. */
      subfield->num_bytes -= byte_adjust;
      field->num_bytes -= byte_adjust;
      record->num_bytes -= byte_adjust;
      ansi_nist->num_bytes -= byte_adjust;

      /* Update the current record's length field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)) != 0)
         return(ret);

      fprintf(stderr, "Deleted item index [%d.%d.%d.%d] "
	      "in record [Type-%d.%03d] = %s\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int, (char *)item->value);

      /* Deallocate item's memory */
      biomeval_nbis_free_ANSI_NIST_item(item);
   }

   /* Return normally. */
   return(0);
}

