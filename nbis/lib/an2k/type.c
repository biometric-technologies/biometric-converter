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

      FILE:    TYPE.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/09/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for identifying the different
      type of logical records, fields, delimiters, etc. according to
      the ANSI/NIST standard.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_tagged_record()
                        biomeval_nbis_binary_record()
                        tagged_image_record()
                        biomeval_nbis_binary_image_record()
                        biomeval_nbis_image_record()
                        biomeval_nbis_binary_signature_record()
                        biomeval_nbis_image_field()
                        biomeval_nbis_is_delimiter()
                        biomeval_nbis_which_hand()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_tagged_record - Determines if the specified record type is one that
#cat:              is formatted with tagged fields.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record is tag formatted
      FALSE       - specified record is NOT tag formatted
************************************************************************/
int biomeval_nbis_tagged_record(const unsigned int record_type)
{
   int i;


   for(i = 0; i < NUM_TAGGED_RECORDS; i++){
      if(record_type == biomeval_nbis_tagged_records[i])
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_binary_record - Determines if the specified record type is one that
#cat:              is formatted with binary fields.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record is binary formatted
      FALSE       - specified record is NOT binary formatted
************************************************************************/
int biomeval_nbis_binary_record(const unsigned int record_type)
{
   int i;


   for(i = 0; i < NUM_BINARY_RECORDS; i++){
      if(record_type == biomeval_nbis_binary_records[i])
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: tagged_image_record - Determines if the specified record type is one
#cat:              that is formatted with tagged fields and contains an image.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record is tag formatted with an image
      FALSE       - specified record is NOT tag formatted with an image
************************************************************************/
int tagged_image_record(const unsigned int record_type)
{
   int i;


   for(i = 0; i < NUM_TAGGED_IMAGE_RECORDS; i++){
      if(record_type == tagged_image_records[i])
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_binary_image_record - Determines if the specified record type is one
#cat:              that is formatted with binary fields and contains an image.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record is binary formatted with an image
      FALSE       - specified record is NOT binary formatted with an image
************************************************************************/
int biomeval_nbis_binary_image_record(const unsigned int record_type)
{
   int i;


   for(i = 0; i < NUM_BINARY_IMAGE_RECORDS; i++){
      if(record_type == biomeval_nbis_binary_image_records[i])
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_image_record - Determines if the specified record type is one
#cat:              that contains an image.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record contains an image
      FALSE       - specified record does NOT contain an image
************************************************************************/
int biomeval_nbis_image_record(const unsigned int record_type)
{
   if(tagged_image_record(record_type) ||
      biomeval_nbis_binary_image_record(record_type)){
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_binary_signature_record - Determines if the specified record type is one
#cat:              that is binary formatted and contains signature data.

   Input:
      record_type - integer record type
   Return Code:
      TRUE        - specified record is binary formatted with
                    signature data
      FALSE       - specified record is NOT binary formatted with
                    signature data
************************************************************************/
int biomeval_nbis_binary_signature_record(const unsigned int record_type)
{
   int i;


   for(i = 0; i < NUM_BINARY_SIGNATURE_RECORDS; i++){
      if(record_type == biomeval_nbis_binary_signature_records[i])
         return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_image_field - Determines if the given field structure contains
#cat:              an image.

   Input:
      field       - field structure in question
   Return Code:
      TRUE        - specified field contains an image
      FALSE       - specified field does NOT contain an image
************************************************************************/
int biomeval_nbis_image_field(const FIELD *field)
{
   if((biomeval_nbis_binary_image_record(field->record_type) &&
       (field->field_int == NUM_BINARY_IMAGE_FIELDS)) ||
      (biomeval_nbis_binary_signature_record(field->record_type) &&
       (field->field_int == NUM_BINARY_SIGNATURE_FIELDS)) ||
      (tagged_image_record(field->record_type) &&
       (field->field_int == IMAGE_FIELD))){
      return(TRUE);
   }
   return(FALSE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_is_delimiter - Determines if the specified character is one of
#cat:              the special delimiters defined in the ANSI/NIST standard.

   Input:
      nextchar    - character in question
   Return Code:
      TRUE        - specified character is a standard delimiter
      FALSE       - specified character is NOT a standard delimiter
************************************************************************/
int biomeval_nbis_is_delimiter(const int nextchar)
{
   switch(nextchar){
      case FS_CHAR:
      case GS_CHAR:
      case RS_CHAR:
      case US_CHAR:
           return(TRUE);
      default:
           return(FALSE);
   }
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_which_hand - Takes a finger position from Table 6 of the ANSI/NIST
#cat:              standard and determines which hand the specified finger
#cat:              is on.  The position may be "unknown".
   
   Input:
      finger_pos - ANSI/NIST finger position from Table 6
   Return Code:
      UNKNOWN_HAND - finger position code specified as "unknown"
      RIGHT_HAND   - finger position code specified for right hand
      LEFT_HAND    - finger position code specified for left hand
      Negative     - system error
************************************************************************/
int biomeval_nbis_which_hand(const int finger_pos)
{
   if(finger_pos == 0){
      return(UNKNOWN_HAND);
   }
   if(((finger_pos >= 1) && (finger_pos <= 5)) ||
      (finger_pos == 11) || (finger_pos == 13)){
      return(RIGHT_HAND);
   }
   else if(((finger_pos >= 6) && (finger_pos <= 10)) ||
      (finger_pos == 12) || (finger_pos == 14)){
      return(LEFT_HAND);
   }
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_which_hand : illegal finger position = %d "
	      "not in range [0..14]\n", finger_pos);
      return(-2);
   }
}
