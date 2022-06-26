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

      FILE:    FMTTEXT.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATE:  03/08/2005 by MDG
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2008 by Joseph C. Konczal
                          - report more details when things go wrong

      Contains routines responsible for reading and writing textually
      formatted files representing the contents of an ANSI/NIST standard
      file.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_read_fmttext_file()
                        biomeval_nbis_read_fmttext()
                        biomeval_nbis_read_fmttext_item()
                        biomeval_nbis_write_fmttext_file()
                        biomeval_nbis_write_fmttext()
                        biomeval_nbis_write_fmttext_record()
                        biomeval_nbis_write_fmttext_field()
                        biomeval_nbis_write_fmttext_image_field()
                        biomeval_nbis_write_fmttext_subfield()
                        biomeval_nbis_write_fmttext_item()

***********************************************************************/


#include <stdio.h>
#include <errno.h>

#include <nbis_sysdeps.h>

#include <an2k.h>
#include <ioutil.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_fmttext_file - Routine reads into memory the contents of the
#cat:              specified textually formatted version of an ANSI/NIST file.

   Input:
      ifile      - name of file to be read
   Output:
      oansi_nist - points to resulting ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_fmttext_file(const char *ifile, ANSI_NIST **oansi_nist)
{
   FILE *fpin;
   int ret;
   ANSI_NIST *ansi_nist;

   if((fpin = fopen(ifile, "rb")) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_file : fopen '%s': %s\n",
	      ifile, strerror(errno));
      return(-2);
   }

   if((ret = biomeval_nbis_alloc_ANSI_NIST(&ansi_nist))){
      if(fclose(fpin)){
         fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_file : fclose '%s': %s\n",
		 ifile, strerror(errno));
        return(-3);
      }
      return(ret);
   }

   if((ret = biomeval_nbis_read_fmttext(fpin, ansi_nist))){
      biomeval_nbis_free_ANSI_NIST(ansi_nist);
      if(fclose(fpin)){
         fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_file : fclose '%s': %s\n",
		 ifile, strerror(errno));
        return(-4);
      }
      return(ret);
   }

   /* Due to parsing and or edits to the text file, the actual  */
   /* bytes in the ANSI-NIST structures may no longer be in     */
   /* sync with byte lengths stored in the records' LEN fields. */
   /* Where needed, change the LEN fields to bring in sync.     */
   if((ret = biomeval_nbis_update_ANSI_NIST_record_LENs(ansi_nist))){
      biomeval_nbis_free_ANSI_NIST(ansi_nist);
      if(fclose(fpin)){
         fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_file : fclose '%s': %s\n",
		 ifile, strerror(errno));
        return(-5);
      }
      exit(ret);
   }

   if(fclose(fpin)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_file : fclose '%s': %s\n",
	      ifile, strerror(errno));
     return(-6);
   }

   *oansi_nist = ansi_nist;
   return(0);
}


