/* cnvtest.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

#include "floatcnv.h"


/* Dump FP, using fp_get() to take it apart.
 */
void fp_dump(void *fpp) {
    unsigned char sign;
    int exponent;
    unsigned char mantissa_h;
    unsigned int mantissa_l;

    fp_get(fpp, &sign, &exponent, &mantissa_h, &mantissa_l);
    printf(" %c0x%02x%04x*2^%d\n", sign ? '-' : '+',
		                   mantissa_h,
			           mantissa_l,
			           exponent);
}


/* We are record known-good 4 byte sequences in our different
 * floating point formats, to convert and print for validation.
 *
 * note msb comes first in am9511 when reading stack.
 */
unsigned char am9511_pi[] = {     /* am9511 pi */
    0xda, 0x0f, 0xc9, 0x02
};
unsigned char am9511_5[] = {      /* am9511 5.0 */
    0x00, 0x00, 0xa0, 0x03
};
unsigned char am9511_p1[] = {     /* am9511 0.1 */
    0xcd, 0xcc, 0xcc, 0x7d
};
unsigned char am9511_mp0006[] = { /* am9511 -0.0006 */
    0x51, 0x49, 0x9d, 0xf6
};
unsigned char ieee_m5[] = {       /* ieee -5.0 */
    0x00, 0x00, 0xa0, 0xc0 
};
unsigned char ms_m1p5[] = {       /* microsoft -1.5 */
    0x00, 0x00, 0xc0, 0x81
};
unsigned char hi_p1[] = {         /* hitech 0.1 */
    0xcd, 0xcc, 0xcc, 0x3d
};

/* Dump 4 bytes in hex.
 */
void dump4(void *p) {
    unsigned char *u = p;
    int i;
    for (i = 0; i < 4; ++i)
	printf("%02x ", u[i]);
    printf("\n");
}

/* Choose target for floating output.
 */
#ifdef z80
#define fp_target(p) fp_hi(fpp, p)
#else
#define fp_target(p) fp_ie(fpp, p)
#endif

int main(int ac, char **av) {
    float x;
    void *fpp;

    fpp = malloc(fp_size());
    if (fpp == NULL) {
	fprintf(stderr, "cannot allocate temp\n");
	return 1;
    }
    am_fp(am9511_pi, fpp);
    fp_target(&x);
    printf("am9511      pi: %10g\n", x);
    fp_dump(fpp);

    am_fp(am9511_5, fpp);
    fp_target(&x);
    printf("am9511       5: %10g\n", x);
    fp_dump(fpp);

    am_fp(am9511_p1, fpp); /* exp=0x40 */
    fp_target(&x);
    printf("am9511     0.1: %10g\n", x);
    fp_dump(fpp);

    am_fp(am9511_mp0006, fpp); /* exp = 0xc0 */
    fp_target(&x);
    printf("am9511 -0.0006: %10g\n", x);
    fp_dump(fpp);

    ie_fp(ieee_m5, fpp);
    fp_target(&x);
    printf("ieee        -5: %10g\n", x);
    fp_dump(fpp);

    hi_fp(hi_p1, fpp);
    fp_target(&x);
    printf("hi         0.1: %10g\n", x);
    fp_dump(fpp);

    ms_fp(ms_m1p5, fpp);
    fp_target(&x);
    printf("ms        -1.5: %10g\n", x);
    fp_dump(fpp);

    return 0;
}
