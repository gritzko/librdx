#ifndef ABC_LSM_H
#define ABC_LSM_H
#include "abc/BUF.h"
#include "abc/OK.h"
#include "abc/PRO.h"

#define LSM_MAX_INPUTS 64

con ok64 LSMEOF = 0x55c58e60f;
con ok64 LSMBAD = 0x55c58b28d;
con ok64 LSMNODATA = 0x157165d834a74a;
con ok64 LSMNOROOM = 0x157165d86d8616;

#define X(M, name) M##u8cs##name
#include "abc/HEAPx.h"
#undef X

typedef u8csb LSM;

fun ok64 LSMMore(u8csb lsm, u8cs x, u8csz z) {
    sane(Bok(lsm) && $ok(x) && z);
    // call(u8cssFeed1, u8csbIdle(lsm), x);
    memcpy(lsm[2], x, sizeof(u8cs));
    u8csbIdle(lsm)[0]++;
    u8cssUpZ(lsm, z);
    done;
}

ok64 LSMNext(u8s into, u8css lsm, u8xs x, u8csz z, u8ys y);

fun b8 _$u8cempty(u8cs const* s) { return $empty(*s); }

fun ok64 LSMMerge(u8s into, u8css lsm, u8xs x, u8csz z, u8ys y) {
    u8css_purge(lsm, &_$u8cempty);
    u8cssHeapZ(lsm, z);
    ok64 o = OK;
    while (o == OK && !$empty(lsm)) {
        o = LSMNext(into, lsm, x, z, y);
    }
    return o;
}

// Almost in-place merge sort of records.
// May not be the best way to sort things in the general case,
// as it implies the data goes roughly in order.
// The scratch space must not be less than the sorted slice.
ok64 LSMSort(u8s data, u8xs x, u8csz z, u8ys y, u8s tmp);

#endif
