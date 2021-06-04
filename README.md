# am9511
This is a library to allow conversion of 32 bit floating point formats. AM9511, Microsoft, IEEE, Hi-Tech C
formats are supported.

This is designed to run both on a Z80, or on a generic sytem with gcc (or tcc).

Some sample reference floating values are given for tests.

The next part is an emulation of the AM9511A chip. We are mapping AM9511 functionality into the host,
for future integration into 8080/Z80 (or other) emulators. This part could be used with 8080, Z80, 8085,
6800, z8000, and even Apple 2 (6502) systems, providing 16 and 32 bit integer and 32 bit floating
point.



