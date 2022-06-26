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

      FILE:    PRINT.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains routines responsible for printing the contents of
      an ANSI/NIST file structure in a textual format that can
      subsequently be viewed edited.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_do_print()
                        biomeval_nbis_print_ANSI_NIST_select()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_do_print - Master routine used to print the requested logical record,
#cat:              field, subfield, or information item to either the
#cat:              specified file or to standard output.  The structure
#cat:              to be printed is represented by a 4-tuple of indices.

   Input:
      record_i   - index of logical record to be printed
      field_i    - index of field if to be printed
      subfield_i - index of subfield if to be printed
      item_i     - index of information item if to be printed
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      ofile      - name of file to be written to (NULL == standard output)
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_do_print(const char *ofile,
             const int record_i, const int field_i,
             const int subfield_i, const int item_i,
             ANSI_NIST *ansi_nist)
{
   int ret;
   FILE *fpout;

   /* If an output file is specified, then open it for writing. */
   if(ofile != NULL){
      fpout = fopen(ofile, "wb");
      if(fpout == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_do_print : fopen : %s\n", ofile);
         exit(-2);
      }
   }
   /* Otherwise, set the output file pointer to a default (ex. stdout). */
   else
      fpout = DEFAULT_FPOUT;

   /* Print the contents of the specified structure to the open */
   /* file pointer. */
   if((ret = biomeval_nbis_print_ANSI_NIST_select(fpout, record_i, field_i,
                                   subfield_i, item_i, ansi_nist))){
      if(ofile != NULL){
         if(fclose(fpout)){
            fprintf(stderr, "ERROR : biomeval_nbis_do_print : fclose : %s\n", ofile);
            exit(-3);
         }
      }

      return(ret);
   }

   /* Close the file pointer if necessary. */
   if(ofile != NULL){
      if(fclose(fpout)){
         fprintf(stderr, "ERROR : biomeval_nbis_do_print : fclose : %s\n", ofile);
         exit(-4);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_print_ANSI_NIST_select - Routine used to print in a textual format
#cat:              the contents of a requested structure based on a 4-tuple
#cat:              of structure indices. These indices represent the physical
#cat:              order of subsequent logical records, fields, subfields, and
#cat:              information items in the ANSI/NIST file structure.
#cat:              In order to specify the printing of higher-level
#cat:              structures, subordinate structures are assigned the value
#cat:              of "UNDEFINED_INT".

   Input:
      record_i   - index of logical record to be printed
      field_i    - index of field if to be printed
      subfield_i - index of subfield if to be printed
      item_i     - index of information item if to be printed
      ansi_nist  - ANSI/NIST file structure to be modified
   Output:
      fpout      - the open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_print_ANSI_NIST_select(FILE *fpout, const int record_i, const int field_i,
                           const int subfield_i, const int item_i,
                           ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM *item;

   /* If record index defined ... */
   if(record_i != UNDEFINED_INT){
      if((record_i < 0) || (record_i >= ansi_nist->num_records)){
         fprintf(stderr, "ERROR : biomeval_nbis_print_ANSI_NIST_select : "
		 "record index [%d] out of range [1..%d]\n",
                 record_i+1, ansi_nist->num_records);
         return(-2);
      }
      record = ansi_nist->records[record_i];

      /* If field index is defined ... */
      if(field_i != UNDEFINED_INT){
         if((field_i < 0) || (field_i >= record->num_fields)){
            fprintf(stderr, "ERROR : biomeval_nbis_print_ANSI_NIST_select : "
		    "field index [%d.%d] out of range [1..%d] "
		    "in record [Type-%d]\n",
                    record_i+1, field_i+1, record->num_fields, record->type);
            return(-3);
         }
         field = record->fields[field_i];

         /* If subfield index is defined ... */
         if(subfield_i != UNDEFINED_INT){
            /* If subfield not found ... */
            if(!biomeval_nbis_lookup_ANSI_NIST_subfield(&subfield, subfield_i, field)){
               fprintf(stderr, "ERROR : biomeval_nbis_print_ANSI_NIST_select : "
		       "subfield index [%d.%d.%d] not found "
		       "in record [Type-%d.%03d]\n",
                       record_i+1, field_i+1, subfield_i+1,
                       record->type, field->field_int);
               return(-4);
            }
            /* Otherwise, subfield found. */

            /* If item index is defined ... */
            if(item_i != UNDEFINED_INT){
               /* If item not found ... */
               if(!biomeval_nbis_lookup_ANSI_NIST_item(&item, item_i, subfield)){
                  fprintf(stderr, "ERROR : biomeval_nbis_print_ANSI_NIST_select : "
			  "item index [%d.%d.%d.%d] not found "
			  "in record [Type-%d.%03d]\n",
                          record_i+1, field_i+1, subfield_i+1, item_i+1,
                          record->type, field->field_int);
                  return(-5);
               }
               /* Otherwise, item found.*/

               /* Write item's value to formatted text file pointer. */
               if((ret = biomeval_nbis_write_fmttext_item(fpout, record_i, field_i,
                                  subfield_i, item_i, ansi_nist)))
                  return(ret);
            }
            /* Otherwise, item index is UNDEFINED ... */
            else{
               /* Write subfield's contents to formatted text file pointer. */
               if((ret = biomeval_nbis_write_fmttext_subfield(fpout, record_i, field_i,
                                      subfield_i, ansi_nist)))
                  return(ret);
            }
         }
         /* Otherwise, subfield index is UNDEFINED ... */
         else{
            /* Write field's contents to formatted text file pointer. */
            if((ret = biomeval_nbis_write_fmttext_field(fpout, record_i, field_i,
                                          ansi_nist)))
               return(ret);
         }
      }
      /* Otherwise, field ID is UNDEFINED ... */
      else{
         /* Write records's contents to formatted text file pointer. */
         if((ret = biomeval_nbis_write_fmttext_record(fpout, record_i, ansi_nist)))
            return(ret);
      }
   }
   /* Otherwise, record type is UNDEFINED ... */
   else{
      /* Write ansi_nist structure's contents to formatted text file ptr. */
      if((ret = biomeval_nbis_write_fmttext(fpout, ansi_nist)))
         return(ret);
   }

   /* Return normally. */
   return(0);
}

