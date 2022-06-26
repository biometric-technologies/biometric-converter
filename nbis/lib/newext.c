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
      LIBRARY: IOUTIL - INPUT/OUTPUT Utilities

      FILE:    NEWEXT.C
      AUTHOR:  Michael Garris
      DATE:    11/27/89
      UPDATED: 03/11/2005 by MDG

      Contains routines responsible for replacing a file name's
      extension with a new one.

      ROUTINES:
#cat: biomeval_nbis_newext - takes a filename, strips off the rightmost extenstion, and
#cat:          appends a new extension.  This routine exits upon error.
#cat: biomeval_nbis_newext_ret - takes a filename, strips off the rightmost extenstion, and
#cat:          appends a new extension.  This routine returns an error code.
#cat: biomeval_nbis_newextlong - takes a pointer to a filename, strips the rightmost
#cat:		 extenstion, and appends an arbitrary new extension.

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

/*************************************************************/
/* Newext() is a destructive procedure which takes a filename*/
/* and strips off the rightmost extension (if one exists) and*/
/* appends the extension passed.  Exits upon error.          */
/*************************************************************/
void biomeval_nbis_newext(char *file, const int len, char *ext)
{
   char *cptr;

   cptr = file + strlen(file);
   while((cptr != file) && (*cptr != '.'))
      cptr--;
   if(cptr == file){
      if(strlen(file) == len)
         biomeval_nbis_fatalerr("biomeval_nbis_newext","File manipulation exceeds allocated memory",NULL);
      cptr += strlen(file);
      *cptr++ = '.';
      /* EDIT MDG 1/25/99
      *cptr = NULL;
      */
      *cptr = '\0';
   }
   else{
      cptr++;
      /* EDIT MDG 1/25/99
      *cptr = NULL;
      */
      *cptr = '\0';
   }
   if(strlen(file) + strlen(ext) > len)
      biomeval_nbis_fatalerr("biomeval_nbis_newext", file, "proposed extension too long");

   strcat(file,ext);
}

/*************************************************************/
/* Newext_ret() is a destructive procedure, taking a filename*/
/* and striping off the rightmost extension (if one exists)  */
/* & appends the extension passed.  Returns code upon error. */
/*************************************************************/
int biomeval_nbis_newext_ret(char *file, int len, char *ext)
{
   char *cptr;

   cptr = file + strlen(file);
   while((cptr != file) && (*cptr != '.'))
      cptr--;
   if(cptr == file){
      if(strlen(file) == len){
         fprintf(
         stderr, "ERROR : biomeval_nbis_newext_ret: file manipulation exceeds memory\n");
         return(-2);
      }
      cptr += strlen(file);
      *cptr++ = '.';
      *cptr = '\0';	 /* (char)NULL -> '\0' by JCK on 2009-02-04 */
   }
   else{
      cptr++;
      *cptr = '\0';	 /* (char)NULL -> '\0' by JCK on 2009-02-04 */
   }
   if(strlen(file) + strlen(ext) > len){
      fprintf(stderr, "ERROR : biomeval_nbis_newext_ret : proposed extension too long\n");
      return(-3);
   }

   strcat(file,ext);

   return(0);
}

/*************************************************************/
void biomeval_nbis_newextlong(char **file, char *ext)
{
int n, m;
char *cptr, *tmp;

   n = strlen(*file);      /* the original length                     */
   m = strlen(ext);

   cptr = *file + n;
   while((cptr != *file) && (*cptr != '.'))
      cptr--;

   if (cptr == *file)      /* add an extension where there wasn't one */
   {
      n += m;
      if ((tmp = (char *)calloc(n+1, sizeof(char))) == NULL)
         biomeval_nbis_syserr("biomeval_nbis_newextlong", "calloc", "space for new string result");
      sprintf(tmp, "%s.%s", *file, ext);
      free(*file);
      *file = tmp;
   }
   else
   {                      /* replace the existing extension          */
      cptr++;
      /* EDIT MDG 1/25/99
      *cptr = NULL;
      */
      *cptr = '\0';
      if ((cptr - *file) + m > n)
      {
         n += m;
         if ((tmp = (char *)calloc(n+1, sizeof(char))) == NULL)
            biomeval_nbis_syserr("biomeval_nbis_newextlong", "calloc", "space for new string result");
         sprintf(tmp, "%s%s", *file, ext);
         free(*file);
         *file = tmp;
      }
      else
         strcat(*file, ext);
   }
}
