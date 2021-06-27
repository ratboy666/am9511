/* am9511.c
 *
 * First cut am9511 emulation. This version is NOT cycle accurate,
 * or even algorithm accurate. It should be a somewhat reasonable
 * stand-in, which should allow us to run base-line comparisions with
 * the real device.
 */


#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "am9511.h"
#include "floatcnv.h"
#include "ova.h"
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


#define AM_OP    0x1f


/* Stack is 16 bytes long. sp is the stack pointer.
 * Points to next location to use.
 */
static unsigned char stack[16] = { 0, };
static int sp = 0;


/* Add to sp
 */
#define sp_add(n) ((sp + (n)) & 0xf)


/* Return pointer into stack
 */
#define stpos(offset) (stack + sp_add(offset))


/* Increment stack pointer
 */
#define inc_sp(n) sp = sp_add(n)


/* Decrement stack pointer
 */
#define dec_sp(n) sp = sp_add(-(n))


/* Push byte to am9511 stack
 */
void am_push(unsigned char v) {
    *stpos(0) = v;
    inc_sp(1);
}


/* Pop byte from am9511 stack
 */
unsigned char am_pop(void) {
    dec_sp(1);
    return *stpos(0);
}


/* AM9511 status and operator latch
 */
static unsigned char status = 0;
static unsigned char op_latch = 0;
#ifndef NDEBUG
static unsigned char last_latch = 0;
#endif

/* Return status of am9511
 */
unsigned char am_status(void) {
    return status;
}


#define IS_SINGLE ((op_latch & AM_SINGLE) == AM_SINGLE)
#define IS_FIXED (op_latch & AM_FIXED)


/* Set SIGN and ZERO according to op type and top of stack.
 * Zero detect for integer is or'ing together all the bytes.
 * Zero detect for float is testing bit 23 for 0.
 * The sign bit for all types is the top-most bit. If 1 then
 * negative.
 */
static void sz(void) {
    if (IS_SINGLE) {
	if ((*stpos(-1) | *stpos(-2)) == 0)
	    status |= AM_ZERO;
    } else if (IS_FIXED) {
	if ((*stpos(-1) | *stpos(-2) | *stpos(-3) | *stpos(-4)) == 0)
	    status |= AM_ZERO;
    } else {
	if ((*stpos(-2) & 0x80) == 0)
	    status |= AM_ZERO;
    }
    if (*stpos(-1) & 0x80)
	status |= AM_SIGN;
}


/* PUPI
 */
static void pupi(void) {
    am_push(0xda); /* little end to big end */
    am_push(0x0f);
    am_push(0xc9);
    am_push(0x02);
    sz();
}


/* PTOS PTOD PTOF
 *
 * This relies on the stack data not moving during a push.
 */
static void pto(void) {
    unsigned char *s; 

    if (IS_SINGLE) {
        s = stpos(-2);
	am_push(*s++);
	am_push(*s);
    } else {
        s = stpos(-4);
        am_push(*s++);
	am_push(*s++);
	am_push(*s++);
	am_push(*s);
    }
    sz();
}


/* POPS POPD POPF
 *
 * Note that the SIGN and ZERO flags are set from the element that
 * is next on stack. But... it may be wrong! We do not know what the
 * new tos element really is! (in terms of type)
 * The guide states and SIGN and ZERO are affected, but no more than that.
 */
static void pop(void) {
    if (IS_SINGLE)
    	dec_sp(2);
    else
    	dec_sp(4);
    sz();
}


/* XCHS XCHD XCHF
 */
static void xch(void) {
    unsigned char *s, *t, v;

    if (IS_SINGLE) {
	s = stpos(-2);
	t = stpos(-4);
	v = *t; *t++ = *s; *s++ = v;
	v = *t; *t   = *s; *s   = v;
    } else {
	s = stpos(-4);
	t = stpos(-8);
	v = *t; *t++ = *s; *s++ = v;
	v = *t; *t++ = *s; *s++ = v;
	v = *t; *t++ = *s; *s++ = v;
	v = *t; *t   = *s; *s   = v;
    }
    sz();
}


/* CHSS CHSD CHSF
 */
