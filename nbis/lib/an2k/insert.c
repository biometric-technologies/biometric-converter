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

      FILE:    INSERT.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  01/31/2008 by Kenneth Ko

      REVISED: Joseph C. Konczal, 03/20/2008 - Broke out core
         functionality into *-core functions and added more *_frmem
         functions to add fields and subfields from structures
         residing only in memory.  Added new_idc parameter to
         biomeval_nbis_adjust_insrec_CNT_IDCs().
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2009 by jck - report more details when things go wrong

      Contains routines responsible for inserting specified
      data structures including logical records, fields, subfields,
      and information items into an ANSI/NIST file structure.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_do_insert()
                        biomeval_nbis_insert_ANSI_NIST_select()
                        biomeval_nbis_insert_ANSI_NIST_record()
                        biomeval_nbis_insert_ANSI_NIST_record_frmem()
                        biomeval_nbis_insert_ANSI_NIST_record_core()
                        biomeval_nbis_adjust_insrec_CNT_IDCs()
                        biomeval_nbis_insert_ANSI_NIST_field()
                        biomeval_nbis_insert_ANSI_NIST_field_frmem()
                        biomeval_nbis_insert_ANSI_NIST_field_core()
                        biomeval_nbis_insert_ANSI_NIST_subfield()
                        biomeval_nbis_insert_ANSI_NIST_subfield_frmem()
                        biomeval_nbis_insert_ANSI_NIST_subfield_core()
                        biomeval_nbis_insert_ANSI_NIST_item()

***********************************************************************/

#include <stdio.h>
#include <errno.h>

