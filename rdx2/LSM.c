//
// Created by gritzko on 11/17/25.
//
#include "LSM.h"

#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/TLV.h"

ok64 rdxNextLSM(rdxp x) {
    sane(x && x->format == RDX_FORMAT_LSM);
    call(rdxNextTLV, x);
    done;
}

ok64 rdxWriteIndexKiloLSM(u8s idx8, rdxp reader) {
    sane(reader && u8sOK(idx8));
    call(u8sFeed1, idx8, 0);  // no label
    u64sp idx = (u64**)idx8;
    // todo padding
    u64 len = $len(reader->data);
    u64 lastkilo = 1;
    u64 pos = 0;
    scan(rdxNext, reader) {  // can index anything
        u64 kilo = pos >> 10;
        if (kilo != lastkilo) {
            call(u64sFeed2, idx, id128Key(reader->id), pos);
            lastkilo = kilo;
        }
        pos = len - $len(reader->data);
    }
    seen(END);
    done;
}

ok64 rdxFindByLSMKiloIndex(u8cs idx8, u64p pos, id128 id) {
    sane(u8csOK(idx8) && pos);
    // u64 padlen = $size(idx8) & 0xf;
    // u128cs idx = {idx8[0] + padlen, idx8[1]};
    a_dup(u128c, idx, (u128c**)idx8);
    test(u128csOK(idx), RDXFILEBAD);
    u64 from = 0, till = u128csLen(idx);
    u64 key = id128Key(id);
    u64 MASK48 = (1UL << 48) - 1;
    while (from < till) {
        u64 mid = (from + till) / 2;
        u128cp e = u128csAtP(idx, mid);
        u64 k = e->_64[0];
        if (k < key) {
            from = mid;
        } else if (k > key) {
            till = mid;
        } else if (mid + 1 != till) {
            till = mid + 1;
        } else {
            break;
        }
    }
    *pos = u128csAt(idx, from)._64[1] & MASK48;
    done;
}

ok64 rdxFindByLSMIndex(rdxp p, u64p pos, id128 id) {
    sane(p);
    rdx i = *p;
    while (!$empty(i.data)) {
        call(rdxNextTLV, &i);
        switch (i.type) {  // FIXME wrong
            case RDX_TYPE_BLOB:
                return rdxFindByLSMKiloIndex(i.plex, pos, id);
            case RDX_TYPE_EULER:
                done;
            default:
                continue;
        }
    }
    done;
}

ok64 rdxIntoLSM(rdxp c, rdxp p) {
    sane(c && p);
    c->format = p->cformat;
    c->data = p->plex;
    c->ptype = p->type;
    if (p->type != RDX_TYPE_EULER) done;
    id128 id = c->id;
    if (id128Empty(&id)) done;
    u64 pos = 0;
    call(rdxFindByLSMIndex, p, &pos, id);
    test(pos < $len(p->plex), BADPOS);
    p->plex[0] += pos;
    call(rdxNextTLV, c);
    while (id128Z(&c->id, &id)) call(rdxNextTLV, c);
    done;
}

ok64 rdxOutoLSM(rdxp c, rdxp p) {
    sane(c && c->format == RDX_FORMAT_TLV);
    p->len = 0;
    done;
}

ok64 rdxWriteNextLSM(rdxp x) {
    sane(x && x->format == (RDX_FORMAT_LSM | RDX_FORMAT_WRITE) && !x->ptype);
    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, x->id.seq, x->id.src);
    u8sp plex = (u8sp)x->plex;
    call(TLVu8sStartHuge, x->into, plex, RDX_TYPE_LIT[x->type]);
    call(u8sFeed1, plex, $len(id_data));
    call(u8sFeed, plex, id_datac);
    x->cformat = RDX_FORMAT_TLV | RDX_FORMAT_WRITE;
    done;
}

ok64 rdxWriteIntoLSM(rdxp c, rdxp p) {
    sane(c && p && p->type && p->cformat);
    c->format = p->cformat;
    c->data = p->plex;
    c->ptype = p->type;
    c->type = 0;
    c->cformat = 0;
    c->len = 0;
    zero(c->r);
    done;
}

ok64 rdxWriteIndexLSM(u8s idle, rdxp reader) {
    sane(reader && u8csOK(reader->plex));
    // here we pick the index format
    u8s idx = {};
    call(TLVu8sStartHuge, idle, idx, RDX_TYPE_LIT[RDX_TYPE_BLOB]);
    if (1) {  // todo other idx types
        call(rdxWriteIndexKiloLSM, idx, reader);
    }
    call(TLVu8sEndHuge, idle, idx, RDX_TYPE_LIT[RDX_TYPE_BLOB]);
    done;
}

ok64 rdxWriteOutoLSM(rdxp c, rdxp p) {
    sane(p && c && c->format == (RDX_FORMAT_TLV | RDX_FORMAT_WRITE) &&
         p->plex[1] == p->data[1] && (u8c**)c->into == p->plex);
    u8 plit = RDX_TYPE_LIT[c->ptype];
    a_dup(u8c, full, p->into);
    call(TLVu8sEndHuge, p->into, c->into, plit);

    rdx reader = {.format = RDX_FORMAT_LSM, .data = full};
    call(rdxNextTLV, &reader);
    test(reader.type == RDX_TYPE_EULER, TLVRECBAD);  // paranoid sanity check
    rdx r = {};
    call(rdxIntoLSM, &r, &reader);
    call(rdxWriteIndexLSM, p->into, &r);
    // at this point, the host buffer is not trimmed yet, p->into==idle
    done;
}

// first into, i.e. init: rdxbLen(x)==0, but the data range is init'ed
// the outer code may expand that range (file, mmap, etc), but the pointers
// should stay valid (so, reserve the address space -- todo routine for
// that)
