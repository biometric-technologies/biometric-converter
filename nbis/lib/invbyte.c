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

      FILE:    INVBYTE.C

      AUTHOR:  Michael Garris
      DATE:    10/16/1995

      Contains global LUT to reverse the bits within a data byte from
      MSB to LSB and vice versa.

***********************************************************************/

/* this from octal dump of /usr/local/image/lut/inv_bytes.lut	*/
/* itself from /usr/local/image/src/bin/inv_byte/makelut.c	*/

/* see the include file /usr/local/image/include/invbyte.h	*/

unsigned char biomeval_nbis_Inv_Byte[256] = {
  0,128, 64,192, 32,160, 96,224,
 16,144, 80,208, 48,176,112,240,
  8,136, 72,200, 40,168,104,232,
 24,152, 88,216, 56,184,120,248,
  4,132, 68,196, 36,164,100,228,
 20,148, 84,212, 52,180,116,244,
 12,140, 76,204, 44,172,108,236,
 28,156, 92,220, 60,188,124,252,
  2,130, 66,194, 34,162, 98,226,
 18,146, 82,210, 50,178,114,242,
 10,138, 74,202, 42,170,106,234,
 26,154, 90,218, 58,186,122,250,
  6,134, 70,198, 38,166,102,230,
 22,150, 86,214, 54,182,118,246,
 14,142, 78,206, 46,174,110,238,
 30,158, 94,222, 62,190,126,254,
  1,129, 65,193, 33,161, 97,225,
 17,145, 81,209, 49,177,113,241,
  9,137, 73,201, 41,169,105,233,
 25,153, 89,217, 57,185,121,249,
  5,133, 69,197, 37,165,101,229,
 21,149, 85,213, 53,181,117,245,
 13,141, 77,205, 45,173,109,237,
 29,157, 93,221, 61,189,125,253,
  3,131, 67,195, 35,163, 99,227,
 19,147, 83,211, 51,179,115,243,
 11,139, 75,203, 43,171,107,235,
 27,155, 91,219, 59,187,123,251,
  7,135, 71,199, 39,167,103,231,
 23,151, 87,215, 55,183,119,247,
 15,143, 79,207, 47,175,111,239,
 31,159, 95,223, 63,191,127,255};
