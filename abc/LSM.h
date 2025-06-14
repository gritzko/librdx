#ifndef ABC_LSM_H
#define ABC_LSM_H
#include "abc/BUF.h"
#include "abc/OK.h"
#include "abc/PRO.h"

#define LSM_MAX_INPUTS 64

static const ok64 LSMeof = 0xab3a56715;
static const ok64 LSMbad = 0xa25996715;
static const ok64 LSMnodata = 0x25e25a33c96715;
static const ok64 LSMnoroom = 0x31cf3db3c96715;

typedef ok64 (*$u8cXfn)(u8c$ next, $u8c rest);
typedef ok64 (*$u8cYfn)($u8 into, $$u8c from);
typedef z32 (*$u8cZfn)($cu8c* a, $cu8c* b);

#define X(M, name) M##$u8c##name
#include "abc/HEAPx.h"
#undef X

typedef B$u8c LSM;

fun ok64 LSMmore(B$u8c lsm, $u8c x, $u8cZfn cmp) {
    sane(Bok(lsm) && $ok(x) && cmp != nil);
    // call($$u8cfeed1, B$u8cidle(lsm), x);
    memcpy(lsm[2], x, sizeof($u8c));
    B$u8cidle(lsm)[0]++;
    HEAP$u8cupf(lsm, cmp);
    done;
}

ok64 LSMnext($u8 into, $$u8c lsm, $u8cZfn cmp, $u8cYfn mrg);

fun b8 $u8cempty($u8c const* s) { return $empty(*s); }

fun ok64 LSMmerge($u8 into, $$u8c lsm, $u8cZfn cmp, $u8cYfn mrg) {
    $$u8cpurge(lsm, &$u8cempty);
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
