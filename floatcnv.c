/* floatcnv.c
 *
 * Floating point conversions.
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
 * Compile and test with GCC, TCC and HI-TECH C. Floating point conversion
 * code is of use on the 8080 platform to prepare input for the AM9511.
 * We add tcc to the "compiler mix" to ensure portability of the code.
 *
 * Function naming conventions:
 *
 * As a matter of interest, since the conversion code is targetted to
 * z80 native, possibly using Microsoft REL format, identifiers are kept
 * to 6 characters signifance. Since Hi-Tech C prepends an '_' to names,
 * this only gives us 5 characters. Therefore, Microsoft 32 to internal is
 * ms_fp(), and internal to AM9511 is fp_am().
 */

#include <stddef.h>
#include "floatcnv.h"

/* I0..I3 support endian systems other than little endian. Unfortunately,
 * I do not have access to big endian systems anymore.
 */
#define I0 0 /* least significant byte */
#define I1 1
#define I2 2
#define I3 3 /* most significant byte */

#ifdef __TINYC__

/* Types for TCC, Linux, 64 bit
 */
typedef unsigned short uint16;
typedef unsigned char  uint8;
typedef short          int16;

#endif

#ifdef __GNUC__

/* Types for GNUC, Linux, 64 bit
 */
typedef unsigned short uint16;
typedef unsigned char  uint8;
typedef short          int16;

#endif

#ifdef z80

/* Types for Hi-Tech C, z80, 8 bit
 */
typedef unsigned int   uint16;
typedef unsigned char  uint8;
typedef int            int16;

#endif

/* Internal floating point representation, separate sign, normalized 24 bit
 * mantissa with explicit high "1" bit.
 * If mantissa == 0, value is 0.0.
 * Exponent is not biased.
 * Separate sign (1 = negative, 0 = positive)
 */
struct fp {
    uint8  sign;
    uint8  mantissa_h;
    uint16 mantissa_l;
    int16  exponent;
};

/* memset() is broken with hi-tech C. Also we may have issues with
 * long shifts. The memset() issue prompts us to write and use clear()
 * instead. We avoid long (32 bit) values to avoid linking in library
 * routines on the z80. We are limiting ourselves to 8 and 16 bit
 * operations. "register" declarations will be added... by examining
 * z80 assembler output...
 */
static void clear(void *p, int n) {
    unsigned char *cp = p;
    while (n--)
        *cp++ = 0;
}

/* Convert IEEE to FP format.
 *
 * These routines are written for clarity and portability, not speed.
 * Assumptions are that "unsigned char" is 8 bit (0..255). Incoming
 * pointer p is to point to a IEEE format 32 bit float. If this is in
 * "little-endian" order, I0=0, I1=1, I2=2 and I3=3 can be used. If
 * this is in "big endian", try I0=3, I1=2, I2=1 and I3=0. We are
 * assuming that the IEEE float occupies 4 bytes of contiguous memory,
 * starting with pointer p.
 */
int ie_fp(struct fp *f, void *p) {
    unsigned char *ieee = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Clear destination
     */
    clear(f, sizeof(*f));

    /* Extract exponent. 7 bits from I2, bit from I3.
     */
    e = ((ieee[I3] & 0x7f) << 1) |
         ((ieee[I2] & 0x80) != 0);

    /* Extract sign. 1 if negative, 0 if positive.
     */
    s = (ieee[I3] & 0x80) != 0;

    /* If exponent == 0, number is zero.
     */
    if (e == 0)
	return FP_OK;

    /* If exponent == 0xff, number is NAN, or one of the other specials.
     */
    if (e == 0xff)
	return FP_ERR;

    /* Unbias the exponent.
     */
    e -= 127;

    /* Get 23 bits of mantissa
     */
    m_h = ieee[I2] & 0x7f;
    m_l = ieee[I1];
    m_l = (m_l << 8) | ieee[I0];

    /* Add leading 1 bit, which was implied.
     */
    m_h |= 0x80;

    /* Save into fp.
     */
    f->exponent = e;
    f->sign = s;
    f->mantissa_h = m_h;
    f->mantissa_l = m_l;

    return FP_OK;
}

/* Convert FP to IEEE format.
 */