static void chs(void) {
    if (IS_SINGLE) {
        if (cm16(stpos(-2), stpos(-2)))
	    status |= AM_ERR_OVF;
    } else if (IS_FIXED) {
	if (cm32(stpos(-4), stpos(-4)))
	    status |= AM_ERR_OVF;
    } else {
	/* Floating point sign change - only flip sign
	 * (if not zero). And, as with the AM9511 chip, CHSF
	 * is even faster than CHSS.
	 */
	if (*stpos(-2) & 0x80)
	    *stpos(-1) ^= 0x80;
    }
    sz();
}


/* Push float to stack, set SIGN and ZERO
 */
static void push_float(float x) {
    unsigned char v[4];

    na_fp(&x);
    fp_am(v);
    am_push(v[0]);
    am_push(v[1]);
    am_push(v[2]);
    am_push(v[3]);
    op_latch = AM_FLOAT;
    sz();
}


/* FLTS
 */
static void flts(void) {
    int n;
    float x;

    n = am_pop();
    n = (n << 8) | am_pop();
    x = n;
    push_float(x);
}


/* FLTD
 */
static void fltd(void) {
    int32 n;
    float x;
    int b;

    /* HI-TECH C long shift bug
     */
    b = am_pop();
    n = b;

    n = n << 8;
    b = am_pop();
    n = n | b;

    n = n << 8;
    b = am_pop();
    n = n | b;

    n = n << 8;
    b = am_pop();
    n = n | b;

    x = n;
    push_float(x);
}


/* FIXS
 */
static void fixs(void) {
    float x;
    unsigned char *s;
    int n;

    s = stpos(-4);
    am_fp(s);
    fp_na(&x);
    if ((x < -32768.0) || (x > 32767.0)) {
	status |= AM_ERR_OVF;
	sz();
	return;
    }
    dec_sp(4);
    n = (int)x;
    am_push(n);
    am_push(n >> 8);
    op_latch = AM_SINGLE;
    sz();

}


/* FIXD
 */
static void fixd(void) {
    float x;
    unsigned char *s;
    int32 n;
    float xl, xh;

    s = stpos(-4);
    am_fp(s);
    fp_na(&x);
    n = -2147483648;
    xl = (float)n;
    n = 2147483647;
    xh = (float)n;
    if ((x < xl) || (x > xh)) {
	status |= AM_ERR_OVF;
	sz();
	return;
    }
    dec_sp(4);
    n = (int32)x;
    am_push(n);
    am_push(n >> 8);
    am_push(n >> 8);
    am_push(n >> 8);
    op_latch = AM_DOUBLE;
    sz();
}


/* SADD DADD
 */
void add(void) {
    int carry;
    int overflow;

    if (IS_SINGLE) {
        carry    = add16( stpos(-4), stpos(-2), stpos(-4));
        overflow = oadd16(stpos(-4), stpos(-2), stpos(-4));
	dec_sp(2);
    } else {
        carry    = add32( stpos(-8), stpos(-4), stpos(-8));
        overflow = oadd32(stpos(-8), stpos(-4), stpos(-8));
	dec_sp(4);
    }
    if (carry)
	status |= AM_CARRY;
    if (overflow)
	status |= AM_ERR_OVF;
    sz();
}


/* SSUB DSUB
 */
void sub(void) {
    int carry;
    int overflow;

    if (IS_SINGLE) {
        carry    = sub16( stpos(-4), stpos(-2), stpos(-4));
        overflow = osub16(stpos(-4), stpos(-2), stpos(-4));
	dec_sp(2);
    } else {
        carry    = sub32( stpos(-8), stpos(-4), stpos(-8));
        overflow = osub32(stpos(-8), stpos(-4), stpos(-8));
	dec_sp(4);
    }
    if (carry)
	status |= AM_CARRY;
    if (overflow)
	status |= AM_ERR_OVF;
    sz();
}


/* MUL
 */
static void mul(void) {
    int overflow;

    if (IS_SINGLE) {
        overflow = mull16(stpos(-4), stpos(-2), stpos(-4));
        dec_sp(2);
    } else {
        overflow = mull32(stpos(-8), stpos(-4), stpos(-8));
        dec_sp(4);
    }
    if (overflow)
	status |= AM_ERR_OVF;
    sz();
}


