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
      LIBRARY: FET - Feature File/List Utilities

      FILE:    READFET.C
      AUTHOR:  Michael Garris
      DATE:    01/11/2001
      UPDATED: 03/10/2005 by MDG
	           02/28/2007 by Kenneth Ko

      Contains routines responsible for reading the contents of
      a file into a data structure holding an attribute-value
      paired list.

      ROUTINES:
#cat: biomeval_nbis_readfetfile - opens an fet file and reads its contents into an
#cat:               fet structure.  Exits on error.
#cat: biomeval_nbis_readfetfile_ret - opens an fet file and reads its contents into an
#cat:               fet structure.  Returns on error.

***********************************************************************/

#include <usebsd.h>
#include <string.h>
#include <fet.h>
#include <util.h>

#include <nbis_sysdeps.h>

/*****************************************************************/
FET *biomeval_nbis_readfetfile(char *file)
{
   FILE *fp;
   FET *fet;
   char c,buf[MAXFETLENGTH];
   size_t len = 0;

   if ((fp = fopen(file,"rb")) == (FILE *)NULL)
      biomeval_nbis_syserr("biomeval_nbis_readfetfile","fopen",file);

   fet = biomeval_nbis_allocfet(MAXFETS);
   while (fscanf(fp,"%s",buf) != EOF){
      while(((c = getc(fp)) == ' ') || (c == '\t'));
      ungetc(c, fp);
      if (fet->num >= fet->alloc)
         biomeval_nbis_reallocfet(fet, fet->alloc + MAXFETS);
      len = strlen(buf) + 1;
      fet->names[fet->num] = malloc(len);
      if(fet->names[fet->num] == (char *)NULL)
         biomeval_nbis_syserr("biomeval_nbis_readfetfile","malloc","fet->names[]");
      strncpy(fet->names[fet->num], buf, len);
      fgets(buf,MAXFETLENGTH-1,fp);
      buf[strlen(buf)-1] = '\0';
      len = strlen(buf) + 1;
      fet->values[fet->num] = malloc(len);
      if(fet->values[fet->num] == (char *)NULL)
         biomeval_nbis_syserr("biomeval_nbis_readfetfile","malloc","fet->values[]");
      strncpy(fet->values[fet->num], buf, len);
      (fet->num)++;
   }
   fclose(fp);
   return(fet);
}

/*****************************************************************/
int biomeval_nbis_readfetfile_ret(FET **ofet, char *file)
{
   int ret;
   FILE *fp;
   FET *fet;
   char c,buf[MAXFETLENGTH];
   size_t len = 0;

   if ((fp = fopen(file,"rb")) == (FILE *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_readfetfile_ret : fopen : %s\n", file);
      return(-2);
   }

   if((ret = biomeval_nbis_allocfet_ret(&fet, MAXFETS))){
      fclose(fp);
      return(ret);
   }

   while (fscanf(fp,"%s",buf) != EOF){
      while(((c = getc(fp)) == ' ') || (c == '\t'));
      ungetc(c, fp);
      if (fet->num >= fet->alloc){
         if((ret = biomeval_nbis_reallocfet_ret(&fet, fet->alloc + MAXFETS))){
            fclose(fp);
            biomeval_nbis_freefet(fet);
            return(ret);
         }
      }
      len = strlen(buf) + 1;
      fet->names[fet->num] = malloc(len);
      if(fet->names[fet->num] == (char *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_readfetfile_ret : malloc : fet->names[]\n");
         fclose(fp);
         biomeval_nbis_freefet(fet);
         return(-3);
      }
      strncpy(fet->names[fet->num], buf, len);
      fgets(buf,MAXFETLENGTH-1,fp);
      buf[strlen(buf)-1] = '\0';
      len = strlen(buf) + 1;
      fet->values[fet->num] = malloc(len);
      if(fet->values[fet->num] == (char *)NULL){
         fprintf(stderr, "ERROR : biomeval_nbis_readfetfile_ret : malloc : fet->values[]\n");
         fclose(fp);
         biomeval_nbis_freefet(fet);
         return(-4);
      }
      strncpy(fet->values[fet->num], buf, len);
      (fet->num)++;
   }
   fclose(fp);
   *ofet = fet;

   return(0);
}
