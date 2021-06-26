# am9511
This is a library to allow conversion of 32 bit floating point formats. AM9511,
Microsoft, IEEE, Hi-Tech C formats are supported. Emulation of the AM9511 is the
end goal.

This is designed to run both on a Z80, or on a generic sytem with gcc (or tcc).

Some sample reference floating values are given for tests.

The next part is an emulation of the AM9511A chip. We are mapping AM9511
functionality into the host, for future integration into 8080/Z80 (or other)
emulators. This part could be used with 8080, Z80, 8085, 6800, z8000, and even
Apple 2 (6502) systems, providing 16 and 32 bit integer and 32 bit floating
point.

test.com is a basic test of the emulator (note -- basic functions are implemented,
advanced features (SQRT, PWR, SIN, etc) are not yet). Beginning test of arithmetic.

testhw.com is the same as test.com, but runs on the actual chip:

testhw -d port -s port

-d port sets data port for the am9511

-s port sets status/command port for the am9511

both port values must be in decimal. Typical values would be 80 and 81

ova.c implements integer 16 and 32 bit arithmetic, with overflow.

am9511 is not complete, and not usable (the floatcnv part is usable). This
is under heavy development. am9511 needs more extensive testing -- test.c
needs more work.

getopt.c is the BSD getopt() function, because HI-TECH C doesn't have it.

HI-TECH C is not "fully" ANSI C 89 compatible. "ansi.h" defines signed,
const, volatile. Can't blame HI-TECH C 3.09 -- the standard was two years
AFTER this compiler.
