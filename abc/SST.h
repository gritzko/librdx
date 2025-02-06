#ifndef ABC_SST_H
#define ABC_SST_H

#include <fcntl.h>

#include "B.h"
#include "LSM.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

const u32 SSTmagic =
    (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 8) | ((u32)'0' << 8);

con ok64 SSTnodata = 0x25e25a33c9d71c;
con ok64 SSTnoroom = 0x31cf3db3c9d71c;
con ok64 SSTbadhead = 0xa25a6ca2599d71c;
con ok64 SSTbadrec = 0x27a76a2599d71c;
con ok64 SSTbad = 0xa2599d71c;

// Header: SST0 (predata) data // 16 bytes
typedef struct {
    u32 magic;
    u32 metalen;
    u64 datalen;
} SSTheader;

#define X(M, name) M##bl08##name
#include "SKIPx.h"
#undef X

fun ok64 SSTcreate(u8B sst, int* fd, $u8c path, size_t size) {
    return FILEmapnew(sst, fd, path, size);
}

fun ok64 SSTopen(u8B sst, $u8c path) {
    int fd = FILE_CLOSED;
    ok64 o = FILEmapro(sst, &fd, path);
    if (o != OK) return o;
    SSTheader const* head = (SSTheader const*)sst[0];
    if (Blen(sst) < sizeof(SSTheader) || head->magic != SSTmagic ||
        Blen(sst) < head->datalen + head->datalen + sizeof(SSTheader))
        return SSTbadhead;
    sst[1] = sst[0] + sizeof(SSTheader);
    sst[2] = sst[1] + head->datalen;
    return FILEclose(&fd);
}

fun ok64 SSThasindex(Bu8 sst) { return Bidlelen(sst) != 0; }

fun ok64 SSTfeed(Bu8 sst, SKIPbl08tab* tab, u8 type, $u8c key, $u8c value) {
    ok64 o = TLVfeedkv(Bu8idle(sst), type, key, value);
    if (o == OK) o = SKIPbl08mayfeed(sst, tab);
    return o;
}

fun ok64 SSTfeedkv(Bu8 sst, SKIPbl08tab* tab, $u8c rec) {
    ok64 o = $u8feedall(Bu8idle(sst), rec);
    if (o == OK) o = SKIPbl08mayfeed(sst, tab);
    return o;
}

ok64 SSTfindge(u8c$ rest, Bu8 sst, $u8c key);
ok64 SSTget(u8c$ rec, Bu8 sst, $u8c key);
ok64 SSTmget($u8 into, BBu8 inputs, $u8c key, $u8cYfn y);
ok64 SSTmerge(Bu8 into, BBu8 inputs, $u8cYfn y);

ok64 SSTclosenew(Bu8 sst, int* fd);

fun ok64 SSTclose(Bu8 sst) { return FILEunmap(sst); }

#endif
