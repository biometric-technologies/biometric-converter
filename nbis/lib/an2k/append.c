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

      FILE:    APPEND.C
      AUTHOR:  Michael D. Garris
      DATE:    09/21/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for appending data structures
      including logical records, fields, subfields, and information
      items into a parent structure.  These routines deal with the
      necessary insertaion of ANSI/NIST separator characters
      and then call their respective "update" routines.  These routines
      are used to dynamically create and add structures into an ANSI/NIST
      hierarchy.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_append_ANSI_NIST_record()
                        biomeval_nbis_append_ANSI_NIST_field()
                        biomeval_nbis_append_ANSI_NIST_subfield()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_append_ANSI_NIST_record - Takes a new field and adds it to the
#cat:              end of the specified record, updating the size of
#cat:              the record and setting any separator characters.

   Input:
      record     - record structure to be added to
      field      - field structure to add
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_append_ANSI_NIST_record(RECORD *record, FIELD *field)
{
   int ret;
   FIELD *lastfield;


   /* If record's FS_CHAR not set ... */
   if(!record->fs_char){
      record->fs_char = TRUE;
      record->num_bytes++;
   }

   /* If record already contains fields ... */
   if(record->num_fields > 0){
      /* If last field in list has GS_CHAR not set ... */
      lastfield = record->fields[record->num_fields-1];
      if(!lastfield->gs_char){
         lastfield->gs_char = TRUE;
         lastfield->num_bytes++;
         record->num_bytes++;
      }
   }
   /* Update record with new field. */
   if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field)))
      return(ret);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_append_ANSI_NIST_field - Takes a new subfield and adds it to the
#cat:              end of the specified field, updating the size of
#cat:              the field and setting any separator characters.

   Input:
      field      - field structure to be added to
      subfield   - subfield structure to add
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_append_ANSI_NIST_field(FIELD *field, SUBFIELD *subfield)
{
   int ret;
   SUBFIELD *lastsubfield;


   /* If field already contains subfields ... */
   if(field->num_subfields > 0){
      /* If last subfield in list has RS_CHAR not set ... */
      lastsubfield = field->subfields[field->num_subfields-1];
      if(!lastsubfield->rs_char){
         lastsubfield->rs_char = TRUE;
         lastsubfield->num_bytes++;
         field->num_bytes++;
      }
   }
   /* Update field with new subfield. */
   if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield)))
      return(ret);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_append_ANSI_NIST_subfield - Takes a new item and adds it to the
#cat:              end of the specified subfield, updating the size of
#cat:              the subfield and setting any separator characters.

   Input:
      subfield   - subfield structure to be added to
      item       - item structure to add
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_append_ANSI_NIST_subfield(SUBFIELD *subfield, ITEM *item)
{
   int ret;
   ITEM *lastitem;


   /* If subfield already contains items ... */
   if(subfield->num_items > 0){
      /* If last item in list has US_CHAR not set ... */
      lastitem = subfield->items[subfield->num_items-1];
      if(!lastitem->us_char){
         lastitem->us_char = TRUE;
         lastitem->num_bytes++;
         subfield->num_bytes++;
      }
   }
   /* Update subfield with new item. */
   if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item)))
      return(ret);

   /* Return normally. */
   return(0);
}

