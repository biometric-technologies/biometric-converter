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

      FILE:    READ.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 03/08/2005 by MDG
      UPDATED: 01/04/2008 by Joseph C. Konczal - simplified biomeval_nbis_read_char
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  12/02/2008 by Kenneth Ko - Fix to support 64-bit
      UPDATE:  01/26/2008 by Joseph C. Konczal
                          - report more details when things go wrong

      Contains routines responsible for reading data from ANSI/NIST
      files or textually formatted version of ANSI/NIST files, including
      temporary raw raster image files.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_read_binary_item_data()
                        biomeval_nbis_read_binary_uint()
                        biomeval_nbis_read_binary_ushort()
                        biomeval_nbis_read_binary_uchar()
                        biomeval_nbis_read_binary_image_data()
                        biomeval_nbis_read_char()
                        biomeval_nbis_read_string()
                        biomeval_nbis_read_integer()
                        biomeval_nbis_skip_white_space()

***********************************************************************/

#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>

#include <swap.h>
#include <an2k.h>  /* Added by MDG on 03-08-05 */

#ifndef TRUE
#define TRUE        1
#define FALSE       0
#endif

#ifndef MAX_UINT_CHARS
#define MAX_UINT_CHARS   10
#define MAX_USHORT_CHARS  5
#define MAX_UCHAR_CHARS   3
#endif

#define READ_CHUNK  100

/*
 * Functions to hide the complexity of reading data from either a file
 * pointer or a wrapped memory buffer.
 */
