#ifndef ABC_Y_H
#define ABC_Y_H
#include "abc/BUF.h"
#include "abc/OK.h"

#define Y_MAX_INPUTS 64

static const ok64 Yeof = 0x8a9cea;
static const ok64 Ybad = 0x8a6968;
static const ok64 Ynodata = 0x22cb3a25e25;
static const ok64 Ynoroom = 0x22cb3db3cf1;

#define X(M, name) M##$u8c##name
#include "abc/HEAPx.h"
#undef X

#endif
