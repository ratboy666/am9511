/* test.c
 *
 * Test am9511 chip and emulator.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef z80
#include <sys.h>
#endif

#include "getopt.h"
#include "am9511.h"
#include "floatcnv.h"
#include "types.h"


/* Define fp_na() -- fp to native and
 *        na_fp() -- native to fp
 */
#ifdef z80
#define fp_na(x,y) fp_hi(x,y)
#define na_fp(x,y) hi_fp(x,y)
#else
#define fp_na(x,y) fp_ie(x,y)
#define na_fp(x,y) ie_fp(x,y)
#endif


#define NOTHING


/* Poll am9511 and wait for not busy
 */
unsigned char am_wait(void *am9511) {
    int s;

    while ((s = am_status(am9511)) & AM_BUSY)
	NOTHING;
    return s;
}

/* am9511 test sequence
 *
 * am_test() also serves to show how to use the AM9511 device.
 *
 * I considered putting in a command interpreter, to feed sequences
 * from a file to the device (or emulator). But... I am going to
 * get the basic functions operational, and then write the "advanced"
 * script based test harness in MBASIC instead.
 */

void *fptmp;

#ifdef TEST1

/* TEST1 is NOP, data register push/pop, PUPI, CHSS, CHSD
 *
 * Note: we split test functions when they get too complex for the
 * optimizer (running under zxcc)
 */

void am_test1(void *am9511) {
    int s;
    int16 n;
    int32 nl;
    unsigned char v[4];
    float x;
 
    printf("am_test1\n");

    /* Basic test - execute a NOP
     */
    am_wait(am9511);
    am_command(am9511, AM_NOP);
    s = am_wait(am9511);
    printf("NOP: am9511 status = %d\n", s);

    /* Push/pop
     *
     * Push low to high, pop high to low.
     */
    am_push(am9511, 1);
    am_push(am9511, 2);
    am_push(am9511, 3);
    am_push(am9511, 4);


    if ((n = am_pop(am9511)) != 4) printf("push/pop error %d (4)\n", n);
    if ((n = am_pop(am9511)) != 3) printf("push/pop error %d (3)\n", n);
    if ((n = am_pop(am9511)) != 2) printf("push/pop error %d (2)\n", n);
    if ((n = am_pop(am9511)) != 1) printf("push/pop error %d (1)\n", n);

    /* Execute PUPI
     *
     * Pushes value of PI. Pop 4 bytes, convert from
     * am9511 to native float, and display value.
     */
    am_command(am9511, AM_PUPI);
    s = am_wait(am9511);

    /* Demonstrate am_dump()
     */
    am_dump(am9511, AM_FLOAT); /* AM_SINGLE, AM_DOUBLE, AM_FLOAT */

    printf("PUPI: am9511 status = %d\n", s);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);

    /* Convert to native floating point
     */
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("PUPI: %g (should be 3.141592)\n", x);

    /* Execute CHSS.
     *
     * For 16 bit tests, we use int16. This is important for GCC
     * (not for z80 -- int is int16 on that platform). We just need
     * to be careful to use int16 and int32 as appropriate.
     */
    n = 2;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_CHS | AM_SINGLE);
    s = am_wait(am9511);
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("   result -> %d\n", n);

    n = 0;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_CHS | AM_SINGLE);
    s = am_wait(am9511);
    printf("CHSS %d status = %d (%d)\n", n, s, AM_ZERO);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("   result -> %d\n", n);

    n = -30;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_CHS | AM_SINGLE);
    s = am_wait(am9511);
    printf("CHSS %d status = %d (%d)\n", n, s, 0);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("   result -> %d\n", n);

    n = 0x7fff;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_CHS | AM_SINGLE);
    s = am_wait(am9511);
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("   result -> %d\n", n);

    n = 0x8000;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_CHS | AM_SINGLE);
    s = am_wait(am9511);
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN | AM_ERR_OVF);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("   result -> %d\n", n);

    /* Execute CHSD
     *
     * For CHSD tests, we cast to long. This is done, because we need
     * to use %ld format on z80, but int32 is not long on GCC. So, we
     * can either vary the format string, -or- cast the argument. On
     * the z80, this is the same and gives the correct result. On GCC,
     * this makes the argument match the format, and again produces
     * the desired result.
     */

    nl = 2;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_CHS | AM_DOUBLE);
    s = am_wait(am9511);
    printf("CHSD %ld status = %d (%d)\n", (long)nl, s, AM_SIGN);
    nl = am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    printf("   result -> %ld\n", (long)nl);
}

