/* hw9511.c
 *
 * Hardware for am9511. Link with this instead of am9511 to use actual
 * am9511 chip.
 */

#ifdef z80
#include <sys.h>
#else
#pragma GCC warning "inp and outp are dummy functions"
#define inp(port) 0
#define outp(port,data)
#endif


/* Status and data ports. These are set by am_reset(), so just
 * choose some convenient values.
 */
static int status = 0x50;
static int data   = 0x51;


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
 */
void am_reset(int s, int d) {
    status = s;
    data = d;
}


/* Dump am9511 stack
 */
void am_dump(unsigned char op) {
    op = op;
}

