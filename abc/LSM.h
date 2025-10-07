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

typedef ok64 (*$u8cXfn)(u8c$ next, u8cs rest);
typedef ok64 (*$u8cYfn)($u8 into, u8css from);
typedef int (*$u8cZfn)($cu8c* a, $cu8c* b);

#define X(M, name) M##u8cs##name
#include "abc/HEAPx.h"
#undef X

typedef u8csB LSM;

fun pro(LSMmore, u8csB lsm, u8cs x, $u8cZfn cmp) {
    sane(Bok(lsm) && $ok(x) && cmp != nil);
    // call(u8css_feed1, Bu8csidle(lsm), x);
    memcpy(lsm[2], x, sizeof(u8cs));
    Bu8csidle(lsm)[0]++;
    HEAPu8csupf(lsm, cmp);
    done;
}

ok64 LSMnext($u8 into, u8css lsm, $u8cZfn cmp, $u8cYfn mrg);

fun b8 _$u8cempty(u8cs const* s) { return $empty(*s); }

fun ok64 LSMmerge($u8 into, u8css lsm, $u8cZfn cmp, $u8cYfn mrg) {
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
ok64 LSMsort($u8 data, $u8cZfn cmp, $u8cYfn mrg, $u8 tmp);

#endif
