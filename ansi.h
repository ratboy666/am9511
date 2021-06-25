/* ansi.h
 *
 * Make HI-TECH C a bit more ANSI C 89 compatible. Note that the
 * last revision of HI-TECH C 3.09 was released in 1987 -- the
 * ANSI C 89 standard was submitted in 1988. So, this is not a
 * fault of HI-TECH C.
 */


#ifndef ANSI_H
#define ANSI_H

#ifdef z80

/* Not reserved in HI-TECH C 3.09
 */
#define signed
#define const
#define volatile

#endif

#endif