/***********************************************************************
************************************************************************
#cat: biomeval_nbis_fbgetc - Reads a single character from a file pointer, or a
#cat:          AN2KBDB-wrapped pointer, and return that character.
#cat:          This function can be a replacement for fgetc(2).

   Input:
      fpin       - open file pointer to be read from; if NULL, buf is used.
      buf        - memory buffer wrapped in a basic_data_buffer; the buffer
                   pointer is advanced by one character on success.
   Return Value:
      The character read
      EOF - stream is at end-of-file, or buffer is exhausted
************************************************************************/
int biomeval_nbis_fbgetc(FILE *stream, AN2KBDB *bdb)
{
	if (stream != NULL) {
		return (fgetc(stream));
	} else {
		if ((bdb->bdb_current + 1) > bdb->bdb_end) {
			return (EOF);
		}
		char rval = (char)*bdb->bdb_current;
		bdb->bdb_current += 1;
		return ((int)rval);
	}
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_fbread - Reads items of a given size from an open file pointer, or
#cat:          AN2KBDB-wrapped pointer, and return count of elements read.
#cat:          This function can be a replacement for fread(2).

   Input:
      fpin       - open file pointer to be read from; if NULL, buf is used.
                   when used, the stream is advanced by the number of items
                   read
      buf        - memory buffer wrapped in a basic_data_buffer
                   when used, the buffer is advanced by the number of items
                   read
      size       - size in bytes of the each item to be read
      nitems     - the number of items to read
   Output:
      ptr        - points to the items that were read
   Return Value:
      The number of items read; on error, ferror(3)/feof(3) can be used
      on the file stream
************************************************************************/
size_t biomeval_nbis_fbread(void *ptr, size_t size, size_t nitems, FILE *stream, AN2KBDB *bdb)
{
	if (stream != NULL) {
		return (fread(ptr, size, nitems, stream));
	} else {
		size_t n;
		for (n = 0; n < nitems; n++) {
			if ((bdb->bdb_current + size) > bdb->bdb_end) {
				return (n);
			}
			(void)memcpy(ptr, bdb->bdb_current, size);
			ptr = (char *)ptr + size;
			bdb->bdb_current += size;
		}
		return (n);
	}
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_fbtell - Returns the current file position, or buffer position.
#cat:          This function can be used as a replacement for ftell(3).

   Input:
      fpin       - open file pointer to be read from; if NULL, buf is used.
      buf        - memory buffer wrapped in a basic_data_buffer
   Return Value:
      The current position of the file stream or buffer.
************************************************************************/
long biomeval_nbis_fbtell(FILE *stream, AN2KBDB *bdb)
{
	if (stream != NULL)
		return (ftell(stream));
	else
		return (bdb->bdb_current - bdb->bdb_start);
}

/*
 * Local functions to do the actual reading or scanning work.
 */
static int biomeval_nbis_i_read_binary_uint(FILE *fpin, AN2KBDB *buf, unsigned int *ouint_val)
{
   unsigned int uint_val;

   /* Read unsigned integer bytes. */
   if(biomeval_nbis_fbread(&uint_val, sizeof(unsigned int), 1, fpin, buf) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_uint : "
	      "read : uint not read, at %ld: %s\n",
	       biomeval_nbis_fbtell(fpin, buf), SHORT_READ_ERR_MSG(fpin));
      return(-2);
   }

#ifdef __NBISLE__
   swap_uint_bytes(uint_val);
#endif

   *ouint_val = uint_val;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_binary_ushort(FILE *fpin, AN2KBDB *buf,
    unsigned short *oushort_val)
{
   unsigned short ushort_val;

   /* Read unsigned short bytes. */
   if(biomeval_nbis_fbread(&ushort_val, sizeof(unsigned short), 1, fpin, buf) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_ushort : "
	      "read : ushort not read, at %ld: %s\n",
	      biomeval_nbis_fbtell(fpin, buf), SHORT_READ_ERR_MSG(fpin));
      return(-2);
   }

#ifdef __NBISLE__
   swap_ushort_bytes(ushort_val);
#endif

   *oushort_val = ushort_val;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_binary_uchar(FILE *fpin, AN2KBDB *buf,
    unsigned char *ouchar_val)
{
   unsigned char uchar_val;

   /* Read unsigned short bytes. */
   if(biomeval_nbis_fbread(&uchar_val, sizeof(unsigned char), 1, fpin, buf) != 1){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_uchar : "
	      "read : uchar not read, at %ld: %s\n",
	      biomeval_nbis_fbtell(fpin, buf), SHORT_READ_ERR_MSG(fpin));
      return(-2);
   }

   *ouchar_val = uchar_val;

   /* Return normally. */
   return(0);
}

/*
 */
static int biomeval_nbis_i_read_binary_item_data(FILE *fpin, AN2KBDB *buf,
    unsigned char **ovalue, const int num_bytes)
{
   int ret;
   unsigned int uint_val;
   unsigned short ushort_val;
   unsigned char uchar_val;
   unsigned char *value;

   switch(num_bytes){
      case 4:
         if((ret = biomeval_nbis_i_read_binary_uint(fpin, buf, &uint_val)))
            return(ret);
         value = (unsigned char *)malloc(MAX_UINT_CHARS + 1);
         if(value == NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_read_binary_item_data : "
		    "malloc : uint string value (%d bytes)\n", 
		    MAX_UINT_CHARS + 1);
            return(-2);
         }
         sprintf((char *)value, "%d", uint_val);
         break;
      case 2:
         if((ret = biomeval_nbis_i_read_binary_ushort(fpin, buf, &ushort_val)))
            return(ret);
         value = (unsigned char *)malloc(MAX_USHORT_CHARS + 1);
         if(value == NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_read_binary_item_data : "
		    "malloc : ushort string value (%d bytes)\n",
		    MAX_USHORT_CHARS + 1);
            return(-3);
         }
         sprintf((char *)value, "%d", ushort_val);
         break;
      case 1:
         if((ret = biomeval_nbis_i_read_binary_uchar(fpin, buf, &uchar_val)))
            return(ret);
         value = (unsigned char *)malloc(MAX_UCHAR_CHARS + 1);
         if(value == NULL){
            fprintf(stderr, "ERROR : biomeval_nbis_read_binary_item_data : "
		    "malloc : uchar string value (%d bytes)\n",
		    MAX_UCHAR_CHARS + 1);
            return(-4);
         }
         sprintf((char *)value, "%d", uchar_val);
         break;
      default:
         fprintf(stderr, "ERROR : biomeval_nbis_read_binary_item_data : "
		 "number of bytes %d to be read unsupported\n", num_bytes);
         return(-5);
   }

   *ovalue = value;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_binary_item_data - Reads a binary unsigned int, unsigned short,
