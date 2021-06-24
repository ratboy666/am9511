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
#define fp_na(x) fp_hi(x)
#define na_fp(x) hi_fp(x)
#else
#define fp_na(x) fp_ie(x)
#define na_fp(x) ie_fp(x)
#endif


#define NOTHING


/* Poll am9511 and wait for not busy
 */
unsigned char am_wait(void) {
    int s;

    while ((s = am_status()) & AM_BUSY)
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
 *
 * Define the test(s) desired
 */

#define TEST1
#define TEST2
#define TEST3

#ifdef TEST1

/* TEST1 is NOP, data register push/pop, PUPI, CHSS, CHSD
 *
 * Note: we split test functions when they get too complex for the
 * optimizer (running under zxcc)
 */

void am_test1(void) {
    int s, n;
    int32 nl;
    unsigned char v[4];
    float x;
 
    printf("am_test1\n");

    /* Basic test - execute a NOP
     */
    am_wait();
    am_command(AM_NOP);
    s = am_wait();
    printf("NOP: am9511 status = %d\n", s);

    /* Push/pop
     *
     * Push low to high, pop high to low.
     */
    am_push(1);
    am_push(2);
    am_push(3);
    am_push(4);
    if ((n = am_pop()) != 4) printf("push/pop error %d (4)\n", n);
    if ((n = am_pop()) != 3) printf("push/pop error %d (3)\n", n);
    if ((n = am_pop()) != 2) printf("push/pop error %d (2)\n", n);
    if ((n = am_pop()) != 1) printf("push/pop error %d (1)\n", n);

    /* Execute PUPI
     *
     * Pushes value of PI. Pop 4 bytes, convert from
     * am9511 to native float, and display value.
     */
    am_command(AM_PUPI);
    s = am_wait();
    printf("PUPI: am9511 status = %d\n", s);
    v[3] = am_pop();
    v[2] = am_pop();
    v[1] = am_pop();
    v[0] = am_pop();

    /* Convert to native floating point
     */
    am_fp(v);
    fp_na(&x);
    printf("PUPI: %g (should be 3.141592)\n", x);

    /* Execute CHSS
     */
    n = 2;
    am_push(n);
    am_push(n >> 8);
    am_command(AM_CHS | AM_FIXED | AM_SINGLE);
    s = am_wait();
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN);
    n = am_pop();
    n = (n << 8) | am_pop();
    printf("   result -> %d\n", n);

    n = 0;
    am_push(n);
    am_push(n >> 8);
    am_command(AM_CHS | AM_FIXED | AM_SINGLE);
    s = am_wait();
    printf("CHSS %d status = %d (%d)\n", n, s, AM_ZERO);
    n = am_pop();
    n = (n << 8) | am_pop();
    printf("   result -> %d\n", n);

    n = -30;
    am_push(n);
    am_push(n >> 8);
    am_command(AM_CHS | AM_FIXED | AM_SINGLE);
    s = am_wait();
    printf("CHSS %d status = %d (%d)\n", n, s, 0);
    n = am_pop();
    n = (n << 8) | am_pop();
    printf("   result -> %d\n", n);

    n = 0x7fff;
    am_push(n);
    am_push(n >> 8);
    am_command(AM_CHS | AM_FIXED | AM_SINGLE);
    s = am_wait();
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN);
    n = am_pop();
    n = (n << 8) | am_pop();
    printf("   result -> %d\n", n);

    n = 0x8000;
    am_push(n);
    am_push(n >> 8);
    am_command(AM_CHS | AM_FIXED | AM_SINGLE);
    s = am_wait();
    printf("CHSS %d status = %d (%d)\n", n, s, AM_SIGN | AM_ERR_OVF);
    n = am_pop();
    n = (n << 8) | am_pop();
    printf("   result -> %d\n", n);

    /* Execute CHSD
     */

    nl = 2;
    am_push(nl);
    am_push(nl >> 8);
    am_push(nl >> 16);
    am_push(nl >> 24);
    am_command(AM_CHS | AM_FIXED | AM_DOUBLE);
    s = am_wait();
    printf("CHSD %ld status = %d (%d)\n", nl, s, AM_SIGN);
    nl = am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    printf("   result -> %ld\n", nl);

    nl = 0;
    am_push(nl);
    am_push(nl >> 8);
    am_push(nl >> 16);
    am_push(nl >> 24);
    am_command(AM_CHS | AM_FIXED | AM_DOUBLE);
    s = am_wait();
    printf("CHSD %ld status = %d (%d)\n", nl, s, AM_ZERO);
    nl = am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    printf("   result -> %ld\n", nl);

    nl = -30;
    am_push(nl);
    am_push(nl >> 8);
    am_push(nl >> 16);
    am_push(nl >> 24);
    am_command(AM_CHS | AM_FIXED | AM_DOUBLE);
    s = am_wait();
    printf("CHSD %ld status = %d (%d)\n", nl, s, 0);
    nl = am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    printf("   result -> %ld\n", nl);
}
#endif

