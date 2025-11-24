#include "RDX.h"

#include "abc/PRO.h"

ok64 RDXCopy(rdxb into, rdxb from) {
    sane(rdxbOK(into) && rdxbOK(from) && rdxbWritable(into) &&
         !rdxbWritable(from));
    scan(RDXNext, from) {
        rdxMv(rdxbLast(into), rdxbLast(from));
        call(RDXNext, into);
        if (rdxbTypePlex(from)) {
            call(RDXInto, from);
            call(RDXInto, into);

            call(RDXCopy, into, from);

            call(RDXOuto, into);
            call(RDXOuto, from);
        }
    }
    seen(END);
    rdxbLast(into)->type = 0;
    call(RDXNext, into);
    done;
}