/* MUU
 */
static void muu(void) {
    int overflow;

    if (IS_SINGLE) {
        overflow = mulu16(stpos(-4), stpos(-2), stpos(-4));
        dec_sp(2);
    } else {
        overflow = mulu32(stpos(-8), stpos(-4), stpos(-8));
        dec_sp(4);
    }
    if (overflow)
	status |= AM_ERR_OVF;
    sz();
}


/* DIV
 */
static void divi(void) {
    int div0;

    if (IS_SINGLE) {
        div0 = div16(stpos(-4), stpos(-2), stpos(-4));
        dec_sp(2);
    } else {
        div0 = div32(stpos(-8), stpos(-4), stpos(-8));
        dec_sp(4);
    }
    if (div0)
	status |= AM_ERR_DIV0;
    sz();
}


/* Detect and report float overflow/underflow
 */
static int fov(double r) {
    int e;

    frexp(r, &e);
    if (e > 63) {
	status |= AM_ERR_OVF;
	return 1;
    } else if (e < -64) {
	status |= AM_ERR_UND;
	return 1;
    }
    return 0;
}


/* basicf - basic FADD/FSUB/FMUL/FDIV
 *
 * The guide says that overflow and underflow are detected on the
 * exponent. The mantissa is maintained, and the exponent is offset
 * by 128. So... that is what we do. Note that frexp() and ldexp()
 * should be implemented via bit operations, not arithmetic.
 */
static void basicf(void) {
    unsigned char *ap, *bp;
    float a, b, r;
    double m;
    int e;
 
    ap = stpos(-4);
    am_fp(ap);
    fp_na(&a);

    bp = stpos(-8);
    am_fp(bp);
    fp_na(&b);

    switch (op_latch & AM_OP) {
    case AM_FADD:
        r = a + b;
	break;
    case AM_FSUB:
        r = b - a;
	break;
    case AM_FMUL:
        r = a * b;
	break;
    case AM_FDIV:
	if (a == 0.0) {
	    r = b;
	    status |= AM_ERR_DIV0;
	} else
            r = b / a;
	break;
    }

    /* We do not use fov() because we want to bias exponent by 128
     * on OVF/UND per the guide.
     */
    m = frexp(r, &e);
    if (e > 63) {
	status |= AM_ERR_OVF;
	e -= 128;
	r = ldexp(m, e);
    } else if (e < -64) {
	status |= AM_ERR_UND;
	e += 128;
	r = ldexp(m, e);
    }
    na_fp(&r);
    fp_am(bp);
    dec_sp(4);
    op_latch = AM_FLOAT;
    sz();
}


/* SQRT EXP SIN COS TAN LN LOG etc (functions with single arg)
 *
 * Note that we use the -lm math library with GCC, and the -LF library
 * with HI-TECH C. This means we are limited to only using functions
 * that are in both. This explains the strange shenanigans with double
 * here.
 */
static void ffunc(void) {
    unsigned char *ap;
    float a;
    double x;

    ap = stpos(-4);
    am_fp(ap);
    fp_na(&a);

    x = a;
    switch (op_latch & AM_OP) {
    case AM_SQRT:
        if (a < 0.0) {
	    status |= AM_ERR_NEG;
	    goto err;
	} else
            x = sqrt(x);
	break;
    case AM_EXP:
        /* -1.0 x 2^5 .. 1.0 x 2^5 */
        if ((a < -32.0) || (a > 32.0)) {
	    status |= AM_ERR_ARG;
	    goto err;
	} else
            x = exp(x);
	break;
    case AM_SIN:
	x = sin(x);
	break;
    case AM_COS:
	x = cos(x);
	break;
    case AM_TAN:
	/* 2^-12 */
	if (a < (1.0 / 4096.0))
	    goto err;
        x = tan(x);
	break;
    case AM_LN:
	if (a < 0.0) {
	    status |= AM_ERR_NEG;
	    goto err;
	}
	x = log(x);
	break;
    case AM_LOG:
	if (a < 0.0) {
	    status |= AM_ERR_NEG;
	    goto err;
	}
	x = log10(x);
	break;
    case AM_ASIN:
	if ((a < -1.0) || (a > 1.0)) {
	   status |= AM_ERR_ARG;
	   goto err;
	}
	x = asin(x);
	break;
    case AM_ACOS:
	if ((a < -1.0) || (a > 1.0)) {
	   status |= AM_ERR_ARG;
	   goto err;
	}
	x = acos(x);
	break;
    case AM_ATAN:
	x = atan(x);
	break;
    }
    if (fov(x))
	goto err;
    a = x;
    na_fp(&a);
    fp_am(ap);
err:
    op_latch = AM_FLOAT;
    sz();
}


