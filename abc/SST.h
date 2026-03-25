#ifndef ABC_SST_H
#define ABC_SST_H

#include <fcntl.h>

#include "BUF.h"
#include "OK.h"
#include "TLV.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

#define SSTab SKIPu8tab

con ok64 SSTNODATA = 0x1c71d5d834a74a;
con ok64 SSTNOROOM = 0x1c71d5d86d8616;
con ok64 SSTBADHEAD = 0x71c74b28d44e28d;
con ok64 SSTBADREC = 0x1c71d2ca35b38c;
con ok64 SSTBAD = 0x71c74b28d;
con ok64 SSTNONE = 0x1c71d5d85ce;

// Header: SST0 (predata) data // 16 bytes
typedef struct {
    u32 magic;
    u32 metalen;
    u64 datalen;
} SSTheader;

#endif
