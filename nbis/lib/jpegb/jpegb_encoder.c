/***********************************************************************
      LIBRARY: JPEGB - Baseline (Lossy) JPEG Utilities

      FILE:    ENCODER.C
      AUTHORS: Michael Garris
               Craig Watson
      DATE:    01/08/2001
      UPDATED: 03/16/2005 by MDG

      Contains routines responsible for Baseline (lossy) JPEG
      encoding image pixel data.

      This software uses the Independent JPEG Group's (IJG)
      libraries for decompressing JPEGB images.  In fact
      these routines were derived from their code and
      modified to fit our needs.

      ROUTINES:
#cat: biomeval_nbis_jpegb_encode_mem - JPEGB encodes image data storing the compressed
#cat:                    bytes to a memory buffer.
#cat: biomeval_nbis_jpegb_encode_file - JPEGB encodes image data storing the compressed
#cat:                    bytes to an open file.

***********************************************************************/

#include <stdio.h>
#include <jpegb.h>

#include <nbis_sysdeps.h>

/**********************************************************************/
int biomeval_nbis_jpegb_encode_mem(unsigned char **oout_buffer, int *oolen,
                     const int quality, unsigned char *in_buffer,
                     const int w, const int h, const int d,
                     const int ppi, char *comment_text)
{
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;

  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;

  /* More stuff */
  JOCTET *out_buffer;
  size_t out_buffer_size;
  int    olen;
  int    ret;

  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  /* Input image Must be grayscale or RGB (interleaved). */
  if((d != 8) && (d != 24)){
     fprintf(stderr, "ERROR : jpeg_encode_mem : ");
     fprintf(stderr, "input image pixel depth %d not 8 or 24\n", d);
     return(-2);
  }

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, memory buffer) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we call our own code to send compressed data to an
   * output memory buffer.
   */
  /* Set output buffer size to hold compressed data to size of */
  /* uncompressed input image. */
  out_buffer_size = w * h * (d>>3);
  /* Allocate output buffer. */
  out_buffer = (JOCTET *)malloc(out_buffer_size * sizeof(JOCTET));
  if(out_buffer == (JOCTET *)NULL){
     fprintf(stderr, "ERROR : biomeval_nbis_jpegb_encode_mem : malloc : out_buffer\n");
     return(-3);
  }
  biomeval_nbis_jpeg_membuf_dest(&cinfo, out_buffer, out_buffer_size);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = w;          /* image width and height, in pixels */
  cinfo.image_height = h;
  /* If grayscale ... */
  if(d == 8){
     cinfo.input_components = 1;  /* # of color components per pixel */
     cinfo.in_color_space = JCS_GRAYSCALE;  /* colorspace of input image */
  }
  /* Otherwise, RGB ... */
  else{
     cinfo.input_components = 3;   /* # of color components per pixel */
     cinfo.in_color_space = JCS_RGB;   /* colorspace of input image */
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Set scan density if defined. */
  if(ppi > 0){
     cinfo.density_unit = 1;  /* ppi units */
     cinfo.X_density = ppi;
     cinfo.Y_density = ppi;
  }
  /* else, defaults values are OK. */

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Write user-supplied comment and/or NISTCOM. */
  if((ret = biomeval_nbis_put_nistcom_jpegb(&cinfo, comment_text, w, h, d, ppi,
                              1 /* lossy */, quality))){
     jpeg_destroy_compress(&cinfo);
     return(ret);
  }

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = w * (d>>3);   /* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &(in_buffer[cinfo.next_scanline * row_stride]);
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);

  /* Get length of filled output buffer. */
  olen = out_buffer_size - cinfo.dest->free_in_buffer;

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* Return compressed data buffer. */
  *oout_buffer = out_buffer;
  *oolen = olen;

  /* And we're done! */
  return(0);
}

/**********************************************************************/
int biomeval_nbis_jpegb_encode_file(FILE *outfp, const int quality,
                      unsigned char *in_buffer,
                      const int w, const int h, const int d,
                      const int ppi, char *comment_text)
{
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
  int ret;

  /* Input image Must be grayscale or RGB (interleaved). */
  if((d != 8) && (d != 24)){
     fprintf(stderr, "ERROR : jpeg_encode_file : ");
     fprintf(stderr, "input image pixel depth %d not 8 or 24\n", d);
     return(-2);
  }

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  jpeg_stdio_dest(&cinfo, outfp);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = w; 	/* image width and height, in pixels */
  cinfo.image_height = h;
  /* If grayscale ... */
  if(d == 8){
     cinfo.input_components = 1;  /* # of color components per pixel */
     cinfo.in_color_space = JCS_GRAYSCALE;  /* colorspace of input image */

  }
  /* Otherwise, RGB ... */
  else{
     cinfo.input_components = 3;   /* # of color components per pixel */
     cinfo.in_color_space = JCS_RGB;   /* colorspace of input image */
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Set scan density if defined. */
  if(ppi > 0){
     cinfo.density_unit = 1;  /* ppi units */
     cinfo.X_density = ppi;
     cinfo.Y_density = ppi;
  }
  /* else, defaults values are OK. */

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Write user-supplied comment and/or NISTCOM. */
  if((ret = biomeval_nbis_put_nistcom_jpegb(&cinfo, comment_text, w, h, d, ppi,
                              1 /* lossy */, quality))){
     jpeg_destroy_compress(&cinfo);
     return(ret);
  }

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = w * (d>>3);   /* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &(in_buffer[cinfo.next_scanline * row_stride]);
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */

  return(0);
}

/*
 * SOME FINE POINTS:
 *
 * In the above loop, we ignored the return value of jpeg_write_scanlines,
 * which is the number of scanlines actually written.  We could get away
 * with this because we were only relying on the value of cinfo.next_scanline,
 * which will be incremented correctly.  If you maintain additional loop
 * variables then you should be careful to increment them properly.
 * Actually, for output to a stdio stream you needn't worry, because
 * then jpeg_write_scanlines will write all the lines passed (or else exit
 * with a fatal error).  Partial writes can only occur if you use a data
 * destination module that can demand suspension of the compressor.
 * (If you don't know what that's for, you don't need it.)
 *
 * If the compressor requires full-image buffers (for entropy-coding
 * optimization or a noninterleaved JPEG file), it will create temporary
 * files for anything that doesn't fit within the maximum-memory setting.
 * (Note that temp files are NOT needed if you use the default parameters.)
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 *
 * Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
 * files to be compatible with everyone else's.  If you cannot readily read
 * your data in that order, you'll need an intermediate array to hold the
 * image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
 * source data using the JPEG code's internal virtual-array mechanisms.
 */

