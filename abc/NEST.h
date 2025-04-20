#ifndef ABC_NEST_H
#define ABC_NEST_H
#include "BUF.h"
#include "OK.h"

#define NESTreset(ct) Breset1(ct)
#define NESTidle(b) ((u8**)b + 1)
#define NESTdata(b) ((u8c**)b + 0)
#define NESTdatac(b) ((u8c**)b + 0)

static const ok64 NESTnoroom = 0x5ce71dcb3db3cf1;
static const ok64 NESTnodata = 0x5ce71dcb3a25e25;
static const ok64 NESTbad = 0x1739c766968;
static const ok64 NESTnone = 0x5ce71dcb3ca9;

ok64 NESTinsert(Bu8 ct, ok64 var);

ok64 NESTsplice(Bu8 ct, ok64 var);

ok64 NESTsplicemany(Bu8 ct, ok64 var, b8 some);

fun ok64 NESTspliceall(Bu8 ct, ok64 var) {
    return NESTsplicemany(ct, var, YES);
}

fun ok64 NESTspliceany(Bu8 ct, ok64 var) { return NESTsplicemany(ct, var, NO); }

ok64 NESTfeed(Bu8 ct, $u8c insert);

ok64 NESTrender($u8 into, Bu8 ct);

#endif
