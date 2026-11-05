#ifndef BIOMETRICAL_CONVERTER_CONVERTER_H
#define BIOMETRICAL_CONVERTER_CONVERTER_H

/*
 * Type strings: "ANSI" (ANSI INCITS 378-2004), "ISO" (ISO/IEC 19794-2:2005
 * record format), "ISONC" (normal card format), "ISOCC" (compact card format).
 *
 * The conversion functions return 0 on success and -1 on error. On success
 * *odata points to a buffer of *olen bytes allocated with malloc(); release
 * it with converter_free() (or free()). On error *odata is NULL, *olen is 0
 * and converter_last_error() describes the problem. The functions may be
 * called from several threads at once.
 */

/* Image (WSQ, JPEG, JPEG 2000, PNG, NIST IHead) to a minutiae record of type otype. */
extern int img2fmr(unsigned char *idata, int ilen, char *otype, unsigned char **odata, int *olen);

/* Minutiae record of type in_type_str to type out_type_str. ISO card input is
 * taken to be at 500 ppi; use fmr2fmr_iso_card() to give its resolution. */
extern int fmr2fmr(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                   char *in_type_str, char *out_type_str);

/* As fmr2fmr(), with the resolution of ISO card input in pixels per centimetre
 * (197 for 500 ppi); 0 means 197. Ignored for other input types. */
extern int fmr2fmr_iso_card(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                            char *in_type_str, char *out_type_str, int iso_c_xres, int iso_c_yres);

/* Why the last call in the calling thread failed; "" after a successful call. */
extern const char *converter_last_error(void);

/* Release a buffer returned in *odata. */
extern void converter_free(void *data);

#endif //BIOMETRICAL_CONVERTER_CONVERTER_H
