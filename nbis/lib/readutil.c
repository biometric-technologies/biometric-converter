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

      FILE:    READUTIL.C
      AUTHOR:  Michael D. Garris
      DATE:    09/09/2004
      UPDATED: 03/15/2005 by MDG
			   02/28/2007 by Kenneth Ko
      UPDATED: 07/10/2014 by Kenneth Ko 

      Contains routines responsible for parsing various types of
      column-formatted text files.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_read_strstr_file();
                        biomeval_nbis_read_fltflt_file();

***********************************************************************/

#include <usebsd.h>
#include <stdlib.h>
#include <string.h>
#include <ioutil.h>

#include <nbis_sysdeps.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_strstr_file - Routine opens a column-formatted text file
#cat:                    parsing in the first column as a list of strings
#cat:                    and the second column as a list of strings.

   Input:
      ifile       - input file name to be read
   Output:
      ostr1list   - resulting first column list of strings read
      ostr2list   - resulting second column list of strings read
      ointlist    - resulting list of integers read
      olistlen    - length of resulting lists
      alloc_flag  - flag == 1, then routine allocates lists
                    flag == 0, then routine uses preallocated lists
                               and olistlen indicates list length
   Return Code:
      Zero        - successful completion
      Negative    - system error
************************************************************************/
int biomeval_nbis_read_strstr_file(char *ifile, char ***ostr1list, char ***ostr2list,
                      int *olistlen, const int alloc_flag)
{
   char **str1list, **str2list;
   char tmpstr1[MaxLineLength], tmpstr2[MaxLineLength];
   int listlen;
   int i, j, n;
   FILE *fp;

   if((fp = fopen(ifile, "rb")) == (FILE *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_strstr_file : fopen : %s\n", ifile);
      return(-2);
   }

   /* If local allocation desired ... */
   if(alloc_flag){
      /* Then do a "priming" read of the file to count length */
      listlen = 0;
      while((n = fscanf(fp, "%s %s\n", tmpstr1, tmpstr2)) != EOF){
         if(n != 2){
            fprintf(stderr, "ERROR : biomeval_nbis_read_strstr_file : fscanf : %d != 2\n",
                    n);
            return(-3);
         }
         listlen++;
      }
      rewind(fp);

      if((str1list = (char **)malloc(listlen * sizeof(char *))) ==
                     (char **)NULL){
         fprintf(stderr,
                 "ERROR : biomeval_nbis_read_strstr_file : malloc : (char **)str1list\n");
         fclose(fp);
         return(-4);
      }

      if((str2list = (char **)malloc(listlen * sizeof(char *))) ==
                     (char **)NULL){
         fprintf(stderr,
                 "ERROR : biomeval_nbis_read_strstr_file : malloc : (char **)str2list\n");
         free(str1list);
         fclose(fp);
         return(-5);
      }
   }
   /* Otherwise, lists passed in are already allocated */
   else{
      listlen = *olistlen;
      str1list = *ostr1list;
      str2list = *ostr2list;
   }

   for(i = 0; i < listlen; i++){
      n = fscanf(fp, "%s %s\n", tmpstr1, tmpstr2);
      if(n != 2){
         fprintf(stderr, "ERROR : biomeval_nbis_read_strstr_file : fscanf : %d != 2\n",
                 n);
         if(alloc_flag){
            for(j = 0; j < i; j++){
                free(str1list[j]);
                free(str2list[j]);
            }
            free(str1list);
            free(str2list);
         }
         fclose(fp);
         return(-6);
      }

      size_t len1 = strlen(tmpstr1) + 1;
      char *value1 = malloc(len1);
      if (value1 != (char *)NULL){
         strncpy(value1, tmpstr1, len1);
         str1list[i] = value1;
      }
      else{
         fprintf(stderr, "ERROR : biomeval_nbis_read_strstr_file : malloc : str1list[%d]\n",
                 i);
         if(alloc_flag){
            for(j = 0; j < i; j++)
            {
                free(str1list[j]);
                free(str2list[j]);               
            }
            free(str1list);
            free(str2list);
         }
         fclose(fp);
         return(-7);
      }

      size_t len2 = strlen(tmpstr2) + 1;
      char *value2 = malloc(len2);
      if (value2 != (char *)NULL){
         strncpy(value2, tmpstr2, len2);
         str2list[i] = value2;
      }
      else{
         fprintf(stderr, "ERROR : biomeval_nbis_read_strstr_file : malloc : str2list[%d]\n",
                 i);
         if(alloc_flag){
            for(j = 0; j < i; j++){
                free(str1list[j]);
                free(str2list[j]);
            }
            free(str1list[i]);
            free(str1list);
            free(str2list);
         }
         fclose(fp);
         return(-8);
      }
   }

   fclose(fp);


   /* If local allocation ... */
   if(alloc_flag){
     *ostr1list = str1list;
     *ostr2list = str2list;
     *olistlen = listlen;
   }

   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_read_fltflt_file - Routine opens a column-formatted text file
#cat:                    parsing in the first column as a list of floats
#cat:                    and the second column as a list of floats.

   Input:
      ifile       - input file name to be read
   Output:
      oflt1list   - resulting first column list of floats read
      oflt2list   - resulting second column list of floats read
      olistlen    - length of resulting lists
      alloc_flag  - flag == 1, then routine allocates lists
                    flag == 0, then routine uses preallocated lists
                               and olistlen indicates list length
   Return Code:
      Zero        - successful completion
      Negative    - system error
************************************************************************/
int biomeval_nbis_read_fltflt_file(char *ifile, float **oflt1list, float **oflt2list,
                     int *olistlen, const int alloc_flag)
{
   float *flt1list, *flt2list;
   float tmpflt1, tmpflt2;
   int listlen;
   int i, n;
   FILE *fp;

   if((fp = fopen(ifile, "rb")) == (FILE *)NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_read_fltflt_file : fopen : %s\n", ifile);
      return(-2);
   }

   /* If local allocation desired ... */
   if(alloc_flag){
      /* Then do a "priming" read of the file to count length */
      listlen = 0;
      while((n = fscanf(fp, "%f %f\n", &tmpflt1, &tmpflt2)) != EOF){
         if(n != 2){
            fprintf(stderr, "ERROR : biomeval_nbis_read_fltflt_file : fscanf : %d != 2\n",
                    n);
            return(-3);
         }
         listlen++;
      }
      rewind(fp);

      if((flt1list = (float *)malloc(listlen * sizeof(float))) ==
                     (float *)NULL){
         fprintf(stderr,
                 "ERROR : biomeval_nbis_read_fltflt_file : malloc : (float *)flt1list\n");
         fclose(fp);
         return(-4);
      }

      if((flt2list = (float *)malloc(listlen * sizeof(float))) ==
                     (float *)NULL){
         fprintf(stderr,
                 "ERROR : biomeval_nbis_read_fltflt_file : malloc : (float *)flt2list\n");
         free(flt1list);
         fclose(fp);
         return(-5);
      }
   }
   /* Otherwise, lists passed in are already allocated */
   else{
      listlen = *olistlen;
      flt1list = *oflt1list;
      flt2list = *oflt2list;
   }

   for(i = 0; i < listlen; i++){
      n = fscanf(fp, "%f %f\n", &tmpflt1, &tmpflt2);
      if(n != 2){
         fprintf(stderr, "ERROR : biomeval_nbis_read_fltflt_file : fscanf : %d != 2\n",
                 n);
         if(alloc_flag){
            free(flt1list);
            free(flt2list);
         }
         fclose(fp);
         return(-6);
      }

      flt1list[i] = tmpflt1;
      flt2list[i] = tmpflt2;
   }

   fclose(fp);

   /* If local allocation ... */
   if(alloc_flag){
     *oflt1list = flt1list;
     *oflt2list = flt2list;
     *olistlen = listlen;
   }

   return(0);
}
