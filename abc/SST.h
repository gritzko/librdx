#ifndef ABC_SST_H
#define ABC_SST_H

#include <fcntl.h>

#include "BUF.h"
#include "OK.h"
#include "TLV.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

#define SSTab SKIPu8tab

static const ok64 SSTnodata = 0x1c71dcb3a25e25;
static const ok64 SSTnoroom = 0x1c71dcb3db3cf1;
static const ok64 SSTbadhead = 0x71c766968b29968;
static const ok64 SSTbadrec = 0x1c71d9a5a36a67;
static const ok64 SSTbad = 0x71c766968;
static const ok64 SSTnone = 0x1c71dcb3ca9;

// Header: SST0 (predata) data // 16 bytes
typedef struct {
    u32 magic;
    u32 metalen;
    u64 datalen;
} SSTheader;

#endif