#ifdef TEST2

/* TEST2: CHSD CHSF
 */
void am_test2(void) {
    int s;
    int32 nl;
    unsigned char v[4];
    float x;
 
    printf("am_test2\n");

    am_wait();

    nl = 0x7fffffff;
    am_push(0xff);
    am_push(0xff);
    am_push(0xff);
    am_push(0x7f);
    am_command(AM_CHS | AM_FIXED | AM_DOUBLE);
    s = am_wait();
    printf("CHSD %ld status = %d (%d)\n", nl, s, AM_SIGN);
    nl = am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    printf("   result -> %ld\n", nl);

    nl = 0x80000000;
    am_push(0x00);
    am_push(0x00);
    am_push(0x00);
    am_push(0x80);
    am_command(AM_CHS | AM_FIXED | AM_DOUBLE);
    s = am_wait();
    printf("CHSD %ld status = %d (%d)\n", nl, s, AM_SIGN | AM_ERR_OVF);
    nl = am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    nl = (nl << 8) | am_pop();
    printf("   result -> %ld\n", nl);

    /* Execute CHSF
     */
    x = 3.2;
    na_fp(&x);
    fp_am(v);
    am_push(v[0]);
    am_push(v[1]);
    am_push(v[2]);
    am_push(v[3]);
    am_command(AM_CHS | AM_FLOAT);
    s = am_wait();
    printf("CHSF %g status = %d (%d)\n", x, s, AM_SIGN);
    v[3] = am_pop();
    v[2] = am_pop();
    v[1] = am_pop();
    v[0] = am_pop();
    am_fp(v);
    fp_na(&x);
    printf("   result -> %g\n", x);

    x = 0.0;
    na_fp(&x);
    fp_am(v);
    am_push(v[0]);
    am_push(v[1]);
    am_push(v[2]);
    am_push(v[3]);
    am_command(AM_CHS | AM_FLOAT);
    s = am_wait();
    printf("CHSF %g status = %d (%d)\n", x, s, AM_ZERO);
    v[3] = am_pop();
    v[2] = am_pop();
    v[1] = am_pop();
    v[0] = am_pop();
    am_fp(v);
    fp_na(&x);
    printf("   result -> %g\n", x);
}

#endif

#ifdef TEST3

/* TEST3:
 */
void am_test3(void) {

    printf("am_test3\n");

    am_wait();

}

#endif

void am_test(void) {
#ifdef TEST1
    am_test1();
#endif
#ifdef TEST2
    am_test2();
#endif
#ifdef TEST3
    am_test3();
#endif
}


/* Give usage for am9511 test
 */
void usage(char *p) {
    printf("usage: %s [-d port] [-s port]\n");
    printf("    -d port    set data port\n");
    printf("    -s port    set status port\n");
    printf("\n");
    printf("port numbers are specified in decimal\n");
    exit(1);
}


int main(int ac, char **av) {
    int ch, s, d;

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

    /* Reset AM9511. If using actual hardware, passes in status and
     * data ports that will be used.
     */
    am_reset(s, d);

    am_test();

    return 0;
}