void am_test1a(void *am9511) {
    int s;
    int32 nl;

    nl = 0;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_CHS |  AM_DOUBLE);
    s = am_wait(am9511);
    printf("CHSD %ld status = %d (%d)\n", (long)nl, s, AM_ZERO);
    nl = am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    printf("   result -> %ld\n", (long)nl);

    nl = -30;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_CHS | AM_DOUBLE);
    s = am_wait(am9511);
    printf("CHSD %ld status = %d (%d)\n", (long)nl, s, 0);
    nl = am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    printf("   result -> %ld\n", (long)nl);
}

#endif

#ifdef TEST2

/* TEST2: CHSD CHSF
 */
void am_test2(void *am9511) {
    int s;
    int32 nl;
    unsigned char v[4];
    float x;
 
    printf("am_test2\n");

    am_wait(am9511);

    nl = 0x7fffffff;
    am_push(am9511, 0xff);
    am_push(am9511, 0xff);
    am_push(am9511, 0xff);
    am_push(am9511, 0x7f);
    am_command(am9511, AM_CHS | AM_DOUBLE);
    s = am_wait(am9511);
    printf("CHSD %ld status = %d (%d)\n", (long)nl, s, AM_SIGN);
    nl = am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    printf("   result -> %ld\n", (long)nl);

    nl = 0x80000000;
    am_push(am9511, 0x00);
    am_push(am9511, 0x00);
    am_push(am9511, 0x00);
    am_push(am9511, 0x80);
    am_command(am9511, AM_CHS | AM_DOUBLE);
    s = am_wait(am9511);
    printf("CHSD %ld status = %d (%d)\n", (long)nl, s, AM_SIGN | AM_ERR_OVF);
    nl = am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    nl = (nl << 8) | am_pop(am9511);
    printf("   result -> %ld\n", (long)nl);

    /* Execute CHSF
     */
    x = 3.2;
    na_fp(&x, fptmp);
    fp_am(fptmp, v);
    am_push(am9511, v[0]);
    am_push(am9511, v[1]);
    am_push(am9511, v[2]);
    am_push(am9511, v[3]);
    am_command(am9511, AM_CHSF);
    s = am_wait(am9511);
    printf("CHSF %g status = %d (%d)\n", x, s, AM_SIGN);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("   result -> %g\n", x);

    x = 0.0;
    na_fp(&x, fptmp);
    fp_am(fptmp, v);
    am_push(am9511, v[0]);
    am_push(am9511, v[1]);
    am_push(am9511, v[2]);
    am_push(am9511, v[3]);
    am_command(am9511, AM_CHSF);
    s = am_wait(am9511);
    printf("CHSF %g status = %d (%d)\n", x, s, AM_ZERO);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("   result -> %g\n", x);
}

#endif

#ifdef TEST3

/* TEST3: PTO, POP, XC, FIXS, FIXD
 */
