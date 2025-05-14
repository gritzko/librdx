#ifndef BRIX_FLY_H
#define BRIX_FLY_H

#include "BRIX.h"
#include "TAG.h"
#include "WAL.h"

typedef struct {
    u64 branch;
    BRIX brix;
    WAL wal;
} FLY;

extern $u8c FLYdir;

fun b8 FLYok(FLY const* fly) { return BRIXok(&fly->brix) && WALok(&fly->wal); }

ok64 FLYnew(FLY* fly, $cu8c path);

ok64 FLYopen(FLY* fly, $cu8c path);

ok64 FLYclose(FLY* fly);

#endif
