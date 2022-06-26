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

      FILE:    DECODER.C
      AUTHORS: Craig Watson
               Michael Garris
      DATE:    12/01/1997
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for decoding a JPEGL (lossless)
      compressed data stream.

      ROUTINES:
#cat: biomeval_nbis_jpegl_decode_mem - Decodes a datastream of JPEGL compressed bytes
#cat:                    from a memory buffer, returning a lossless
#cat:                    reconstructed pixmap.
#cat: biomeval_nbis_build_huff_decode_table - Builds a table of pixel difference values.
#cat:
#cat: biomeval_nbis_decode_data - Decodes compressed data buffer.
#cat:
#cat: biomeval_nbis_nextbits_jpegl - Gets next sequence of bits for data decoding
#cat:                    from an open file.
#cat: biomeval_nbis_getc_nextbits_jpegl - Gets next sequence of bits for data decoding
#cat:                    from a memory buffer.

***********************************************************************/

#include <stdio.h>
#include <jpegl.h>
#include <dataio.h>

/******************/
/*Start of Decoder*/
/******************/
int biomeval_nbis_jpegl_decode_mem(IMG_DAT **oimg_dat, int *lossyflag,
                     unsigned char *idata, const int ilen)
{
   int ret;
   int i, cmpnt_i;
   unsigned short diff_code;       /*"raw" difference pixel*/
   int diff_cat;           /*code word category*/
   int pixel;              /*current pixel number*/
   int full_diff_code;     /*difference code extend to full precision*/
   int num_pixels;         /*number of pixels image will expand too*/
   int bit_count = 0;      /*marks the bit to receive from the input byte*/
   short data_pred;        /*prediction of pixel value*/
   /*holds the code for all possible
     difference values that occur when encoding*/
   int huff_decoder[MAX_CATEGORY][LARGESTDIFF+1];
   int ppi;
   HUF_TABLE         *huf_table[MAX_CMPNTS];
   FRM_HEADER_JPEGL  *frm_header;
   SCN_HEADER        *scn_header;
   JFIF_HEADER       *jfif_header;
   IMG_DAT           *img_dat;
   unsigned short marker;
   unsigned char *cbufptr, *ebufptr;
   unsigned char *optr;

   /*this routine builds a table used in decoding coded difference pixels*/
   biomeval_nbis_build_huff_decode_table(huff_decoder);

   for(i = 0; i < MAX_CMPNTS; i++)
      huf_table[i] = (HUF_TABLE *)NULL;

   /* Set memory buffer pointers. */
   cbufptr = idata;
   ebufptr = idata + ilen;

   if((ret = biomeval_nbis_getc_marker_jpegl(&marker, SOI, &cbufptr, ebufptr)))
      return(ret);

   if((ret = biomeval_nbis_getc_marker_jpegl(&marker, APP0, &cbufptr, ebufptr)))
      return(ret);
   if((ret = biomeval_nbis_getc_jfif_header(&jfif_header, &cbufptr, ebufptr)))
      return(ret);
   if((ret = biomeval_nbis_get_ppi_jpegl(&ppi, jfif_header))){
      free(jfif_header);
      return(ret);
   }
   free(jfif_header);

   if((ret = biomeval_nbis_getc_marker_jpegl(&marker, TBLS_N_SOF, &cbufptr, ebufptr)))
      return(ret);

   /* While not at Start of Frame ... */
   while(marker != SOF3){
      /* Get next Huffman table or comment ... */
      if((ret = biomeval_nbis_getc_table_jpegl(marker, huf_table, &cbufptr, ebufptr))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         return(ret);
      }
      /* Get next marker ... */
      if((ret = biomeval_nbis_getc_marker_jpegl(&marker, TBLS_N_SOF, &cbufptr, ebufptr))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         return(ret);
      }
   }

   /* Get the Frame Header. */
   if((ret = biomeval_nbis_getc_frame_header_jpegl(&frm_header, &cbufptr, ebufptr))){
      biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
      return(ret);
   }

   /* Allocate and initialize the output IMG_DAT structure. */
   if((ret = biomeval_nbis_setup_IMG_DAT_decode(&img_dat, ppi, frm_header))){
      biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
      free(frm_header);
      return(ret);
   }
   free(frm_header);

   if((ret = biomeval_nbis_getc_marker_jpegl(&marker, TBLS_N_SOS, &cbufptr, ebufptr))){
      biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
      biomeval_nbis_free_IMG_DAT(img_dat, NO_FREE_IMAGE);
      return(ret);
   }

   /* While not at End of Image ... */
   while(marker != EOI) {
      /* While not at Start of Scan ... */
      while(marker != SOS) {
         /* Get next Huffman table or comment ... */
         if((ret = biomeval_nbis_getc_table_jpegl(marker, huf_table, &cbufptr, ebufptr))){
            biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
            return(ret);
         }
         /* Get next marker ... */
         if((ret = biomeval_nbis_getc_marker_jpegl(&marker, TBLS_N_SOS, &cbufptr, ebufptr))){
            biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
            biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
            return(ret);
         }
      }

      /* Get the Scan Header. */
      if((ret = biomeval_nbis_getc_scan_header(&scn_header, &cbufptr, ebufptr))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }

      /* Decode image data ... */

      if((ret = biomeval_nbis_update_IMG_DAT_decode(img_dat, scn_header, huf_table))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         free(scn_header);
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }

      /* If encoded data is NOT interleaved ... */
      if(!(img_dat->intrlv)) {
	 cmpnt_i = scn_header->Cs[0];
	 optr = img_dat->image[cmpnt_i];
         /*decompress the pixel "differences" sequentially*/
	 num_pixels = img_dat->samp_width[cmpnt_i] *
                      img_dat->samp_height[cmpnt_i];
         bit_count = 0;
	 for(pixel = 0; pixel < num_pixels; pixel++) {
            /*get next huffman category code from compressed input
              data stream*/
            if((ret = biomeval_nbis_decode_data(&diff_cat, huf_table[cmpnt_i]->mincode,
                                 huf_table[cmpnt_i]->maxcode,
                                 huf_table[cmpnt_i]->valptr,
                                 huf_table[cmpnt_i]->values,
                                 &cbufptr, ebufptr, &bit_count))){
               biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
               biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
               free(scn_header);
               return(ret);
            }

            /*get the required bits (given by huffman code) to reconstruct
              the difference value for the pixel*/
            if((ret = biomeval_nbis_getc_nextbits_jpegl(&diff_code, &cbufptr, ebufptr,
                                   &bit_count, diff_cat))){
               biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
               biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
               free(scn_header);
               return(ret);
            }

            /*extend the difference value to full precision*/
            full_diff_code = huff_decoder[diff_cat][diff_code];

            /*reverse the pixel prediction and store the pixel value in the
              output buffer*/
            if((ret = biomeval_nbis_predict(&data_pred, optr, img_dat->samp_width[cmpnt_i],
                             pixel, img_dat->cmpnt_depth,
                             img_dat->predict[cmpnt_i],
                             img_dat->point_trans[cmpnt_i]))){
               biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
               biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
               free(scn_header);
               return(ret);
            }

            *optr++ = full_diff_code + data_pred;
         }
      }
      /* Otherwise, encoded data IS interleaved ... */
      else {
         fprintf(stderr, "ERROR: biomeval_nbis_jpegl_decode_mem : ");
         fprintf(stderr, "Sorry, this decoder does not handle ");
         fprintf(stderr, "encoded data that is interleaved.\n");
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
	 free(scn_header);
         return(-2);
      }

      free(scn_header);

      for(i = 0; i < img_dat->n_cmpnts; i++) {
         if(img_dat->point_trans[i]) {
            for(pixel = 0;
                pixel < img_dat->samp_width[i] * img_dat->samp_height[i];
                pixel++)
               *(img_dat->image[i]+pixel) <<= img_dat->point_trans[i];
         }
      }

      /* Get next marker ... */
      if((ret = biomeval_nbis_getc_ushort(&marker, &cbufptr, ebufptr))){
         biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
         biomeval_nbis_free_IMG_DAT(img_dat, FREE_IMAGE);
         return(ret);
      }
   }

   biomeval_nbis_free_HUFF_TABLES(huf_table, MAX_CMPNTS);
   *oimg_dat = img_dat;
   *lossyflag = 0;

   return(0);
}

