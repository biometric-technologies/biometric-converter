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

      FILE:    UPDATFET.C
      AUTHOR:  Michael Garris
      DATE:    01/11/2001
      UPDATED: 03/10/2005 by MDG
	           02/28/2007 by Kenneth Ko

      Contains routines responsible for replacing the value of
      a specified attribute in an attribute-value paird list.

      ROUTINES:
#cat: biomeval_nbis_updatefet - replaces a feature entry in an fet structure, or creates
#cat:             a new entry if the feature does not already exist.
#cat:             Exits on error.
#cat: biomeval_nbis_updatefet_ret - replaces a feature entry in an fet structure, or
#cat:             creates a new entry if the feature does not already exist.
#cat:             Returns on error.

***********************************************************************/

#include <usebsd.h>
#include <string.h>
#include <fet.h>
#include <defs.h>
#include <util.h>

#include <nbis_sysdeps.h>

/***********************************************************************/
void biomeval_nbis_updatefet(char *feature, char *value, FET *fet)
{
  int item;
  int increased, incr;
  size_t len = 0;

  for (item = 0;
       (item < fet->num) && (strcmp(fet->names[item],feature) != 0);
       item++);
  if (item < fet->num){
     if(fet->values[item] != (char *)NULL){
        free(fet->values[item]);
        fet->values[item] = (char *)NULL;
     }
     if(value != (char *)NULL){
        len = strlen(value) + 1;
        fet->values[item] = malloc(len);
        if(fet->values[item] == (char *)NULL)
           biomeval_nbis_syserr("biomeval_nbis_updatefet","malloc","fet->values[]");
	strncpy(fet->values[item], value, len);
     }
  }
  else{
     if(fet->num >= fet->alloc){
        incr      = fet->alloc / 10;		/* add 10% or 10 which-	*/
        increased = fet->alloc + max(10, incr);	/* ever is larger	*/
        biomeval_nbis_reallocfet(fet, increased);
     }
     len = strlen(feature) + 1;
     fet->names[fet->num] = malloc(len);
     if(fet->names[fet->num] == (char *)NULL)
        biomeval_nbis_syserr("biomeval_nbis_updatefet","malloc","fet->names[]");
     strncpy(fet->names[fet->num], feature, len);
     if(value != (char *)NULL){
        len = strlen(value) + 1;
        fet->values[fet->num] = malloc(len);
        if(fet->values[fet->num] == (char *)NULL)
           biomeval_nbis_syserr("biomeval_nbis_updatefet","malloc","fet->values[]");
	strncpy(fet->values[fet->num], value, len);
     }
     (fet->num)++;
  }
}

/***********************************************************************/
int biomeval_nbis_updatefet_ret(char *feature, char *value, FET *fet)
{
  int ret, item;
  int increased, incr;
  size_t len = 0;

  for (item = 0;
       (item < fet->num) && (strcmp(fet->names[item],feature) != 0);
       item++);
  if (item < fet->num){
     if(fet->values[item] != (char *)NULL){
        free(fet->values[item]);
        fet->values[item] = (char *)NULL;
     }
     if(value != (char *)NULL){
        len = strlen(value) + 1;
        fet->values[item] = malloc(len);
        if(fet->values[item] == (char *)NULL){
           fprintf(stderr, "ERROR : biomeval_nbis_updatefet_ret : malloc : fet->values[]\n");
           return(-2);
        }
	strncpy(fet->values[item], value, len);
     }
  }
  else{
     if(fet->num >= fet->alloc){
        incr      = fet->alloc / 10;		/* add 10% or 10 which-	*/
        increased = fet->alloc + max(10, incr);	/* ever is larger	*/
        if((ret = biomeval_nbis_reallocfet_ret(&fet, increased)))
           return(ret);
     }
     len = strlen(feature) + 1;
     fet->names[fet->num] = malloc(len);
     if(fet->names[fet->num] == (char *)NULL){
        fprintf(stderr, "ERROR : biomeval_nbis_updatefet_ret : malloc : fet->names[]\n");
        return(-3);
     }
     strncpy(fet->names[fet->num], feature, len);
     if(value != (char *)NULL){
     	len = strlen(value) + 1;
        fet->values[fet->num] = malloc(len);
        if(fet->values[fet->num] == (char *)NULL){
           fprintf(stderr, "ERROR : biomeval_nbis_updatefet_ret : malloc : fet->values[]\n");
           return(-4);
        }
	strncpy(fet->values[fet->num], value, len);
     }
     (fet->num)++;
  }

  return(0);
}
