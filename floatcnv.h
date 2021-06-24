/* floatcnv.h
 *
 * Floating point Conversions
 *
 * We are able to convert data to and from different floating point
 * formats.
 *
 * The conversion code uses an intermediate format "fp".
 * "fp" is not a superset of the formats, because it does not implement
 * IEEE NAN or denormalized numbers.
 *
 * Formats:
 *
 *     Format   Notes
 *
 *     ms     Microsoft 32 bit (mbasic/f80)
 *     am     AM9511A 32 bit
 *     hi     Hitech C 32 bit
 *     ie     IEEE 32 bit (Turbo Modula 2 REAL, gcc float)
 *
 * Function naming conventions:
 *
 * As a matter of interest, since the conversion code is targetted to
 * z80 native, possibly using Microsoft REL format, identifiers are kept
 * to 6 characters signifance. Since Hi-Tech C prepends an '_' to names,
 * this only gives us 5 characters. Therefore, Microsoft 32 to internal is
 * ms_fp(), and internal to AM9511 is fp_am().
 */

#ifndef FLOATCONV_H
#define FLOATCONV_H

#define FP_OK   0
#define FP_ERR  1

int ie_fp(void *);
int fp_ie(void *);
int hi_fp(void *);
int fp_hi(void *);
int ms_fp(void *);
int fp_ms(void *);
int am_fp(void *);
int fp_am(void *);
void fp_get(unsigned char *sign,
	    int *exponent,
	    unsigned char *mantissa_h,
	    unsigned int *mantissa_l);
void fp_put(unsigned char sign,
	    int exponent,
	    unsigned char mantissa_h,
	    unsigned int mantissa_l);

#endif
