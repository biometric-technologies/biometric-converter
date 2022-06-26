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
      LIBRARY: UTIL - General Purpose Utility Routines

      FILE:    BRES.C
      AUTHORS: Michael Garris
               Craig Watson (Added error code returns)
      DATE:    06/07/1999
      UPDATED: 04/25/2005 by MDG

      Returns integer points between two line end points.
***********************************************************************

      ROUTINES:
#cat: biomeval_nbis_bres_line_alloc - generates the intervening discrete points along a
#cat:                   line spanning (pnt1 to pnt2) two specified endpoints.

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <defs.h>
#include <memalloc.h>


/***************************************************************************/
/* At call nalloc is how much memory is currently allocated for x_list and */
/* y_list.  If nalloc == 0 routine will allocate needed memory based on:   */
/*                                                                         */
/*         asize = max(abs(x2-x1)+2,abs(y2-y1)+2))                         */
/*                                                                         */
/* If nalloc < asize will realloc memory for x_list and y_list to asize.   */
/***************************************************************************/
int biomeval_nbis_bres_line_alloc(const int x1, const int y1, const int x2, const int y2,
                    int **x_list, int **y_list, int *num, int *nalloc)
{
   int x, y, i;
   int dx, dy, dy2, dx2, dymdx2;
   int x_end, y_end;
   int x_incr, y_incr;
   int p;
   int *xptr, *yptr;
   int asize, ret;

   asize = max(abs(x2-x1)+2, abs(y2-y1)+2);
   if(*nalloc == 0) {
      *nalloc = asize;
      if((ret = biomeval_nbis_malloc_int_ret(x_list, *nalloc, "biomeval_nbis_bres_line_alloc x_list")))
         return(ret);
      if((ret = biomeval_nbis_malloc_int_ret(y_list, *nalloc, "biomeval_nbis_bres_line_alloc y_list"))) {
         free(*x_list);
         return(ret);
      }
   }
   else if(*nalloc < asize) {
      *nalloc = asize;
      if((ret = biomeval_nbis_realloc_int_ret(x_list, *nalloc, "biomeval_nbis_bres_line_alloc x_list"))) {
         free(*x_list);
         free(*y_list);
         return(ret);
      }
      if((ret = biomeval_nbis_realloc_int_ret(y_list, *nalloc, "biomeval_nbis_bres_line_alloc y_list"))) {
         free(*x_list);
         free(*y_list);
         return(ret);
      }
   }


   i = 0;
   dx = abs(x1 - x2);
   dy = abs(y1 - y2);
   dx2 = dx<<1;
   dy2 = dy<<1;
   dymdx2 = (dy - dx)<<1;
   p = dy2 - dx;

   xptr = *x_list;
   yptr = *y_list;
   if(dx == 0){
      x = x1;
      y = y1;
      y_end = y2;
      xptr[i] = x;
      yptr[i++] = y;
      if(y1 > y2){
         while(y >= y_end){
            xptr[i] = x;
            yptr[i++] = --y;
         }
      }
      else{
         while(y <= y_end){
            xptr[i] = x;
            yptr[i++] = ++y;
         }
      }
      *num = i;
      return(0);
   }
   if(dy == 0){
      x = x1;
      y = y1;
      x_end = x2;
      xptr[i] = x;
      yptr[i++] = y;
      if(x1 > x2){
         while(x >= x_end){
            xptr[i] = --x;
            yptr[i++] = y;
         }
      }
      else{
         while(x <= x_end){
            xptr[i] = ++x;
            yptr[i++] = y;
         }
      }
      *num = i;
      return(0);
   }

   if(dx > dy){
      if(y1 < y2)
         y_incr = 1;
      else
         y_incr = -1;

      x = x1;
      y = y1;
      x_end = x2;
      xptr[i] = x;
      yptr[i++] = y;

      if(x1 <= x2) {
         while(x < x_end){
            ++x;
            if(p < 0)
               p = p + dy2;
            else{
               y += y_incr;
               p = p + dymdx2;
            }
            xptr[i] = x;
            yptr[i++] = y;
         }
         if(x != x2 || y != y2) {
            xptr[i] = x2;
            yptr[i++] = y2;
         }
      }
      else {
         while(x > x_end){
            --x;
            if(p < 0)
               p = p + dy2;
            else{
               y += y_incr;
               p = p + dymdx2;
            }
            xptr[i] = x;
            yptr[i++] = y;
         }
         if(x != x2 || y != y2) {
            xptr[i] = x2;
            yptr[i++] = y2;
         }
      }
      *num = i;
      return(0);
   }
   else {
      if(x1 < x2)
         x_incr = 1;
      else
         x_incr = -1;

      x = x1;
      y = y1;
      y_end = y2;
      xptr[i] = x;
      yptr[i++] = y;

      if(y1 <= y2) {
         while(y < y_end){
            ++y;
            if(p > 0)
               p = p - dx2;
            else{
               x += x_incr;
               p = p + dymdx2;
            }
            xptr[i] = x;
            yptr[i++] = y;
         }
         if(x != x2 || y != y2) {
            xptr[i] = x2;
            yptr[i++] = y2;
         }
      }
      else {
         while(y > y_end){
            --y;
            if(p > 0)
               p = p - dx2;
            else{
               x += x_incr;
               p = p + dymdx2;
            }
            xptr[i] = x;
            yptr[i++] = y;
         }

         if(x != x2 || y != y2) {
            xptr[i] = x2;
            yptr[i++] = y2;
         }
      }
      *num = i;
      return(0);
   }
   return(0);
}