int fp_ie(void *p, struct fp *f) {
    unsigned char *ieee = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Get from fp.
     */
    e = f->exponent;
    m_h = f->mantissa_h;
    m_l = f->mantissa_l;
    s = f->sign;

    /* Clear destination.
     */
    clear(ieee, 4);

    /* If mantissa bit 23 is 0, result is 0.0.
     */
    if ((m_h & 0x80) == 0)
	return FP_OK;

    /* Strip leading 1 bit from mantissa
     */
    m_h &= 0x7f;

    /* Range check exponent.
     */
    if (e < -126)
	return FP_ERR;
    if (e > 127)
	return FP_ERR;

    /* Bias exponent.
     */
    e += 127;

    /* Low bit of exponent goes into mantissa
     */
    if (e & 1)
	m_h |= 0x80;
    e = (e >> 1) & 0x7f;

    /* Sign goes to into exponent
     */
    if (s)
	e |= 0x80;

    /* Put results into memory
     */
    ieee[I0] = m_l & 0xff;
    ieee[I1] = (m_l >> 8) & 0xff;
    ieee[I2] = m_h & 0xff;
    ieee[I3] = e & 0xff;

    return FP_OK;
}

/* Convert HITECH to FP format.
 */
int hi_fp(struct fp *f, void *p) {
    unsigned char *hitech = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Clear destination
     */
    clear(f, sizeof(*f));

    /* If bit 23 is zero, must be zero (or not normalized).
     */
    if ((hitech[I2] & 0x80) == 0)
	return FP_OK;

    /* Gather up sign, and exponent. Remove sign
     * bit from exponent.
     */
    s = (hitech[I3] & 0x80) != 0;
    e = hitech[I3] & 0x7f;

    /* Gather mantissa. Note that leading '1' bit is there.
     */
    m_h = hitech[I2];
    m_l = hitech[I1];
    m_l = (m_l << 8) | hitech[I0];

    /* Unbias the exponent.
     */
    e -= 65;

    /* Put into fp.
     */
    f->sign = s;
    f->exponent = e;
    f->mantissa_l = m_l;
    f->mantissa_h = m_h;

    return FP_OK;
}

/* Convert FP to HITECH format.
 */
int fp_hi(void *p, struct fp *f) {
    unsigned char *hitech = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Get from f.
     */
    e = f->exponent;
    m_l = f->mantissa_l;
    m_h = f->mantissa_h;
    s = f->sign;

    /* Clear destination.
     */
    clear(hitech, 4);

    /* If mantissa bit 23 is 0, result is 0.0.
     */
    if ((m_h & 0x80) == 0)
	return FP_OK;

    /* Range check the exponent.
     */
    if (e < -65)
	return FP_ERR;
    if (e > 63)
	return FP_ERR;

    /* Bias the exponent.
     */ 
    e += 65;
    e &= 0x7f;

    /* Add in sign bit to exponent.
     */
    if (s)
	e |= 0x80;

    /* Build up output.
     */
    hitech[I3] = e & 0xff;
    hitech[I2] = m_h & 0xff;
    hitech[I1] = (m_l >> 8) & 0xff;
    hitech[I0] = m_l & 0xff;

    return FP_OK;
}

/* Convert MS to FP format.
 */
int ms_fp(struct fp *f, void *p) {
    unsigned char *ms = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Clear destination
     */
    clear(f, sizeof(*f));

    /* If exponent is zero, must be zero.
     */
    if (ms[I3] == 0)
	return FP_OK;

    /* Gather up sign, and exponent.
     */
    s = (ms[I2] & 0x80) != 0;
    e = ms[I3];

    /* Gather mantissa. Replace sign bit with leading '1'.
     */
    m_h = ms[I2] | 0x80;
    m_l = ms[I1];
    m_l = (m_l << 8) | ms[I0];

    /* Unbias the exponent.
     */
    e -= 129;

    /* Put into fp.
     */
    f->sign = s;
    f->exponent = e;
    f->mantissa_l = m_l;
    f->mantissa_h = m_h;

    return FP_OK;
}

/* Convert FP to MS format.
 */