void am_test3(void *am9511) {

    printf("am_test3\n");

    am_wait(am9511);

    /* Execute PTO
     */

    am_push(am9511, 1);
    am_push(am9511, 2);
    am_command(am9511, AM_PTO | AM_SINGLE);
    am_wait(am9511);
    am_command(am9511, AM_PTO | AM_DOUBLE);
    am_wait(am9511);
    am_command(am9511, AM_PTO | AM_FLOAT);
    am_wait(am9511);

    /* am_dump(am9511, AM_DOUBLE); */

    /* Execute POP
     */

    am_command(am9511, AM_POP | AM_FLOAT);
    am_wait(am9511);
    /* am_dump(am9511, AM_DOUBLE); */

    /* Execute POP and XCH
     */

    am_command(am9511, AM_POP | AM_DOUBLE);
    am_wait(am9511);
    /* am_dump(am9511, AM_DOUBLE); */
    am_command(am9511, AM_XCH | AM_DOUBLE);
    am_wait(am9511);
    /* am_dump(am9511, AM_DOUBLE); */

    /* FIXS and FIXD
     */

    am_command(am9511, AM_PUPI);
    am_wait(am9511);
    am_command(am9511, AM_PTO | AM_FLOAT);
    am_wait(am9511);
    am_command(am9511, AM_FIXS);
    am_wait(am9511);
    /* am_dump(am9511, AM_SINGLE); */
    printf("PUPI/FIXS ->\n");
    printf("   %d\n", am_pop(am9511));
    printf("   %d\n", am_pop(am9511));

    am_command(am9511, AM_FIXD);
    am_wait(am9511);
    /* am_dump(am9511, AM_DOUBLE); */
    printf("PUPI/FIXD ->\n");
    printf("   %d\n", am_pop(am9511));
    printf("   %d\n", am_pop(am9511));
    printf("   %d\n", am_pop(am9511));
    printf("   %d\n", am_pop(am9511));

    /* Execute FLTD
     */

    am_command(am9511, AM_FLTD);
    am_wait(am9511);
    /* am_dump(am9511, AM_FLOAT); */

    /* Execute FLTS
     */

    am_push(am9511, 1000 & 0xff);
    am_push(am9511, 1000 >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    am_command(am9511, AM_FIXS);
    /* am_dump(am9511, AM_SINGLE); */
    printf("1000/FLTS/FIXS ->\n");
    printf("   %d\n", am_pop(am9511));
    printf("   %d\n", am_pop(am9511));
}

#endif


#ifdef TEST4

/* Basic arithmetic tests
 */

void am_test4(void *am9511) {
    int n, s, b;
    int32 nl;
    float x;
    unsigned char v[4];

    printf("am_test4\n");

    am_wait(am9511);

    /* Add: SADD DADD FADD
     */

    /* SADD */
    n = 1;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 2;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_ADD | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SADD: 1 + 2 = %d status = %d\n", n, s);

    /* DADD */
    nl = 1;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 2;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_ADD | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DADD: 1 + 2 = %ld status = %d\n", (long)nl, s);

    /* FADD */
    n = 1;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    n = 2;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    am_command(am9511, AM_FADD);
    s = am_wait(am9511);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("FADD: 1.0 + 2.0 = %g status = %d\n", x, s);

}

#endif


#ifdef TEST5

void am_test5(void *am9511) {
    int s, b;
    int16 n;
    int32 nl;
    float x;
    unsigned char v[4];

    printf("am_test5\n");

    am_wait(am9511);

    /* Add: SSUB DSUB FSUB
     */

    /* SSUB */
    n = 1;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 2;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_SUB | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SSUB: 1 - 2 = %d status = %d\n", n, s);

    /* DSUB */
    nl = 1;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 2;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_SUB | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DSUB: 1 - 2 = %ld status = %d\n", (long)nl, s);

    /* FSUB */
    n = 1;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    n = 2;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    am_command(am9511, AM_FSUB);
    am_wait(am9511);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("FSUB: 1.0 - 2.0 = %g status = %d\n", x, s);

    /* DIV: SDIV DDIV FDIV
     */

    /* SDIV */
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_DIV | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SDIV: 10 / 3 = %d status = %d\n", n, s);

    /* DSUB */
    nl = 10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_DIV | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DDIV: 10 /  3 = %ld status = %d\n", (long)nl, s);

    /* FDIV */
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    n = 3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    am_command(am9511, AM_FDIV);
    s = am_wait(am9511);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("FDIV: 10.0 / 3.0 = %g status = %d\n", x, s);
}

#endif


#ifdef TEST6

/* multiply, float, single lower/upper, double lower/upper
 */

void am_test6(void *am9511) {
    int s, b;
    int16 n;
    int32 nl;
    float x;
    unsigned char v[4];

    printf("am_test6\n");

    am_wait(am9511);

    /* MUL: SMUL DMUL FMUL
     */

    /* SMUL */
    n = 3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUL | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUL: 10 * 3 = %d status = %d\n", n, s);

    /* DMUL*/
    nl = 10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUL | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUL: 10 * 3 = %ld status = %d\n", (long)nl, s);

    /* FMUL */
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    n = 3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_FLTS);
    am_wait(am9511);
    am_command(am9511, AM_FMUL);
    am_wait(am9511);
    v[3] = am_pop(am9511);
    v[2] = am_pop(am9511);
    v[1] = am_pop(am9511);
    v[0] = am_pop(am9511);
    am_fp(v, fptmp);
    fp_na(fptmp, &x);
    printf("FMUL: 10.0 * 3.0 = %g status = %d\n", x, s);

    /* MUU */

    /* SMUU */
    printf("0x1000 * 0x40 = 0x0004 0000\n");
    n = 0x1000;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 0x40;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    /* 0x1000 * 0x40 = 0x40 0000 */
    am_command(am9511, AM_MUL | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUL: 0x1000 * 0x40 = %d (0) status = %d (34)\n", n, s);

    n = 0x1000;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 0x40;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    /* 0x1000 * 0x40 = 0x40 0000 */
    am_command(am9511, AM_MUU | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUU: 0x1000 * 0x40 = %d (4) status = %d (0)\n", n, s);
}

#endif

#ifdef TEST7

/* Signed multiply SMUL SMUU DMUL DMUD
 */
void am_test7(void *am9511) {
    int s, b;
    int16 n;
    int32 nl;

    printf("am_test7\n");

    am_wait(am9511);

    /* SMUL 10 * 3 result in am_test6()
     */
    n = 3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUU | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUU: 10 * 3 = %d (0) status = %d (32)\n", n, s);

    n = -3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = -10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUL | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUL: -10 * -3 = %d (30) status = %d (0)\n", n, s);
 
    n = -3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = -10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUU | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUU: -10 * -3 = %d (0) status = %d (32)\n", n, s);

    n = -3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUL | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUL: 10 * -3 = %d (-30) status = %d (64)\n", n, s);

    n = -3;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    n = 10;
    am_push(am9511, n);
    am_push(am9511, n >> 8);
    am_command(am9511, AM_MUU | AM_SINGLE);
    s = am_wait(am9511);
    n = am_pop(am9511);
    n = (n << 8) | am_pop(am9511);
    printf("SMUU: 10 * -3 = %d (-1) status = %d (64)\n", n, s);

    /* Test DMUL / DMUU */

    nl = 3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUU | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUU: 10 * 3 = %ld (0) status = %d (32)\n", (long)nl, s);

    nl = -3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = -10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUL | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUL: -10 * -3 = %ld (30) status = %d (32)\n", (long)nl, s);
 
}

#endif

#ifdef TEST8

void am_test8(void *am9511) {
    printf("am_test8\n");

    am_wait(am9511);
}

#endif

#ifdef TEST9

void am_test9(void *am9511) {
    int s, b;
    int32 nl;

    printf("am_test9\n");

    am_wait(am9511);

    nl = -3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = -10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUU | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUU: -10 * -3 = %ld (0) status = %d (32)\n", (long)nl, s);
 
    nl = -3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUL | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUL: 10 * -3 = %ld (30) status = %d (32)\n", (long)nl, s);

    nl = -3;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    nl = 10;
    am_push(am9511, nl);
    am_push(am9511, nl >> 8);
    am_push(am9511, nl >> 16);
    am_push(am9511, nl >> 24);
    am_command(am9511, AM_MUU | AM_DOUBLE);
    s = am_wait(am9511);
    nl = am_pop(am9511);
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    b = am_pop(am9511);
    nl = nl << 8;
    nl = nl | b;
    printf("DMUU: 10 * -3 = %ld (-1) status = %d (64)\n", (long)nl, s);
}

#endif

#ifdef TEST10

void am_test10(void *am9511) {
    printf("am_test10\n");

    am_wait(am9511);
}

#endif

#ifdef TEST11

void am_test11(void *am9511) {
    printf("am_test11\n");

    am_wait(am9511);
}

#endif

#ifdef TEST12

void am_test12(void *am9511) {
    printf("am_test12\n");

    am_wait(am9511);
}

#endif

void am_test(void *am9511) {
#ifdef TEST1
    am_test1(am9511);
    am_test1a(am9511);
#endif
#ifdef TEST2
    am_test2(am9511);
#endif
#ifdef TEST3
    am_test3(am9511);
#endif
#ifdef TEST4
    am_test4(am9511);
#endif
#ifdef TEST5
    am_test5(am9511);
#endif
#ifdef TEST6
    am_test6(am9511);
#endif
#ifdef TEST7
    am_test7(am9511);
#endif
#ifdef TEST8
    am_test8(am9511);
#endif
#ifdef TEST9
    am_test9(am9511);
#endif
#ifdef TEST10
    am_test10(am9511);
#endif
#ifdef TEST11
    am_test11(am9511);
#endif
#ifdef TEST12
    am_test12(am9511);
#endif
}


/* Give usage for am9511 test
 */
void usage(char *p) {
    printf("usage: %s [-d port] [-s port]\n", p);
    printf("    -d port    set data port\n");
    printf("    -s port    set status port\n");
    printf("\n");
    printf("port numbers are specified in decimal\n");
    exit(1);
}


int main(int ac, char **av) {
    int ch, s, d;
    void *am9511;

#ifdef z80
    /* Expand arguments for HI-TECH C.
     */
    av = _getargs((char *)0x81, "am9511");
    ac = _argc_;
#endif

    printf("test am9511: %s\n", av[0]);

    s = -1;
    d = -1;
    while ((ch = getopt(ac, av, "s:d:")) != EOF)
	switch (ch) {
	case 's':
	    s = atoi(optarg);
	    break;
	case 'd':
	    d = atoi(optarg);
	    break;
	case '?':
	default:
	    usage(av[0]);
	}
    ac -= optind;
    av += optind;

    fptmp = malloc(fp_size());
    if (fptmp == NULL) {
	printf("cannot malloc fptmp\n");
	return 1;
    }

    /* Create AM9511
     */
    am9511 = am_create(s, d);
    if (am9511 == NULL) {
	fprintf(stderr, "Cannot create\n");
	return 1;
    }

    /* Reset AM9511. If using actual hardware, passes in status and
     * data ports that will be used.
     */
    am_reset(am9511);

    am_test(am9511);

    return 0;
}
