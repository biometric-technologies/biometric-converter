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

      FILE:    GLOBALS.C
      AUTHOR:  Michael D. Garris
      DATE:    03/28/2000
      UPDATED: 10/10/2007 by Kenneth Ko
      UPDATED: 01/31/2008 by Kenneth Ko
      UPDATE:  09/03/2008 by Kenneth Ko
	  
      Contains global variable declarations required by the ANSI/NIST
      Standard Reference Implementation.

***********************************************************************/

#include <stdio.h>
#include <an2k.h>

/* List of tagged records. */
unsigned int biomeval_nbis_tagged_records[] = {
                      TYPE_1_ID,
                      TYPE_2_ID,
                      TYPE_9_ID,
                      TYPE_10_ID,
                      TYPE_13_ID,
                      TYPE_14_ID,
                      TYPE_15_ID,
                      TYPE_16_ID,
                      TYPE_17_ID,
                      TYPE_99_ID};

/* List of binary records. */
unsigned int biomeval_nbis_binary_records[] = {
                      TYPE_3_ID,
                      TYPE_4_ID,
                      TYPE_5_ID,
                      TYPE_6_ID,
                      TYPE_7_ID,
                      TYPE_8_ID};

/* List of tagged image records. */
unsigned int tagged_image_records[] = {
                      TYPE_10_ID,
                      TYPE_13_ID,
                      TYPE_14_ID,
                      TYPE_15_ID,
                      TYPE_16_ID,
                      TYPE_17_ID,
                      TYPE_99_ID};

/* List of binary image records. */
unsigned int biomeval_nbis_binary_image_records[] = {
                      TYPE_3_ID,
                      TYPE_4_ID,
                      TYPE_5_ID,
                      TYPE_6_ID,
                      TYPE_7_ID};

/* List of binary signature records. */
unsigned int biomeval_nbis_binary_signature_records[] = {
                      TYPE_8_ID};
