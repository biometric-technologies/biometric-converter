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

      FILE:    ENCODER.C
      AUTHORS: Craig Watson
               Michael Garris
      DATE:    12/01/1997
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for JPEGL (lossless) encoding
      image pixel data.

      ROUTINES:
#cat: biomeval_nbis_jpegl_encode_mem - JPEGL encodes image data storing the compressed
#cat:                    bytes to a memory buffer.
#cat: biomeval_nbis_gen_diff_freqs - Computes pixel differences and their fequency.
#cat:
#cat: compress_image_intrlv - Compresses difference values from
#cat:                    non-interleaved image data.
#cat: biomeval_nbis_code_diff - Huffman encodes difference values.
#cat:

***********************************************************************/

#include <stdio.h>
#include <jpegl.h>
#include <dataio.h>

/******************/
/*Start of Encoder*/
/******************/
int biomeval_nbis_jpegl_encode_mem(unsigned char **odata, int *olen, IMG_DAT *img_dat,
                     char *comment_text)
{
   int ret, i;
   HUF_TABLE *huf_table[MAX_CMPNTS];
   FRM_HEADER_JPEGL *frm_header;
   JFIF_HEADER *jfif_header;
   unsigned char *outbuf;
   int outlen, outalloc;

   if(biomeval_nbis_debug > 0){
      fprintf(stdout, "Image Data Structure\n");
      fprintf(stdout, "w = %d, h = %d, d = %d, ppi = %d\n",
              img_dat->max_width, img_dat->max_height, img_dat->pix_depth,
              img_dat->ppi);
      fprintf(stdout, "intrlv = %d\n\n", img_dat->intrlv);
      fprintf(stdout, "N = %d\n", img_dat->n_cmpnts);
      for(i = 0; i < img_dat->n_cmpnts; i++)
         fprintf(stdout, "H[%d] = %d, V[%d] = %d\n",
                 i, img_dat->hor_sampfctr[i], i, img_dat->vrt_sampfctr[i]);
      for(i = 0; i < img_dat->n_cmpnts; i++)
         fprintf(stdout, "Pt[%d] = %d, p[%d] = %d\n",
                 i, img_dat->point_trans[i], i, img_dat->predict[i]);
   }

   /* Set output buffer length to size of uncompressed image pixels. */
   outalloc = 0;
   for(i = 0; i < img_dat->n_cmpnts; i++){
      outalloc += (img_dat->samp_width[i] * img_dat->samp_height[i]);
   }

   /* Allocate output buffer. */
   outlen = 0;
   if((outbuf = (unsigned char *)malloc(outalloc)) == (unsigned char *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_jpegl_encode_mem : malloc : outbuf\n");
      return(-2);
   }

   if((ret = biomeval_nbis_putc_ushort(SOI, outbuf, outalloc, &outlen))){
      free(outbuf);
      return(ret);
   }

   if((ret = biomeval_nbis_setup_jfif_header(&jfif_header,
                              PPI_UNITS, img_dat->ppi, img_dat->ppi))){
      free(outbuf);
      return(ret);
   }

   if((ret = biomeval_nbis_putc_jfif_header(jfif_header, outbuf, outalloc, &outlen))){
      free(outbuf);
      free(jfif_header);
      return(ret);
   }
   free(jfif_header);

   if((ret = biomeval_nbis_putc_nistcom_jpegl(comment_text,
                              img_dat->max_width, img_dat->max_height,
                              img_dat->pix_depth, img_dat->ppi,
                              0 /* lossless */, img_dat->n_cmpnts,
                              img_dat->hor_sampfctr, img_dat->vrt_sampfctr,
                              img_dat->predict[0],
                              outbuf, outalloc, &outlen))){
      free(outbuf);
      return(ret);
   }

   if((ret = biomeval_nbis_setup_frame_header_jpegl(&frm_header, img_dat))){
      free(outbuf);
      return(ret);
   }

   if((ret = biomeval_nbis_putc_frame_header_jpegl(frm_header, outbuf, outalloc, &outlen))){
      free(outbuf);
      free(frm_header);
      return(ret);
   }
   free(frm_header);

   if((ret = biomeval_nbis_gen_diff_freqs(img_dat, huf_table))){
      free(outbuf);
      return(ret);
   }

   if((ret = biomeval_nbis_gen_huff_tables(huf_table, img_dat->n_cmpnts))){
      free(outbuf);
      biomeval_nbis_free_HUFF_TABLES(huf_table, img_dat->n_cmpnts);
      return(ret);
   }

   if((ret = biomeval_nbis_compress_image_non_intrlv(img_dat, huf_table,
                                      outbuf, outalloc, &outlen))){
      free(outbuf);
      biomeval_nbis_free_HUFF_TABLES(huf_table, img_dat->n_cmpnts);
      return(ret);
   }
   biomeval_nbis_free_HUFF_TABLES(huf_table, img_dat->n_cmpnts);

   if((ret = biomeval_nbis_putc_ushort(EOI, outbuf, outalloc, &outlen))){
      free(outbuf);
      return(ret);
   }

   *odata = outbuf;
   *olen = outlen;
   return(0);
}

/*****************************************/
/*routine to obtain the pixel differences*/
/*****************************************/
int biomeval_nbis_gen_diff_freqs(IMG_DAT *img_dat, HUF_TABLE **huf_table)
{
   int ret, i, pixel, np; /*current pixel and total number of pixels*/
   short data_pred;       /*predicted pixel value*/
   short *data_diff;      /*difference values*/
   short diff_cat;        /*difference category*/
   unsigned char *indata;
   unsigned char p, Pt;

   /* Need this initialization for deallocation of huf_table upon ERROR. */
   for(i = 0; i < img_dat->n_cmpnts; i++)
      huf_table[i] = (HUF_TABLE *)NULL;

   /* Foreach component ... */
   for(i = 0; i < img_dat->n_cmpnts; i++) {
      np = (img_dat->samp_width[i] * img_dat->samp_height[i]);
      /* Calloc inits all member addresses to NULL. */
      huf_table[i] = (HUF_TABLE *)calloc(1, sizeof(HUF_TABLE));
      if(huf_table[i] == (HUF_TABLE *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_gen_diff_freqs : calloc : ");
         fprintf(stderr, "huf_table[%d]\n", i);
         biomeval_nbis_free_HUFF_TABLES(huf_table, i);
         return(-2);
      }
      huf_table[i]->freq = (int *)calloc(MAX_HUFFCOUNTS_JPEGL+1, sizeof(int));
      if(huf_table[i]->freq == (int *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_gen_diff_freqs : calloc : ");
         fprintf(stderr, "huf_table[%d]->freq\n", i);
         biomeval_nbis_free_HUFF_TABLES(huf_table, i+1);
         return(-3);
      }

      huf_table[i]->freq[MAX_HUFFCOUNTS_JPEGL] = 1;

      img_dat->diff[i] = (short *)malloc(np * sizeof(short));
      if(img_dat->diff[i] == (short *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_gen_diff_freqs : malloc : ");
         fprintf(stderr, "img_dat->diff[%d]\n", i);
         biomeval_nbis_free_HUFF_TABLES(huf_table, i+1);
         return(-4);
      }

      /* If intrlv ... */
      if(!(img_dat->intrlv)) {
         Pt = img_dat->point_trans[i];
         p = img_dat->predict[i];
      }
      /* Otherwise, nonintrlv ... */
      else {
         Pt = img_dat->point_trans[0];
         p = img_dat->predict[0];
      }

      /* Set pointer to next component plane origin. */
      indata = img_dat->image[i];
      data_diff = img_dat->diff[i];
      for(pixel = 0; pixel < np; pixel++) {
         *indata >>= Pt;
         if((ret = biomeval_nbis_predict(&data_pred, indata, img_dat->samp_width[i], pixel,
                          img_dat->cmpnt_depth, p, Pt))){
            biomeval_nbis_free_HUFF_TABLES(huf_table, i+1);
            return(ret);
         }
         *data_diff = ((short)*indata) - data_pred;
         indata++;
         diff_cat = biomeval_nbis_categorize(*data_diff);
         if((diff_cat < 0) || (diff_cat > MAX_HUFFCOUNTS_JPEGL)){
            fprintf(stderr, "ERROR : biomeval_nbis_gen_diff_freqs : ");
            fprintf(stderr, "Invalid code length = %d\n", diff_cat);
            biomeval_nbis_free_HUFF_TABLES(huf_table, i+1);
            return(-5);
         }
         huf_table[i]->freq[diff_cat]++;
         data_diff++;
      }

      if(biomeval_nbis_debug > 2){
         for(pixel = 0; pixel < MAX_HUFFCOUNTS_JPEGL+1; pixel++)
            fprintf(stdout, "freqs[%d] = %d\n", pixel,
                    huf_table[i]->freq[pixel]);
      }
   }

   return(0);
}

/*********************************************/
/*Routine to "compress" the difference values*/
/*********************************************/
int biomeval_nbis_compress_image_non_intrlv(IMG_DAT *img_dat, HUF_TABLE **huf_table,
             unsigned char *outbuf, const int outalloc, int *outlen)
{
   int ret, size;       /*huffman code size*/
   unsigned int code;           /*huffman code*/
   int i, i2, p, np; /*current pixel and total number of pixels*/
   unsigned char bits;          /*bits to transfer*/
   HUFFCODE *huff_encoder;
   short *diffptr;
   int outbit = FIRSTBIT;
   SCN_HEADER *scn_header;
   unsigned char *outptr;


   for(i = 0; i < img_dat->n_cmpnts; i++) {
      if((ret = biomeval_nbis_putc_huffman_table(DHT, huf_table[i]->table_id,
                            huf_table[i]->bits, huf_table[i]->values,
                            outbuf, outalloc, outlen)))
         return(ret);

      if((ret = biomeval_nbis_setup_scan_header(&scn_header, img_dat, i)))
         return(ret);

      if((ret = biomeval_nbis_putc_scan_header(scn_header, outbuf, outalloc, outlen)))
         return(ret);
      free(scn_header);

      huff_encoder = (HUFFCODE *)calloc((LARGESTDIFF<<1)+1,
                                        sizeof(HUFFCODE));
      if(huff_encoder == (HUFFCODE *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_compress_image_non_intrlv : ");
         fprintf(stderr, "calloc : huff_encoder[%d]\n", i);
         return(-2);
      }

      np =(img_dat->samp_width[i] * img_dat->samp_height[i]);
      diffptr = img_dat->diff[i];

      if((*outlen) >= outalloc){
         fprintf(stderr, "ERROR : biomeval_nbis_compress_image_non_intrlv : ");
         fprintf(stderr, "buffer overlow: alloc = %d, request = %d\n",
                          outalloc, *outlen);
         free(huff_encoder);
         return(-3);
      }
      outptr = outbuf + (*outlen);
      *outptr = 0;

      for(p = 0; p < np; p++) {
         if((ret = biomeval_nbis_code_diff(huf_table[i]->huffcode_table,
                   (huff_encoder + (*diffptr) + LARGESTDIFF),
                   &size, &code, diffptr))){
            free(huff_encoder);
            return(ret);
         }

         diffptr++;

         for(--size; size >= 0; size--) {
            bits = ((unsigned char)((code >> size) & LSBITMASK));
            if((bits & BITSET) != 0)
               *outptr |= (BITSET << outbit);

            if(--(outbit) < 0) {
               if(*outptr == 0xff) {
                  (*outlen)++;
                  outptr++;
                  if((*outlen) >= outalloc){
                     fprintf(stderr, "ERROR : compress_image_intrlv : ");
                     fprintf(stderr, "buffer overlow: ");
                     fprintf(stderr, "alloc = %d, request = %d\n",
                                      outalloc, *outlen);
                     free(huff_encoder);
                     return(-4);
                  }
                  *outptr = 0;
               }
               (*outlen)++;
               outptr++;
               if((*outlen) >= outalloc){
                  fprintf(stderr, "ERROR : compress_image_intrlv : ");
                  fprintf(stderr, "buffer overlow: ");
                  fprintf(stderr, "alloc = %d, request = %d\n",
                                   outalloc, *outlen);
                  free(huff_encoder);
                  return(-5);
               }
               *outptr = 0;
               outbit = FIRSTBIT;
            }
         }
      }
      free(huff_encoder);

      /* Flush the Buffer */
      if(outbit != FIRSTBIT) {
         for(i2 = outbit; i2 >= 0; i2--)
            *outptr |= (BITSET << i2);
         if(*outptr == 0xff) {
            (*outlen)++;
            outptr++;
            if((*outlen) >= outalloc){
               fprintf(stderr, "ERROR : biomeval_nbis_compress_image_non_intrlv : ");
               fprintf(stderr, "buffer overlow: ");
               fprintf(stderr, "alloc = %d, request = %d\n",
                                outalloc, *outlen);
               return(-6);
            }
            *outptr = 0;
         }
         (*outlen)++;
      }
      outbit = FIRSTBIT;
   }

   return(0);
}

/*****************************************************************/
/*Routine to build code table and code existing difference values*/
/*****************************************************************/
int biomeval_nbis_code_diff(HUFFCODE *huffcode_table, HUFFCODE *huff_encoder,
              int *new_size, unsigned int *new_code, short *pdiff)
{

   int nextbit, shift, i, cat;      /*variables used to obtain desired
					codes and sizes*/
   short diff;

   diff = *pdiff;

   if((huff_encoder->size) == 0) {
      cat = (int)biomeval_nbis_categorize(diff);
      if((cat < 0) || (cat > MAX_HUFFCOUNTS_JPEGL)){
	 fprintf(stderr, "ERROR : biomeval_nbis_code_diff : invalid code length = %d\n",
                 cat);
         return(-2);
      }
      *new_size = (huffcode_table + cat)->size;
      nextbit = (MAX_HUFFBITS<<1) - *new_size;
      *new_code = ((huffcode_table + cat)->code) << (nextbit);
      nextbit--;
      if(diff < 0)
	 diff--;
      shift = cat - 1;
      for(i = 0; i < cat; i++) {
	 if(((diff >> shift) & LSBITMASK) != 0)
	    *new_code |= (LSBITMASK << (nextbit));
	 shift--;
	 nextbit--;
      }
      *new_size += cat;
      *new_code >>= ((MAX_HUFFBITS<<1) - *new_size);
      huff_encoder->size = *new_size;
      huff_encoder->code = *new_code;
   }
   else {
      *new_code = huff_encoder->code;
      *new_size = huff_encoder->size;
   }

   return(0);
}