#cat:              or unsigned char from the open file pointer and returns
#cat:              the value read converted into an ASCII string.
#cat: biomeval_nbis_scan_binary_item_data - Reads a binary unsigned int, unsigned short,
#cat:              or unsigned char from the wrapped buffer and returns
#cat:              the value read converted into an ASCII string.

   Input:
      fpin       - open file pointer to be read from
      buf        - memory buffer wrapped in a basic_data_buffer
      num_bytes  - size in bytes of the binary unsigned item to be read
   Output:
      ovalue     - points to the ASCII string of what was read
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_binary_item_data(FILE *fpin, unsigned char **ovalue,
    const int num_bytes)
{
    return(biomeval_nbis_i_read_binary_item_data(fpin, NULL, ovalue, num_bytes));
}

int biomeval_nbis_scan_binary_item_data(AN2KBDB *buf, unsigned char **ovalue,
    const int num_bytes)
{
    return(biomeval_nbis_i_read_binary_item_data(NULL, buf, ovalue, num_bytes));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_binary_uint - Reads a binary unsigned integer from the open
#cat:              file pointer and swaps the bytes if architecture is
#cat:              not Big Endian.
#cat: biomeval_nbis_scan_binary_uint - Reads a binary unsigned integer from the wrapped
#cat:              buffer and swaps the bytes if architecture is
#cat:              not Big Endian.

   Input:
      fpin       - open file pointer to be read from
      buf        - memory buffer wrapped in a basic_data_buffer
   Output:
      ouint_val  - points to unsigned integer read
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_binary_uint(FILE *fpin, unsigned int *ouint_val)
{
    return (biomeval_nbis_i_read_binary_uint(fpin, NULL, ouint_val));
}
int biomeval_nbis_scan_binary_uint(AN2KBDB *buf, unsigned int *ouint_val)
{
    return (biomeval_nbis_i_read_binary_uint(NULL, buf, ouint_val));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_binary_ushort - Reads a binary unsigned short from the open
#cat:              file pointer and swaps the bytes if architecture is
#cat:              not Big Endian.
#cat: biomeval_nbis_scan_binary_ushort - Reads a binary unsigned short from the wrapped
#cat:              buffer and swaps the bytes if architecture is
#cat:              not Big Endian.

   Input:
      fpin        - open file pointer to be read from
      buf         - memory buffer wrapped in a basic_data_buffer
   Output:
      oushort_val - points to unsigned short read
   Return Code:
      Zero        - successful completion
      Negative    - system error
************************************************************************/
int biomeval_nbis_read_binary_ushort(FILE *fpin, unsigned short *oushort_val)
{
    return(biomeval_nbis_i_read_binary_ushort(fpin, NULL, oushort_val));
}
int biomeval_nbis_scan_binary_ushort(AN2KBDB *buf, unsigned short *oushort_val)
{
    return(biomeval_nbis_i_read_binary_ushort(NULL, buf, oushort_val));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_binary_uchar - Reads a binary unsigned character from the open
#cat:              file pointer.
#cat: biomeval_nbis_scan_binary_uchar - Reads a binary unsigned character from the wrapped
#cat:              buffer.

   Input:
      fpin       - open file pointer to be read from
      buf        - memory buffer wrapped in a basic_data_buffer
   Output:
      ouchar_val - points to unsigned character read
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_binary_uchar(FILE *fpin, unsigned char *ouchar_val)
{
    return(biomeval_nbis_i_read_binary_uchar(fpin, NULL, ouchar_val));
}
int biomeval_nbis_scan_binary_uchar(AN2KBDB *buf, unsigned char *ouchar_val)
{
    return(biomeval_nbis_i_read_binary_uchar(NULL, buf, ouchar_val));
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_binary_image_data - Reads a raw raster image from the specified
#cat:              filename.

   Input:
      bfile      - name of raw raster file to be read
   Output:
      obindata   - points to raster image data read
      obinbytes  - number of bytes in raster image data
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_read_binary_image_data(const char *bfile, unsigned char **obindata,
                           int *obinbytes)
{
   FILE *bfp;
   unsigned char *bindata;
   int binbytes, nread;
   struct stat binstat;

   if(stat(bfile, &binstat)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
	      "stat failed : %s\n", bfile);
      return(-2);
   }

   binbytes = (int)binstat.st_size;

   bfp = fopen(bfile, "rb");
   if(bfp == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
	      "fopen '%s': %s\n", bfile, strerror(errno));
      return(-3);
   }

   bindata = (unsigned char *)malloc(binbytes);
   if(bindata == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
	      "malloc : bindata (%d bytes)\n", binbytes);
      return(-4);
   }

   nread = fread(bindata, 1, binbytes, bfp);
   if(nread != binbytes){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
	      "fread : only %d bytes of %d read, at %ld: %s\n",
	      nread, binbytes, ftell(bfp), SHORT_READ_ERR_MSG(bfp));
      if(fclose(bfp)){
         fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
		 "fclose '%s': %s\n", bfile, strerror(errno));
         return(-5);
      }
      return(-6);
   }

   if(fclose(bfp)){
      fprintf(stderr, "ERROR : biomeval_nbis_read_binary_image_data : "
	      "fclose '%s': %s\n", bfile, strerror(errno));
      return(-7);
   }

   *obindata = bindata;
   *obinbytes = binbytes;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_char - Reads a single specified byte from the open file pointer.
