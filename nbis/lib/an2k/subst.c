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

      FILE:    SUBST.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines responsible for substituting the contents of
      specified data structures including logical records, fields,
      subfields, and information items within an ANSI/NIST file structure.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_do_substitute()
                        biomeval_nbis_substitute_ANSI_NIST_select()
                        biomeval_nbis_substitute_ANSI_NIST_record()
                        biomeval_nbis_substitute_ANSI_NIST_field()
                        biomeval_nbis_substitute_ANSI_NIST_subfield()
                        biomeval_nbis_substitute_ANSI_NIST_item()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_do_substitute - Master routine used to carry out a requested structure
#cat:              substitution and write the results to file.  The structure
#cat:              to be substituted is represented by a 4-tuple of indices.

   Input:
      record_i   - index of logical record to be substituted
      field_i    - index of field if to be substituted
      subfield_i - index of subfield if to be substituted
      item_i     - index of information item if to be substituted
      newvalue   - string value or filename whose contents is to
                   be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ofile      - name of file results are to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_do_substitute(const char *ofile,
                  const int record_i, const int field_i,
                  const int subfield_i, const int item_i,
                  const char *newvalue, ANSI_NIST *ansi_nist)
{
   int ret;
   FILE *fpout;

   /* Conduct the item substitution. */
   if((ret = biomeval_nbis_substitute_ANSI_NIST_select(record_i, field_i, subfield_i, item_i,
                                        newvalue, ansi_nist))){
      return(ret);
   }

   /* If an output file is specified, then open it for writing. */
   if(ofile != NULL){
      fpout = fopen(ofile, "wb");
      if(fpout == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_do_substitute : fopen : %s\n", ofile);
         return(-2);
      }
   }
   /* Otherwise, set the output file pointer to a default (ex. stdout). */
   else
      fpout = DEFAULT_FPOUT;

   if((ret = biomeval_nbis_write_ANSI_NIST(fpout, ansi_nist))){
      if(fclose(fpout)){
         fprintf(stderr, "ERROR : biomeval_nbis_do_substitute : fclose : %s\n", ofile);
         return(-3);
      }
      return(ret);
   }

   /* Close the file pointer if necessary. */
   if(ofile != NULL){
      if(fclose(fpout)){
         fprintf(stderr, "ERROR : biomeval_nbis_do_substitute : fclose : %s\n", ofile);
         return(-4);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_substitute_ANSI_NIST_select - Routine used to carry out a requested
#cat:              structure substitution based on a 4-tuple of structure
#cat:              indices. These indices represent the physical order
#cat:              of subsequent logical records, fields, subfields, and
#cat:              information items in the ANSI/NIST file structure.
#cat:              In order to specify the substitution of higher-level
#cat:              structures, subordinate structures are assigned the
#cat:              value of "UNDEFINED_INT".  When higher-level structures
#cat:              are specified, "newvalue" takes on a filename which
#cat:              contains the data to be substituted.

   Input:
      record_i   - index of logical record to be substituted
      field_i    - index of field if to be substituted
      subfield_i - index of subfield if to be substituted
      item_i     - index of information item if to be substituted
      newvalue   - string value or filename whose contents is to
                   be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_substitute_ANSI_NIST_select(const int record_i, const int field_i,
                         const int subfield_i, const int item_i,
                         const char *newvalue, ANSI_NIST *ansi_nist)
{
   int ret;

   if(record_i != UNDEFINED_INT){
      if(field_i != UNDEFINED_INT){
         if(subfield_i != UNDEFINED_INT){
            if(item_i != UNDEFINED_INT){
               /* Substitute item's value. */
               if((ret = biomeval_nbis_substitute_ANSI_NIST_item(record_i, field_i,
                                   subfield_i, item_i, newvalue, ansi_nist)))
                  return(ret);
            }
            /* Otherwise, item index undefined, so substitute subfield. */
            else{
               if((ret = biomeval_nbis_substitute_ANSI_NIST_subfield(record_i, field_i,
                                        subfield_i, newvalue, ansi_nist)))
                  return(ret);
            }
         }
         /* Otherwise, subfield index undefined, so substitute field. */
         else{
            if((ret = biomeval_nbis_substitute_ANSI_NIST_field(record_i, field_i,
                                                newvalue, ansi_nist)))
               return(ret);
         }
      }
      /* Otherwise, field index undefined, so substitute record. */
      else{
         if((ret = biomeval_nbis_substitute_ANSI_NIST_record(record_i, newvalue, ansi_nist)))
            return(ret);
      }
   }
   /* Otherwise, record index undefined, so ERROR. */
   else{
      /* So ignore request. */
      fprintf(stderr, "WARNING : biomeval_nbis_substitute_ANSI_NIST_select : "
	      "record index not specified so request ignored\n");
      return(-2);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_substitute_ANSI_NIST_record - Routine used to substitute the logical
#cat:              record contained in a specified file into an ANSI/NIST
#cat:              file structure.  This routine also updates the CNT field
#cat:              of the Type-1 record to accurately reflect the substitution.

   Input:
      record_i   - index of logical record to be substituted
      subfile    - filename whose contents is to be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_substitute_ANSI_NIST_record(const int record_i, const char *subfile,
                                ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *sub_ansi_nist;
   RECORD *record, *sub_record;
   int byte_adjust;

   if((ret = biomeval_nbis_read_fmttext_file(subfile, &sub_ansi_nist)))
      return(ret);

   if(sub_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_record : "
	      "number of records %d != 1 in fmttext file %s\n",
	      sub_ansi_nist->num_records, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-2);
   }
   sub_record = sub_ansi_nist->records[0];

   /* Substitution of a Type-1 record is an ERROR. */
   if(sub_record->type == TYPE_1_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_record : "
	      "substituting a Type-1 record not permitted\n");
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-3);
   }

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_record : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-4);
   }
   record = ansi_nist->records[record_i];

   /* If record types don't match ... */
   if(sub_record->type != record->type){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_record : "
	      "substitution record [Type-%d] in fmttext file %s != [Type-%d]\n",
	      sub_record->type, subfile, record->type);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-5);
   }

   /* Store old record's size. */
   byte_adjust = record->num_bytes;

   /* Deallocate old record. */
   biomeval_nbis_free_ANSI_NIST_record(record);
   /* Subtract old record's size from parent structures. */
   ansi_nist->num_bytes -= byte_adjust;
   /* Assign new record to list. */
   ansi_nist->records[record_i] = sub_record;
   /* Every tagged record is terminated with an FS separator, */
   /* so separator flag should already be set. */
   /* (Binary record fields do not use separator characters.) */
   /* Set new record's size. */
   byte_adjust = sub_record->num_bytes;

   /* Add new record's size to parent structures. */
   ansi_nist->num_bytes += byte_adjust;

   /* New record's LEN field is up to date. */

   /* Free parent substitution structures. */
   sub_ansi_nist->num_records = 0;
   biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);

   fprintf(stderr, "Substituted record index [%d] [Type-%d] "
	   "with contents of %s\n",
           record_i+1, ansi_nist->records[record_i]->type, subfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_substitute_ANSI_NIST_field - Routine used to substitute the field
#cat:              contained in a specified file into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of field to be substituted
      field_i    - index of field to be substituted
      subfile    - filename whose contents is to be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_substitute_ANSI_NIST_field(const int record_i, const int field_i,
                               const char *subfile, ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *sub_ansi_nist;
   RECORD *record;
   FIELD *field, *sub_field;
   int byte_adjust;

   if((ret = biomeval_nbis_read_fmttext_file(subfile, &sub_ansi_nist)))
      return(ret);

   if(sub_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "number of records %d != 1 in fmttext file %s\n",
              sub_ansi_nist->num_records, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-2);
   }

   if(sub_ansi_nist->records[0]->num_fields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "number of fields %d != 1 in fmttext file %s\n",
              sub_ansi_nist->records[0]->num_fields, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-3);
   }
   sub_field = sub_ansi_nist->records[0]->fields[0];

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-4);
   }
   record = ansi_nist->records[record_i];

   /* If record types don't match ... */
   if(sub_ansi_nist->records[0]->type != record->type){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "substitution record [Type-%d] in fmttext file %s != [Type-%d]\n",
              sub_ansi_nist->records[0]->type, subfile, record->type);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-5);
   }

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-6);
   }
   field = record->fields[field_i];

   /* If field IDs don't match ... */
   if(sub_ansi_nist->records[0]->fields[0]->field_int != field->field_int){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_field : "
	      "substitution field ID [Type-%d.%03d] in fmttext file %s "
	      "!= [Type-%d.%03d]\n",
              sub_ansi_nist->records[0]->type,
              sub_ansi_nist->records[0]->fields[0]->field_int,
	      subfile, record->type, field->field_int);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-7);
   }

   /* Store old field's size. */
   byte_adjust = field->num_bytes;

   /* Deallocate old field. */
   biomeval_nbis_free_ANSI_NIST_field(field);
   /* Subtract old field's size from parent structures. */
   record->num_bytes -= byte_adjust;
   ansi_nist->num_bytes -= byte_adjust;
   /* Assign new field to list. */
   record->fields[field_i] = sub_field;
   /* If field in a tagged field and field is NOT last in list ... */
   /* (Binary record fields do not use separator characters.) */
   if(biomeval_nbis_tagged_record(record->type) &&
      (field_i != record->num_fields-1)){
      /* Then set GS separtor. */
      /* (New field is only one, so GS separator should be FALSE.) */
      sub_field->gs_char = TRUE;
      /* Bump field size. */
      sub_field->num_bytes++;
   }
   /* Set new field's size. */
   byte_adjust = sub_field->num_bytes;

   /* Add new field's size to parent structures. */
   record->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Update the record's length LEN field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
      return(ret);

   /* Free parent substitution structures. */
   sub_ansi_nist->records[0]->num_fields = 0;
   biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);

   fprintf(stderr, "Substituted field index %d.%d [Type-%d.%03d] "
	   "with contents of %s\n",
           record_i+1, field_i+1, record->type, field->field_int, subfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_substitute_ANSI_NIST_subfield - Routine used to substitute the subfield
#cat:              contained in a specified file into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of subfield to be substituted
      field_i    - index of field of subfield to be substituted
      subfield_i - index of subfield to be substituted
      subfile    - filename whose contents is to be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_substitute_ANSI_NIST_subfield(const int record_i, const int field_i,
                                  const int subfield_i, const char *subfile,
                                  ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *sub_ansi_nist;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield, *sub_subfield;
   int byte_adjust;

   if((ret = biomeval_nbis_read_fmttext_file(subfile, &sub_ansi_nist)))
      return(ret);

   if(sub_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "number of records %d != 1 in fmttext file %s\n",
              sub_ansi_nist->num_records, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-2);
   }

   if(sub_ansi_nist->records[0]->num_fields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "number of fields %d != 1 in fmttext file %s\n",
              sub_ansi_nist->records[0]->num_fields, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-3);
   }
   if(sub_ansi_nist->records[0]->fields[0]->num_subfields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "number of subfields %d != 1 in fmttext file %s\n",
              sub_ansi_nist->records[0]->fields[0]->num_subfields, subfile);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-4);
   }
   sub_subfield = sub_ansi_nist->records[0]->fields[0]->subfields[0];

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-5);
   }
   record = ansi_nist->records[record_i];

   /* If record types don't match ... */
   if(sub_ansi_nist->records[0]->type != record->type){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : ");
      fprintf(stderr, "substitution record [Type-%d] in fmttext file %s "
	      "!= [Type-%d]\n",
              sub_ansi_nist->records[0]->type, subfile, record->type);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-6);
   }

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-7);
   }
   field = record->fields[field_i];

   /* If field IDs don't match ... */
   if(sub_ansi_nist->records[0]->fields[0]->field_int != field->field_int){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "substitution field ID [Type-%d.%03d] in fmttext file %s "
	      "!= [Type-%d.%03d]\n",
              sub_ansi_nist->records[0]->type,
              sub_ansi_nist->records[0]->fields[0]->field_int,
	      subfile, record->type, field->field_int);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-8);
   }

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i >= field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_subfield : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, field->num_subfields,
              record->type, field->field_int);
      biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);
      return(-9);
   }
   subfield = field->subfields[subfield_i];

   /* Store old subfield's size. */
   byte_adjust = subfield->num_bytes;

   /* Deallocate old subfield. */
   biomeval_nbis_free_ANSI_NIST_subfield(subfield);
   /* Subtract old subfield's size from parent structures. */
   field->num_bytes -= byte_adjust;
   record->num_bytes -= byte_adjust;
   ansi_nist->num_bytes -= byte_adjust;
   /* Assign new subfield to list. */
   field->subfields[subfield_i] = sub_subfield;
   /* If subfield in a tagged field and subfield is NOT last in list ... */
   /* (Binary record fields do not use separator characters.) */
   if(biomeval_nbis_tagged_record(record->type) &&
      (subfield_i != field->num_subfields-1)){
      /* Then set RS separtor. */
      /* (New subfield is only one, so RS separator should be FALSE.) */
      sub_subfield->rs_char = TRUE;
      /* Bump subfield size. */
      sub_subfield->num_bytes++;
   }
   /* Set new subfield's size. */
   byte_adjust = sub_subfield->num_bytes;

   /* Add new subfield's size to parent structures. */
   field->num_bytes += byte_adjust;
   record->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Update the record's length LEN field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
      return(ret);

   /* Free parent substitution structures. */
   sub_ansi_nist->records[0]->fields[0]->num_subfields = 0;
   biomeval_nbis_free_ANSI_NIST(sub_ansi_nist);

   fprintf(stderr, "Substituted subfield index [%d.%d.%d] [Type-%d.%03d] "
	   "with contents of %s\n",
           record_i+1, field_i+1, subfield_i+1,
           record->type, field->field_int, subfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_substitute_ANSI_NIST_item - Routine used to substitute the specified
#cat:              informaton item value into an ANSI/NIST file structure.

   Input:
      record_i   - index of logical record of item to be substituted
      field_i    - index of field of item to be substituted
      subfield_i - index of subfield of item to be substituted
      item_i     - index of information item to be substituted
      newvalue   - string value to be substituted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_substitute_ANSI_NIST_item(const int record_i, const int field_i,
                           const int subfield_i, const int item_i,
                           const char *newvalue, ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   int byte_adjust;
   int oldlen, newlen, newalloc;
   unsigned char *image_value;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_item : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_item : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      return(-3);
   }
   field = record->fields[field_i];

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i >= field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_item : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, field->num_subfields,
              record->type, field->field_int);
      return(-5);
   }
   subfield = field->subfields[subfield_i];

   /* If item index is out of range ... */
   if((item_i < 0) || (item_i >= subfield->num_items)){
      fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_item : "
	      "item index [%d.%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              subfield->num_items, record->type, field->field_int);
      return(-6);
   }
   item = subfield->items[item_i];

   /* If binary image field ...               */
   /* then item value must be read from file. */
   if(biomeval_nbis_image_field(field)){
      biomeval_nbis_read_binary_image_data(newvalue, &image_value, &newalloc);
      oldlen = item->num_bytes;
      newlen = newalloc;

      /* Assign new image value into item, and reset attributes. */
      free(item->value);
      item->value = image_value;
      item->alloc_chars = newalloc;
      item->num_chars = newalloc;
      item->num_bytes = newalloc;

      /* Set byte adjustment to difference in characters between */
      /* old and new values.                                     */
      byte_adjust = newlen - oldlen;
      /* Adjust size of subfield, field, record and ansi_nist structures. */
      subfield->num_bytes += byte_adjust;
      field->num_bytes += byte_adjust;
      record->num_bytes += byte_adjust;
      ansi_nist->num_bytes += byte_adjust;

      /* Update the record's length LEN field. */
      if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
         return(ret);

      fprintf(stderr, "Substituted binary image item index [%d.%d.%d.%d] "
	      "[Type-%d.%03d] with contents in %s",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int, newvalue);

   }
   /* Otherwise, tagged or binary (non-image) field ... */
   else{
      /* Store number of characters in old item value. */
      oldlen = item->num_chars;
      /* Compute number of characters in new item value. */
      newlen = strlen(newvalue);
      /* Set new value's allocation size (including NULL terminator). */
      newalloc = newlen+1;

      /* If necessary, reallocate the size of the item's value. */
      if(newalloc >= item->alloc_chars){
	 unsigned char * new_ptr =
	    (unsigned char *)realloc(item->value, newalloc);

         if(new_ptr == NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_substitute_ANSI_NIST_item : "
		    "realloc : item value (increase %d bytes to %d)\n",
		    item->alloc_chars, newalloc);
            return(-8);
         }
	 item->value = new_ptr;
         item->alloc_chars = newalloc;
      }

      fprintf(stderr, "Substituted item index [%d.%d.%d.%d] [Type-%d.%03d]\n"
	      "   Old value = %s\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int, item->value);

      /* Assign new value into item, and reset attributes. */
      strcpy((char *)item->value, newvalue);
      item->num_chars = strlen((char *)item->value);
      /* Binary fields should not have their num_bytes reset. */
      if(biomeval_nbis_tagged_record(record->type))
         item->num_bytes = item->num_chars;

      /* If item has a trailing US separator ... */
      /* Binary fields will not have US separator set. */
      if(item->us_char)
         /* Include separator in item's byte size. */
         item->num_bytes++;

      fprintf(stderr, "   New value = %s\n", item->value);

      /* If byte size of tagged field item has changed ... */
      /* Binary fields (other than the bnary image/data field handled above) */
      /* are fixed length and will not change size even if the number of     */
      /* charaters stored in their "string" value changes.                   */
      if(biomeval_nbis_tagged_record(record->type) && (oldlen != newlen)){
         /* Set byte adjustment to difference in characters between */
         /* old and new values.                                     */
         byte_adjust = newlen - oldlen;
         /* Adjust size of subfield, field, record and ansi_nist structures. */
         subfield->num_bytes += byte_adjust;
         field->num_bytes += byte_adjust;
         record->num_bytes += byte_adjust;
         ansi_nist->num_bytes += byte_adjust;

         /* Update the record's length LEN field. */
         if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
            return(ret);
      }
   }

   /* Return normally. */
   return(0);
}

