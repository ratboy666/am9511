/* ova.c
 *
 * Not an egg -- overflow arithmetic. 16 and 32 bit.
 *
 * Designed to work with both gcc and hi-tech C. This is then meant
 * to allow operation of AM9511 emulation with hi-tech C natively
 * on z80 platform. There are faster ways, but we want to be able
 * to validate the code against a real AM9511A, as long as some of
 * those chips are still operational.
 *
 * Because this code is meant to be (potentially) used with
 * hi-tech C, and other very old environments, external names are
 * unique to 5 characters (6 with prepended '_').
 */

#include <stdio.h>

#include "ova.h"
#include "types.h"


/* Return overflow flag after add16().
 *
 * Overflow is detected by sign bits of the arguments and result.
 * This can be done after the add (or subtract) is done. That is
 * why add and subtract return carry, and leave overflow to a
 * separate function.
 */
int oadd16(unsigned char *pa,
           unsigned char *pb,
	   unsigned char *pc) {
    unsigned char sa, sb, sc;
    int overflow = 0;
    sa = pa[1] & 0x80;
    sb = pb[1] & 0x80;
    sc = pc[1] & 0x80;
    if ((sa == 0x00) && (sb == 0x00)  && (sc == 0x80))
	overflow = 1;
    if ((sa == 0x80) && (sb == 0x80)  && (sc == 0x00))
	overflow = 1;
    return overflow;
}


/* Return overflow flag after add32().
 */
int oadd32(unsigned char *pa,
           unsigned char *pb,
	   unsigned char *pc) {
    unsigned char sa, sb, sc;
    int overflow = 0;
    sa = pa[3] & 0x80;
    sb = pb[3] & 0x80;
    sc = pc[3] & 0x80;
    if ((sa == 0x00) && (sb == 0x00)  && (sc == 0x80))
	overflow = 1;
    if ((sa == 0x80) && (sb == 0x80)  && (sc == 0x00))
	overflow = 1;
    return overflow;
}


/* Return overflow flag after sub16().
 */
int osub16(unsigned char *pa,
           unsigned char *pb,
	   unsigned char *pc) {
    unsigned char sa, sb, sc;
    int overflow = 0;
    sa = pa[1] & 0x80;
    sb = pb[1] & 0x80;
    sc = pc[1] & 0x80;
    if ((sa == 0x00) && (sb == 0x80)  && (sc == 0x80))
	overflow = 1;
    if ((sa == 0x80) && (sb == 0x00)  && (sc == 0x00))
	overflow = 1;
    return overflow;
}


/* Return overflow after sub32().
 */
int osub32(unsigned char *pa,
           unsigned char *pb,
           unsigned char *pc) {
    unsigned char sa, sb, sc;
    int overflow = 0;
    sa = pa[3] & 0x80;
    sb = pb[3] & 0x80;
    sc = pc[3] & 0x80;
    if ((sa == 0x00) && (sb == 0x80) && (sc == 0x80))
        overflow = 1;
    if ((sa == 0x80) && (sb == 0x00) && (sc == 0x00))
        overflow = 1;
    return overflow;
}


/* 16 bit add, returns carry.
 *
 * All parameters are little endian.
 */
int add16(unsigned char *pa,
          unsigned char *pb,
	  unsigned char *pc) {
    uint16 a, b, c;
    int carry;

    a = pa[0] | (pa[1] << 8);
    b = pb[0] | (pb[1] << 8); 
    c = a + b;
    carry = (c < a) || (c < b);
    pc[0] = c;
    pc[1] = c >> 8;
    return carry;
}


/* 32 bit add, returns carry. Uses add16().
 */
int add32(unsigned char *pa,
	  unsigned char *pb,
	  unsigned char *pc) {
    unsigned char *pah, *pbh, *pch;
    static unsigned char one[] = { 0x01, 0x00 };
    int carry, c2;

    pah = pa + 2;
    pbh = pb + 2;
    pch = pc + 2;

    carry = add16(pa, pb, pc);
    if (!carry) {
        carry = add16(pah, pbh, pch);
	return carry;
    }

    carry = add16(one, pbh, pch);
    c2 = add16(pch, pah, pch);
    return carry || c2;
}


/* 64 bit add, returns carry. Uses add32().
 *
 * This supports 32x32->64 bit multiply.
 */
static int add64(unsigned char *pa,
	         unsigned char *pb,
	         unsigned char *pc) {
    unsigned char *pah, *pbh, *pch;
    static unsigned char one[] = { 0x01, 0x00, 0x00, 0x00 };
    int carry, c2;

    pah = pa + 4;
    pbh = pb + 4;
    pch = pc + 4;

    carry = add32(pa, pb, pc);
    if (!carry) {
        carry = add32(pah, pbh, pch);
	return carry;
    }

    carry = add32(one, pbh, pch);
    c2 = add32(pch, pah, pch);
    return carry || c2;
}