#cat:              An error code is returned if the next byte read does
#cat:              not match the desired character.

   Input:
      fpin         - open file pointer to be read from
      desired_char - the character value to be read
   Return Code:
      TRUE         - desired character read
      FALSE        - desired character NOT read
************************************************************************/
int biomeval_nbis_read_char(FILE *fpin, const int desired_char)
{
   int c;

   c = fgetc(fpin);   
   if (c != desired_char) {  /* assuming EOF is never desired - jck */
      if (c == EOF) {
	 fprintf(stderr, "ERROR : biomeval_nbis_read_char : "
		 "fgetc '%c' (0x%02x), at %ld: %s\n",
		 desired_char, desired_char, ftell(fpin),
		 SHORT_READ_ERR_MSG(fpin));
      }
      return(FALSE);
   }
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_string - Reads a sequence of characters up to a specified
#cat:              delimiter and returns the characters concatenated
#cat:              together in NULL-terminiated character string.  The
#cat:              delimiter is NOT stored in the string.

   Input:
      fpin        - open file pointer to be read from
      delimiter   - character on which to terminate reading
   Output:
      ostring     - points to sequence of read characters
   Return Code:
      TRUE        - string successfully read
      FALSE       - EOF occured without reading delimiter character
      Negative    - system error
************************************************************************/
int biomeval_nbis_read_string(FILE *fpin, char **ostring, const int delimiter)
{
   int alloc_chars, num_chars;
   unsigned char *string;
   int c;

   num_chars = 0;
   alloc_chars = READ_CHUNK;

   string = (unsigned char *)malloc(alloc_chars);
   if(string == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_string : malloc : string (%d bytes)\n",
	      alloc_chars);
      return(-2);
   }

   /* Loop unitl ... */
   do{
      /* Read next character and if EOF ... */
      if((c = fgetc(fpin)) == EOF){
	 fprintf(stderr, "ERROR : biomeval_nbis_read_string : fgetc, at %ld: %s\n",
		 ftell(fpin), SHORT_READ_ERR_MSG(fpin));
         free(string);
         return(FALSE);
      }
      /* If delimiter character read, then break from loop. */
      /* Otherwise ... */
      if(c != delimiter){
         /* Include eventual Null terminator character in the test. */
         if(num_chars+1 >= alloc_chars){	    
	    unsigned char * new_ptr = 
	       (unsigned char *)realloc(string, alloc_chars + READ_CHUNK);

            if(new_ptr == NULL){
	       free(string);
               fprintf(stderr, "ERROR : biomeval_nbis_read_string : "
		       "realloc : string (increase %d bytes to %d), at %ld\n",
		       alloc_chars, alloc_chars + READ_CHUNK, ftell(fpin));
               return(-3);
            }
	    string = new_ptr;
	    alloc_chars += READ_CHUNK;
         }
         string[num_chars++] = c;
      }
   /* Continue looping as long as delimiter is NOT read. */
   }while(c != delimiter);

   /* Null terminate the value string. */
   string[num_chars] = '\0';

   *ostring = (char *)string;

   /* Return successfully. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_integer - Reads a sequence of characters up to a specified
#cat:              delimiter and converts those character into an
#cat:              integer which is returned.  The delimiter is NOT
#cat:              part of the integer value.

   Input:
      fpin        - open file pointer to be read from
      delimiter   - character on which to terminate reading
   Output:
      ointeger    - points to the resulting integer value
   Return Code:
      TRUE        - integer successfully read
      FALSE       - EOF occured without reading delimiter character
      Negative    - system error
************************************************************************/
int biomeval_nbis_read_integer(FILE *fpin, int *ointeger, const int delimiter)
{
   char string[MAX_UINT_CHARS+2]; /* add 1 byte each for sign and terminator */
   int num_chars = 0;
   int c;

   /* Loop unitl ... */
   do{
      /* Read next character and if EOF ... */
      if((c = fgetc(fpin)) == EOF){
	 fprintf(stderr, "ERROR : biomeval_nbis_read_integer : fgetc, at %ld: %s\n",
		 ftell(fpin), SHORT_READ_ERR_MSG(fpin));
         return(FALSE);
      }
      /* If delimiter character read, then break from loop. */
      /* Otherwise ... */
      if(c != delimiter){
         if((c < '0') || (c > '9')){
            fprintf(stderr, "ERROR : biomeval_nbis_read_integer : "
		    "non-numeric character '%c' (0x%02x) read, at %ld\n",
		     c, c, ftell(fpin));
            return(-3);
         }

         string[num_chars++] = c;

         /* Include eventual Null terminator character in the test. */
         if(num_chars >= sizeof(string)){
	    fprintf(stderr, "ERROR : biomeval_nbis_read_integer : "
		    "read %*s, maximum integer length %d exceeded, at %ld\n",
		    (unsigned int)sizeof(string), string,
		    (unsigned int)sizeof(string), ftell(fpin));
               return(-4);
         }
      }
   /* Continue looping as long as delimiter is NOT read. */
   }while(c != delimiter);

   /* Null terminate the value string. */
   string[num_chars] = '\0';

   *ointeger = atoi(string);

   /* Return successfully. */
   return(TRUE);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_skip_white_space - Reads a contiguous sequence of white space characters
#cat:              (spaces, tabs, and new lines) up to the next non-white
#cat:              space character is read from the open file pointer.
#cat:              Upon reading a non-white space character, the reading
#cat:              is terminated and the last character read is put back
#cat:              into the input stream.

   Input:
      fpin        - open file pointer to be read from
   Return Code:
      Positive    - last (non-white space) character read and put back
                    into input stream
      EOF         - EOF occured
************************************************************************/
int biomeval_nbis_skip_white_space(FILE *fpin)
{
   int c;

   /* Loop until ... */
   do{
      /* Read next character and if EOF ... */
      if((c = fgetc(fpin)) == EOF) {
	 if (! feof(fpin)) {
	    fprintf(stderr, "ERROR : biomeval_nbis_skip_white_space : "
		    "fgetc, at %ld: %s\n",
		    ftell(fpin), SHORT_READ_ERR_MSG(fpin));
	 }
	 return(EOF);
      }
      
      /* Continue looping as long as white space char is read. */
   }while((c == ' ') || (c == '\t') || (c == '\n'));
   
   /* Put last non-white space char read back into stream. */
   if(ungetc(c, fpin) == EOF)
      return(EOF);
   
   /* If white space skipped, return last char read. */
   return(c);
}
