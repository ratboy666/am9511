/* hw9511.c
 *
 * Hardware for am9511. Link with this instead of am9511 to use actual
 * am9511 chip.
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef z80
#include <sys.h>
#endif


struct am9511 {
    int status;
    int data;
};


/* If we use the HI-TECH C functions inp() and outp() note that
 * they use Z80 instructions for i/o -- these are NOT supported
 * by Zxcc (and maybe not by RunCPM).
 *
 * So, we use HI-TECH C in-line assembler. Yes, this *is*
 * self-modifying code. But, doing the i/o this way works with
 * Zxcc.
 *
 * DO NOT OPTIMIZE WHEN COMPILING
 *
 * THIS IS NOT RE-ENTRANT. NEED TO FIX THAT LATER.
 */
#ifdef z80

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

#else

#pragma GCC warning "inp and outp are dummy functions"
#define inp(port) 0
#define outp(port,data)

#endif


/* Push byte to am9511 stack
 */
void am_push(void *p, unsigned char n) {
    outp(((struct am9511 *)p)->data, n);
}


/* Pop byte from am9511 stack
 */
unsigned char am_pop(void *p) {
    return inp(((struct am9511 *)p)->data);
}


/* Return am9511 status
 */
unsigned char am_status(void *p) {
    return inp(((struct am9511 *)p)->status);
}


/* Send command to am9511
 */
void am_command(void *p, unsigned char n) {
    outp(((struct am9511 *)p)->status, n);
}


/* Reset am9511 -- set status and data ports
 *
 * If -ve value passed for port, use default value. The
 * default value is set to match with the Zxcc and RunCPM
 * emulators.
 */
void am_reset(void *p) {
    p = p;
}


/* Dump am9511 stack
 */
void am_dump(void *p, unsigned char op) {
    op = op;
    p = p;
}


/* Create AM9511 access structure
 */
void *am_create(int status, int data) {
    struct am9511 *p;
    p = malloc(sizeof (struct am9511));
    if (p == NULL)
	return NULL;
    p->status = 0x51;
    p->data = 0x50;
    if (status >= 0)
	p->status = status;
    if (data >= 0)
	p->data = data;
    return p;
}
