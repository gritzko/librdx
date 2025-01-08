#ifndef ABC_LINE_H
#define ABC_LINE_H
#include "abc/BUF.h"
#include "abc/OK.h"
#include "abc/PRO.h"

#define LSM_MAX_INPUTS 64

con ok64 LSMeof = 0xab3a56715;
con ok64 LSMbad = 0xa25996715;
con ok64 LSMnodata = 0x25e25a33c96715;
con ok64 LSMnoroom = 0x31cf3db3c96715;

typedef ok64 (*$u8cXfn)(u8c$ next, $u8c rest);
typedef ok64 (*$u8cYfn)($u8 into, $$u8c from);
typedef int (*$u8cZfn)($cu8c* a, $cu8c* b);

#define X(M, name) M##$u8c##name
#include "abc/HEAPx.h"
#undef X

typedef B$u8c LSM;

fun pro(LSMmore, B$u8c lsm, $u8c x, $u8cZfn cmp) {
    sane(Bok(lsm) && $ok(x) && cmp != nil);
    // call($$u8cfeed1, B$u8cidle(lsm), x);
    memcpy(lsm[2], x, sizeof($u8c));
    B$u8cidle(lsm)[0]++;
    HEAP$u8cupf(lsm, cmp);
    done;
}

ok64 LSMnext($u8 into, $$u8c lsm, $u8cZfn cmp, $u8cYfn mrg);

fun b8 $u8cempty($u8c const* s) { return $empty(*s); }

fun pro(LSMsort, $$u8c lsm, $u8cZfn cmp) {
    sane($ok(lsm) && cmp != nil);
    $$u8cpurge(lsm, &$u8cempty);
    $sort(lsm, cmp);
    done;
}

fun ok64 LSMmerge($u8 into, $$u8c lsm, $u8cZfn cmp, $u8cYfn mrg) {
    ok64 o = LSMsort(lsm, cmp);
    if (o != OK) return o;
    while (o == OK && !$empty(lsm)) {
        o = LSMnext(into, lsm, cmp, mrg);
    }
    return o;
}

ok64 LSMmergehard($u8 into, $$u8c inputs, $u8cZfn cmp, $u8cYfn mrg);

#endif
