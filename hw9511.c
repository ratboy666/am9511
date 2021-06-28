/* hw9511.c
 *
 * Hardware for am9511. Link with this instead of am9511 to use actual
 * am9511 chip.
 */

#include <stdio.h>
#ifdef z80
#include <sys.h>
#endif

/* If we use the HI-TECH C functions inp() and outp() note that
 * they use Z80 instructions for i/o -- these are NOT supported
 * by Zxcc (and maybe not be RunCPM).
 *
 * So, we use HI-TECH C in-line assembler. Yes, this *is*
 * self-modifying code. But, doing the i/o this way works with
 * Zxcc (MBASIC does it this way).
 */
#ifdef z80

#if 1

static char io_port;
static char io_data;

int inp(int port) {
    io_port = (char)port;
#asm
    ld a,(_io_port)
    ld (mod1),a
    defb 0dbh
mod1:
    defb 0
    ld (_io_data),a
#endasm
    return io_data;
}

void outp(int port, int data) {
    io_port = (char)port;
    io_data = (char)data;
#asm
    ld a,(_io_port)
    ld (mod2),a
    ld a,(_io_data)
    defb 0d3h
mod2:
    defb 0
#endasm
}

#endif

#else

#pragma GCC warning "inp and outp are dummy functions"
#define inp(port) 0
#define outp(port,data)

#endif

/* Status and data ports. These can set by am_reset(), so just
 * choose some convenient values. These defaults are the "usual"
 * for a lot of AM9511 implementations.
 */
static int status = 0x51;
static int data   = 0x50;


/* Push byte to am9511 stack
 */
void am_push(unsigned char n) {
    outp(data, n);
}


/* Pop byte from am9511 stack
 */
unsigned char am_pop(void) {
    return inp(data);
}


/* Return am9511 status
 */
unsigned char am_status(void) {
    return inp(status);
}


/* Send command to am9511
 */
void am_command(unsigned char n) {
    outp(status, n);
}


/* Reset am9511 -- set status and data ports
 *
 * If -ve value passed for port, use default value. The
 * default value is set to match with the Zxcc and RunCPM
 * emulators.
 */
void am_reset(int s, int d) {
    if (s > 0)
        status = s;
    if (d > 0)
        data = d;
}


/* Dump am9511 stack
 */
void am_dump(unsigned char op) {
    op = op;
}

