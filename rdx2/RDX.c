#include "RDX.h"

#include "abc/PRO.h"

ok64 rdxCopy(rdxp into, rdxp from) {
    sane(into && from);
    scan(rdxNext, from) {
        rdxMv(into, from);
        call(rdxNext, into);
    }
    seen(END);
    into->type = 0;
    call(rdxNext, into);
    done;
}

ok64 rdxbCopy(rdxb into, rdxb from) {
    sane(rdxbOK(into) && rdxbOK(from) && rdxbWritable(into) &&
         !rdxbWritable(from));
    rdxp i = rdxbLast(into);
    rdxp f = rdxbLast(from);
    scan(rdxNext, f) {
        rdxMv(i, f);
        call(rdxNext, i);
        if (rdxTypePlex(f)) {
            test(rdxbIdleLen(into) && rdxbIdleLen(from), NOROOM);
            call(rdxbInto, into);
            call(rdxbInto, from);
            call(rdxbCopy, into, from);
            call(rdxbOuto, into);
            call(rdxbOuto, from);
        }
    }
    seen(END);
    i->type = 0;
    f->type = 0;
    done;
}

ok64 rdxbInto(rdxb b) {
    sane(rdxbOK(b) && rdxbDataLen(b));
    rdxp p = rdxbLast(b);
    call(rdxbFed1, b);
    rdxp c = rdxbLast(b);
    call(VTABLE_INTO[p->format], c, p);
    done;
}

ok64 rdxbOuto(rdxb its) {
    sane(rdxbOK(its) && rdxbDataLen(its));
    rdxp c = rdxbLast(its);
    rdxbPop(its);
    rdxp p = rdxbLast(its);
    call(VTABLE_OUTO[p->format], c, p);
    done;
}