/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_fmttext - Routine reads into memory the contents from an
#cat:              open file pointer of a textually formatted version
#cat:              of an ANSI/NIST file.

   Input:
      fpin       - open textually formatted file pointer
   Output:
      oansi_nist - points to resulting ANSI/NIST file structure
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_fmttext(FILE *fpin, ANSI_NIST *ansi_nist)
{
   int ret, ret_bytes;
   int record_i, field_i, subfield_i, item_i;
   int record_type, field_int, alloc_chars, byte_adjust;
   char *itemvalue;
   unsigned char *bindata;
   int binbytes;
   RECORD   *record;
   FIELD    *field;
   SUBFIELD *subfield;
   ITEM     *item;
   int orecord_i   = UNDEFINED_INT;
   int ofield_i    = UNDEFINED_INT;
   int osubfield_i = UNDEFINED_INT;
   int oitem_i     = UNDEFINED_INT;
   int field_repeat    = FALSE;
   int subfield_repeat = FALSE;
   int item_repeat     = FALSE;

   while(1){
      /* Read next information item line from formatted text file ... */
      ret = biomeval_nbis_read_fmttext_item(fpin, &record_i, &field_i,
                 &subfield_i, &item_i, &record_type, &field_int, &itemvalue);

      if(ret < 0)
         /* Return ERROR. */
         return(ret);

      if(ret == FALSE)
         /* Return normally. */
         return(0);

      /* Otherwise, new information item read ... */

      /* If item read belonging to new record ... */
      if(record_i != orecord_i){
         /* Allocate new record. */
         if((ret = biomeval_nbis_alloc_ANSI_NIST_record(&record)))
            return(ret);
         /* Add new record to ANSI-NIST structure. */
         if((ret = biomeval_nbis_update_ANSI_NIST(ansi_nist, record))){
            biomeval_nbis_free_ANSI_NIST_record(record);
            return(ret);
         }

         /* Set record type. */
         record->type = record_type;

         /* Set child structure repeats to FALSE. */
         field_repeat = FALSE;
         subfield_repeat = FALSE;
         item_repeat = FALSE;

         /* Only tagged field records have separator characters, */
         /* binary image records do not.                         */
         if(biomeval_nbis_tagged_record(record_type)){
            /* Every tagged record ends with an FS separator. */
            /* Add trailing FS separator to record.           */
            record->fs_char = TRUE;
            /* Bump bytes of record and parent structure. */
            record->num_bytes++;
            ansi_nist->num_bytes++;
         }
      }

      /* If item read belonging to new field ... */
      if((record_i != orecord_i) ||
         (field_i != ofield_i)){
         /* If field(s) already read and a tagged field record ... */
         /* (Only tagged fields have separator characters.)        */
         if((field_repeat) && (biomeval_nbis_tagged_record(record->type))){
            /* Add GS separator to previous field. */
            field->gs_char = TRUE;
            /* Bump bytes in field and parent structures. */
            field->num_bytes++;
            record->num_bytes++;
            ansi_nist->num_bytes++;
         }
         else
            /* Set field repeat to TRUE. */
            field_repeat = TRUE;

         /* Set child structure repeats to FALSE. */
         subfield_repeat = FALSE;
         item_repeat = FALSE;

         /* Allocate new field. */
         if((ret = biomeval_nbis_alloc_ANSI_NIST_field(&field)))
            return(ret);
         /* Add new field to current record ... */
         if((ret = biomeval_nbis_update_ANSI_NIST_record(record, field))){
            return(ret);
         }

         /* Set field attributes. */
         field->record_type = record_type;
         field->field_int = field_int;

         /* Only tagged fields (including 999 image fields) have */
         /* field ID strings, binary image record fields do not. */
         if(biomeval_nbis_tagged_record(record->type)){
            /* Allocate and generate the field's ID string. */
            field->id = (char *)malloc((2 * FIELD_NUM_LEN) + 3);
            if(field->id == NULL){
               biomeval_nbis_free_ANSI_NIST_field(field);
               fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : "
                       "malloc : ID string for field index [%d] [Type-%d.%03d] "
		       "(%d bytes)\n", field_i, record_type, field_int,
		       (2 * FIELD_NUM_LEN) + 3);
               return(-2);
            }
            sprintf(field->id, FLD_FMT, record_type, field_int);

            /* Bump bytes in field and parent structures. */
            byte_adjust = strlen(field->id);
            field->num_bytes += byte_adjust;
            record->num_bytes += byte_adjust;
            ansi_nist->num_bytes += byte_adjust;

         }
      }

      /* If item read belonging to new subfield ... */
      if((record_i != orecord_i) ||
         (field_i != ofield_i) ||
         (subfield_i != osubfield_i)){
         /* If subfield(s) already read and a tagged field record ... */
         /* (Only tagged field records have separator characters.)    */
         if((subfield_repeat) && (biomeval_nbis_tagged_record(record->type))){
            /* Add RS separator to previous subfield. */
            subfield->rs_char = TRUE;
            /* Bump bytes in subfield and parent structures. */
            subfield->num_bytes++;
            field->num_bytes++;
            record->num_bytes++;
            ansi_nist->num_bytes++;
         }
         else
            /* Set subfield repeat to TRUE. */
            subfield_repeat = TRUE;

         /* Set child structure repeats to FALSE. */
         item_repeat = FALSE;

         /* Allocate new subfield. */
         if((ret = biomeval_nbis_alloc_ANSI_NIST_subfield(&subfield)))
            return(ret);
         /* Add new subfield to current field ... */
         if((ret = biomeval_nbis_update_ANSI_NIST_field(field, subfield))){
            return(ret);
         }
      }

      /* If item read belonging to new item ... */
      if((record_i != orecord_i) ||
         (field_i != ofield_i) ||
         (subfield_i != osubfield_i) ||
         (item_i != oitem_i)){
         /* If item(s) already read and a tagged field record ...  */
         /* (Only tagged field records have separator characters.) */
         /* Could test for tagged 999 image field here, but it should  */
         /* always be the last field in the record with a single item, */
         /* so don't bother.                                           */
         if((item_repeat) && (biomeval_nbis_tagged_record(record->type))){
            /* Add US separator to previous item. */
            item->us_char = TRUE;
            /* Bump bytes in item and parent structures. */
            item->num_bytes++;
            subfield->num_bytes++;
            field->num_bytes++;
            record->num_bytes++;
            ansi_nist->num_bytes++;
         }
         else
            /* Set item repeat to TRUE. */
            item_repeat = TRUE;

         /* Allocate new item. */
         if((ret = biomeval_nbis_alloc_ANSI_NIST_item(&item)))
            return(ret);
         /* Add new item to current subfield ... */
         if((ret = biomeval_nbis_update_ANSI_NIST_subfield(subfield, item))){
            return(ret);
         }

         /* If image field (binary or tagged field) ... */
         if(biomeval_nbis_image_field(field)){
            /* Read in the externally referenced binary image block. */
            if((ret = biomeval_nbis_read_binary_image_data(itemvalue, &bindata, &binbytes)))
               return(ret);
            fprintf(stderr, "Read %d bytes of binary image data\n", binbytes);

            free(item->value);
            item->value = bindata;
            item->alloc_chars = binbytes;
            byte_adjust = binbytes;
            item->num_bytes += byte_adjust;
            item->num_chars += byte_adjust;

            /* Bump bytes in parent structures. */
            subfield->num_bytes += byte_adjust;
            field->num_bytes += byte_adjust;
            record->num_bytes += byte_adjust;
            ansi_nist->num_bytes += byte_adjust;

#ifdef UNLINK
            /* Unlink the externally referenced file. */
            if(unlink(itemvalue)){
               fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : unlink failed : %s\n",
		       itemvalue);
               return(-3);
            }
            fprintf(stderr, "Removed file %s\n", itemvalue);
#endif
         }
         /* If binary (non-image) field ... */
         else if(biomeval_nbis_binary_record(record->type)){
            /* Set item attributes. */
            alloc_chars = strlen(itemvalue) + 1;
            if(alloc_chars >= item->alloc_chars){
	       unsigned char * new_ptr
		  = (unsigned char *)realloc(item->value, alloc_chars);

               if(new_ptr == NULL){
                  fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : "
			  "realloc : item->value (increase %d bytes to %d)\n",
			  item->alloc_chars, alloc_chars);
                  return(-4);
               }
	       item->value = new_ptr;
               item->alloc_chars = alloc_chars;
            }
            strcpy((char *)item->value, itemvalue);
            item->num_chars = strlen(itemvalue);

            /* Determine fixed byte size for binary field... */
            if(biomeval_nbis_binary_image_record(record->type)){
               if(field->field_int == FGP_ID)
                  byte_adjust = 1;
               else{
                  if((ret_bytes =
                         biomeval_nbis_binary_image_field_bytes(field->field_int)) < 0)
                     return(ret_bytes);
                  byte_adjust = ret_bytes;
               }
            }
            else if(biomeval_nbis_binary_signature_record(record->type)){
               if((ret_bytes =
                      biomeval_nbis_binary_signature_field_bytes(field->field_int)) < 0)
                  return(ret_bytes);
               byte_adjust = ret_bytes;
            }
            else{
               fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : "
		       "unknown binary record [Type-%d]\n", record->type);
               return(-5);
            }

            /* Bump bytes in item and parent structures. */
            item->num_bytes += byte_adjust;
            subfield->num_bytes += byte_adjust;
            field->num_bytes += byte_adjust;
            record->num_bytes += byte_adjust;
            ansi_nist->num_bytes += byte_adjust;
         }
         /* Otherwise, we have read a tagged field item from the */
         /* formatted text file ...                              */
         else{
            /* Set item attributes. */
            alloc_chars = strlen(itemvalue) + 1;
            if(alloc_chars >= item->alloc_chars){
	       unsigned char * new_ptr
		  = (unsigned char *)realloc(item->value, alloc_chars);

               if(new_ptr == NULL){
                  fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : "
			  "realloc : item->value (increase %d bytes to %d)\n",
			  item->alloc_chars, alloc_chars);
                  return(-6);
               }
	       item->value = new_ptr;
               item->alloc_chars = alloc_chars;
            }
            strcpy((char *)item->value, itemvalue);
            byte_adjust = strlen(itemvalue);
            item->num_bytes += byte_adjust;
            item->num_chars += byte_adjust;

            /* Bump bytes in parent structures. */
            subfield->num_bytes += byte_adjust;
            field->num_bytes += byte_adjust;
            record->num_bytes += byte_adjust;
            ansi_nist->num_bytes += byte_adjust;
         }
      }
      else{
         fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext : "
		 "duplicate information item read\n");
         return(-7);
      }

      /* Set previous structure indices. */
      orecord_i = record_i;
      ofield_i = field_i;
      osubfield_i = subfield_i;
      oitem_i = item_i;
   }

   /* not reached */
   return(-99);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_fmttext_item - Routine reads into memory an entry (or line of
#cat:              text) from an open file pointer of a textually formatted
#cat:              version of an ANSI/NIST file.  Each line read is an
#cat:              information item according to the standard, and returned
#cat:              with the item is it's record, field, subfield, and
#cat:              information item indices, which can be analyzed to
#cat:              determine if a new record, field, or subfield has begun.

   Input:
      fpin         - open textually formatted file pointer
   Output:
      orecord_i    - information item's record index
      ofield_i     - information item's field index
      osubfield_i  - information item's subfield index
      oitem_i      - information item's item index
      orecord_type - information item's record type
      ofield_int   - information item's field ID
      oitemvalue   - ASCI string value for the information item read
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_fmttext_item(FILE *fpin, int *orecord_i, int *ofield_i,
                                int *osubfield_i, int *oitem_i,
                                int *orecord_type, int *ofield_int,
                                char **oitemvalue)
{
   /* Skip white space, and return FALSE, if EOF. */
   if(biomeval_nbis_skip_white_space(fpin) == EOF)
      return(FALSE);

   /* Read record index. */
   if(biomeval_nbis_read_integer(fpin, orecord_i, '.') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : record index not read\n");
      return(-2);
   }

   /* Read field index. */
   if(biomeval_nbis_read_integer(fpin, ofield_i, '.') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : field index not read\n");
      return(-3);
   }

   /* Read subfield index. */
   if(biomeval_nbis_read_integer(fpin, osubfield_i, '.') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : subfield index not read\n");
      return(-4);
   }

   /* Read item index. */
   if(biomeval_nbis_read_integer(fpin, oitem_i, ' ') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : item index not read\n");
      return(-5);
   }

   if(biomeval_nbis_skip_white_space(fpin) == EOF){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : "
	      "premature EOF after item index\n");
      return(-6);
   }

   /* Read '['. */
   if(!biomeval_nbis_read_char(fpin, '[')){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : "
	      "\"[\" not read following item index\n");
      return(-7);
   }

   /* Read record type. */
   if(biomeval_nbis_read_integer(fpin, orecord_type, '.') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : record type not read\n");
      return(-8);
   }

   /* Read field ID. */
   if(biomeval_nbis_read_integer(fpin, ofield_int, ']') <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : field ID not read\n");
      return(-9);
   }

   /* Read '='. */
   if(!biomeval_nbis_read_char(fpin, ITEM_START)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : "
	      "\"%c\" (start item value character) not read\n", ITEM_START);
      return(-10);
   }

   /* Read item's value. */
   if(biomeval_nbis_read_string(fpin, oitemvalue, ITEM_END) <= 0){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fmttext_item : item value not read\n");
      return(-11);
   }

   /* Return successful. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_file - Routine writes an ANSI/NIST file structure
#cat:              to the specified filename in a textually formatted
#cat:              representation that can be viewed and edited.

   Input:
      ansi_nist  - ANSI/NIST file structure to be written out
   Output:
      ofile      - name of file to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_file(const char *ofile, const ANSI_NIST *ansi_nist)
{
   FILE *fpout;
   int  ret;

   /* Open the output file pointer. */
   fpout = fopen(ofile, "wb");
   if(fpout == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_file : fopen '%s': %s\n",
	      ofile, strerror(errno));
      return(-2);
   }

   if((ret = biomeval_nbis_write_fmttext(fpout, ansi_nist))){
      if(fclose(fpout)){
         fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_file : fclose '%s': %s\n",
		 ofile, strerror(errno));
         return(-3);
      }
      return(ret);
   }

   /* Close the output file pointer. */
   if(fclose(fpout)){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_file : fopen '%s': %s\n",
	      ofile, strerror(errno));
      return(-4);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext - Routine writes an ANSI/NIST file structure
