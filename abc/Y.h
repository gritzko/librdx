#ifndef ABC_Y_H
#define ABC_Y_H
#include "abc/BUF.h"
#include "abc/OK.h"

#define Y_MAX_INPUTS 64

con ok64 YEOF = 0x88e60f;
con ok64 YBAD = 0x88b28d;
con ok64 YNODATA = 0x225d834a74a;
con ok64 YNOROOM = 0x225d86d8616;

#define X(M, name) M##u8cs##name
#include "abc/HEAPx.h"
#undef X

#endif
