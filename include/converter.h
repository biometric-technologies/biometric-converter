#ifndef BIOMETRICAL_CONVERTER_CONVERTER_H
#define BIOMETRICAL_CONVERTER_CONVERTER_H

extern int img2fmr(unsigned char *idata, int ilen, char *otype, unsigned char **odata, int *olen);

extern int fmr2fmr(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                   char *in_type_str, char *out_type_str);

extern int fmr2fmr_iso_card(unsigned char *idata, int ilen, unsigned char **odata, int *olen,
                            char *in_type_str, char *out_type_str, int iso_c_xres, int iso_c_yres);

#endif //BIOMETRICAL_CONVERTER_CONVERTER_H
