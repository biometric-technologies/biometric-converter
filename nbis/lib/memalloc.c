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

      FILE:    MEMALLOC.C
      AUTHORS: Craig Watson
      DATE:    08/22/1996

      Contains general purpose routines for allocating/deallocating
      memory units and buffers.

      ROUTINES:
#cat: biomeval_nbis_malloc_char_ret - allocates (malloc) a char of specified size n
#cat:                   returns on error.
#cat: biomeval_nbis_malloc_uchar_ret - allocates (malloc) an unsigned char of specified size n
#cat:                    returns on error.
#cat: biomeval_nbis_malloc_int_ret - allocates (malloc) an int of specified size n
#cat:                  returns on error.
#cat: biomeval_nbis_calloc_int_ret - allocates (calloc) an int of specified size n
#cat:                  returns on error.
#cat: biomeval_nbis_realloc_int_ret - reallocates an int to new size n. returns on error.
#cat:
#cat: biomeval_nbis_datadup - mallocs new space and copies supplied data into it, see also
#cat:           ../image/imageops.c imagedup
#cat: biomeval_nbis_malloc_char - allocates (malloc) a char of specified size n
#cat:
#cat: biomeval_nbis_malloc_uchar - allocates (malloc) an unsigned char of specified size n
#cat:
#cat: biomeval_nbis_malloc_shrt - allocates (malloc) a short of specified size n
#cat:
#cat: biomeval_nbis_malloc_int - allocates (malloc) an int of specified size n
#cat:
#cat: biomeval_nbis_malloc_flt - allocates (malloc) a float of specified size n
#cat:
#cat: biomeval_nbis_calloc_char - allocates (calloc) a char of specified size n
#cat:
#cat: biomeval_nbis_calloc_uchar - allocates (calloc) an unsigned char of specified size n
#cat:
#cat: biomeval_nbis_malloc_shrt - allocates (calloc) a short of specified size n
#cat:
#cat: biomeval_nbis_calloc_int - allocates (calloc) an int of specified size n
#cat:
#cat: biomeval_nbis_calloc_flt - allocates (calloc) a float of specified size n
#cat:
#cat: biomeval_nbis_malloc_dbl_char_l1 - allocates (malloc) a set of char pointers of
#cat:                      specified number n
#cat: biomeval_nbis_malloc_dbl_uchar_l1 - allocates (malloc) a set of unsigned char pointers
#cat:                       of specified number n
#cat: biomeval_nbis_malloc_dbl_shrt_l1 - allocates (malloc) a set of short pointers of
#cat:                     specified number n
#cat: biomeval_nbis_malloc_dbl_int_l1 - allocates (malloc) a set of int pointers of specified
#cat:                     number n
#cat: biomeval_nbis_malloc_dbl_flt_l1 - allocates (malloc) a set of float pointers of
#cat:                     specified number n
#cat: biomeval_nbis_realloc_char - reallocates a char to new size n
#cat:
#cat: biomeval_nbis_realloc_uchar - reallocates an unsigned char to new size n
#cat:
#cat: biomeval_nbis_realloc_shrt - reallocates a short to new size n
#cat:
#cat: biomeval_nbis_realloc_int - reallocates an int to new size n
#cat:
#cat: biomeval_nbis_realloc_flt - reallocates an float to new size n
#cat:
#cat: biomeval_nbis_realloc_dbl_int_l1 - reallocates a set of int pointers to new number n
#cat:
#cat: biomeval_nbis_realloc_dbl_char_l1 - reallocates a set of char pointers to a
#cat:                        new number n
#cat: biomeval_nbis_realloc_dbl_uchar_l1 - reallocates a set of unsigned char pointers
#cat:                        to new number n
#cat: biomeval_nbis_realloc_dbl_flt_l1 - reallocates a set of float pointers to new number n
#cat:
#cat: biomeval_nbis_free_dbl_char - free a double char pointer
#cat:
#cat: biomeval_nbis_free_dbl_uchar - free a double uchar pointer
#cat:
#cat: biomeval_nbis_free_dbl_flt - free a double float pointer
#cat:
#cat: biomeval_nbis_malloc_dbl_char - allocates (malloc) a set of char pointers
#cat:                   of specified number n
#cat: biomeval_nbis_malloc_dbl_flt - allocates (malloc) a set of float pointers
#cat:                  of specified number n

