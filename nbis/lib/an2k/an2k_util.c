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

      FILE:    UTIL.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains miscellaneous routines used to support the reading,
      writing, and manipulating of the contents of ANSI/NIST files.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_increment_numeric_item()
                        biomeval_nbis_decrement_numeric_item()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_increment_numeric_item - Takes a specified information item and
#cat:              replaces its value with its increment.  The length
#cat:              of the item's record is updated appropriately.  This
#cat:              routines assumes all structure indices are value.

   Input:
      record_i   - index of logical record of item to be incremented
      field_i    - index of field of item to be incremented
      subfield_i - index of subfield of item to be incremented
      item_i     - index of information item to be incremented
      ansi_nist  - ANSI/NIST file structure to be modified
      format     - optional format string to control printing of
                   incremented integer value to a string (may be NULL)
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_increment_numeric_item(const int record_i, const int field_i,
                           const int subfield_i, const int item_i,
                           ANSI_NIST *ansi_nist, char *format)
{
   int ret, itemint;
   char *itemvalue;
   ITEM *item;

   /* Set pointer to referenced item. */
   item = ansi_nist->records[record_i]->fields[field_i]->subfields[subfield_i]->items[item_i];

   /* Convert item value to numeric integer. */
   itemint = atoi((char *)item->value);
   /* Increment item's value. */
   itemint++;

   /* Allocate buffer to hold integer string.  Allocate based on one larger */
   /* than current length, because the most the number should grow is by    */
   /* one character.  Remember that num_chars does NOT include the NULL     */
   /*  terminator. */
   itemvalue = (char *)malloc(item->num_chars + 2);
   if(itemvalue == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_increment_numeric_item : "
	      "malloc : itemvalue (%d bytes)\n", item->num_chars + 2);
      return(-2);
   }

   /* If format string specified ... */
   if(format != NULL)
      sprintf(itemvalue, format, itemint);
   else
      /* Otherwise, use default format. */
      sprintf(itemvalue, "%d", itemint);

   /* Replace item's value with incremented value. */
   if((ret = biomeval_nbis_substitute_ANSI_NIST_item(record_i, field_i, subfield_i, item_i,
                                      itemvalue, ansi_nist))){
      free(itemvalue);
      return(ret);
   }

   /* Deallocate working memory. */
   free(itemvalue);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_decrement_numeric_item - Takes a specified information item and
#cat:              replaces its value with its decrement.  The length
#cat:              of the item's record is updated appropriately.  This
#cat:              routines assumes all structure indices are value.

   Input:
      record_i   - index of logical record of item to be decremented
      field_i    - index of field of item to be decremented
      subfield_i - index of subfield of item to be decremented
      item_i     - index of information item to be decremented
      ansi_nist  - ANSI/NIST file structure to be modified
      format     - optional format string to control printing of
                   decremented integer value to a string (may be NULL)
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_decrement_numeric_item(const int record_i, const int field_i,
                           const int subfield_i, const int item_i,
                           ANSI_NIST *ansi_nist, char *format)
{
   int ret, itemint;
   char *itemvalue;
   ITEM *item;

   /* Set pointer to referenced item. */
   item = ansi_nist->records[record_i]->fields[field_i]->subfields[subfield_i]->items[item_i];

   /* Convert item value to numeric integer. */
   itemint = atoi((char *)item->value);
   /* Decrement item's value. */
   itemint--;

   /* Allocate buffer to hold integer string.  Allocate based on one larger */
   /* than current length, that way if result is negative, the most the     */
   /* number should grow is by one character.   Otherwise, the result is    */
   /* positive and the number of characters in the result should decrease   */
   /* by at most one.  Remember that num_chars does NOT include the NULL    */
   /* terminator.*/
   itemvalue = (char *)malloc(item->num_chars + 2);
   if(itemvalue == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_decrement_numeric_item : "
	      "malloc : itemvalue (%d bytes)\n", item->num_chars + 2);
      return(-2);
   }

   /* If format string specified ... */
   if(format != NULL)
      sprintf(itemvalue, format, itemint);
   else
      /* Otherwise, use default format. */
      sprintf(itemvalue, "%d", itemint);

   /* Replace item's value with decremented value. */
   if((ret = biomeval_nbis_substitute_ANSI_NIST_item(record_i, field_i, subfield_i, item_i,
                                      itemvalue, ansi_nist))){
      free(itemvalue);
      return(ret);
   }

   /* Deallocate working memory. */
   free(itemvalue);

   /* Return normally. */
   return(0);
}
