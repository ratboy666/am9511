/* types.h
 */


#ifndef TYPES_H
#define TYPES_H

#ifdef __TINYC__

/* Types for TCC, Linux, 64 bit
 */
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef signed char    int8;
typedef short          int16;
typedef int            int32;

#endif


#ifdef __GNUC__

/* Types for GNUC, Linux, 64 bit
 */
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef signed char    int8;
typedef short          int16;
typedef int            int32;

#endif

#ifdef z80

#include <ansi.h>

/* Types for Hi-Tech C, z80, 8 bit
 */
typedef unsigned char  uint8;
typedef unsigned int   uint16;
typedef unsigned long  uint32;
typedef signed char    int8;
typedef int            int16;
typedef long           int32;

#endif

#endif
