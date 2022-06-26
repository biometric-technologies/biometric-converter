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
      LIBRARY: JPEGL - Lossless JPEG Image Compression

      FILE:    SD4UTIL.C
      AUTHOR:  Craig Watson
      DATE:    12/15/2000
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for decoding an old image format
      used for JPEGL-compressing images in NIST Special Database 4.
      This format should be considered obsolete.

      ROUTINES:
#cat: biomeval_nbis_jpegl_sd4_decode_mem - Decompresses a JPEGL-compressed datastream
#cat:           according to the old image format used in NIST Special
#cat:           Database 4.  This routine should be used to decompress
#cat:           legacy data only.  This old format should be considered
#cat:           obsolete.

***********************************************************************/

#include <stdio.h>
#include <jpeglsd4.h>
#include <dataio.h>

static int biomeval_nbis_getc_huffman_table_jpegl_sd4(HUF_TABLE **, unsigned char **,
                 unsigned char *);
static int biomeval_nbis_decode_data_jpegl_sd4(int *, int *, int *, int *,
                 unsigned char *, unsigned char **, unsigned char *, int *);
static int biomeval_nbis_getc_nextbits_jpegl_sd4(unsigned short *, unsigned char **,
                 unsigned char *, int *, const int);

/************************************************************************/
/*                        Algorithms coded from:                        */
/*                                                                      */
/*                            Committee draft ISO/IEC CD 10198-1 for    */
/*                            "Digital Compression and Coding of        */
/*                             Continuous-tone Still images"            */
/*                                                                      */
/************************************************************************/
int biomeval_nbis_jpegl_sd4_decode_mem(unsigned char *idata, const int ilen, const int width,
                     const int height, const int depth, unsigned char *odata)
{
   HUF_TABLE *huf_table[MAX_CMPNTS]; /*These match the jpegl static
                                           allocation but jpegl_sd4 only has
                                           1 plane of data not MAX_CMPNTS*/
   int i, ret;
   unsigned char *cbufptr, *ebufptr;
   unsigned char predictor;                /*predictor type used*/
   unsigned char Pt = 0;                   /*Point Transform*/
   /*holds the code for all possible difference values */
   /*that occur when encoding*/
   int huff_decoder[MAX_CATEGORY][LARGESTDIFF+1];
                                                    
   int diff_cat;             /*code word category*/
   int bit_count = 0;        /*marks the bit to receive from the input byte*/
   unsigned short diff_code; /*"raw" difference pixel*/
   int full_diff_code;       /*difference code extend to full precision*/
   short data_pred;          /*prediction of pixel value*/
   unsigned char *outbuf;

   /* Set memory buffer pointers. */
   cbufptr = idata;
   ebufptr = idata + ilen;
   outbuf = odata;


   for(i = 0; i < MAX_CMPNTS; i++)
      huf_table[i] = (HUF_TABLE *)NULL;


   if((ret = biomeval_nbis_getc_huffman_table_jpegl_sd4(huf_table, &cbufptr, ebufptr)))
      return(ret);

   if((ret = biomeval_nbis_getc_byte(&predictor, &cbufptr, ebufptr))) {
      biomeval_nbis_free_HUFF_TABLES(huf_table, 1);
      return(ret);
   }

				/*this routine builds a table used in
				  decoding coded difference pixels*/
   biomeval_nbis_build_huff_decode_table(huff_decoder);

				/*decompress the pixel "differences"
				  sequentially*/
   for(i = 0; i < width*height; i++) {

      			        /*get next huffman category code from
				  compressed input data stream*/
      if((ret = biomeval_nbis_decode_data_jpegl_sd4(&diff_cat, huf_table[0]->mincode,
                           huf_table[0]->maxcode,
                           huf_table[0]->valptr, huf_table[0]->values,
                           &cbufptr, ebufptr, &bit_count))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, 1);
         return(ret);
      }

				/*get the required bits (given by huffman
				  code to reconstruct the difference
				  value for the pixel*/
      if((ret = biomeval_nbis_getc_nextbits_jpegl_sd4(&diff_code, &cbufptr, ebufptr,
                             &bit_count, diff_cat))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, 1);
         return(ret);
      }

				/*extend the difference value to
				  full precision*/
      full_diff_code = huff_decoder[diff_cat][diff_code];

				/*reverse the pixel prediction and
				  store the pixel value in the
				  output buffer*/
      if((ret = biomeval_nbis_predict(&data_pred, outbuf, width, i, depth,
                          predictor, Pt))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, 1);
         return(ret);
      }

      *outbuf = full_diff_code + data_pred;
      outbuf++;
   }
   biomeval_nbis_free_HUFF_TABLES(huf_table, 1);

   return(0);
}


