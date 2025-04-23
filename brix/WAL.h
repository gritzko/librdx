#ifndef FLY_WAL_H
#define FLY_WAL_H
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "brix/BRIX.h"

static const ok64 WALbad = 0x80a566968;
static const ok64 WALbadlen = 0x202959a5a30a72;

typedef struct {
    id128 id;
    u32 recs[4];
} fly256;

fun u64 fly256hash(fly256 const* v) { return mix128(v->id); }

fun int fly256cmp(fly256 const* a, fly256 const* b) {
    return id128cmp(&a->id, &b->id);
}

#define X(M, name) M##fly256##name
#include "abc/Bx.h"
#include "abc/HASHx.h"
#undef X

typedef struct {
    Bu8 log;
    Bfly256 idx;
} WAL;

#define WALok(wal) (wal != nil && Bok(wal->log) && Bok(wal->idx))

ok64 WALcreate(WAL* wal, $u8c filename, u64 logsz);

ok64 WALopen(WAL* wal, $u8c filename);

// may return XYZnoroom if either the log or the index oferfill
ok64 WALadd(WAL* wal, $u8c rdx);
ok64 WALadd1(WAL* wal, $cu8c rdx);

ok64 WALget($$u8c ins, WAL const* wal, id128 id);

ok64 WALget1($u8 res, WAL const* wal, id128 id);

ok64 WAL2brick(sha256* sha, sha256c* top, WAL* wal, $cu8c home);

ok64 WALclose(WAL* wal);

#endif
