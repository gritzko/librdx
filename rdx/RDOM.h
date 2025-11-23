//
// Created by gritzko on 11/17/25.
//

#ifndef RDX_RDOM_H
#define RDX_RDOM_H
#include "RDX2.h"

// in-memory RDX tree-ish structure
typedef struct {
    ref128 id;
    union {
        f64 f;
        i64 i;
        ref128 r;
        u8cs s;
        u8cs t;
        u8cs plex;
    };
    u32 parent_ndx;
    u32 child_ndx;
    u32 next_ndx;
    u32 skip_ndx;
} rdom;

#define X(M, name) M##rdom##name
#include "abc/Bx.h"
#undef X

con ok64 OKchanged = 0x27b25caba68;

fun ok64 rdombNext(rdomb list, u32p ndx) {
    rdomp x = rdombAtP(list, *ndx);
    if (x->next_ndx == 0) return FAILnone;
    *ndx = x->next_ndx;
    return OK;
}
fun ok64 rdombInto(rdomb list, u32p ndx) {
    rdomp x = rdombAtP(list, *ndx);
    if (x->child_ndx == 0) return FAILnone;
    *ndx = x->child_ndx;
    return OK;
}
fun ok64 rdombOuto(rdomb list, u32p ndx) {
    rdomp x = rdombAtP(list, *ndx);
    if (x->parent_ndx == 0 && *ndx == 0) return FAILnone;
    *ndx = x->parent_ndx;
    return OK;
}
ok64 rdombGoto(rdomb list, u32p ndx, rdx256 const* mark);

// @return OK/errs or OKchanged (i.e. see the rdomb version)
ok64 rdx2Next(rdxp it, rdomb list, u32p ndx);
ok64 rdx2Into(rdxp it, rdomb list, u32p ndx);
ok64 rdx2Outo(rdxp it, rdomb list, u32p ndx);
ok64 rdx2Goto(rdxp it, rdomb list, u32p ndx, rdx256 const* mark);

#endif  // RDX_RDOM_H
