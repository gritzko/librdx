//
// Created by gritzko on 11/17/25.
//
#include "RDX.h"
#include "abc/PRO.h"

ok64 RDXNextLSM(rdxb x) { return NOTimplyet; }
ok64 RDXIntoLSM(rdxb x) { return NOTimplyet; }
ok64 RDXOutoLSM(rdxb x) { return NOTimplyet; }
ok64 RDXSeekLSM(rdxb x) { return NOTimplyet; }

ok64 RDXWriteNextLSM(rdxb x) { return NOTimplyet; }
ok64 RDXWriteIntoLSM(rdxb x) { return NOTimplyet; }
ok64 RDXWriteOutoLSM(rdxb x) { return NOTimplyet; }
ok64 RDXWriteSeekLSM(rdxb x) { return NOTimplyet; }

/*
ok64 RDXNextLSM(rdxb x) {
    sane(rdxbOK(x) && rdxbDataLen(x) > 0);
    rdxp top = rdxbLast(x);
    // todo header, index
    u64 hdrlen, datalen;
    rdxmeta128 *meta = *top->data;
    a_tail(u8c, rest, top->data, top->pos + top->len);
    // todo pos starts at meta end
    // todo bail off before index
    call(RDXNextTLV, x);  // fixme index?!!
    done;
}

ok64 RDXIntoLSM(rdxb x) {
    sane(rdxbOK(x));
    // we have the slice
    // plant the header
    // fixme meta
    if (no header) {
        // add no-meta header
    }
    call(rdxsFed1, rdxbIdle(x));


    if (rdxbDataLen(x)) {  // go tlv
        /*rdxp top = rdxbLast(x);
        rdxp neu;
        neu->rw = RDX_RW_READ;
        neu->format = RDX_FORMAT_TLV;
        $mv(neu->data, top->plex);
        return RDXIntoTLV(x);* /
    } else {  // init
        done;
    }
    // first into, i.e. init: rdxbLen(x)==0, but the data range is init'ed
    // the outer code may expand that range (file, mmap, etc), but the pointers
    // should stay valid (so, reserve the address space -- todo routine for
    // that)
}

ok64 RDXOutoLSM(rdxb x) {
    sane(rdxbOK(x));
    call(rdxbPop, x);
    done;
}

ok64 RDXSeekLSM(rdxb x) {
    // FIXME index? header?
    return NOTimplyet;
}

ok64 RDXWriteNextLSM(rdxb x) { return NOTimplyet; }

ok64 RDXWriteIntoLSM(rdxb x) {
    rdxp neu;
    neu->rw = RDX_RW_WRITE;
    neu->format = RDX_FORMAT_TLV;
    return NOTimplyet;
}

ok64 RDXWriteOutoLSM(rdxb x) {
    sane(rdxbDataLen(x) == 1);
    rdxp top = rdxbAtP(x, 0);
    // len is file desc?!
    // feeds the index
    u64s idle = top->data;  // fixme aligns
    idle[0] += top->pos;    // fixme
    u8cs data;
    a_pad(rdx, reread, 1);
    call(RDXOpenRead, reread, RDX_LSM_READER, data);
    while (OK == RDXNextLSM(reread)) {
        if (1) {
            u64 key;
            u64 ops;
            call(u64sFeed2, idle, key, ops);
        }
    }
    return NOTimplyet;
}

ok64 RDXWriteSeekLSM(rdxb x) { return NOTimplyet; }
*/