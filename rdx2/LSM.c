//
// Created by gritzko on 11/17/25.
//
#include "RDX.h"
#include "abc/PRO.h"

ok64 rdxNextLSM(rdxp x) { return NOTIMPLYET; }

ok64 rdxIntoLSM(rdxp c, rdxp p) {
    // head, idx
    // use idx?
    return NOTIMPLYET;
}

ok64 rdxOutoLSM(rdxp c, rdxp p) { return OK; }

ok64 rdxWriteNextLSM(rdxp x) {
    // m d: b (ms) ...
    return NOTIMPLYET;
}

ok64 rdxWriteIntoLSM(rdxp c, rdxp p) {
    // lets go
    return NOTIMPLYET;
}

ok64 rdxWriteOutoLSM(rdxp c, rdxp p) {
    //
    rdx re = {};
    u64s idx = {};
    while (OK==rdxNextTLV(&re)) {

    }
    return NOTIMPLYET;
}

    // first into, i.e. init: rdxbLen(x)==0, but the data range is init'ed
    // the outer code may expand that range (file, mmap, etc), but the pointers
    // should stay valid (so, reserve the address space -- todo routine for
    // that)