***********************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <memalloc.h>
#include <util.h>

/*******************************************************************/
int biomeval_nbis_malloc_char_ret(char **ptr, const int n, char *s)
{
   if(((*ptr) = (char *)malloc(n * sizeof(char))) == NULL) {
      fprintf(stderr, "ERROR: biomeval_nbis_malloc_char_ret: %s\n", s);
      return(-2);
   }

   return(0);
}

/*******************************************************************/
int biomeval_nbis_malloc_uchar_ret(unsigned char **ptr, const int n, char *s)
{
   if(((*ptr) = (unsigned char *)malloc(n * sizeof(unsigned char))) == NULL) {
      fprintf(stderr, "ERROR: biomeval_nbis_malloc_uchar_ret: %s\n", s);
      return(-2);
   }

   return(0);
}

/*******************************************************************/
int biomeval_nbis_malloc_int_ret(int **ptr, const int n, char *s)
{
   if(((*ptr) = (int *)malloc(n * sizeof(int))) == NULL) {
      fprintf(stderr, "ERROR: biomeval_nbis_malloc_int_ret: %s\n", s);
      return(-2);
   }

   return(0);
}

/*******************************************************************/
int biomeval_nbis_calloc_int_ret(int **ptr, const int n, char *s)
{
   if(((*ptr) = (int *)calloc(n, sizeof(int))) == NULL) {
      fprintf(stderr, "ERROR: biomeval_nbis_calloc_int_ret: allocating memory\n");
      return(-2);
   }
   return(0);
}

/*******************************************************************/
int biomeval_nbis_realloc_int_ret(int **ptr, const int n, char *s)
{
   if((*ptr = (int *)realloc(*ptr, n * sizeof(int))) == NULL) {
      fprintf(stderr, "ERROR: biomeval_nbis_realloc_int_ret: %s\n", s);
      return(-2);
   }

   return(0);
}