#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_do_insert - Master routine used to carry out a requested structure
#cat:              insertion and write the results to file.  The structure
#cat:              to be inserted is represented by a 4-tuple of indices.

   Input:
      record_i   - index of logical record to be inserted
      field_i    - index of field if to be inserted
      subfield_i - index of subfield if to be inserted
      item_i     - index of information item if to be inserted
      newvalue   - string value or filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ofile      - name of file results are to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_do_insert(const char *ofile,
              const int record_i, const int field_i,
              const int subfield_i, const int item_i,
              const char *newvalue, ANSI_NIST *ansi_nist)
{
   int ret;
   FILE *fpout;

   /* Conduct the item insertion. */
   if((ret = biomeval_nbis_insert_ANSI_NIST_select(record_i, field_i, subfield_i, item_i,
                                    newvalue, ansi_nist))){
      return(ret);
   }

   /* If an output file is specified, then open it for writing. */
   if(ofile != NULL){
      fpout = fopen(ofile, "wb");
      if(fpout == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_do_insert : fopen '%s': %s\n",
		 ofile, strerror(errno));
         return(-2);
      }
   }
   /* Otherwise, set the output file pointer to a default (ex. stdout). */
   else
      fpout = DEFAULT_FPOUT;

   if((ret = biomeval_nbis_write_ANSI_NIST(fpout, ansi_nist)))
      return(ret);

   /* Close the file pointer if necessary. */
   if(ofile != NULL){
      if(fclose(fpout)){
         fprintf(stderr, "ERROR : biomeval_nbis_do_insert : fclose '%s': %s\n",
		 ofile, strerror(errno));
         return(-3);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_select - Routine used to carry out a requested
#cat:              structure insertion based on a 4-tuple of structure
#cat:              indices. These indices represent the physical order
#cat:              of subsequent logical records, fields, subfields, and
#cat:              information items in the ANSI/NIST file structure.
#cat:              In order to specify the insertion of higher-level
#cat:              structures, subordinate structures are assigned the
#cat:              value of "UNDEFINED_INT".  When higher-level structures
#cat:              are specified, "newvalue" takes on a filename which
#cat:              contains the data to be inserted.

   Input:
      record_i   - index of logical record to be inserted
      field_i    - index of field if to be inserted
      subfield_i - index of subfield if to be inserted
      item_i     - index of information item if to be inserted
      newvalue   - string value or filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_select(const int record_i, const int field_i,
                         const int subfield_i, const int item_i,
                         const char *newvalue, ANSI_NIST *ansi_nist)
{
   int ret;

   if(record_i != UNDEFINED_INT){
      if(field_i != UNDEFINED_INT){
         if(subfield_i != UNDEFINED_INT){
            if(item_i != UNDEFINED_INT){
               /* Insert item's value. */
               if((ret = biomeval_nbis_insert_ANSI_NIST_item(record_i, field_i,
                                    subfield_i, item_i, newvalue, ansi_nist)))
                  return(ret);
            }
            /* Otherwise, item index undefined, so insert subfield. */
            else{
               if((ret = biomeval_nbis_insert_ANSI_NIST_subfield(record_i, field_i,
                                         subfield_i, newvalue, ansi_nist)))
                  return(ret);
            }
         }
         /* Otherwise, subfield index undefined, so insert field. */
         else{
            if((ret = biomeval_nbis_insert_ANSI_NIST_field(record_i, field_i,
                                            newvalue, ansi_nist)))
               return(ret);
         }
      }
      /* Otherwise, field index undefined, so insert record. */
      else{
         if((ret = biomeval_nbis_insert_ANSI_NIST_record(record_i, newvalue, ansi_nist)))
            return(ret);
      }
   }
   /* Otherwise, record index undefined, so ERROR. */
   else{
      /* So ignore request. */
      fprintf(stderr, "WARNING : biomeval_nbis_insert_ANSI_NIST_select : "
	      "record index not specified so request ignored\n");
      return(-2);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_record - Routine used to insert the logical record
#cat:              contained in a specified file into an ANSI/NIST file
#cat:              structure.  This routine also updates the CNT field of
#cat:              the Type-1 record to accurately reflect the insertion.
#cat:              Note - This routine used different IDC assignment scheme
#cat:              form biomeval_nbis_insert_ANSI_NIST_record_frmem.  In this routine, an
#cat:              IDC is assigned based on the record's position in the
#cat:              ANSI/NIST structure.

   Input:
      record_i   - index of logical record to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_record(const int record_i, const char *insfile,
                                ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *ins_ansi_nist;

   if((ret = biomeval_nbis_read_fmttext_file(insfile, &ins_ansi_nist)))
      return(ret);

   if(ins_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_record : "
	      "number of records %d != 1 in fmttext file %s\n",
	      ins_ansi_nist->num_records, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-2);
   }

   ret = biomeval_nbis_insert_ANSI_NIST_record_core(record_i, ins_ansi_nist->records[0],
				      TRUE, ansi_nist);
  /* Free parent information structures, or everything if insertion failed. */
   if (ret == 0)
      ins_ansi_nist->num_records = 0;
   biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
   if (ret < 0)
      return ret;

   fprintf(stderr, "Inserted record index [%d] [Type-%d] with contents of %s\n",
           record_i+1, ansi_nist->records[record_i]->type, insfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_record_frmem - Routine used to insert the logical
#cat:              record from memory into an ANSI/NIST file
#cat:              structure.  This routine also updates the CNT field of
#cat:              the Type-1 record to accurately reflect the insertion.
#cat:              Note - This routine used different IDC assignment scheme
#cat:              form biomeval_nbis_insert_ANSI_NIST_record.  In this routine, the
#cat:              IDC in the record to be inserted is used directly to
#cat:              update the Type-1 CNT field.

   Input:
      record_i   - index of logical record to be inserted
      record     - record structure to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_record_frmem(const int record_i, RECORD *record,
                                  ANSI_NIST *ansi_nist)
{
   int ret;

   if ((ret = biomeval_nbis_insert_ANSI_NIST_record_core(record_i, record,
					   FALSE, ansi_nist)) < 0)
      return ret;

   fprintf(stderr, "Inserted record index [%d] [Type-%d]\n",
           record_i+1, ansi_nist->records[record_i]->type);

   /* Return normally. */
   return 0;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_record_core - This routine incorporates the
#cat:              core functionality of the two preceeding routines
#cat:              used to insert a logical record into an ANSI/NIST
#cat:              file structure.  This routine also calls the next
#cat:              routine in order to update the CNT field of the
#cat:              Type-1 record to accurately reflect the insertion.
#cat:              Note - This routine selects one of two different IDC
#cat:              assignment schemes, keep the IDC in the record, or
#cat:              assign a new IDC based on the record's new location
#cat:              in the file structure.

   Input:
      record_i   - index of logical record to be inserted
      record     - record structure to be inserted
      new_idc    - TRUE: change the IDC to a new unused value, or
                   FALSE: use the IDC that is already in the record
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_record_core(const int record_i, RECORD *record,
				 int new_idc, ANSI_NIST *ansi_nist)
{
   int j, k;

   /* Insertion of a Type-1 record is an ERROR. */
   if(record->type == TYPE_1_ID){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_record_core : "
	      "inserting a Type-1 record not permitted\n");
      return(-3);
   }

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i > ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_record_core : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records+1);
      return(-4);
   }

   /* Make sure list has room for new record. */
   if(ansi_nist->num_records >= ansi_nist->alloc_records){
      size_t new_size = (ansi_nist->alloc_records
			   + ANSI_NIST_CHUNK) * sizeof(RECORD *);
      RECORD ** new_ptr = (RECORD **)realloc(ansi_nist->records, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_record_core : "
		 "realloc : record list (increase %lu bytes to %lu)\n",
		 (unsigned long)(ansi_nist->alloc_records*sizeof(RECORD *)),
		 (unsigned long)new_size);
         return(-5);
      }
      ansi_nist->records = new_ptr;
      ansi_nist->alloc_records += ANSI_NIST_CHUNK;
   }

   /* Make room for new record in list. */
   for(j = ansi_nist->num_records-1, k = ansi_nist->num_records;
       j >= record_i;
       j--, k--){
       ansi_nist->records[k] = ansi_nist->records[j];
   }

   /* Insert new record into list. */
   ansi_nist->records[record_i] = record;
   ansi_nist->num_records++;
   /* Insertion record will already have FS separator properly set. */
   /* Bump parent structure by size of insertion record. */
   ansi_nist->num_bytes += record->num_bytes;
   /* Insertion records length is already up to date. */

   /* By adding a record, the Type-1 CNT field must be updated to  */
   /* include a new subfield with the type and IDC of the inserted */
   /* record.                                                      */
   return biomeval_nbis_adjust_insrec_CNT_IDCs(record_i, new_idc, ansi_nist);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_adjust_insrec_CNT_IDCs - Routine used to update an ANSI/NIST
#cat:              file structure's Type-1 record, CNT 1.3 field, due
#cat:              to a logical record's insertion.  This routine also
#cat:              manages the preexisting records' IDC values in an
#cat:              attempt to maintain contiguous IDCs.  This routine
#cat:              assumes the new record has already been inserted.
#cat:              The existing IDC in the inserted record can be
#cat:              retained, or a new IDC can be assigned, depending
#cat:              on the value of the new_idc argument.  All the
#cat:              appropriate adjustments are made to the IDC values
#cat:              in the Type-1 record and in the other corresponding
#cat:              records.

   Input:
      record_i   - index of inserted logical record
      new_idc    - TRUE: assign a new IDC, FALSE: keep the IDC in the record
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_adjust_insrec_CNT_IDCs(const int record_i, const int new_idc, 
			   ANSI_NIST *ansi_nist)
{
   int ret; 
   FIELD *cntfield, *idcfield;
   SUBFIELD *subfield;
   ITEM *item;
   int cntfield_i, idcfield_i;
   int itemint, insidc;
   int j, k, maxv, byte_adjust;
   char numstr[FIELD_NUM_LEN+1];

   /* Need to adjust Type-1 CNT 1.3 field ... */
   fprintf(stderr, "Updating CNT field [Type-1.%03d]\n", CNT_ID);
   
   /* If Type-1 record not found ... */
   /* Type-1 record must be first in record list ... */
   if((ansi_nist->num_records <= 0) ||
      (ansi_nist->records[0]->type != TYPE_1_ID)){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
	      "Type-1 record not found\n");
      return(-2);
   }
   
   /* If CNT field within Type-1 record not found ... */
   if(!biomeval_nbis_lookup_ANSI_NIST_field(&cntfield, &cntfield_i, CNT_ID,
			      ansi_nist->records[0])){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
	      "CNT field not found in record index [1] [Type-1.%03d]\n",
	      CNT_ID);
      return(-3);
   }
   
   /* If record index is out of range ... */
   if((record_i < 0) || (record_i > cntfield->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
	      "record index [%d] out of range [1..%d] of subfields "
	      "in CNT field index [1.%d] [Type-1.%03d]\n",
	      record_i+1, cntfield->num_subfields+1, cntfield_i+1, CNT_ID);
      return(-4);
   }

   if (new_idc) {
      /* Determine new record's IDC by searching CNT list of IDC's */
      /* and determining maximum IDC in list up to insertion point */
      /* of new record in the list.  The new record's IDC should   */
      /* be one greater than the maximum IDC to this point.        */
      
      /* Find maximum IDC to record's insertion point in CNT field. */
      maxv = -1;
      /* For each subfield in CNT field (skipping the first one which */
      /* has special meaning) ... */
      for(j = 1; j < record_i; j++){
	 /* Look up the IDC in the current subfield. */
	 if(!biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, cntfield->subfields[j])){
	    fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		    "IDC item index [1.%d.%d.2] not found in "
		    "CNT field [Type-1.%03d]\n", cntfield_i+1, j+1, CNT_ID);
	    return(-5);
	 }
	 /* Convert item value to numeric integer. */
	 itemint = atoi((char *)item->value);
	 /* Keep track of running maximum ... */
	 if(itemint > maxv)
	    maxv = itemint;
      }
      
      /* If record being inserted at beginning of list ... */
      if(maxv == -1)
	 /* Then its IDC is set to 0. */
	 insidc = 0;
      else
	 /* Otherwise, its IDC is set to max found + 1. */
	 insidc = maxv + 1;
   } else {
      /* Keep the record's existing IDC. */

      /* If IDC field within inserted record not found ... */
      if(!biomeval_nbis_lookup_ANSI_NIST_field(&idcfield, &idcfield_i, IDC_ID,
				 ansi_nist->records[record_i])){
	 fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		 "IDC field not found in inserted record [Type-%d.%03d]\n", 
		 ansi_nist->records[record_i]->type, IDC_ID);
	 return(-6);
      }
      /* Set inserted IDC value. */
      insidc = atoi((char *)idcfield->subfields[0]->items[0]->value);
   }
      
   /* Make sure list has room for new subfield. */
   if(cntfield->num_subfields >= cntfield->alloc_subfields){
      size_t new_size = (cntfield->alloc_subfields
			   + ANSI_NIST_CHUNK) * sizeof(SUBFIELD *);
      SUBFIELD ** new_ptr = (SUBFIELD **)realloc(cntfield->subfields, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		 "realloc : CNT subfield list (increase %lu bytes to %lu)\n",
		 (unsigned long)(cntfield->alloc_subfields*sizeof(SUBFIELD *)),
		 (unsigned long)new_size);
         return(-6);
      }
      cntfield->subfields = new_ptr;
      cntfield->alloc_subfields += ANSI_NIST_CHUNK;
   }

   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)))
      return(ret);

   /* Make room for new subfield in list. */
   for(j = cntfield->num_subfields-1, k = cntfield->num_subfields;
       j >= record_i;
       j--, k--){
      cntfield->subfields[k] = cntfield->subfields[j];
   }

   /* Insert new subfield into the list. */
   cntfield->subfields[record_i] = subfield;
   cntfield->num_subfields++;
   /* New subfield is currently empty. */
   byte_adjust = 0;
   
   /* If CNT field has more than 1 subfield ... */
   /* then need to handle RS separators.        */
   if(cntfield->num_subfields > 1){
      /*  If new subfield was added at end of list ... */
      if(record_i == (cntfield->num_subfields - 1)){
         /* Then the next to last subfield in the list must have */
         /* an RS separator added.                               */
         cntfield->subfields[record_i-1]->rs_char = TRUE;
         cntfield->subfields[record_i-1]->num_bytes++;
      }
      /* Otherwise, the new subfield is not at the end of the list ... */
      else{
         /* So, a RS separator must be added to the new subfield. */
         subfield->rs_char = TRUE;
         subfield->num_bytes++;
      }
      /* Either way a separator byte has been added. */
      byte_adjust++;
   }
   /* Otherwise, field was empty so new subfield will */
   /* not have an RS separator.                       */

   /* Add byte adjustment to parent structures. */
   cntfield->num_bytes += byte_adjust;
   ansi_nist->records[0]->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Add inserted record TYPE into first item of new CNT subfield. */
   sprintf(numstr, "%d", ansi_nist->records[record_i]->type);
   if((ret = biomeval_nbis_insert_ANSI_NIST_item(0, cntfield_i, record_i, 0,
                                  numstr, ansi_nist)))
      return(ret);

   /* Add inserted record's IDC into second item of new CNT subfield. */
   sprintf(numstr, IDC_FMT, insidc);
   if((ret = biomeval_nbis_insert_ANSI_NIST_item(0, cntfield_i, record_i, 1,
                                  numstr, ansi_nist)))
      return(ret);

   if(cntfield->subfields[0]->num_items != 2){
      fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
	      "bad format of CNT subfield index [1.%d.1]"
	      "in record [Type-1.%03d]\n", cntfield_i+1, CNT_ID);
				/* Updated by MDG on 03-08-05 */
      return(-7);
   }

   /* Bump number of records in CNT field. */
   if((ret = biomeval_nbis_increment_numeric_item(0, cntfield_i, 0, 1, ansi_nist, NULL)))
      return(ret);

   if (new_idc) {
      /* If a new IDC was assigned, the IDC fields in the following
	 records, and their corresponding CNT items, need to be
	 incremented. */
      
      /* Lookup the IDC in inserted record. */
      if(!biomeval_nbis_lookup_ANSI_NIST_field(&idcfield, &idcfield_i, IDC_ID,
				 ansi_nist->records[record_i])){
	 fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		 "IDC field not found in record index [%d] [Type-%d.%03d]\n",
		 record_i+1, ansi_nist->records[record_i]->type, IDC_ID);
	 return(-8);
      }
      
      /* Substitute item in IDC field in the inserted record. */
      if((ret = biomeval_nbis_substitute_ANSI_NIST_item(record_i, idcfield_i, 0, 0,
					  numstr, ansi_nist)))
	 return(ret);
      
      /* Any IDC (prior to record insertion) in CNT field >= new record's IDC */
      /* needs to be incremented.  IDCs in corresponding records need to be   */
      /* incremented as well.                                                 */
      
      fprintf(stderr, "Adjusting previous IDCs in CNT field [Type-1.%03d]\n",
	      CNT_ID);
      
      /* We know the new record's IDC is greater than any preceeding record's */
      /* IDC ... So, for each record after the new record in the list ...     */
      for(j = record_i+1; j < cntfield->num_subfields; j++){
	 /* Look up IDC of current record. */
	 if(!biomeval_nbis_lookup_ANSI_NIST_item(&item, 1, cntfield->subfields[j])){
	    fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		    "IDC item index [1.%d.2] not found in record "
		    "[Type-1.%03d]\n", j+1, CNT_ID);
	    return(-9);
	 }
	 /* Convert item's value to numeric integer. */
	 itemint = atoi((char *)item->value);
	 /* If the current IDC is >= new record's IDC ... */
	 if(itemint >= insidc){
	    /* Increment the current IDC. */
	    if((ret = biomeval_nbis_increment_numeric_item(0, cntfield_i, j, 1,
					     ansi_nist, IDC_FMT)))
	       return(ret);
	    
	    /* 'j' is the index of current record. */
	    
	    /* Lookup IDC field in corresponding record. */
	    if(!biomeval_nbis_lookup_ANSI_NIST_field(&idcfield, &idcfield_i, IDC_ID,
				       ansi_nist->records[j])){
	       fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		       "IDC field not found in record index [%d] "
		       "[Type-%d.%03d]\n",
		       j+1, ansi_nist->records[j]->type, IDC_ID);
	       return(-10);
	    }
	    
	    if((idcfield->num_subfields != 1) ||
	       (idcfield->subfields[0]->num_items != 1)){
	       fprintf(stderr, "ERROR : biomeval_nbis_adjust_insrec_CNT_IDCs : "
		       "bad format of IDC field "
		       "in record index [%d] [Type-%d.%03d]\n",
		       j+1, ansi_nist->records[j]->type, IDC_ID);
	       return(-11);
	    }
	    
	    if((ret = biomeval_nbis_increment_numeric_item(j, idcfield_i, 0, 0,
					     ansi_nist, IDC_FMT)))
	       return(ret);
	 }
	 /* Otherwise, current record's IDC is OK. */
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_field - Routine used to insert the field
#cat:              contained in a specified file into an ANSI/NIST
#cat:              file structure.

   Input:
      record_i   - index of logical record of field to be inserted
      field_i    - index of field to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_field(const int record_i, const int field_i,
                           const char *insfile, ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *ins_ansi_nist;

   if((ret = biomeval_nbis_read_fmttext_file(insfile, &ins_ansi_nist)) < 0)
      return(ret);
      
   if(ins_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field : "
	      "number of records %d != 1 in fmttext file %s\n",
              ins_ansi_nist->num_records, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-2);
   }

   if(ins_ansi_nist->records[0]->num_fields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field : "
	      "number of fields %d != 1 in fmttext file %s\n",
              ins_ansi_nist->records[0]->num_fields, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-3);
   }

   ret = biomeval_nbis_insert_ANSI_NIST_field_core(record_i, field_i,
				     ins_ansi_nist->records[0]->fields[0],
				     ansi_nist);
  /* Free parent information structures, or everything if insertion failed. */
   if (ret == 0)
      ins_ansi_nist->records[0]->num_fields = 0;
   biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
   if (ret < 0)
      return ret;
   
   /* Above, biomeval_nbis_insert_ANSI_NIST_field_core() has already verified that
      the specified record and field exist. */
   fprintf(stderr, "Inserted field index [%d.%d] [Type-%d.%03d] "
           "with contents of %s\n", record_i+1, field_i+1,
	   ansi_nist->records[record_i]->type,
           ansi_nist->records[record_i]->fields[field_i]->field_int, insfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_field_frmem - Routine used to insert a field
#cat:              contained in memory into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of field to be inserted
      field_i    - index of field to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_field_frmem(const int record_i, const int field_i,
				 FIELD *ins_field, ANSI_NIST *ansi_nist)
{
   int ret;

   if ((ret = biomeval_nbis_insert_ANSI_NIST_field_core(record_i, field_i,
					  ins_field, ansi_nist)) < 0)
      return ret;

   /* Above, biomeval_nbis_insert_ANSI_NIST_field_core() has already verified that
      the specified record and field exist. */
   fprintf(stderr, "Inserted field index [%d.%d] [Type-%d.%03d]\n",
	   record_i+1, field_i+1, ansi_nist->records[record_i]->type,
           ansi_nist->records[record_i]->fields[field_i]->field_int);
   
   /* return normally */
   return 0;
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_field_core - Routine that implements the core
#cat:              functionality of the preceeding two functions,
#cat:              which insert a field into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of field to be inserted
      field_i    - index of field to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_field_core(const int record_i, const int field_i,
				FIELD *ins_field, ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   int byte_adjust, j, k;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-5);
   }
   record = ansi_nist->records[record_i];

   /* Do not permit the insertion of binary fields as binary */
   /* records are "fixed."                                   */
   if(biomeval_nbis_binary_record(record->type)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
	      "insertion of binary field [Type-%d.%03d] not permitted\n",
              ins_field->record_type, ins_field->field_int);
      return(-4);
   }

   /* If record types do not match ... */
   if(ins_field->record_type != record->type){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
	      "insertion record type [Type-%d] != [Type-%d]\n",
	      ins_field->record_type, record->type);
      return(-6);
   }

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i > record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields+1, record->type);
      return(-7);
   }

   /* Test for duplicate field. */
   for(j = 0; j < record->num_fields; j++){
      if(record->fields[j]->field_int == ins_field->field_int){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
		 "duplicate field ID [Type-%d.%03d] at field index [%d.%d]\n",
                 record->type, record->fields[j]->field_int, record_i+1, j+1);
         return(-8);
      }
   }

   /* Make sure list has room for new field. */
   if(record->num_fields >= record->alloc_fields){
      size_t new_size =  (record->alloc_fields
			    + ANSI_NIST_CHUNK) * sizeof(FIELD *);
      FIELD ** new_ptr = (FIELD **)realloc(record->fields, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field_core : "
		 "realloc : field list (increase %lu bytes to %lu)\n",
		 (unsigned long)(record->alloc_fields*sizeof(FIELD *)),
		 (unsigned long)new_size);
         return(-9);
      }
      record->fields = new_ptr;
      record->alloc_fields += ANSI_NIST_CHUNK;
   }

   /* Make room for new field in list. */
   for(j = record->num_fields-1, k = record->num_fields;
       j >= field_i;
       j--, k--){
      record->fields[k] = record->fields[j];
   }

   /* Insert new field into the list. */
   record->fields[field_i] = ins_field;
   record->num_fields++;
   byte_adjust = ins_field->num_bytes;

   /* If tagged field record and record has more than 1 field ... */
   /* then need to handle GS separators.                          */
   if(biomeval_nbis_tagged_record(record->type) &&
      (record->num_fields > 1)){
      /*  If new field was added at end of list ... */
      if(field_i == (record->num_fields - 1)){
         /* Then the next to last field in the list must have */
         /* an GS separator added.                            */
         record->fields[field_i-1]->gs_char = TRUE;
         record->fields[field_i-1]->num_bytes++;
      }
      /* Otherwise, the new field is not at the end of the list ... */
      else{
         /* So, a GS separator must be added to the new field. */
         ins_field->gs_char = TRUE;
         ins_field->num_bytes++;
      }
      /* Either way a separator byte has been added. */
      byte_adjust++;
   }
   /* Otherwise, record is binary or was empty so new field will */
   /* not have a GS separator.                                   */

   /* Adjust size of parent structures. */
   record->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Update the record's length LEN field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
      return(ret);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_subfield - Routine used to insert the subfield