/* 16 bit 2's complement, return 1 if 0x8000
 */
int cm16(unsigned char *pa,
	 unsigned char *pb) {
    uint16 a, b;
    int r = 0;

    a = pa[0] | (pa[1] << 8);
    if (a == 0x8000) {
        b = a;
        r = 1;
    } else {
        b = ~a;
        ++b;
    }
    pb[0] = b;
    pb[1] = b >> 8;
    return r;
}


/* 32 bit 2's complement, return 1 if 0x80000000
 */
int cm32(unsigned char *pa,
	 unsigned char *pb) {
    uint16 a, ah, b, bh;
    int r = 0;

    a = pa[0] | (pa[1] << 8);
    ah = pa[2] | (pa[3] << 8);
    if ((a == 0x0000) && (ah == 0x8000)) {
        b = a;
        bh = ah;
	r = 1;
    } else {
        b = ~a;
	bh = ~ah;
	if (++b == 0)
	    ++bh;
    }
    pb[0] = b;
    pb[1] = b >> 8;
    pb[2] = bh;
    pb[3] = bh >> 8;
    return r;
}


/* 16 bit subtract. Return 1 if carry.
 */
int sub16(unsigned char *pa,
          unsigned char *pb,
	  unsigned char *pc) {
    uint16 a, b, c;
    int carry;

    a = pa[0] | (pa[1] << 8);
    b = pb[0] | (pb[1] << 8); 
    c = a - b;
    if (a == 0x8000)
	carry = 1;
    else
        carry = a < b;
    pc[0] = c;
    pc[1] = c >> 8;
    return carry;
}


/* 32 subtract. Return carry.
 */
int sub32(unsigned char *pa,
	  unsigned char *pb,
	  unsigned char *pc) {
    unsigned char *pah, *pbh, *pch;
    static unsigned char one[] = { 0x01, 0x00 };
    int carry, c2;

    pah = pa + 2;
    pbh = pb + 2;
    pch = pc + 2;

    carry = sub16(pa, pb, pc);
    if (!carry) {
        carry = sub16(pah, pbh, pch);
	return carry;
    }

    carry = sub16(pah, one, pch);
    c2 = sub16(pch, pbh, pch);
    return carry || c2;
}


/* 16x16 giving 32 bit multiplication
 *
 * We break it down into 8x8 giving 16 bit
 *
 *                                   [mHigh mLow]
 *                                 * [nHigh nLow]
 *                                   ------------
 *                   [mHigh * nLow] [mLow * nLow]
 * + [mHigh * nHigh] [mLow * nHigh]
 *   --------------------------------------------
 */

/* Multiply 8x8->16 bit result. Hopefully, we have hardware
 * which does this. The Z80 does not. But most of its successors
 * do, and most gcc platforms do.
 */
static uint16 mul8(unsigned char m, unsigned char n) {
    return m * n;
}

/* Multiply 16x16->32
 */
static void mul16(unsigned char *m, unsigned char *n, unsigned char *r) {
#if 0
    /* If we have 16x16->32 multiply, use it.
     */
    uint32 a, b, c;

    a = m[0] + (m[1] << 8);
    b = n[0] + (n[1] << 8);
    c = a * b;
    r[0] = c & 0xff;
    r[1] = (c << 8) & 0xff;
    r[2] = (c << 16) & 0xff;
    r[3] = (c << 24) & 0xff;
#else
    /* Build 16x16->32 from 8x8->16 bit multiply
     */
    unsigned char mLow = m[0];
    unsigned char mHigh = m[1];

    unsigned char nLow = n[0];
    unsigned char nHigh = n[1];

    uint16 mLow_nLow = mul8(mLow, nLow);
    uint16 mHigh_nLow = mul8(mHigh, nLow);
    uint16 mLow_nHigh = mul8(mLow, nHigh);
    uint16 mHigh_nHigh = mul8(mHigh, nHigh);

    uint16 a;
    int carry;
    unsigned char r2[4], r3[4];
    int i;

    for (i = 0; i < 4; ++i)
	r[i] = r2[i] = r3[i] = 0;

    /* 3
     * 2
     * 1 | mLow_nLow
     * 0 |
     */
    r[0] = mLow_nLow & 0xff;
    r[1] = (mLow_nLow >> 8) & 0xff;

    /* 3
     * 2 | mHigh_nLow + mLow_nHigh
     * 1 |
     * 0
     */
    carry = add16((unsigned char *)&mHigh_nLow,
		  (unsigned char *)&mLow_nHigh,
		  r2 + 1);

    /* 3 | mHigh_nHigh (+ carry)
     * 2 |
     * 1
     * 0
     */
    a = mHigh_nHigh + carry;
    r3[2] = a & 0xff;
    r3[3] = (a >> 8) & 0xff;

    /* r = r2 + r3
     */
    add32(r, r2, r);
    add32(r, r3, r);
#endif
}


