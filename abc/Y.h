#ifndef ABC_Y_H
#define ABC_Y_H
#include "abc/BUF.h"
#include "abc/OK.h"

#define Y_MAX_INPUTS 64

con ok64 Yeof = 0xab3a62;
con ok64 Ybad = 0xa259a2;
con ok64 Ynodata = 0x25e25a33ca2;
con ok64 Ynoroom = 0x31cf3db3ca2;

#define X(M, name) M##$u8c##name
#include "abc/HEAPx.h"
#undef X

#endif