#cat:              in a textually formatted representation that can be
#cat:              viewed and edited to an open file pointer.

   Input:
      ansi_nist  - ANSI/NIST file structure to be written out
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext(FILE *fpout, const ANSI_NIST *ansi_nist)
{
   int  ret, record_i;


   /* For each record in the ANSI_NIST structure ... */
   for(record_i = 0; record_i < ansi_nist->num_records; record_i++){
      if((ret = biomeval_nbis_write_fmttext_record(fpout, record_i, ansi_nist)))
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_record - Routine writes the contents of a logical
#cat:              record to the open file pointer in a textually formatted
#cat:              representation that can be viewed and edited.

   Input:
      record_i   - integer index of record to be written out
      ansi_nist  - ANSI/NIST file structure containing the record
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_record(FILE *fpout, const int record_i,
                                    const ANSI_NIST *ansi_nist)
                                   
{
   RECORD *record;
   int ret, field_i;

   record = ansi_nist->records[record_i];

   /* Foreach field in record ... */
   for(field_i = 0; field_i < record->num_fields; field_i++){
      if((ret = biomeval_nbis_write_fmttext_field(fpout, record_i, field_i,
                                    ansi_nist)))
         return(ret);
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_field - Routine writes the contents of a field
#cat:              to the open file pointer in a textually formatted
#cat:              representation that can be viewed and edited.

   Input:
      record_i   - integer index of record of field to be written out
      field_i    - integer index of field to be written out
      ansi_nist  - ANSI/NIST file structure containing the field
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_field(FILE *fpout, const int record_i,
                           const int field_i, const ANSI_NIST *ansi_nist)
{
   RECORD *record;
   FIELD *field;
   int ret, subfield_i;

   record = ansi_nist->records[record_i];
   field = record->fields[field_i];

   /* If image field ... */
   if(biomeval_nbis_image_field(field)){
      /* Write the image field's contents to temporary file. */
      if((ret = biomeval_nbis_write_fmttext_image_field(fpout, record_i,
                                         field_i, ansi_nist)))
         return(ret);
   }
   else{
      for(subfield_i = 0; subfield_i < field->num_subfields; subfield_i++){
         /* Write the subfield in text format to the file pointer. */
         if((ret = biomeval_nbis_write_fmttext_subfield(fpout, record_i, field_i,
                                         subfield_i, ansi_nist)))
            return(ret);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_image_field - Routine writes the contents of a binary
#cat:              image field to a temporary file and generates an external
#cat:              file reference to be written to the  open file pointer
#cat:              in the textually formatted representation.  This way
#cat:              the textually formatted file contains the reference to
#cat:              the temporary file containing the binary image data.

   Input:
      record_i   - integer index of record of field to be written out
      field_i    - integer index of field to be written out
      ansi_nist  - ANSI/NIST file structure containing the field
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_image_field(FILE *fpout, const int record_i,
                             const int field_i, const ANSI_NIST *ansi_nist)
{
   FILE *bfp;
   RECORD *record;
   FIELD *field;
   ITEM *data_item;
   int n, ret;
   char bfile[MAXPATHLEN];

   sprintf(bfile, "fld_%d_%d.tmp", record_i+1, field_i+1);

   ret = biomeval_nbis_file_exists(bfile);
   /* If error ... */
   if(ret < 0)
      return(ret);
   /* If file exists ... */
   if(ret){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_image_field :"
	      "file %s already exists (won't overwrite)\n", bfile);
      return(-2);
   }

   bfp = fopen(bfile, "wb");
   if(bfp == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_image_field : "
	      "fopen '%s': %s\n", bfile, strerror(errno));
      return(-3);
   }
   record = ansi_nist->records[record_i];
   field = record->fields[field_i];
   data_item = field->subfields[0]->items[0];

   n = fwrite(data_item->value, 1, data_item->num_chars, bfp);

   if(n != data_item->num_chars){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_image_field :"
	      "fwrite : only %d bytes of %d written\n",
	      n, data_item->num_chars);
      return(-4);
   }

   if(fclose(bfp)){
      fprintf(stderr, "ERROR : biomeval_nbis_write_fmttext_image_field : "
	      "fclose : %s\n", bfile);
      return(-5);
   }

   fprintf(stderr, "Temp image file \"%s\" created "
	   "for field index [%d.%d] [Type-%d.%03d]\n",
           bfile, record_i+1, field_i+1, record->type, field->field_int);

   /* Write external file reference to text file pointer. */
   fprintf(fpout, "%d.%d.%d.%d [%d.%03d]%c%s%c\n",
           record_i+1, field_i+1, 1, 1,
           record->type, field->field_int,
           ITEM_START, bfile, ITEM_END);

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_subfield - Routine writes the contents of subfield
#cat:              structure to the  open file pointer in a textually
#cat:              formatted representation that can be viewed and edited.

   Input:
      record_i   - integer index of record of subfield to be written out
      field_i    - integer index of field of subfield to be written out
      subfield_i - integer index of subfield to be written out
      ansi_nist  - ANSI/NIST file structure containing the field
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_subfield(FILE *fpout, const int record_i,
                             const int field_i, const int subfield_i,
                             const ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   int item_i;

   record = ansi_nist->records[record_i];
   field = record->fields[field_i];
   subfield = field->subfields[subfield_i];

   /* If image field ... */
   if(biomeval_nbis_image_field(field)){
      /* Write the binary field's contents to temporary file. */
      if((ret = biomeval_nbis_write_fmttext_image_field(fpout, record_i,
                                                  field_i, ansi_nist)))
         return(ret);
   }
   else{
      /* For each item in subfield ... */
      for(item_i = 0; item_i < subfield->num_items; item_i++){
         /* Write item in text format to file pointer. */
         if((ret = biomeval_nbis_write_fmttext_item(fpout, record_i, field_i, subfield_i,
                                      item_i, ansi_nist)))
            return(ret);
      }
   }

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_write_fmttext_item - Routine writes the contents of an information
#cat:              item to the  open file pointer in a textually
#cat:              formatted representation that can be viewed and edited.

   Input:
      record_i   - integer index of record of item to be written out
      field_i    - integer index of field of item to be written out
      subfield_i - integer index of subfield of item to be written out
      item_i     - integer index of informaton item to be written out
      ansi_nist  - ANSI/NIST file structure containing the field
   Output:
      fpout      - open file pointer to be written to
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_write_fmttext_item(FILE *fpout, const int record_i,
                    const int field_i, const int subfield_i, const int item_i,
                    const ANSI_NIST *ansi_nist)
{
   int ret;
   RECORD *record;
   FIELD *field;
   SUBFIELD *subfield;
   ITEM  *item;

   record = ansi_nist->records[record_i];
   field = record->fields[field_i];
   subfield = field->subfields[subfield_i];
   item = subfield->items[item_i];

   /* If image field ... */
   if(biomeval_nbis_image_field(field)){
      /* Write the binary field's contents to temporary file. */
      if((ret = biomeval_nbis_write_fmttext_image_field(fpout, record_i,
                                                  field_i, ansi_nist)))
         return(ret);
   }
   else{
      /* Write item's value to text file pointer. */
      fprintf(fpout, "%d.%d.%d.%d [%d.%03d]%c%s%c\n",
              record_i+1, field_i+1, subfield_i+1, item_i+1,
              record->type, field->field_int,
              ITEM_START, item->value, ITEM_END);
   }

   /* Return normally. */
   return(0);
}