/* 16 bit multiply, lower. Uses mul16(). Returns overflow.
 * (1 if high 16 is non-zero).
 */
int mull16(unsigned char *pa,
	   unsigned char *pb,
	   unsigned char *pc) {
    unsigned char r[4];

    if (((pa[0] == 0x00) && (pa[1] == 0x80)) ||
	((pb[0] == 0x00) && (pb[1] == 0x80))) {
        pc[0] = 0x00;
        pc[1] = 0x80;
        return 1;
    }

    mul16(pa, pb, r);

    pc[0] = r[0];
    pc[1] = r[1];

    return (r[2] | r[3]) != 0;
}


/* 16 bit multiply, upper. Uses mul16(). returns overflow.
 */
int mulu16(unsigned char *pa,
	   unsigned char *pb,
	   unsigned char *pc) {
    unsigned char r[4];

    if (((pa[0] == 0x00) && (pa[1] == 0x80)) ||
	((pb[0] == 0x00) && (pb[1] == 0x80))) {
        pc[0] = 0x00;
        pc[1] = 0x80;
        return 1;
    }

    mul16(pa, pb, r);

    pc[0] = r[2];
    pc[1] = r[3];

    return 0;
}


/* 16 bit division.
 *
 * Eventually, I will put my own code in here... but, the objective
 * is to have the emulator running.
 */
int div16(unsigned char *pa,
          unsigned char *pb,
          unsigned char *pc) {
    int16 a, b, c;

    a = pa[0] |
        (pa[1] << 8);
    b = pb[0] |
        (pb[1] << 8);
    if (b == 0)
        return 1;
    c = a / b;
    pc[0] = c & 0xff;
    pc[1] = (c >> 8) & 0xff;
    return 0;
}


/* 32 bit division.
 *
 * Eventually, I will put my own code in here... but, the objective
 * is to have the emulator running.
 */
int div32(unsigned char *pa,
          unsigned char *pb,
          unsigned char *pc) {
    int32 a, b, c;
    a = pa[0] |
	(pa[1] << 8) |
	(pa[2] << 16) |
	(pa[3] << 24);
    b = pb[0] |
	(pb[1] << 8) |
	(pb[2] << 16) |
	(pb[3] << 24);
    if (b == 0)
	return 1;
    c = a / b;
    pc[0] = c & 0xff;
    pc[1] = (c >> 8) & 0xff;
    pc[2] = (c >> 16) & 0xff;
    pc[3] = (c >> 24) & 0xff;
    return 0;
}


/* 32x32->64 multiply
 */
static void mul32(unsigned char *pa,
                  unsigned char *pb,
                  unsigned char *pc) {
    unsigned char *pah, *pal;
    unsigned char *pbh, *pbl;

    unsigned char r0[8];
    unsigned char r1[8];
    unsigned char r2[8];
    unsigned char r3[8];

    int i;

    for (i = 0; i < 8; ++i)
	pc[i] = r0[i] = r1[i] = r2[i] = r3[i] = 0;

    pal = pa;
    pah = pa + 2;
    pbl = pb;
    pbh = pb + 2;

    mul16(pbl, pal, r0);
    mul16(pbl, pah, r1 + 2);
    mul16(pbh, pal, r2 + 2);
    mul16(pbh, pah, r3 + 4);
    add64(r0, r1, pc);
    add64(pc, r2, r0);
    add64(r0, r3, pc);
}


/* mull32 mulu32 needed. FIXME
 */

int mull32(unsigned char *pa,
	   unsigned char *pb,
	   unsigned char *pc) {
    mul32(0, 0, 0);
    printf("mull32 not implemented\n");
    return 0;
}


int mulu32(unsigned char *pa,
	   unsigned char *pb,
	   unsigned char *pc) {
    printf("mulu32 not implemented\n");
    return 0;
}


/*
          16B,16B (32-bit multiplicand)
        * 16B,16B (32-bit multiplier)
-----------------
          16B,16B (32-bit partial product)
+     16B,16B     (32-bit partial product)
+     16B,16B     (32-bit partial product)
+ 16B,16B         (32-bit partial product)
=================
  16B,16B,16B,16B (64-bit product)

*/

