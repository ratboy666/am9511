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


#define AM_SMALL 2.71051e-20
#define AM_BIG   9.22337e+18
#define AM_PI    3.141592

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
static unsigned char op_latch;


/* Return status of am9511
 */
unsigned char am_status(void) {
    return status;
}


/* PUPI
 */
static void pupi(void) {
    am_push(0xda); /* little end to big end */
    am_push(0x0f); /* +ve, non-zero, no error, */
    am_push(0xc9); /* so leave status at 0 */
    am_push(0x02);
}


/* CHSS CHSD CHSF
 */
static void chs(void) {
    if (op_latch & AM_FIXED) {
        if (op_latch & AM_SINGLE) {
            if (cm16(stpos(-2), stpos(-2)))
	        status |= AM_ERR_OVF;
	    if (*stpos(-1) & 0x80)
		status |= AM_SIGN;
	    if ((*stpos(-1) | *stpos(-2)) == 0)
	        status |= AM_ZERO;
	} else {
	    if (cm32(stpos(-4), stpos(-4)))
		status |= AM_ERR_OVF;
	    if (*stpos(-1) & 0x80)
		status |= AM_SIGN;
	    if ((*stpos(-1) | *stpos(-2) |
		 *stpos(-3) | *stpos(-4)) == 0)
		status |= AM_ZERO;
	}
    } else {
	/* Floating point sign change - only flip sign
	 * if not zero. And, as with the AM9511 chip, CHSF
	 * even is faster than CHSS.
	 */
	if (*stpos(-2) & 0x80)
	    *stpos(-1) ^= 0x80;
	if (*stpos(-1) & 0x80)
	    status |= AM_SIGN;
	if ((*stpos(-2) & 0x80) == 0)
	    status |= AM_ZERO;
    }
}


/* Issue am9511 command. Does not return until command
 * is complete.
 */
void am_command(unsigned char op) {

    op_latch = op;

    status = AM_BUSY;

    switch (op_latch & AM_OP) {

    case AM_NOP:  /* no operation */
        break;

    case AM_PUPI: /* push pi */
	pupi();
	break;

    case AM_CHS:  /* change sign */
	chs();
	break;

    case AM_ADD:  /* add */
	printf("ADD not implemented\n");
	break;

    case AM_SUB:  /* subtract nos-tos */
	printf("SUB not implemented\n");
        break;

    case AM_MUL:  /* multiply, lower half */
	printf("MUL not implemented\n");
        break;

    case AM_MUU:  /* multiply, upper half */
	printf("MUU not implemented\n");
        break;

    case AM_DIV:  /* divide nos/tos */
	printf("DIV not implemented\n");
        break;


    case AM_PTO:  /* push tos to nos (copy) */
	printf("PTO not implemented\n");
        break;

    case AM_POP:  /* pop */
	printf("POP not implemented\n");
        break;

    case AM_XCH:  /* exchange tos and nos */
	printf("XCH not implemented\n");
        break;

    case AM_FLTD: /* 32 bit to float */
	printf("FLTD not implemented\n");
        break;

    case AM_FLTS: /* 16 bit to float */
	printf("FLTS not implemented\n");
        break;

    case AM_FIXD: /* float to 32 bit */
	printf("FIXD not implemented\n");
        break;

    case AM_FIXS: /* float to 16 bit */
	printf("FIXS not implemented\n");
        break;

    case AM_SQRT: /* square root */
	printf("SQRT not implemented\n");
        break;

    case AM_SIN:  /* sine */
	printf("SIN not implemented\n");
        break;

    case AM_COS:  /* cosine */
	printf("COS not implemented\n");
        break;

    case AM_TAN:  /* tangent */
	printf("TAN not implemented\n");
        break;

    case AM_ASIN: /* inverse sine */
	printf("ASIN not implemented\n");
        break;

    case AM_ACOS: /* inverse cosine */
	printf("ACOS not implemented\n");
        break;

    case AM_ATAN: /* inverse tangent */
	printf("ATAN not implemented\n");
        break;

    case AM_LOG:  /* common logarithm (base 10) */
	printf("LOG not implemented\n");
        break;

    case AM_LN:   /* natural logairthm (base e) */
	printf("LN not implemented\n");
        break;

    case AM_EXP:  /* exponential (e^x) */
	printf("EXP not implemented\n");
        break;

    case AM_PWR:  /* power nos^tos */
	printf("PWR not implemented\n");
        break;

    case AM_FADD: /* floating add */
	printf("FADD not implemented\n");
        break;

    case AM_FSUB: /* floating subtract */
	printf("FSUB not implemented\n");
        break;

    case AM_FMUL: /* floating multiply */
	printf("FMUL not implemented\n");
        break;

    case AM_FDIV: /* floating divide */
	printf("FDIV not implemented\n");
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
    for (i = 0; i < 16; ++i)
	stack[i] = 0;
}