/*******************************************************************/
void *biomeval_nbis_datadup(void *p, int nbytes, char *s)
{
   void *q;
   if ( p == NULL )
      return NULL;

   if ((q = (void *)malloc(nbytes)) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_datadup", "malloc", s);

   memcpy(q, p, nbytes);
   return q;
}

/*******************************************************************/
void biomeval_nbis_malloc_char(char **ptr, int n, char *s)
{
   if(((*ptr) = (char *)malloc(n * sizeof(char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_char","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_uchar(unsigned char **ptr, int n, char *s)
{
   if(((*ptr) = (unsigned char *)malloc(n * sizeof(unsigned char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_uchar","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_shrt(short **ptr, int n, char *s)
{
   if(((*ptr) = (short *)malloc(n * sizeof(short))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_shrt","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_int(int **ptr, int n, char *s)
{
   if(((*ptr) = (int *)malloc(n * sizeof(int))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_int","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_flt(float **ptr, int n, char *s)
{
   if(((*ptr) = (float *)malloc(n * sizeof(float))) == NULL)
      biomeval_nbis_syserr("malloc_float","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_calloc_char(char **ptr, int n, char *s)
{
   if(((*ptr) = (char *)calloc(n, sizeof(char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_calloc_char","calloc",s);
}

/*******************************************************************/
void biomeval_nbis_calloc_uchar(unsigned char **ptr, int n, char *s)
{
   if(((*ptr) = (unsigned char *)calloc(n, sizeof(unsigned char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_calloc_uchar","calloc",s);
}

/*******************************************************************/
void biomeval_nbis_calloc_shrt(short **ptr, int n, char *s)
{
   if(((*ptr) = (short *)calloc(n, sizeof(short))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_calloc_shrt","calloc",s);
}

/*******************************************************************/
void biomeval_nbis_calloc_int(int **ptr, int n, char *s)
{
   if(((*ptr) = (int *)calloc(n, sizeof(int))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_calloc_int","calloc",s);
}

/*******************************************************************/
void biomeval_nbis_calloc_flt(float **ptr, int n, char *s)
{
   if(((*ptr) = (float *)calloc(n, sizeof(float))) == NULL)
      biomeval_nbis_syserr("calloc_float","calloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_char_l1(char ***ptr, int ndbl, char *s)
{
   if(((*ptr) = (char **)malloc(ndbl * sizeof(char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_char_l1","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_uchar_l1(unsigned char ***ptr, int ndbl, char *s)
{
   if(((*ptr) = (unsigned char **)malloc(ndbl * sizeof(unsigned char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_uchar_l1","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_shrt_l1(short ***ptr, int ndbl, char *s)
{
   if(((*ptr) = (short **)malloc(ndbl * sizeof(short *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_shrt_l1","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_int_l1(int ***ptr, int ndbl, char *s)
{
   if(((*ptr) = (int **)malloc(ndbl * sizeof(int *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_int_l1","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_flt_l1(float ***ptr, int ndbl, char *s)
{
   if(((*ptr) = (float **)malloc(ndbl * sizeof(float *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_flt_l1","malloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_char(char **ptr, int n, char *s)
{
   if((*ptr = (char *)realloc(*ptr, n * sizeof(char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_char","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_uchar(unsigned char **ptr, int n, char *s)
{
   if((*ptr = (unsigned char *)realloc(*ptr, n * sizeof(unsigned char))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_uchar","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_shrt(short **ptr, int n, char *s)
{
   if((*ptr = (short *)realloc(*ptr, n * sizeof(short))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_shrt","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_int(int **ptr, int n, char *s)
{
   if((*ptr = (int *)realloc(*ptr, n * sizeof(int))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_int","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_flt(float **ptr, int n, char *s)
{
   if((*ptr = (float *)realloc(*ptr, n * sizeof(float))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_flt","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_dbl_int_l1(int ***ptr, int ndbl, char *s)
{
   if((*ptr = (int **)realloc(*ptr, ndbl * sizeof(int *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_dbl_int_l1","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_dbl_char_l1(char ***ptr, int ndbl, char *s)
{
   if((*ptr = (char **)realloc(*ptr, ndbl * sizeof(char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_dbl_char_l1","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_dbl_uchar_l1(unsigned char ***ptr, int ndbl, char *s)
{
   if((*ptr = (unsigned char **)realloc(*ptr, ndbl * sizeof(unsigned char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_dbl_uchar_l1","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_realloc_dbl_flt_l1(float ***ptr, int ndbl, char *s)
{
   if((*ptr = (float **)realloc(*ptr, ndbl * sizeof(float *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_realloc_dbl_flt_l1","realloc",s);
}

/*******************************************************************/
void biomeval_nbis_free_dbl_char(char **ptr, const int n)
{
   int i;

   for(i = 0; i < n; i++)
      free(ptr[i]);
   free(ptr);
}


/*******************************************************************/
void biomeval_nbis_free_dbl_uchar(unsigned char **ptr, const int n)
{
   int i;

   for(i = 0; i < n; i++)
      free(ptr[i]);
   free(ptr);
}

/*******************************************************************/
void biomeval_nbis_free_dbl_flt(float **ptr, const int n)
{
   int i;

   for(i = 0; i < n; i++)
      free(ptr[i]);
   free(ptr);
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_char(char ***ptr, const int ndbl, const int n, char *s)
{
   int i;
   char **p;

   if((p = (char **)malloc(ndbl * sizeof(char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_char","malloc",s);
   for(i= 0; i < ndbl; i++)
      biomeval_nbis_malloc_char(&(p[i]), n, s);

   *ptr = p;
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_uchar(unsigned char ***ptr, const int ndbl, const int n,
                      char *s)
{
   int i;
   unsigned char **p;

   if((p = (unsigned char **)malloc(ndbl * sizeof(unsigned char *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_uchar","malloc",s);
   for(i= 0; i < ndbl; i++)
      biomeval_nbis_malloc_uchar(&(p[i]), n, s);

   *ptr = p;
}

/*******************************************************************/
void biomeval_nbis_malloc_dbl_flt(float ***ptr, const int ndbl, const int n, char *s)
{
   int i;
   float **p;

   if((p = (float **)malloc(ndbl * sizeof(float *))) == NULL)
      biomeval_nbis_syserr("biomeval_nbis_malloc_dbl_flt","malloc",s);
   for(i= 0; i < ndbl; i++)
      biomeval_nbis_malloc_flt(&(p[i]), n, s);

   *ptr = p;
}
