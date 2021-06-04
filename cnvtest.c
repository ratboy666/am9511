/* cnvtest.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

#include "floatcnv.h"


/* Dump FP.
 */
void fp_dump(struct fp *f) {
    unsigned char sign;
    int exponent;
    unsigned char mantissa_h;
    unsigned int mantissa_l;

    fp_get(f, &sign, &exponent, &mantissa_h, &mantissa_l);
    printf(" %c0x%02x%04x*2^%d ", sign ? '-' : '+',
		                  mantissa_h,
			          mantissa_l,
			          exponent);
    printf("\n");
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

void dump4(void *p) {
    unsigned char *u = p;
    int i;
    for (i = 0; i < 4; ++i)
	printf("%02x ", u[i]);
    printf("\n");
}

/* Choose target for floating output
 */
#ifdef z80
#define fp_target(p1, p2) fp_hi(p1, p2)
#endif
#ifdef __GNUC__
#define fp_target(p1, p2) fp_ie(p1, p2)
#endif
#ifdef __TINYC__
#define fp_target(p1, p2) fp_ie(p1, p2)
#endif

int main(int ac, char **av) {
    float x;
    struct fp *f;

    f = malloc(fp_size());

    am_fp(f, am9511_pi);
    fp_target(&x, f);
    printf("am9511      pi: %10g\n", x);
    fp_dump(f);

    am_fp(f, am9511_5);
    fp_target(&x, f);
    printf("am9511       5: %10g\n", x);
    fp_dump(f);

    am_fp(f, am9511_p1); // exp=0x40
    fp_target(&x, f);
    printf("am9511     0.1: %10g\n", x);
    fp_dump(f);

    am_fp(f, am9511_mp0006); // exp = 0xc0
    fp_target(&x, f);
    printf("am9511 -0.0006: %10g\n", x);
    fp_dump(f);

    ie_fp(f, ieee_m5);
    fp_target(&x, f);
    printf("ieee        -5: %10g\n", x);
    fp_dump(f);

    hi_fp(f, hi_p1);
    fp_target(&x, f);
    printf("hi         0.1: %10g\n", x);
    fp_dump(f);

    ms_fp(f, ms_m1p5);
    fp_target(&x, f);
    printf("ms        -1.5: %10g\n", x);
    fp_dump(f);

    return 0;
}