#cat:              contained in a specified file into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of subfield to be inserted
      field_i    - index of field of subfield to be inserted
      subfield_i - index of subfield to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_subfield(const int record_i, const int field_i,
                              const int subfield_i, const char *insfile,
                              ANSI_NIST *ansi_nist)
{
   int ret;
   ANSI_NIST *ins_ansi_nist;
   RECORD *record;
   FIELD *field;

   if((ret = biomeval_nbis_read_fmttext_file(insfile, &ins_ansi_nist)))
      return(ret);

   if(ins_ansi_nist->num_records != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "number of records %d != 1 in fmttext file %s\n",
              ins_ansi_nist->num_records, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-2);
   }

   if(ins_ansi_nist->records[0]->num_fields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "number of fields %d != 1 in fmttext file %s\n",
              ins_ansi_nist->records[0]->num_fields, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-4);
   }

   if(ins_ansi_nist->records[0]->fields[0]->num_subfields != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "number of subfields %d != 1 in fmttext file %s\n",
              ins_ansi_nist->records[0]->fields[0]->num_subfields, insfile);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-5);
   }

   /* If record index is out of range (since we need the record below) ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_field : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-6);
   }
   record = ansi_nist->records[record_i];

   /* If record types don't match ... */
   if(ins_ansi_nist->records[0]->type != record->type){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "insertion record [Type-%d] in fmttext file %s != [Type-%d]\n",
              ins_ansi_nist->records[0]->type, insfile, record->type);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-7);
   }

   /* If field index is out of range (since we need the field below)... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
      return(-8);
   }
   field = record->fields[field_i];

   /* If field ID's don't match ... */
   if(ins_ansi_nist->records[0]->fields[0]->field_int != field->field_int){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "insertion field ID [Type-%d.%03d] in fmttext file %s "
	      "!= [Type-%d.%03d]\n",
	      ins_ansi_nist->records[0]->type,
              ins_ansi_nist->records[0]->fields[0]->field_int,
	      insfile, record->type, field->field_int);
      return(-9);
   }

   ret = biomeval_nbis_insert_ANSI_NIST_subfield_core( record_i, field_i, subfield_i,
	ins_ansi_nist->records[0]->fields[0]->subfields[0], ansi_nist );

  /* Free parent information structures, or everything if insertion failed. */
   if (ret == 0)
      ins_ansi_nist->records[0]->fields[0]->num_subfields = 0;
   biomeval_nbis_free_ANSI_NIST(ins_ansi_nist);
   if (ret < 0)
      return ret;

   fprintf(stderr, "Inserted subfield index [%d.%d.%d] [Type-%d.%03d] "
	   "with contents of %s\n", record_i+1, field_i+1, subfield_i+1,
           record->type, record->fields[field_i]->field_int, insfile);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_subfield_frmem - Routine used to insert the