/***************************************************/
/*Routine to build code table for difference values*/
/***************************************************/
void biomeval_nbis_build_huff_decode_table(int huff_decoder[MAX_CATEGORY][LARGESTDIFF+1])
{
   short diff;
   int cat, count;     /*variables used to obtain desired
                         codes for difference values*/
   int bit, difftemp;  /*variables used to determine codes for
			 -ve difference codes*/

   for(count = -LARGESTDIFF; count <= LARGESTDIFF; count++) {
      diff = (short)count;
      cat = (int)biomeval_nbis_categorize(diff);
      if(diff < 0) {
	 diff--;
	 difftemp = 0;
	 for(bit = 0; bit < cat; bit++){
	    if(((diff >> bit) & LSBITMASK) != 0)
	       difftemp |= (LSBITMASK << bit);
	 }
	 diff = difftemp;
      }

      huff_decoder[cat][diff] = count;
   }
}

/************************************/
/*routine to decode the encoded data*/
/************************************/
int biomeval_nbis_decode_data(int *odiff_cat, int *mincode, int *maxcode,
                int *valptr, unsigned char *huffvalues,
                unsigned char **cbufptr, unsigned char *ebufptr, int *bit_count)
{
   int ret;
   int inx, inx2;    /*increment variables*/
   int code;         /*becomes a huffman code word one bit at a time*/
   unsigned short tcode, tcode2;
   int diff_cat;     /*category of the huffman code word*/

   if((ret = biomeval_nbis_getc_nextbits_jpegl(&tcode, cbufptr, ebufptr, bit_count, 1)))
      return(ret);
   code = tcode;

   for(inx = 1; code > maxcode[inx]; inx++){
      if((ret = biomeval_nbis_getc_nextbits_jpegl(&tcode2, cbufptr, ebufptr, bit_count, 1)))
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
int biomeval_nbis_nextbits_jpegl(unsigned short *obits, FILE *infp,
                  int *bit_count, const int bits_req)
{
   int ret;
   static unsigned char code;    /*next byte of data*/
   unsigned char code2;
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
      if((ret = biomeval_nbis_read_byte(&code, infp)))
         return(ret);
      *bit_count = BITSPERBYTE;
      if(code == 0xff) {
         if((ret = biomeval_nbis_read_byte(&code2, infp)))
            return(ret);
	 if(code2 != 0x00) {
	    fprintf(stderr, "ERROR: biomeval_nbis_nextbits_jpegl : no stuffed zeros\n");
	    return(-2);
	 }
      }
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
      if((ret = biomeval_nbis_nextbits_jpegl(&tbits, infp, bit_count, bits_needed)))
         return(ret);
      bits |= tbits;
   }

   *obits = bits;
   return(0);
}

/**************************************************************/
/*routine to get nextbit(s) of data stream from memory buffer */
/**************************************************************/
int biomeval_nbis_getc_nextbits_jpegl(unsigned short *obits, unsigned char **cbufptr,
                  unsigned char *ebufptr, int *bit_count, const int bits_req)
{
   int ret;
   static unsigned char code;    /*next byte of data*/
   unsigned char code2;
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
      if(code == 0xff) {
         if((ret = biomeval_nbis_getc_byte(&code2, cbufptr, ebufptr)))
            return(ret);
	 if(code2 != 0x00) {
	    fprintf(stderr, "ERROR: biomeval_nbis_getc_nextbits_jpegl : no stuffed zeros\n");
	    return(-2);
	 }
      }
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
      if((ret = biomeval_nbis_getc_nextbits_jpegl(&tbits, cbufptr, ebufptr, bit_count,
                                   bits_needed)))
         return(ret);
      bits |= tbits;
   }

   *obits = bits;
   return(0);
}
