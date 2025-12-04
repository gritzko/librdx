#ifndef ABC_LSM_H
#define ABC_LSM_H
#include "abc/BUF.h"
#include "abc/OK.h"
#include "abc/PRO.h"

#define LSM_MAX_INPUTS 64

con ok64 LSMeof = 0xab3a56715;
con ok64 LSMbad = 0xa25996715;
con ok64 LSMnodata = 0x25e25a33c96715;
con ok64 LSMnoroom = 0x31cf3db3c96715;

typedef ok64 (*u8csxXfn)(u8c$ next, u8cs rest);
typedef ok64 (*$u8cYfn)($u8 into, u8css from);
typedef ok64 (*$u8cZfn)($cu8c* a, $cu8c* b);

#define X(M, name) M##u8cs##name
#include "abc/HEAPx.h"
#undef X

typedef u8csb LSM;

fun pro(LSMmore, u8csb lsm, u8cs x, u8csz z) {
    sane(Bok(lsm) && $ok(x) && z);
    // call(u8cssFeed1, u8csbIdle(lsm), x);
    memcpy(lsm[2], x, sizeof(u8cs));
    u8csbIdle(lsm)[0]++;
    u8cssUpZ(lsm, z);
    done;
}

ok64 LSMnext(u8s into, u8css lsm, u8csz z, u8sy mrg);

fun b8 _$u8cempty(u8cs const* s) { return $empty(*s); }

fun ok64 LSMmerge(u8s into, u8css lsm, u8csz cmp, u8sy mrg) {
    u8css_purge(lsm, &_$u8cempty);
    $sort(lsm, cmp);
    ok64 o = OK;
    while (o == OK && !$empty(lsm)) {
        o = LSMnext(into, lsm, cmp, mrg);
    }
    return o;
}

// Almost in-place merge sort of TLV records.
// May not be the best way to sort things in the general case,
// as it implies the data goes roughly in order.
// The scratch space must not be less than the sorted slice.
ok64 LSMsort(u8s data, u8csz cmp, u8sy mrg, u8s tmp);

#endif
