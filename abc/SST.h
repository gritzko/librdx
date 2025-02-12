#ifndef ABC_SST_H
#define ABC_SST_H

#include <fcntl.h>

#include "BUF.h"
#include "OK.h"
#include "TLV.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

#define SSTab SKIPu8tab

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

#endif
