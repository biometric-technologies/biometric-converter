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

#ifndef _NFSEG_H
#define _NFSEG_H

#ifndef _JPEGL_H
#include <jpegl.h>
#endif
#ifndef _WSQ_H
#include <wsq.h>
#endif
#ifndef _DEFS_H
#include <defs.h>
#endif
#ifndef _MEMALLOC_H
#include <memalloc.h>
#endif
#ifndef _UTIL_H
#include <util.h>
#endif
#ifndef _DILATE_H
#include <dilate.h>
#endif
#ifndef _IMGSNIP_H
#include <imgsnip.h>
#endif
#ifndef _IMGAVG_H
#include <imgavg.h>
#endif
#ifndef _THRESH_H
#include <thresh.h>
#endif

#define BLK_DW          10
#define WHT_DW          4
#define ZERO_DW         24
#define OFF_DW1         160
#define OFF_DW2         45
#define OFF_STP         5
#define FIN_DW          6
#define EDGE_DW         5
#define FING_SPACE_MIN  25
#define FING_SPACE_MAX  60
#define FING_WIDTH_MIN  25
#define FING_HEIGHT_MIN 32
#define INBCNT          5000
#define Z_FAC           8.0
#define THR_PER         0.4
#define W_STP           5
#define TOP_LI          125

typedef struct {
   int tx, ty, bx, by;
} line_coords;

typedef struct {
   int tlx, tly;
   int tRightX, tRightY;
   int blx, bly;
   int brx, bry;
   int sx, sy, sw, sh, nrsw, nrsh;
   float theta;
   int dty, dby, dlx, drx;
   int err;
} seg_rec_coords;

extern int segment_fingers(unsigned char *, const int, const int,
               seg_rec_coords **, const int, const int, const int, const int);
extern int dynamic_threshold(unsigned char *, const int, const int, const int,
               const int, const int);
extern void remove_lines(unsigned char *, const int, const int);
extern int accum_blk_wht(unsigned char *, const int, const int, int **, int **,
               int *, const int, const int, const int);
extern int find_digits(int *, int *, const int, int *, int *, const int,
               float *, int *, int *, const int);
extern void find_digit_edges(int *, const int, int *, int *, const int, int *,
               float *);
extern int get_edge_coords(const int, const int, int *, const int,
               line_coords *, const int);
extern int get_fing_boxes(const int, const int, const float, line_coords *,
               const int, seg_rec_coords *, const int);
extern void get_fing_seg_pars(const float, seg_rec_coords *, const int);
extern int get_segfing_bounds(unsigned char *, const int, const int,
               seg_rec_coords *, const int);
extern int accum_top_row(unsigned char *, const int, const int, int **, int **,
               int **, int *);
extern int accum_top_col_blk(unsigned char *, const int, const int, int **,
               int **, int **, int *);
extern int accum_top_col_wht(unsigned char *, const int, const int, int **,
               int **, int **, int *);
extern int get_top_score(int *, const int, const int, int *, int *, int *,
               const int, int *, int *, int *, const int, int *, int *, int *,
               const int, int *);
extern int adjust_top_up(int *, unsigned char *, const int, const int,
               const int, const int);
extern void find_segfing_bottom(seg_rec_coords *, const int, unsigned char *,
               const int, const int, const int, const int, const float);
extern void find_segfing_sides(seg_rec_coords *, const int, unsigned char *,
               const int, const int, const int);
extern void adjust_fing_seg_pars(seg_rec_coords *, const int);
extern void err_check_finger(int *, seg_rec_coords *, const int);
extern void scale_seg_fingers(seg_rec_coords *, const int, const int,
               const int, const int);
extern int parse_segfing(unsigned char ***, unsigned char *, const int,
               const int, seg_rec_coords *, const int, const int);
extern int write_parsefing(char *, const int, const int, const int, const int,
	       const int, unsigned char **, seg_rec_coords *, const int, 
	       const int);
extern int insert_parsefing(ANSI_NIST *const ansi_nist, const int imgrecord_i,
	       const int fgp, const seg_rec_coords *const fing_boxes,
	       const int nf, const int rot_search);

#endif /* !_NFSEG_H */
