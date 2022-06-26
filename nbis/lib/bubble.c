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
      LIBRARY: FING - NIST Fingerprint Systems Utilities

      FILE:           BUBBLE.C
      ALGORITHM:      Michael Garris
      DATE:           09/20/2004

***********************************************************************

      ROUTINES:
#cat: biomeval_nbis_bubble_sort_int - sorts a list of integers into decreasing order with a
#cat:                   bubble sort. It is destructive to the integer list
#cat:                   passed to it.

***********************************************************************/

#include<swap.h>

/***************************************************************************/
void biomeval_nbis_bubble_sort_int(int *pts, const int n)
{
   int done = 0;
   int i, p;

   p = n;
   while(!done){
    done = 1;
    for (i=1;i<p;i++){
      if(pts[i-1] < pts[i]){
         swap_int(pts[i-1], pts[i]);
         done = 0;
      }
    }
    p--;
  }     
}
