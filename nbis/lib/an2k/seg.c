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
      LIBRARY: AN2K - ANSI/NIST Reference Implementation

      FILE:    SEG.C
      AUTHOR:  Margaret Lepley
      DATE:    3/12/2008

      Contains routines for working with SEG structures found in Type-14
      records.
      

***********************************************************************
               ROUTINES:
                        biomeval_nbis_lookup_type14_segments()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>
#include <an2kseg.h>
#include <defs.h>

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_lookup_type14_segments - Examines Type-14 record for any FINGER 
#cat:              SEGMENT POSITION and if found stores it in a SEG array.

   Input:
      record   - Type-14 tagged field image record.
   Output:
      osegs    - points to array of SEG structures
      onsgs    - number of SEG entries
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int biomeval_nbis_lookup_type14_segments(SEG **osegs, int *onsgs, RECORD *record)
{
  FIELD *field;
  SUBFIELD *subfield;
  int field_i, subfield_i;
  SEG *segs;
  int nsgs;

  /* Check that record is Type-14 */
  if(record->type != TYPE_14_ID) {
    fprintf(stderr, "ERROR : biomeval_nbis_lookup_type14_segments : "
	    "unsupported record type : Type-%d\n", record->type);
    return(-2);
  }

  /* Find segment data ... */
  if(!biomeval_nbis_lookup_ANSI_NIST_field(&field, &field_i, SEG_ID, record)){
    *onsgs = 0;
    *osegs = (SEG *) NULL;
    return(0);
  }
  else {
    nsgs = field->num_subfields;
  }
   
  segs = (SEG *)malloc(nsgs * sizeof(SEG));
   
  if(segs == NULL){
    fprintf(stderr, "ERROR : biomeval_nbis_lookup_type14_segments : "
	    "malloc : %d segs (%lu bytes)\n", nsgs,
	    (unsigned long)(nsgs * sizeof(SEG)));
    return(-3);
  }

  /* For each subfield in SEG field ... */
  for(subfield_i = 0; subfield_i < nsgs; subfield_i++){
    subfield = field->subfields[subfield_i];
    segs[subfield_i].finger = atoi((char *)subfield->items[0]->value);
    segs[subfield_i].left = atoi((char *)subfield->items[1]->value);
    segs[subfield_i].right = atoi((char *)subfield->items[2]->value);
    segs[subfield_i].top = atoi((char *)subfield->items[3]->value);
    segs[subfield_i].bottom = atoi((char *)subfield->items[4]->value);
  }

  /* Set output pointers. */
  *osegs = segs;
  *onsgs = nsgs;

  /* Return normally. */
  return(0);
}
