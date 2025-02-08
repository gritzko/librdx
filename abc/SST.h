#ifndef ABC_SST_H
#define ABC_SST_H

#include <fcntl.h>

#include "B.h"
#include "LSM.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

static const u32 SSTmagic =
    (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) | ((u32)'0' << 24);

#define SSTab SKIPu8tab
#define SST Bu8

con ok64 SSTnodata = 0x25e25a33c9d71c;
con ok64 SSTnoroom = 0x31cf3db3c9d71c;
con ok64 SSTbadhead = 0xa25a6ca2599d71c;
con ok64 SSTbadrec = 0x27a76a2599d71c;
con ok64 SSTbad = 0xa2599d71c;
con ok64 SSTnone = 0x29cb3c9d71c;

// Header: SST0 (predata) data // 16 bytes
typedef struct {
    u32 magic;
    u32 metalen;
    u64 datalen;
} SSTheader;

#define X(M, name) M##u8##name
#include "SKIPx.h"
#undef X

#endif