#cat:              subfield contained in memory into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of subfield to be inserted
      field_i    - index of field of subfield to be inserted
      subfield_i - index of subfield to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_subfield_frmem(const int record_i, const int field_i,
				    const int subfield_i,
				    SUBFIELD *ins_subfield,
				    ANSI_NIST *ansi_nist)
{
   int ret;
   
   if ((ret = biomeval_nbis_insert_ANSI_NIST_subfield_core(record_i, field_i, subfield_i,
					     ins_subfield, ansi_nist)) < 0)
      return ret;
   
   fprintf(stderr, "Inserted subfield index [%d.%d.%d] [Type-%d.%03d]\n", 
	   record_i+1, field_i+1, subfield_i+1,
	   ansi_nist->records[record_i]->type,
	   ansi_nist->records[record_i]->fields[field_i]->field_int);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_subfield_core - Routine used to insert the subfield
#cat:              contained in a specified file into an ANSI/NIST file
#cat:              structure.

   Input:
      record_i   - index of logical record of subfield to be inserted
      field_i    - index of field of subfield to be inserted
      subfield_i - index of subfield to be inserted
      insfile    - filename whose contents is to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_subfield_core(const int record_i, const int field_i,
                              const int subfield_i, SUBFIELD *ins_subfield,
                              ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   FIELD *field;
   int byte_adjust, j, k;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-6);
   }
   record = ansi_nist->records[record_i];

   /* Do not permit subfield insertions in a binary record ... */
   /* (binary records are "fixed").                            */
   if(biomeval_nbis_binary_record(record->type)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	     "insertion of subfield in binary record [Type-%d] not permitted\n",
              record->type);
      return(-3);
   }

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      return(-8);
   }
   field = record->fields[field_i];

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i > field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n", record_i+1, field_i+1,
	      subfield_i+1, field->num_subfields+1, record->type,
	      field->field_int);
      return(-10);
   }

   /* Make sure list has room for new subfield. */
   if(field->num_subfields >= field->alloc_subfields){
      size_t new_size = (field->alloc_subfields
			   + ANSI_NIST_CHUNK) * sizeof(SUBFIELD *);
      SUBFIELD ** new_ptr = (SUBFIELD **)realloc(field->subfields, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_subfield : "
		 "realloc : subfield list (increase %lu bytes to %lu)\n",
		 (unsigned long)(field->alloc_subfields*sizeof(SUBFIELD *)),
		 (unsigned long)new_size);
         return(-11);
      }
      field->subfields = new_ptr;
      field->alloc_subfields += ANSI_NIST_CHUNK;
   }

   /* Make room for new subfield in list. */
   for(j = field->num_subfields-1, k = field->num_subfields;
       j >= subfield_i;
       j--, k--){
      field->subfields[k] = field->subfields[j];
   }

   /* Insert new subfield into the list. */
   field->subfields[subfield_i] = ins_subfield;
   field->num_subfields++;
   byte_adjust = ins_subfield->num_bytes;

   /* If tagged field record and field has more than 1 subfield ... */
   /* then need to handle RS separators.                            */
   if(biomeval_nbis_tagged_record(record->type) &&
      (field->num_subfields > 1)){
      /*  If new subfield was added at end of list ... */
      if(subfield_i == (field->num_subfields - 1)){
         /* Then the next to last subfield in the list must have */
         /* an RS separator added.                               */
         field->subfields[subfield_i-1]->rs_char = TRUE;
         field->subfields[subfield_i-1]->num_bytes++;
      }
      /* Otherwise, the new subfield is not at the end of the list ... */
      else{
         /* So, a RS separator must be added to the new subfield. */
         ins_subfield->rs_char = TRUE;
         ins_subfield->num_bytes++;
      }
      /* Either way a separator byte has been added. */
      byte_adjust++;
   }
   /* Otherwise, field is binary or was empty so new subfield will */
   /* not have an RS separator.                                */

   /* Adjust size of parent structures. */
   field->num_bytes += byte_adjust;
   record->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Update the current record's length field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
      return(ret);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_insert_ANSI_NIST_item - Routine used to insert the specified
