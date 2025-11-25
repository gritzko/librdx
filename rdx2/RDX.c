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

ok64 rdxbOpen(rdxb b, u8cs data, RDX_FORMAT fmt) {
    sane(rdxbOK(b));
    call(rdxbFed1, b);
    rdxp last = rdxbLast(b);
    zerop(last);
    $mv(last->data, data);
    last->format = fmt;
    last->type = 0;
    done;
}

ok64 rdxbInto(rdxb b) {
    sane(rdxbOK(b) && rdxbDataLen(b));
    rdxp prev = rdxbLast(b);
    call(rdxbFed1, b);
    rdxp last = rdxbLast(b);
    zerop(last);
    $mv(last->data, prev->plex);
    last->format = prev->format;
    last->type = 0;
    last->prnt = prev->type;
    done;
}

ok64 rdxbOuto(rdxb its) {
    sane(rdxbOK(its) && rdxbDataLen(its));
    u8cs data;
    $mv(data, rdxbLast(its)->data);
    rdxbPop(its);
    if (rdxbDataLen(its)) {
        $mv(rdxbLast(its)->plex, data);
    }
    done;
}

ok64 rdxbClose(rdxb its, u8cs data) {
    sane(rdxbOK(its) && rdxbDataLen(its));
    $mv(data, rdxbLast(its)->data);
    rdxbPop(its);
    done;
}
