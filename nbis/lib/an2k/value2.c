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

      FILE:    VALUE2.C
      AUTHOR:  Michael D. Garris
      DATE:    09/22/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for taking a string value and
      encapsulating the string into an allocated information item
      and then embedding the informaton item into the specified
      level of ANSI/NIST file structure.  These routines are useful
      for constructing ANSI/NIST structures that contain a single
      information item.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_value2field()
                        biomeval_nbis_value2subfield()
                        biomeval_nbis_value2item()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_value2field - Takes a string value encapsulating it into an
#cat:              information item and then embedding the item into a
#cat:              newly allocated and initialized field structure.

   Input:
      record_type - type of the record the field is to be a part of
      field_int   - the new field's ID
      value      - string value to be encapsulated in single embedded
                   information item
   Output:
      ofield     - points to new field structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_value2field(FIELD **ofield, const int record_type, const int field_int,
                const char *value)
{
   int ret;
   SUBFIELD *subfield;
   FIELD *field;

   /* Create subfield from value string. */
   if((ret = biomeval_nbis_value2subfield(&subfield, value)))
      return(ret);

   /* Allocate and initialize new field. */
   if((ret = biomeval_nbis_new_ANSI_NIST_field(&field, record_type, field_int))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      return(ret);
   }

   /* Update field with subfield. */
   if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield))){
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      biomeval_nbis_free_ANSI_NIST_field(field);
      return(ret);
   }

   /* Assign output pointer. */
   *ofield = field;
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_value2subfield - Takes a string value encapsulating it into an
#cat:              information item and then embedding the item into a
#cat:              newly allocated subfield structure.

   Input:
      value      - string value to be encapsulated in single embedded
                   information item
   Output:
      osubfield  - points to new subfield structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_value2subfield(SUBFIELD **osubfield, const char *value)
{
   int ret;
   ITEM *item;
   SUBFIELD *subfield;

   /* Create item from value string. */
   if((ret = biomeval_nbis_value2item(&item, value)))
      return(ret);

   /* Allocate subfield. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      return(ret);
   }

   /* Update subfield with information item. */
   if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item))){
      biomeval_nbis_free_ANSI_NIST_item(item);
      biomeval_nbis_free_ANSI_NIST_subfield(subfield);
      return(ret);
   }

   /* Assign output pointer. */
   *osubfield = subfield;
   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_value2item - Takes a string value encapsulating it into a
#cat:              newly allocated information item.

   Input:
      value      - string value to be encapsulated in single embedded
                   information item
   Output:
      oitem      - points to new information item
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_value2item(ITEM **oitem, const char *value)
{
   int ret;
   ITEM *item;
   int value_len, alloc_len;

   /* Compute and store length of value string. */
   value_len = strlen(value);
   /* Set allocation length to include NULL terminator. */
   alloc_len = value_len + 1;

   /* Allocate item. */
   if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)))
      return(ret);

   /* Realloc item's value buffer if needed. */
   if(alloc_len >= item->alloc_chars){
      unsigned char *new_ptr = (unsigned char *)realloc(item->value, alloc_len);
      if(new_ptr == NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_value2field : "
		 "realloc : item->value (increase %d bytes to %d)\n",
		 item->alloc_chars, alloc_len);
         return(-2);
      }
      item->value = new_ptr;
      item->alloc_chars = alloc_len;
   }

   /* Copy value into information item. */
   strcpy((char *)item->value, value);
   /* Update item's size. */
   item->num_chars += value_len;
   item->num_bytes += value_len;

   /* Assign output pointer. */
   *oitem = item;
   /* Return normally. */
   return(0);
}