/************************************/
/*routine to get huffman code tables*/
/************************************/
static int biomeval_nbis_getc_huffman_table_jpegl_sd4(HUF_TABLE **huf_table,
                        unsigned char **cbufptr, unsigned char *ebufptr)
{
   int i, ret;                  /*increment variable*/
   unsigned char number;               /*number of huffbits and huffvalues*/
   unsigned char *huffbits, *huffvalues;
   HUF_TABLE *thuf_table;

   if(biomeval_nbis_debug > 0)
      fprintf(stdout, "Start reading huffman table jpegl_sd4.\n");

   if((ret = biomeval_nbis_getc_byte(&number, cbufptr, ebufptr)))
      return(ret);

   huffbits = (unsigned char *)calloc(MAX_HUFFBITS, sizeof(unsigned char));
   if(huffbits == (unsigned char *)NULL){
      fprintf(stderr,
              "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : calloc : huffbits\n");
      return(-2);
   }

   for (i = 0; i < MAX_HUFFBITS_JPEGL_SD4;  i++)
      if((ret = biomeval_nbis_getc_byte(&(huffbits[i]), cbufptr, ebufptr))){
         free(huffbits);
         return(ret);
      }

   if(biomeval_nbis_debug > 1)
      for (i = 0; i < MAX_HUFFBITS_JPEGL_SD4;  i++)
         fprintf(stdout, "bits[%d] = %d\n", i, huffbits[i]);

   huffvalues = (unsigned char *)calloc(MAX_HUFFCOUNTS_JPEGL,
                                        sizeof(unsigned char));
   if(huffvalues == (unsigned char *)NULL){
      fprintf(stderr,
              "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : calloc : huffvalues\n");
      free(huffbits);
      return(-3);
   }
   for (i = 0; i < (number - MAX_HUFFBITS_JPEGL_SD4); i ++)
      if((ret = biomeval_nbis_getc_byte(&(huffvalues[i]), cbufptr, ebufptr))){
         free(huffbits);
         free(huffvalues);
         return(ret);
      }

   if(biomeval_nbis_debug > 1)
      for (i = 0; i < number-MAX_HUFFBITS_JPEGL_SD4;  i++)
         fprintf(stdout, "values[%d] = %d\n", i, huffvalues[i]);


   thuf_table = (HUF_TABLE *)calloc(1, sizeof(HUF_TABLE));
   if(thuf_table == (HUF_TABLE *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : ");
      fprintf(stderr, "calloc : thuf_table\n");
      return(-4);
   }
   thuf_table->freq = (int *)NULL;
   thuf_table->codesize = (int *)NULL;

   thuf_table->bits = huffbits;
   thuf_table->values = huffvalues;

   huf_table[0] = thuf_table;

   /* Build rest of table. */

   thuf_table->maxcode = (int *)calloc(MAX_HUFFCOUNTS_JPEGL+1,
                                          sizeof(int));
   if(thuf_table->maxcode == (int *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : ");
      fprintf(stderr, "calloc : maxcode\n");
      biomeval_nbis_free_HUFF_TABLE(thuf_table);
      return(-5);
   }

   thuf_table->mincode = (int *)calloc(MAX_HUFFCOUNTS_JPEGL+1,
                                          sizeof(int));
   if(thuf_table->mincode == (int *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : ");
      fprintf(stderr, "calloc : mincode\n");
      biomeval_nbis_free_HUFF_TABLE(thuf_table);
      return(-6);
   }

   thuf_table->valptr = (int *)calloc(MAX_HUFFCOUNTS_JPEGL+1, sizeof(int));
   if(thuf_table->valptr == (int *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_getc_huffman_table_jpegl_sd4 : ");
      fprintf(stderr, "calloc : valptr\n");
      biomeval_nbis_free_HUFF_TABLE(thuf_table);
      return(-7);
   }

				/*the next two routines reconstruct
   				  the huffman tables that were used
				  in the Jpeg lossless compression*/
   if((ret = biomeval_nbis_build_huffsizes(&(thuf_table->huffcode_table),
                            &(thuf_table->last_size),
                            thuf_table->bits, MAX_HUFFCOUNTS_JPEGL))){
      biomeval_nbis_free_HUFF_TABLES(huf_table, 1);
      return(ret);
   }

   biomeval_nbis_build_huffcodes(thuf_table->huffcode_table);

				/*this routine builds a set of three
				  tables used in decoding the compressed
				  data*/
   biomeval_nbis_gen_decode_table(thuf_table->huffcode_table,
                    thuf_table->maxcode, thuf_table->mincode,
                    thuf_table->valptr, thuf_table->bits);

   free(thuf_table->huffcode_table);
   thuf_table->huffcode_table = (HUFFCODE *)NULL;

   if(biomeval_nbis_debug > 0)
      fprintf(stdout, "Done reading huffman table jpegl_sd4.\n");

   return(0);
}

/************************************/
/*routine to decode the encoded data*/
/************************************/
static int biomeval_nbis_decode_data_jpegl_sd4(int *odiff_cat, int *mincode, int *maxcode,
                int *valptr, unsigned char *huffvalues,
                unsigned char **cbufptr, unsigned char *ebufptr,
                int *bit_count)
{
   int ret;
   int inx, inx2;    /*increment variables*/
   int code;         /*becomes a huffman code word one bit at a time*/
   unsigned short tcode, tcode2;
   int diff_cat;     /*category of the huffman code word*/

   if((ret = biomeval_nbis_getc_nextbits_jpegl_sd4(&tcode, cbufptr, ebufptr, bit_count, 1)))
      return(ret);
   code = tcode;

   for(inx = 1; code > maxcode[inx]; inx++){
      if((ret = biomeval_nbis_getc_nextbits_jpegl_sd4(&tcode2, cbufptr, ebufptr,
                                        bit_count, 1)))
         return(ret);
      code = (code << 1) + tcode2;
   }

   inx2 = valptr[inx];
   inx2 = inx2 + code - mincode[inx];
   diff_cat = huffvalues[inx2];

   *odiff_cat = diff_cat;
   return(0);
}

/**************************************************************/
/*routine to get nextbit(s) of data stream from memory buffer */
/**************************************************************/
static int biomeval_nbis_getc_nextbits_jpegl_sd4(unsigned short *obits,
                  unsigned char **cbufptr, unsigned char *ebufptr,
                  int *bit_count, const int bits_req)
{
   int ret;
   static unsigned char code;    /*next byte of data*/
   unsigned short bits, tbits;   /*bits of current data byte requested*/
   int bits_needed;      /*additional bits required to finish request*/

   /*used to "mask out" n number of bits from data stream*/
   static unsigned char bit_mask[9] = {0x00,0x01,0x03,0x07,0x0f,
                                       0x1f,0x3f,0x7f,0xff};

   if(bits_req == 0){
      *obits = 0;
      return(0);
   }

   if(*bit_count == 0) {
      if((ret = biomeval_nbis_getc_byte(&code, cbufptr, ebufptr)))
         return(ret);
      *bit_count = BITSPERBYTE;
   }
   if(bits_req <= *bit_count) {
      bits = (code >>(*bit_count - bits_req)) & (bit_mask[bits_req]);
      *bit_count -= bits_req;
      code &= bit_mask[*bit_count];
   }
   else {
      bits_needed = bits_req - *bit_count;
      bits = code << bits_needed;
      *bit_count = 0;
      if((ret = biomeval_nbis_getc_nextbits_jpegl_sd4(&tbits, cbufptr, ebufptr, bit_count,
                                   bits_needed)))
         return(ret);
      bits |= tbits;
   }

   *obits = bits;
   return(0);
}
