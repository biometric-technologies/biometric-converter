/***********************************************************************
      LIBRARY: JPEGB - Baseline (Lossy) JPEG Utilities

      FILE:    DECODER.C
      AUTHORS: Michael Garris
               Craig Watson
      DATE:    01/08/2001
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for decoding a Baseline (lossy)
      JPEG compressed data stream.

      This software uses the Independent JPEG Group's (IJG)
      libraries for decompressing JPEGB images.  In fact
      these routines were derived from their code and
      modified to fit our needs.

      ROUTINES:
#cat: biomeval_nbis_jpegb_decode_mem - Decodes a datastream of JPEGB compressed bytes
#cat:                    from a memory buffer, returning a lossy
#cat:                    reconstructed pixmap.
#cat: biomeval_nbis_jpegb_decode_file - Decodes a datastream of JPEGB compressed bytes
#cat:                    from an open file, returning a lossy
#cat:                    reconstructed pixmap.

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <jpegb.h>

#include <nbis_sysdeps.h>

#define CM_PER_INCH 2.54

/*********************************************************************/
/* JPEGB Decoder routine.  Takes a Baseline JPEG compressed          */
/* memory buffer and decodes it, returning the reconstructed pixmap. */
/*********************************************************************/
int biomeval_nbis_jpegb_decode_mem(unsigned char **oout_buffer,
                     int *ow, int *oh, int *od, int *oppi, int *lossy_flag,
                     unsigned char *in_buffer, const int in_buffer_size)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  unsigned char *out_buffer;
  unsigned char *bptr;
  unsigned char **tbuffer;	/* Output buffer */
  int row_stride;		/* physical row width in output buffer */
  int ret;

  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  biomeval_nbis_jpeg_membuf_src(&cinfo, (JOCTET *)in_buffer, (size_t)in_buffer_size);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);

/* To force RGB to grayscale on decompression ...
  cinfo.out_color_space = JCS_GRAYSCALE;
  cinfo.out_color_components = 1;
  cinfo.output_components = 1;
*/

  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  jpeg_start_decompress(&cinfo);

/* May want to honor rec_outbuf_height */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */

  /* Allocate output buffer the size of expected decompressed image pixmap. */
  out_buffer = (unsigned char *)malloc(cinfo.output_width *
                         cinfo.output_height * cinfo.output_components *
                         sizeof(unsigned char));
  if(out_buffer == (unsigned char *)NULL){
     fprintf(stderr, "ERROR : jpeg_decode_mem : malloc : out_buffer\n");
     return(-2);
  }

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  tbuffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  bptr = out_buffer;
  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, tbuffer, 1);
    memcpy(bptr, tbuffer[0], row_stride);
    bptr += row_stride;
  }

  /* Set return pointers. */
  *oout_buffer = out_buffer;
  *ow = cinfo.output_width;
  *oh = cinfo.output_height;
  *od = cinfo.output_components<<3;
  if((ret = biomeval_nbis_get_ppi_jpegb(oppi, &cinfo))){
      free(out_buffer);
      return(ret);
  }

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  *lossy_flag = TRUE;

  /* And we're done! */
  return(0);
}

/*********************************************************************/
int biomeval_nbis_jpegb_decode_file(unsigned char **oout_buffer,
                      int *ow, int *oh, int *od, int *oppi, int *lossy_flag,
                      FILE *infp)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  unsigned char *out_buffer;
  unsigned char *bptr;
  unsigned char **tbuffer;	/* Output buffer */
  int row_stride;		/* physical row width in output buffer */
  int ret;

  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, infp);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);

/* To force RGB to grayscale on decompression ...
  cinfo.out_color_space = JCS_GRAYSCALE;
  cinfo.out_color_components = 1;
  cinfo.output_components = 1;
*/

  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  jpeg_start_decompress(&cinfo);

/* May want to honor rec_outbuf_height */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */

  /* Allocate buffer the size of expected decompressed image pixmap. */
  out_buffer = (unsigned char *)malloc(cinfo.output_width *
                         cinfo.output_height * cinfo.output_components *
                         sizeof(unsigned char));
  if(out_buffer == (unsigned char *)NULL){
    fprintf(stderr, "ERROR : biomeval_nbis_jpegb_decode_file : malloc : out_buffer\n");
    return(-2);
  }

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  tbuffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  bptr = out_buffer;
  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, tbuffer, 1);
    memcpy(bptr, tbuffer[0], row_stride);
    bptr += row_stride;
  }

  /* Set return pointers. */
  *oout_buffer = out_buffer;
  *ow = cinfo.output_width;
  *oh = cinfo.output_height;
  *od = cinfo.output_components<<3;
  if((ret = biomeval_nbis_get_ppi_jpegb(oppi, &cinfo))){
      free(out_buffer);
      return(ret);
  }

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  *lossy_flag = TRUE;

  /* And we're done! */
  return(0);
}


/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */

