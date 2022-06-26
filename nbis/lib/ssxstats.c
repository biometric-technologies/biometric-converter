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

      FILE:    SSXSTATS.C
      AUTHOR:  Darrin Dimmick
      DATE:    04/25/90

      Contains general purpose routines for computing variance
      based statistics using simple numerical methods.

      ROUTINES:
#cat: biomeval_nbis_ssx_stddev - computes standard deviation based on sum of samples
#cat:              and sum of the square of samples
#cat: biomeval_nbis_ssx_variance - comptues variance based on sum of samples
#cat:              and sum of the square of samples
#cat: biomeval_nbis_ssx - fundamental computation to support variance and stddev
#cat:

***********************************************************************/

#include <stdio.h>
#include <util.h>
#include <math.h>

/*****************************************************/
/* Routine: Standard_deviation                       */
/* Author:  Darrin L. Dimmick                        */
/* Date:    4/25/90                                  */
/*****************************************************/
/*****************************************************
   biomeval_nbis_ssx_stddev() accepts the sum of the
   values, the sum of the squares of the values, and
   the number of values contained in the sum and
   returns the standard deviation of the data.

   double sum_x;  # sum of the x values
   double sum_x2; # sum of the squares of the x values
   int    count;  # number of items sumed
******************************************************/
double biomeval_nbis_ssx_stddev(const double sum_x, const double sum_x2,
                          const int count)
{
  double var = 0;

  var = biomeval_nbis_ssx_variance(sum_x,sum_x2,count);
  if (var >= 0.0)
     return (sqrt(var));
  else
     /* otherwise error code */
     return(var);
}

/*****************************************************/
/* Routine: Variance                                 */
/* Author:  Darrin L. Dimmick                        */
/* Date:    4/25/90                                  */
/*****************************************************/
/*****************************************************
   biomeval_nbis_ssx_variance() accepts the sum of the values, the sum
   of the squares of the values, and the number of
   values contained in the sum and returns the
   variance of the data.

   double sum_x;   # sum of the x values
   double sum_x2;  # sum of the squares of the values x
   int    count;   # number of items that were sumed
******************************************************/
double biomeval_nbis_ssx_variance(const double sum_x, const double sum_x2, const int count)
{
  double ssxval;   /* holds value from SSx() */
  double variance; 

  if (count < 2){  
     fprintf(stderr,"ERROR : biomeval_nbis_ssx_variance : invalid count : %d < 2\n", count);
     return(-2.0);
  }
  ssxval = biomeval_nbis_ssx(sum_x,sum_x2,count);
  variance = ssxval/(count-1);

  return(variance);
}

/*****************************************************/
/* Routine: SSx                                      */
/* Author:  Darrin L. Dimmick                        */
/* Date:    4/25/90                                  */
/*****************************************************/
/*****************************************************
   biomeval_nbis_ssx() accepts the sum of the values, sum_x, the
   sum of the squares of the values, sum_x2 and the
   number of values contained in the sums, count and
   returns the value of the sum of the squares
   calculation, SS(x).

   double sum_x;  # sum of x  values
   double sum_x2; # sum of the squares of the x values
   int count;     # number of values sumed
******************************************************/
double biomeval_nbis_ssx(const double sum_x, const double sum_x2, const int count)
{ /* SS(x) SS(y) */
  /* SS(x) = (sum_x2 - ((sum_x * sum_x)/count)) */
  double ssx;

  ssx = sum_x * sum_x;
  ssx = ssx/count;
  ssx = sum_x2 - ssx;

  return (ssx);
}

