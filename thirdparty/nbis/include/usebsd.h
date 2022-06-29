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
      FILE:    USEBSD.H
      AUTHOR:  Michael Garris
      DATE:    06/09/2005

      Controls definitions to facilitate the portable use of
      BSD-based library routines on both Linux and Cygwin platforms.
      This file must be included when code is calling for example:

         strdup()
         rindex()
         setlinebuf()

      and the strict ANSI flag "-ansi" is used.  The including of
      this file is intended to have no effect when the "-ansi"
      compiler flag is not in use.

***********************************************************************/
#ifndef _USEBSD_H
#define _USEBSD_H

#if defined(__linux__) && !defined(_BSD_SOURCE)
#define _BSD_SOURCE
#elif defined(__CYGWIN__) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__
#endif

/* The setting of _BSD_SOURCE under Linux sets up subsequent */
/* definitions in <stdio.h>.  Therefore this file should be  */
/* included first, so to help ensure this, <stdio.h> is      */
/* included here at the end of this file */
#include <stdio.h>

#endif /* !_USEBSD_H */
