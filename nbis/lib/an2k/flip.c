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

      FILE:    FLIP.C
      AUTHOR:  Michael D. Garris
      DATE:    09/21/2000
      UPDATE:  01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko

      Contains routines responsible for flipping 0.01 mm
      y-coordinate points based on image heigth and flipping
      direction angles 180 degrees from specified substrings.
      These routines are useful when converting minutiae between
      different AFIS systems within the ANSI/NIST standard.

***********************************************************************
               ROUTINES:
                        biomeval_nbis_flip_y_coord()
                        biomeval_nbis_flip_direction()

***********************************************************************/

#include <stdio.h>
#include <an2k.h>
#include <defs.h>

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_flip_y_coord - Takes a y-coordinatenew in 0.01 mm units
#cat:              and inverts it based on the height of the image.
#cat:              The coordinate is encoded within a specified substring.

   Input:
      ystr         - substring containing y-coordinate starting at its
                     first character position
      fixed_length - character length of the y-coordinate substring,
                     must to be less than MAX_UINT_CHARS
      ih           - hieght of the corresponding image in pixels
      ppmm         - scan resolution of the image in pixels/mm
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_flip_y_coord(char *ystr, const int fixed_length,
                 const int ih, const double ppmm)
{
   char tchar;
   int ycoord;
   double hcoord;
   char tstr[MAX_UINT_CHARS];

   /* Input string may have additional information concatentated to it. */
   /* For example, there may be theta "TTT", so store char at end of    */
   /* fixed length and NULL terminate the y-coord. */
   tchar = *(ystr+fixed_length);
   *(ystr+fixed_length) = '\0';
   /* Convert y-coord string to integer. */
   ycoord = atoi(ystr);

   /* Convert image pixel height to 0.01 millimeter units which */
   /* is the scale X & Y coordinates are reported in. */
   hcoord = ((ih / ppmm) * 100.0);
   /* Flip y-coord based on image height. */
   ycoord = sround(hcoord - ycoord - 1);

   /* Create a new y-coord string.
      If new string not fixed_length in size ... */
   if(sprintf(tstr, "%0*d", fixed_length, ycoord) != fixed_length){
      fprintf(stderr, "ERROR : biomeval_nbis_flip_y_coord : "
	      "resulting string %s has length = %d > %d\n",
	      tstr, (int)strlen(tstr), fixed_length);
      return(-2);
   }
   /* Copy new y-coord into input string memory. */
   strcpy(ystr, tstr);
   /* Assign stored character. */
   *(ystr+fixed_length) = tchar;

   /* Return normally. */
   return(0);
}

/***********************************************************************
************************************************************************
#cat: biomeval_nbis_flip_direction - Takes a direction in degrees and flips it 180
#cat:              degrees.  The direction is encoded within a specified
#cat:              substring.

   Input:
      ystr         - substring containing direction starting at its
                     first character position
      fixed_length - character length of the direction substring
   Return Code:
      Zero       - successful completion
      Negative   - system error
************************************************************************/
int biomeval_nbis_flip_direction(char *ystr, const int fixed_length)
{
   char tchar;
   int dir;
   char tstr[MAX_UINT_CHARS];

   /* Input string may have additional information concatentated to it,   */
   /* store char at end of fixed length and NULL terminate the direction. */
   tchar = *(ystr+fixed_length);
   *(ystr+fixed_length) = '\0';
   /* Convert direction string to integer. */
   dir = atoi(ystr);
   /* Flip direction 180 degrees. */
   dir = (dir + 180) % 360;
   /* Create new direction string. 
      If new string not fixed_length in size ... */
   if(sprintf(tstr, "%0*d", fixed_length, dir) != fixed_length){
      fprintf(stderr, "ERROR : biomeval_nbis_flip_direction : "
	      "resulting string length = %d > %d\n",
	      (int)strlen(tstr), fixed_length);
      return(-2);
   }
   /* Copy new direction into input string memory. */
   strcpy(ystr, tstr);
   /* Assign stored character. */
   *(ystr+fixed_length) = tchar;

   /* Return normally. */
   return(0);
}