int fp_ms(void *p, struct fp *f) {
    unsigned char *ms = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Get from fp.
     */
    e = f->exponent;
    m_l = f->mantissa_l;
    m_h = f->mantissa_h;
    s = f->sign;

    /* Clear destination.
     */
    clear(ms, 4);

    /* If mantissa bit 23 is 0, result is 0.0.
     */
    if ((m_h & 0x80) == 0)
	return FP_OK;

    /* Range check the exponent.
     */
    if (e < -129)
	return FP_ERR;
    if (e > 126)
	return FP_ERR;

    /* Bias the exponent.
     */ 
    e += 129;

    /* Replace leading '1' with sign.
     */
    m_l &= 0x7f;
    if (s)
	m_l |= 0x80;

    /* Build up output.
     */
    ms[I3] = e & 0xff;
    ms[I2] = m_h & 0xff;
    ms[I1] = (m_l >> 8) & 0xff;
    ms[I0] = m_l & 0xff;

    return FP_OK;
}

/* Convert AM9511 to FP format.
 */
int am_fp(struct fp *f, void *p) {
    unsigned char *am9511 = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Clear destination
     */
    clear(f, sizeof(*f));

    /* If bit 23 is low, must be zero. AM9511 format is
     * normalized, but the leading '1' bit is explicit.
     * The exponent byte has the sign of the number, and
     * 7 bit 2's complement exponent (2^-64..2^63). This
     * is then the most limited 32 bit float format we
     * have.
     */
    if ((am9511[I2] & 0x80) == 0)
	return FP_OK;

    /* Gather up sign, and exponent.
     */
    s = (am9511[I3] & 0x80) != 0;
    e = am9511[I3];
    
    /* Gather mantissa.
     */
    m_h = am9511[I2];
    m_l = am9511[I1];
    m_l = (m_l << 8) | am9511[I0];

    /* Exponent is 7 bit, 2's complement. Convert
     * to int.
     */
    e = e & 0x7f;
    if (e & 0x40) {
	e = (e ^ 0x7f) + 1;
	e = -e;
    }
    e -= 1;

    /* Fill in fp.
     */
    f->sign = s;
    f->exponent = e;
    f->mantissa_h = m_h;
    f->mantissa_l = m_l;

    return FP_OK;
}

int fp_am(void *p, struct fp *f) {
    unsigned char *am9511 = p;
    int16 e;
    uint16 m_l;
    uint8 m_h, s;

    /* Get from fp.
     */
    e = f->exponent;
    m_l = f->mantissa_l;
    m_h = f->mantissa_h;
    s = f->sign;

    /* Clear destination.
     */
    clear(am9511, 4);

    /* If mantissa bit 23 is 0, result is 0.0.
     */
    if ((m_h & 0x80) == 0)
	return FP_OK;

    /* Range check on exponent.
     */
    if (e < -64)
	return FP_ERR;
    if (e > 63)
	return FP_ERR;
    e += 1;

    /* Exponent to 7 bit (assumes 2's complement machine)
     */
    e &= 0x7f;

    /* Merge in sign to exponent.
     */
    if (s)
	e |= 0x80;

    /* Build up output.
     */
    am9511[I3] = e & 0xff;
    am9511[I2] = m_h & 0xff;
    am9511[I1] = (m_l >> 8) & 0xff;
    am9511[I0] = m_l & 0xff;

    return FP_OK;
}

/* Return size of fp.
 */
size_t fp_size(void) {
    return sizeof(struct fp);
}

/* fp field getter. Note that the type of mantissa_l is wrong a lot of the
 * time. int is 32 bit with gcc. The type of exponent is also wrong.
 *
 * Be careful when using fp_get()/fp_put().
 *
 * sign is 0 or 1
 * exponent is -127..128
 * mantissa_h is 128..255 (high bit is '1', mantissa normalized)
 * mantissa_l is 0..65535
 *
 * if (mantissa_h & 0x80) == 0, value is 0.0
 */
void fp_get(struct fp *f, unsigned char *sign,
		          int *exponent,
			  unsigned char *mantissa_h,
			  unsigned int *mantissa_l) {
    *sign = f->sign;
    *exponent = f->exponent;
    *mantissa_h = f->mantissa_h;
    *mantissa_l = f->mantissa_l;
}

/* Put into fp.
 */
void fp_put(struct fp *f, unsigned char sign,
		          int exponent,
			  unsigned char mantissa_h,
			  unsigned int mantissa_l) {
    f->sign = sign;
    f->exponent = exponent;
    f->mantissa_h = mantissa_h;
    f->mantissa_l = mantissa_l;
}
