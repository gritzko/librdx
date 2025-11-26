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
    call(rdxNext, i);
    done;
}

ok64 rdxbInto(rdxb b) {
    sane(rdxbOK(b) && rdxbDataLen(b));
    rdxp prev = rdxbLast(b);
    call(rdxbFed1, b);
    rdxp last = rdxbLast(b);
    zerop(last);  //???
    last->data = prev->plex;
    last->format = prev->format;
    last->type = 0;
    last->prnt = prev->type;
    last->enc = 0;
    last->len = 0;
    zero(last->r);
    done;
}

ok64 rdxbOuto(rdxb its) {
    sane(rdxbOK(its) && rdxbDataLen(its));
    rdxp lo = rdxbLast(its);
    rdxbPop(its);
    rdxp hi = rdxbLast(its);
    $mv(hi->plex, lo->data);
    done;
}
