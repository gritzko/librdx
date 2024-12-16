#ifndef ABC_NEST_H
#define ABC_NEST_H
#include "BUF.h"
#include "OK.h"

#define NESTreset(ct) Breset1(ct)
#define NESTidle(b) ((u8**)b + 1)
#define NESTdata(b) ((u8c**)b + 0)
#define NESTdatac(b) ((u8c**)b + 0)

con ok64 NESTnoroom = 0xc73cf6cf275c397;
con ok64 NESTnodata = 0x978968cf275c397;
con ok64 NESTbad = 0x2896675c397;
con ok64 NESTnone = 0xa72cf275c397;

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