/* PWR
 *
 * B^A = EXP( A * LN(B) )
 */
static void pwr(void) {
    /* B^A = EXP( A * LN(B) ) */
    unsigned char *ap, *bp;
    float a, b;
    double x;

    /* A */
    ap = stpos(-4);
    am_fp(ap);
    fp_na(&a);

    /* B */
    bp = stpos(-8);
    am_fp(bp);
    fp_na(&b);

    /* LN(B) */
    if (b < 0.0) {
	status |= AM_ERR_NEG;
	goto err;
    }
    x = b;
    x = log(x);

    /* A * LN(B) */
    x = (double)a * x;

    /* EXP( A * LN(B) ) */
    if ((x < -32.0) || (x > 32.0)) {
        status |= AM_ERR_ARG;
	goto err;
    }
    x = exp(x);

    if (fov(x))
        goto err;

    /* replace B with result */
    b = x;
    na_fp(&b);
    fp_am(bp);

    /* roll stack */
    dec_sp(4);
err:
    sz();
}


/* Issue am9511 command. Does not return until command
 * is complete.
 */
void am_command(unsigned char op) {

    op_latch = op;

#ifndef NDEBUG
    last_latch = op;
#endif

    status = AM_BUSY;

    switch (op_latch & AM_OP) {

    case AM_NOP:  /* no operation */
	status = 0;
        break;

    case AM_PUPI: /* push pi */
	pupi();
	break;

    case AM_CHS:  /* change sign */
	chs();
	break;

    case AM_POP:  /* pop */
	pop();
        break;

    case AM_PTO:  /* push tos (copy) */
	pto();
        break;

    case AM_XCH:  /* exchange tos and nos */
	xch();
        break;

    case AM_FLTD: /* 32 bit to float */
	fltd();
        break;

    case AM_FLTS: /* 16 bit to float */
	flts();
        break;

    case AM_FIXD: /* float to 32 bit */
	fixd();
        break;

    case AM_FIXS: /* float to 16 bit */
	fixs();
        break;

    case AM_ADD:  /* add */
	add();
	break;

    case AM_SUB:  /* subtract nos-tos */
	sub();
        break;

    case AM_MUL:  /* multiply, lower half */
	mul();
        break;

    case AM_MUU:  /* multiply, upper half */
	muu();
        break;

    case AM_DIV:  /* divide nos/tos */
	divi();
        break;

    case AM_FADD: /* floating add */
    case AM_FSUB: /* floating subtract */
    case AM_FMUL: /* floating multiply */
    case AM_FDIV: /* floating divide */
	basicf();
        break;

    case AM_SQRT: /* square root */
    case AM_EXP:  /* exponential (e^x) */
    case AM_SIN:  /* sine */
    case AM_COS:  /* cosine */
    case AM_TAN:  /* tangent */
    case AM_LOG:  /* common logarithm (base 10) */
    case AM_LN:   /* natural logarthm (base e) */
    case AM_ASIN: /* inverse sine */
    case AM_ACOS: /* inverse cosine */
    case AM_ATAN: /* inverse tangent */
	ffunc();
        break;

    case AM_PWR:  /* power nos^tos */
	pwr();
        break;

    default:
        break;
    }

    status &= ~AM_BUSY;
}


/* Reset the am9511 emulator
 */
void am_reset(int status, int data) {
    int i;

    sp = 0;
    status = 0;
    op_latch = 0;
    last_latch = 0;
    for (i = 0; i < 16; ++i)
	stack[i] = 0;
}


