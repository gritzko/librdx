#ifndef LIBRDX_SMI_H
#define LIBRDX_SMI_H

#include "SM.h"

con ok64 SMIBAD = 0x71648b28d;

#define SMI_ITALIC  1
#define SMI_BOLD    2
#define SMI_CODE    4
#define SMI_STRIKE  8
#define SMI_LINK   16

typedef struct {
    u8cs data;
    u8cp data_start;
    u8cp data_end;
    SMstate *sm;
    u8 mode;
    u8 base;
    u8 link_phase;  // 0=none, 1=display [, 2=after ], 3=ref [
} SMIstate;

ok64 SMILexer(SMIstate* state);

#endif
