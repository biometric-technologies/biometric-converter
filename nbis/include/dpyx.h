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
      PACKAGE: ANSI/NIST 2007 Standard Reference Implementation

      FILE:    DPYX.H

      AUTHORS: Michael D. Garris
               Stan Janet
      DATE:    12/30/1990
      UPDATED: 05/23/2005 by MDG
      UPDATED: 04/25/2008 by Joseph C. Konczal - added display of SEG/ASEG data

***********************************************************************/
#ifndef _DPYX_H
#define _DPYX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include <limits.h>
#include <display.h>
#include <dpydepth.h>
#include <event.h>
#include <dpy.h>

#define WIN_XY_INCR             25

#define BITMAP_UNIT_24          4 /* 4 bytes ==> 32 bits */

#define PT(x,y,w,h)             (((x)>=0)&&((x)<(w))&&((y)>=0)&&((y)<(h)))

#define ALL_BUTTONS  ((unsigned int)  (Button1Mask| \
                                Button2Mask| \
                                Button3Mask| \
                                Button4Mask| \
                                Button5Mask))


/* X-Window global references. */
extern Display *display;
extern char *display_name;
extern Window window, rw;
extern Visual *visual;
extern int screen;
extern Colormap def_cmap, cmap;
extern int cmap_size;
extern GC gc, boxgc, pointgc, seggc[3]; /* jck - added seggc */
extern unsigned long bp, wp;
extern unsigned int border_width;
extern int no_window_mgr;
extern int no_keyboard_input;

/************************************************************************/
/* dpyx.c */
extern void cleanup(void);
extern int xconnect(void);
extern int initwin(int wx, int wy, unsigned int ww,
                   unsigned int wh, unsigned int depth, unsigned long wp);
extern int set_gray_colormap(Display *, Colormap, unsigned int, unsigned long);
extern int gray_colormap(Colormap *, Display *, Visual **, unsigned int);

#endif  /* !_DPYX_H */
