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
      LIBRARY: AN2K - ANSI/NIST 2007 Reference Implementation

      FILE:    DATE.C
      AUTHOR:  Michael D. Garris
      DATE:    10/23/2000
      UPDATED: 02/28/2007 by Kenneth Ko
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
      UPDATE:  01/26/2008 by Joseph C. Konczal
                          - report more details when things go wrong

      Contains routines responsible for formatting host system date
      into a creation date formatted according to ANSI/NIST 2007.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_get_ANSI_NIST_date()

***********************************************************************/

#include <time.h>
#include <string.h>

#include <an2k.h>

/*************************************************************************
**************************************************************************
#cat:   biomeval_nbis_get_ANSI_NIST_date - Gets the formatted system date string and
#cat:                converts and returns a new date string in CCYYMMDD
#cat:                format.

   Output:
      odate_str - points to ANSI/NIST formatted date string
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int biomeval_nbis_get_ANSI_NIST_date(char **odate_str)
{
   char *tdate_str, *date_str;
   char *dayptr, *monthptr, *yearptr, *cptr;
   char uint_str[MAX_UINT_CHARS+1];
   time_t tm;
   int day_i;

   /* Access time clock. */
   tm = time(NULL);
   /* Convert to system date string "DDD MMM dd HH:MM:SS YYYY\n". */
   tdate_str = ctime(&tm);

   /* Skip Day of the week. */
   if((cptr = strchr(tdate_str, ' ')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "day of week not found in date string '%s'\n", tdate_str);
      return(-2);
   }
   /* Skip space char. */
   cptr++;

   /* Set monthptr. */
   monthptr = cptr;
   /* Skip Month. */
   if((cptr = strchr(cptr, ' ')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "month not found in date string '%s'\n", tdate_str);
      return(-3);
   }
   /* Terminiate month substring and bump ahead. */
   *cptr++ = '\0';
   /* Skip any extra spaces. */
   while(*cptr == ' ')
      cptr++;

   /* Set dayptr. */
   dayptr = cptr;
   /* Skip Numeric Day. */
   if((cptr = strchr(cptr, ' ')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "numeric day not found in date string '%s'\n", tdate_str);
      return(-4);
   }
   /* Terminiate day substring and bump ahead. */
   *cptr++ = '\0';
   /* Skip any extra spaces. */
   while(*cptr == ' ')
      cptr++;

   /* Skip Time of Day. */
   if((cptr = strchr(cptr, ' ')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "time of day not found in date string '%s'\n", tdate_str);
      return(-5);
   }
   /* Skip space char. */
   cptr++;
   /* Skip any extra spaces. */
   while(*cptr == ' ')
      cptr++;

   /* Set yearptr. */
   yearptr = cptr;
   /* Skip Year. */
   if((cptr = strchr(cptr, '\n')) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "year not found in date string '%s'\n", tdate_str);
      return(-6);
   }
   /* Terminiate year substring and bump ahead. */
   *cptr = '\0';

   /* Allocate ANSI/NIST date string "CCYYMMDD". */
   if((date_str = (char *)calloc(9, sizeof(char))) == NULL){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "calloc : date_str (%lu bytes)\n",
	      (unsigned long)(9 * sizeof(char)));
      return(-7);
   }

   /* Set CCYY. */
   if(strlen(yearptr) != 4){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "year string %s not 4 chars long\n", yearptr);
      free(date_str);
      return(-8);
   }
   strcpy(date_str, yearptr);

   /* Set MM. */
   if(strcmp(monthptr, "Jan") == 0){
      strcat(date_str, "01");
   }
   else if(strcmp(monthptr, "Feb") == 0){
      strcat(date_str, "02");
   }
   else if(strcmp(monthptr, "Mar") == 0){
      strcat(date_str, "03");
   }
   else if(strcmp(monthptr, "Apr") == 0){
      strcat(date_str, "04");
   }
   else if(strcmp(monthptr, "May") == 0){
      strcat(date_str, "05");
   }
   else if(strcmp(monthptr, "Jun") == 0){
      strcat(date_str, "06");
   }
   else if(strcmp(monthptr, "Jul") == 0){
      strcat(date_str, "07");
   }
   else if(strcmp(monthptr, "Aug") == 0){
      strcat(date_str, "08");
   }
   else if(strcmp(monthptr, "Sep") == 0){
      strcat(date_str, "09");
   }
   else if(strcmp(monthptr, "Oct") == 0){
      strcat(date_str, "10");
   }
   else if(strcmp(monthptr, "Nov") == 0){
      strcat(date_str, "11");
   }
   else if(strcmp(monthptr, "Dec") == 0){
      strcat(date_str, "12");
   }
   else{
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "illegal month string = %s\n", monthptr);
      free(date_str);
      return(-9);
   }

   /* Set DD. */
   day_i = atoi(dayptr);
   if(sprintf(uint_str, "%02d", day_i) != 2){
      fprintf(stderr, "ERROR : biomeval_nbis_get_ANSI_NIST_date : "
	      "numeric day string %s not 2 chars long\n", uint_str);
      free(date_str);
      return(-10);
   }
   strcat(date_str, uint_str);

   /* Set output pointer. */
   *odate_str = date_str;

   /* Return normally. */
   return(0);
}