/* Dump stack A..H or A..D, format depends on arg (AM_SINGLE,
 * AM_DOUBLE, AM_FLOAT). Dump status and last op_latch.
 */
void am_dump(unsigned char op) {
#ifdef NDEBUG
    op = op;
#else
    int i;
    int16 n;
    int32 nl;
    float x;
    unsigned char t = status;
    int b;
    static char *opnames[] = {
        "NOP",  "SQRT", "SIN",  "COS",
        "TAN",  "ASIN", "ACOS", "ATAN",
        "LOG",  "LN",   "EXP",  "PWR",
        "ADD",  "SUB",  "MUL",  "DIV",
        "FADD", "FSUB", "FMUL", "FDIV",
        "CHS",  "",     "MUU",  "PTO",
        "POP",  "XCH",  "PUPI", "",
        "FLTD", "FLTS", "FIXD", "FIXS"
    };

    printf("AM9511 STATUS: %02x ", status);
        if (t & AM_BUSY)  printf("BUSY ");
        if (t & AM_SIGN)  printf("SIGN ");
        if (t & AM_ZERO)  printf("ZERO ");
        if (t & AM_CARRY) printf("CARRY ");
        printf("ERROR: ");
        t &= AM_ERR_MASK;
        if (t == AM_ERR_NONE) printf("NONE");
        if (t & AM_ERR_DIV0)  printf("DIV0");
        if (t & AM_ERR_NEG)   printf("NEG");
        if (t & AM_ERR_ARG)   printf("ARG");
        if (t & AM_ERR_ARG)   printf("ARG");
        if (t & AM_ERR_UND)   printf("UND");
        if (t & AM_ERR_OVF)   printf("OVF");
        printf("\n");
    t = last_latch;
    printf("LAST COMMAND: ");
        if (t & AM_SR)                    printf("SR ");
        if ((t & AM_SINGLE) == AM_SINGLE) printf("SINGLE ");
        else if (t & AM_FIXED)            printf("DOUBLE ");
        else                              printf("FLOAT ");
        printf("%s\n", opnames[t & AM_OP]);
    t = op_latch;
    op_latch = op;
    printf("AM9511 STACK ");
    if (IS_SINGLE) {
	printf("(SINGLE)\n");
	for (i = 0; i < 8; ++i) {
            n =            *stpos(-(i * 2) - 1);
            n = (n << 8) | *stpos(-(i * 2) - 2);
	    printf("%c: %02x %02x %d\n", 'A' + i,
                                         *stpos(-(i * 2) - 1),
                                         *stpos(-(i * 2) - 2),
					 n);
	}
    } else {
        if (IS_FIXED)
	    printf("(DOUBLE)\n");
	else
	    printf("(FLOAT)\n");
	for (i = 0; i < 4; ++i) {
    	    printf("%c: %02x %02x %02x %02x ", 'A' + i,
                                               *stpos(-(i * 4) - 1),
                                               *stpos(-(i * 4) - 2),
                                               *stpos(-(i * 4) - 3),
                                               *stpos(-(i * 4) - 4));
	    if (IS_FIXED) {
#if 0
		/* Borked -- HI-TECH C bug
		 *
		 * We have seen this before -- use a "fix" (work-around)
		 */
                nl =             *stpos(-(i * 4) - 1);
                nl = (nl << 8) | *stpos(-(i * 4) - 2);
                nl = (nl << 8) | *stpos(-(i * 4) - 3);
                nl = (nl << 8) | *stpos(-(i * 4) - 4);
#else
		/* We use the following instead, which seems to work
		 */
                b = *stpos(-(i * 4) - 1);
		nl = b;

		nl = nl << 8;
                b = *stpos(-(i * 4) - 2);
		nl = nl | b;

		nl = nl << 8;
                b = *stpos(-(i * 4) - 3);
		nl = nl | b;

		nl = nl << 8;
                b = *stpos(-(i * 4) - 4);
		nl = nl | b;
#endif
		printf("%ld\n", (long)nl);
	    } else {
		am_fp(stpos(-(i * 4) - 4));
		fp_na(&x);
		printf("%g\n", x);
	    }
	}
    }
    op_latch = t;
#endif
}