#cat:              informaton item value into an ANSI/NIST file structure.

   Input:
      record_i   - index of logical record of item to be inserted
      field_i    - index of field of item to be inserted
      subfield_i - index of subfield of item to be inserted
      item_i     - index of information item to be inserted
      itemvalue  - string value to be inserted
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ansi_nist  - the modified ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_insert_ANSI_NIST_item(const int record_i, const int field_i,
                          const int subfield_i, const int item_i,
                          const char *itemvalue, ANSI_NIST *ansi_nist)
{
   int ret, j, k;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;
   int alloc_chars, byte_adjust;

   /* If record index is out of range ... */
   if((record_i < 0) || (record_i >= ansi_nist->num_records)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "record index [%d] out of range [1..%d]\n",
              record_i+1, ansi_nist->num_records);
      return(-2);
   }
   record = ansi_nist->records[record_i];

   /* Do not permit item insertions in a binary record ... */
   /* (binary records are "fixed").                        */
   if(biomeval_nbis_binary_record(record->type)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "insertion of item in binary record [Type-%d] not permitted\n",
              record->type);
      return(-3);
   }

   /* If field index is out of range ... */
   if((field_i < 0) || (field_i >= record->num_fields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "field index [%d] out of range [1..%d] in record [Type-%d]\n",
              field_i+1, record->num_fields, record->type);
      return(-4);
   }
   field = record->fields[field_i];

   /* Do not permit item insertions in tagged image fields ... */
   /* (tagged image fields have 1 and only one item)           */
   if(biomeval_nbis_image_field(field)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "insertion of image item in field index [%d.%d] [Type-%d.%03d] "
	      "not permitted\n", record_i+1, field_i+1,
	      record->type, field->field_int);
      return(-5);
   }

   /* If subfield index is out of range ... */
   if((subfield_i < 0) || (subfield_i >= field->num_subfields)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "subfield index [%d.%d.%d] out of range [1..%d] "
	      "in record [Type-%d.%03d]\n", record_i+1, field_i+1,
	      subfield_i+1, field->num_subfields,
	      record->type, field->field_int);
      return(-6);
   }
   subfield = field->subfields[subfield_i];

   /* If item index is not out of range for insertion ... */
   if((item_i < 0) || (item_i > subfield->num_items)){
      fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
	      "item index [%d.%d.%d.%d] out of range [1..%d] "
              "in record [Type-%d.%03d]\n", record_i+1, field_i+1,
	      subfield_i+1, item_i+1, subfield->num_items+1,
	      record->type, field->field_int);
      return(-7);
   }

   /* Make sure list has room for new item. */
   if(subfield->num_items >= subfield->alloc_items){
      size_t new_size = (subfield->alloc_items
			   + ANSI_NIST_CHUNK) * sizeof(ITEM *);
      ITEM ** new_ptr = (ITEM **)realloc(subfield->items, new_size);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
		 "realloc : item list (increase %lu bytes to %lu)\n",
		 (unsigned long)(subfield->alloc_items * sizeof(ITEM *)),
		 (unsigned long)new_size);
         return(-8);
      }
      subfield->items = new_ptr;
      subfield->alloc_items += ANSI_NIST_CHUNK;
   }

   /* Allocate and initialize the new item. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)))
      return(ret);
   /* Remeber 'alloc_chars' include NULL terminator. */
   alloc_chars = strlen(itemvalue)+1;
   if(alloc_chars >= item->alloc_chars){
      unsigned char * new_ptr = 
	 (unsigned char *)realloc(item->value, alloc_chars);

      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_insert_ANSI_NIST_item : "
		 "realloc : item value (increase %d bytes to %d)\n",
		 item->alloc_chars, alloc_chars);
         return(-9);
      }
      item->value = new_ptr;
      item->alloc_chars = alloc_chars;
   }
   /* Set item attributes. */
   strcpy((char *)item->value, itemvalue);
   item->num_chars = strlen((char *)item->value);
   item->num_bytes = item->num_chars;

   /* Make room for new item in list. */
   for(j = subfield->num_items-1, k = subfield->num_items;
       j >= item_i;
       j--, k--){
      subfield->items[k] = subfield->items[j];
   }

   /* Insert new item into the subfield list. */
   subfield->items[item_i] = item;
   subfield->num_items++;
   /* Remember byte adjustment does not include NULL terminator. */
   byte_adjust = item->num_chars;

   /* If subfield has more than one item ... */
   /* then need to handle US separators.     */
   if(subfield->num_items > 1){
      /*  If new item was added at end of list ... */
      if(item_i == (subfield->num_items-1)){
         /* Then the next to last item in list must have */
         /* a US separator added.                        */
         subfield->items[item_i-1]->us_char = TRUE;
         subfield->items[item_i-1]->num_bytes++;
      }
      /* Otherwise, the new item is not at the end of the list ... */
      else{
         /* So, a US separator must be added to the new item. */
         item->us_char = TRUE;
         item->num_bytes++;
      }
      /* Either way a separator byte has been added. */
      byte_adjust++;
   }
   /* Otherwise, subfield is empty so new item will not have a */
   /* US separator.                                            */

   /* Adjust size of parent structures. */
   subfield->num_bytes += byte_adjust;
   field->num_bytes += byte_adjust;
   record->num_bytes += byte_adjust;
   ansi_nist->num_bytes += byte_adjust;

   /* Update the current record's length field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LEN(ansi_nist, record_i)))
      return(ret);

   fprintf(stderr, "Inserted item index [%d.%d.%d.%d] [Type-%d.%03d] = %s\n",
           record_i+1, field_i+1, subfield_i+1, item_i+1,
           record->type, field->field_int, item->value);

   /* Return normally. */
   return(0);
}

