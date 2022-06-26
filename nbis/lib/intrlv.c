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
      LIBRARY: IMAGE - Image Manipulation and Processing Routines

      FILE:    INTRLV.C

      AUTHORS: Criag Watson
               Michael Garris
      DATE:    01/19/2001
      UPDATED: 03/15/2005 by MDG

      Contains routines responsible for interleaving and de-interleaving
      an image pixmap.

      ROUTINES:
#cat: biomeval_nbis_intrlv2not_mem - takes an interleaved pixmap and de-interleaves
#cat:                  its pixels into separate component planes.
#cat: biomeval_nbis_not2intrlv_mem - takes a non-interleaved pixmap and interleaves
#cat:                  its component planes into a single composite plane.
#cat: biomeval_nbis_compute_component_padding - computes any pixel padding required to
#cat:                  interleave a pixmap.
#cat: biomeval_nbis_pad_component_planes - pads component planes prior to interleaving
#cat:                  them into a single plane.
#cat: test_image_size - compares the byte size of a pixmap's datastream
#cat:                  to component plane downsampling factors passed
#cat:                  and detects any discrepancy.

***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <intrlv.h>

#include <nbis_sysdeps.h>

/*****************************************************************/
int biomeval_nbis_intrlv2not_mem(unsigned char **oodata, int *oolen, unsigned char *idata,
                   const int width, const int height, const int depth,
                   int *hor_sampfctr, int *vrt_sampfctr, const int n_cmpnts)
{
   unsigned char *odata, *iptr, *optrs[MAX_CMPNTS];
   int c, a, b, offset, olen;
   int samp_width[MAX_CMPNTS], samp_height[MAX_CMPNTS];
   int pad_width[MAX_CMPNTS], pad_height[MAX_CMPNTS];
   int last_tile_width[MAX_CMPNTS], last_tile_height[MAX_CMPNTS];
   int skip_vrt_tile_pad[MAX_CMPNTS];
   int max_hor, max_vrt;
   int n_hor_tiles, n_vrt_tiles;
   int tx, ty;
   int ox[MAX_CMPNTS], oy[MAX_CMPNTS];

   if(n_cmpnts > MAX_CMPNTS){
      fprintf(stderr,
              "ERROR : biomeval_nbis_intrlv2not_mem : number of components = %d > %d\n",
              n_cmpnts, MAX_CMPNTS);
      return(-2);
   }

   /* Determine max tile dimensions across all components ... */
   max_hor = -1;
   max_vrt = -1;
   for(c = 0; c < n_cmpnts; c++){
      if(hor_sampfctr[c] > max_hor)
         max_hor = hor_sampfctr[c];
      if(vrt_sampfctr[c] > max_vrt)
         max_vrt = vrt_sampfctr[c];
   }

   olen = 0;
   for(c = 0; c < n_cmpnts; c++){
      /* Compute the pixel width & height of the component's output plane  */
      /* MINUS ANY IMAGE PADDING according to requirements on the          */
      /* downsampling function. */
      samp_width[c] = (int)ceil(width * (hor_sampfctr[c] / (double)max_hor));
      samp_height[c] = (int)ceil(height * (vrt_sampfctr[c] / (double)max_vrt));
      /* Increment required size of entire output buffer */
      /* MINUS ANY IMAGE PADDING! */
      olen += samp_width[c] * samp_height[c];
   }

   biomeval_nbis_compute_component_padding(pad_width, pad_height,
                     width, height, samp_width, samp_height,
                     hor_sampfctr, vrt_sampfctr, n_cmpnts);


   /* Allocate output buffer, which is a concatenation of the */
   /* component planes MINUS ANY IMAGE PADDING! */
   odata = (unsigned char *)malloc(olen * sizeof(unsigned char));
   if(odata == (unsigned char *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_intrlv2not_mem : malloc : odata\n");
      return(-3);
   }

   offset = 0;   
   for(c = 0; c < n_cmpnts; c++){
      /* Set pointer to origin of output component plane. */
      optrs[c] = odata + offset;
      /* Skip to end of current plane in output buffer. */
      offset += samp_width[c] * samp_height[c];
      /* Initialize tile origin y-pixel coords to top of */
      /* output component plane. */
      oy[c] = 0;
   }

   /* Set input data pointer which will be sequantially */
   /* addressed throughout the uninterleaving process. */
   iptr = idata;

   /* Foreach component ... */
   for(c = 0; c < n_cmpnts; c++){
      last_tile_width[c] = hor_sampfctr[c] - pad_width[c];
      last_tile_height[c] = vrt_sampfctr[c] - pad_height[c];
      skip_vrt_tile_pad[c] = pad_height[c] * hor_sampfctr[c];
   }

   /* Compute number of MCU tiles in each component plane. */
   /* Note: All component planes are required to share the */
   /* same "tile" dimensions. */
   n_hor_tiles = (samp_width[0] + pad_width[0]) / hor_sampfctr[0];
   n_vrt_tiles = (samp_height[0] + pad_height[0]) / vrt_sampfctr[0];


   /* 1. Deal with all but bottom-most row of tiles. */

   /* Foreach row of tiles in output component planes ... */
   /* Stop one tile short in case of padding ... */
   for(ty = 0; ty < n_vrt_tiles-1; ty++){

      /* Reset tile origin x-pixel coords to left margin of */
      /* each component plane. */
      for(c = 0; c < n_cmpnts; c++)
         ox[c] = 0;

      /* 1a. Deal with all tile in row minus right-most tile. */

      /* Foreach col of tiles in output component planes ... */
      /* Stop one tile short in case of padding ... */
      for(tx = 0; tx < n_hor_tiles-1; tx++){
         /* Foreach component ... */
         for(c = 0; c < n_cmpnts; c++){
            /* Foreach row in component's tile ... */
            for(b = 0; b < vrt_sampfctr[c]; b++){
               /* Foreach column in component's tile ... */
               for(a = 0; a < hor_sampfctr[c]; a++){
                  /* (Address component plane's origin) + */
                  *(optrs[c] +
                    /* (y-pixel offset into component's plane) + */
                    ((oy[c]+b)*samp_width[c]) +
                     /* (x-pixel offset into component's plane) */
                     (ox[c]+a)) = *iptr++;
               }
            }
         }

         /* Advance to next tile to the right in each component plane. */      
         for(c = 0; c < n_cmpnts; c++)
            ox[c] += hor_sampfctr[c];
      }

      /* 1b. Deal with right-most tile in row. */

      /* Deal with residual in last tile in row. */
      /* Foreach component ... */
      for(c = 0; c < n_cmpnts; c++){
         /* Foreach row in component's tile ... */
         for(b = 0; b < vrt_sampfctr[c]; b++){
            /* Foreach column in component's tile ... */
            for(a = 0; a < last_tile_width[c]; a++){
               /* (Address component plane's origin) + */
               *(optrs[c] +
                 /* (y-pixel offset into component's plane) + */
                 ((oy[c]+b)*samp_width[c]) +
                  /* (x-pixel offset into component's plane) */
                  (ox[c]+a)) = *iptr++;
            }
            /* Bump input data pointer. */
            iptr += pad_width[c];
         }
      }
      
      /* Advance to next tile down in each component plane. */      
      for(c = 0; c < n_cmpnts; c++)
         oy[c] += vrt_sampfctr[c];

   }

   /* 2. Deal with bottom-most row of tiles minus last tile. */

   /* Reset tile origin x-pixel coords to left margin of */
   /* each component plane. */
   for(c = 0; c < n_cmpnts; c++)
      ox[c] = 0;

   /* Foreach col in last row of tiles ... */
   /* Stop one tile short in case of padding ... */
   for(tx = 0; tx < n_hor_tiles-1; tx++){

      /* Deal with residual in last row of tiles. */
      /* Foreach component ... */
      for(c = 0; c < n_cmpnts; c++){
         /* Foreach row in component's tile ... */
         for(b = 0; b < last_tile_height[c]; b++){
            /* Foreach column in component's tile ... */
            for(a = 0; a < hor_sampfctr[c]; a++){
               /* (Address component plane's origin) + */
               *(optrs[c] +
                 /* (y-pixel offset into component's plane) + */
                 ((oy[c]+b)*samp_width[c]) +
                  /* (x-pixel offset into component's plane) */
                  (ox[c]+a)) = *iptr++;
            }
         }
         /* Bump input data pointer. */
         iptr += skip_vrt_tile_pad[c];
      }

      /* Advance to next tile to the right in each component plane. */      
      for(c = 0; c < n_cmpnts; c++)
         ox[c] += hor_sampfctr[c];
   }

   /* 3. Deal with residual in bottom-rightmost tile. */

   /* Foreach component ... */
   for(c = 0; c < n_cmpnts; c++){
      /* Foreach row in component's tile ... */
      for(b = 0; b < last_tile_height[c]; b++){
         /* Foreach column in component's tile ... */
         for(a = 0; a < last_tile_width[c]; a++){
            /* (Address component plane's origin) + */
            *(optrs[c] +
              /* (y-pixel offset into component's plane) + */
              ((oy[c]+b)*samp_width[c]) +
               /* (x-pixel offset into component's plane) */
               (ox[c]+a)) = *iptr++;
         }
         /* Bump input data pointer. */
         iptr += pad_width[c];
      }
      /* Bump input data pointer. */
      iptr += skip_vrt_tile_pad[c];
   }

   *oodata = odata;
   *oolen = olen;
   return(0);
}

/*****************************************************************/
int biomeval_nbis_not2intrlv_mem(unsigned char **oodata, int *oolen, unsigned char *idata,
                   const int width, const int height, const int depth,
                   int *hor_sampfctr, int *vrt_sampfctr, const int n_cmpnts)
{
   unsigned char *odata, *optr, *iptrs[MAX_CMPNTS];
   int ret, c, a, b, offset, olen;
   int samp_width[MAX_CMPNTS], samp_height[MAX_CMPNTS];
   int pad_width[MAX_CMPNTS], pad_height[MAX_CMPNTS];
   int new_samp_width[MAX_CMPNTS], new_samp_height[MAX_CMPNTS];
   int n_hor_tiles, n_vrt_tiles;
   int tx, ty;
   int ix[MAX_CMPNTS], iy[MAX_CMPNTS];

   if(n_cmpnts > MAX_CMPNTS){
      fprintf(stderr,
              "ERROR : biomeval_nbis_not2intrlv_mem : number of components = %d > %d\n",
              n_cmpnts, MAX_CMPNTS);
      return(-2);
   }

   /* 1. Compute Pixel Pads */

   biomeval_nbis_compute_component_padding(pad_width, pad_height,
                     width, height, samp_width, samp_height,
                     hor_sampfctr, vrt_sampfctr, n_cmpnts);

   /* 2. Pad Component Planes */

   if((ret = biomeval_nbis_pad_component_planes(idata, &olen, new_samp_width,
                                  new_samp_height, samp_width, samp_height,
                                  pad_width, pad_height, n_cmpnts)))
      return(ret);

   /* 3. Interleave Component Planes */

   /* Compute number of MCU tiles in each component plane. */
   /* Note: All component planes are required to share the */
   /* same "tile" dimensions. */
   n_hor_tiles = new_samp_width[0] / hor_sampfctr[0];
   n_vrt_tiles = new_samp_height[0] / vrt_sampfctr[0];

   /* Allocate output buffer, which is same size as concatenation of the */
   /* padded component planes. */
   odata = (unsigned char *)malloc(olen * sizeof(unsigned char));
   if(odata == (unsigned char *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_not2intrlv_mem : malloc : odata\n");
      return(-3);
   }

   offset = 0;   
   for(c = 0; c < n_cmpnts; c++){
      /* Set pointer to origin of input component plane. */
      iptrs[c] = idata + offset;
      /* Skip to end of current plane in padded input buffer. */
      offset += new_samp_width[c] * new_samp_height[c];
      /* Initialize tile origin y-pixel coords to top of */
      /* input component plane. */
      iy[c] = 0;
   }

   /* Set output data pointer which will be sequantially */
   /* addressed throughout the interleaving process. */
   optr = odata;

   /* Foreach row of tiles in input component planes ... */
   for(ty = 0; ty < n_vrt_tiles; ty++){

      /* Reset tile origin x-pixel coords to left margin of */
      /* each component plane. */
      for(c = 0; c < n_cmpnts; c++)
         ix[c] = 0;

      /* Foreach col of tiles in input component planes ... */
      for(tx = 0; tx < n_hor_tiles; tx++){
         /* Foreach component ... */
         for(c = 0; c < n_cmpnts; c++){
            /* Foreach row in component's tile ... */
            for(b = 0; b < vrt_sampfctr[c]; b++){
               /* Foreach column in component's tile ... */
               for(a = 0; a < hor_sampfctr[c]; a++){

                            /* (Address component plane's origin) + */
                  *optr++ = *(iptrs[c] +
                              /* (y-pixel offset into component's plane) + */
                              ((iy[c]+b)*new_samp_width[c]) +
                               /* (x-pixel offset into component's plane) */
                               (ix[c]+a));
               }
            }
         }

         /* Advance to next tile to the right in each component plane. */      
         for(c = 0; c < n_cmpnts; c++)
            ix[c] += hor_sampfctr[c];
      }
      /* Advance to next tile down in each component plane. */      
      for(c = 0; c < n_cmpnts; c++)
         iy[c] += vrt_sampfctr[c];

   }

   *oodata = odata;
   *oolen = olen;
   return(0);
}

/*****************************************************************/
void biomeval_nbis_compute_component_padding(int *pad_width, int *pad_height,
                       const int width, const int height,
                       int *samp_width, int *samp_height,
                       int *hor_sampfctr, int *vrt_sampfctr,
                       const int n_cmpnts)
{
   int c;
   int max_hor, max_vrt;

   /* Determine max tile dimensions across all components ... */
   max_hor = -1;
   max_vrt = -1;
   for(c = 0; c < n_cmpnts; c++){
      if(hor_sampfctr[c] > max_hor)
         max_hor = hor_sampfctr[c];
      if(vrt_sampfctr[c] > max_vrt)
         max_vrt = vrt_sampfctr[c];
   }

   /* Compute Pixel Pads */

   /* Compute "real" pixel dimensions of component planes and */
   /* compute potential pixel pads for each component plane. */
   for(c = 0; c < n_cmpnts; c++){
      /* Compute the pixel width of the component's output plane. */
      samp_width[c] = (int)ceil(width * (hor_sampfctr[c] / (double)max_hor));
      pad_width[c] = ((int)ceil(samp_width[c]/(double)hor_sampfctr[c]) *
                     hor_sampfctr[c]) - samp_width[c];

      /* Compute the pixel height of the component's output plane. */
      samp_height[c] = (int)ceil(height * (vrt_sampfctr[c] / (double)max_vrt));
      pad_height[c] = ((int)ceil(samp_height[c]/(double)vrt_sampfctr[c]) *
                     vrt_sampfctr[c]) - samp_height[c];
   }
}

/*****************************************************************/
int biomeval_nbis_pad_component_planes(unsigned char *idata, int *onlen,
                      int *new_samp_width, int *new_samp_height,
                      int *samp_width, int *samp_height,
                      int *pad_width, int *pad_height, const int n_cmpnts)
{
   int c, padflag, i, j, x, y, nlen;
   unsigned char *soptr, *sptr, *doptr, *dptr, *d2ptr, *spptr, *dpptr;
   int padval;

   nlen = 0;
   padflag = 0;
   for(c = 0; c < n_cmpnts; c++){
      if((pad_width[c] != 0) || (pad_height[c] != 0))
         padflag = 1;
      new_samp_width[c] = samp_width[c] + pad_width[c];
      new_samp_height[c] = samp_height[c] + pad_height[c];
      nlen += new_samp_width[c] * new_samp_height[c];
   }

   /* If not padding is necessary ... */
   if(!padflag){
      *onlen = nlen;
      return(0);
   }

   if(realloc(idata, nlen * sizeof(unsigned char)) == (void *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_pad_component_planes : realloc : idata\n");
      return(-2);
   }

   /* Set pointer to origin of last component plane. */
   soptr = idata;
   doptr = idata;
   for(c = 0; c < n_cmpnts-1; c++){
      soptr += (samp_width[c] * samp_height[c]);
      doptr += (new_samp_width[c] * new_samp_height[c]);
   }

   /* From last to first component plane ... */
   for(c = n_cmpnts-1; c >= 0; c--){
      /* Start at last scanline in source component plane. */
      sptr = soptr + ((samp_height[c] - 1) * samp_width[c]);
      /* Start at corresponding scanline in destination component plane. */
      dptr = doptr + ((samp_height[c] - 1) * new_samp_width[c]);

      /* Copy source scanline to destination component plane, */
      /* ONE PIXEL AT A TIME for memory space reasons. */
      spptr = sptr + samp_width[c] - 1;
      dpptr = dptr + samp_width[c] - 1;
      for(x = 0; x < samp_width[c]; x++){
         *dpptr-- = *spptr--;
      }
      /* Pad right of bottom scanline. */
      padval = *(dptr + samp_width[c] - 1);
      for(i = 0; i < pad_width[c]; i++)
         *(dptr+samp_width[c]+i) = padval;

      /* Pad bottom of component plane, if needed. */
      if(pad_height[c] > 0){
         /* Set new destination pointer down one scanline. */
         d2ptr = dptr + new_samp_width[c];
         for(j = 0; j < pad_height[c]; j++){
            /* Copy bottom scanline down. */
            memcpy(d2ptr, dptr, new_samp_width[c]);
            d2ptr += new_samp_width[c];
         }
      }

      /* Back up one source scanline. */
      sptr -= samp_width[c];
      /* Back up one destination scanline. */
      dptr -= new_samp_width[c];

      /* Foreach row in original source component plane */
      /* (minus the last one which is already copied) ... */
      for(y = 1; y < samp_height[c]; y++){
         /* Copy source scanline to destination component plane, */
         /* ONE PIXEL AT A TIME for memory space reasons. */
         spptr = sptr + samp_width[c] - 1;
         dpptr = dptr + samp_width[c] - 1;
         for(x = 0; x < samp_width[c]; x++){
            *dpptr-- = *spptr--;
         }

         /* Pad right of destination scanline. */
         padval = *(dptr + samp_width[c] - 1);
         for(i = 0; i < pad_width[c]; i++)
            *(dptr+samp_width[c]+i) = padval;

         /* Back up one source scanline. */
         sptr -= samp_width[c];
         /* Back up one destination scanline. */
         dptr -= new_samp_width[c];
      }

      /* If not at first component plane ... */
      if(c > 0){
         /* Set origin pointers to new component planes up the list. */
         soptr -= (samp_width[c-1] * samp_height[c-1]);
         doptr -= (new_samp_width[c-1] * new_samp_height[c-1]); 
     }
   }

   *onlen = nlen;

   return(0);
}

/*****************************************************************/
int test_image_size(const int ilen, const int w, const int h, int *hor_sampfctr,
                   int *vrt_sampfctr, const int n_cmpnts, const int intrlvflag)
{
   int c, olen;
   int samp_width[MAX_CMPNTS], samp_height[MAX_CMPNTS];
   int pad_width[MAX_CMPNTS], pad_height[MAX_CMPNTS];
   int max_hor, max_vrt;

   max_hor = -1;
   max_vrt = -1;
   for(c = 0; c < n_cmpnts; c++){
      if(hor_sampfctr[c] > max_hor)
         max_hor = hor_sampfctr[c];
      if(vrt_sampfctr[c] > max_vrt)
         max_vrt = vrt_sampfctr[c];
   }

   for(c = 0; c < n_cmpnts; c++){
      /* Compute the pixel width & height for each plane in the image */
      samp_width[c] = (int)ceil(w * (hor_sampfctr[c] / (double)max_hor));
      samp_height[c] = (int)ceil(h * (vrt_sampfctr[c] / (double)max_vrt));
      pad_width[c] = 0;
      pad_height[c] = 0;
   }

   if(intrlvflag)
      biomeval_nbis_compute_component_padding(pad_width, pad_height,
                        w, h, samp_width, samp_height,
                        hor_sampfctr, vrt_sampfctr, n_cmpnts);

   olen = 0;
   for(c = 0; c < n_cmpnts; c++)
      olen += (samp_width[c]+pad_width[c])*(samp_height[c]+pad_height[c]);

   if(olen != ilen) {
      fprintf(stderr, "ERROR : check_filesize : given file size %d ", ilen);
      fprintf(stderr, "not equal to computed biomeval_nbis_filesize %d\n", olen);
      return(-2);
   }
   return(0);
}
