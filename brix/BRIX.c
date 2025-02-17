#include "BRIX.h"

#include <fcntl.h>

#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/SHA.h"
#include "abc/SST.h"
#include "abc/TLV.h"
#include "rdx/RDX.h"
#include "rdx/RDXY.h"
#include "rdx/RDXZ.h"

a$strc(BRIKtmp, ".tmp");
a$strc(BRIKext, ".brik");
a$strc(BRIXdir, ".brix");
a$strc(BRIXindex, "INDEX");

ok64 _BRIXpath(BRIX* brix, $u8c path) {
    sane(brix != nil && $ok(path));
    call(Bu8alloc, brix->home, $len(path) + 128);
    u8** homeidle = Bu8idle(brix->home);
    u8c** homedata = Bu8cdata(brix->home);
    call($u8feed, homeidle, path);
    call($u8feed1, homeidle, '/');
    call($u8feed, homeidle, BRIXdir);
    call($u8feed1, homeidle, '/');
    done;
}

ok64 BRIKfeedpath($u8 into, BRIX const* brix, h60 let) {
    sane($ok(into) && brix != nil);
    call($u8feedall, into, Bu8c$1(brix->home));
    call(BRIXfeedh60, into, let);
    call($u8feedall, into, BRIKext);
    done;
}

ok64 BRIXinit(BRIX* brix, $u8c path) {
    sane(brix != nil && !Bok(brix->store) && $ok(path));
    call(BBu8alloc, brix->store, LSM_MAX_INPUTS);
    call(_BRIXpath, brix, path);
    u8c** homedata = Bu8cdata(brix->home);
    call(FILEmakedir, homedata);
    a$dup(u8c, olddata, homedata);
    int fd = FILE_CLOSED;

    call($u8feed, Bu8$2(brix->home), BRIXindex);
    call(FILEmapnew, (u8**)brix->index, &fd, homedata, PAGESIZE);
    call(FILEclose, &fd);
    $mv(homedata, olddata);

    call(Bu8map, brix->pad, GB);

    done;
}

ok64 BRIXopen(BRIX* brix, $u8c path) {
    sane(brix != nil && Bnil(brix->store));
    call(BBu8alloc, brix->store, LSM_MAX_INPUTS);
    call(_BRIXpath, brix, path);

    u8c** homedata = Bu8cdata(brix->home);
    u64 dl = $len(homedata);
    int fd = FILE_CLOSED;

    a$dup(u8c, old, homedata);
    call($u8feed, Bu8$2(brix->home), BRIXindex);
    call(FILEmapro, (u8**)brix->index, &fd, homedata);
    call(FILEclose, &fd);
    $mv(homedata, old);

    call(Bu8map, brix->pad, GB);

    done;
}

ok64 BRIXclose(BRIX* brix) {
    sane(BRIXok(brix));
    if (Bok(brix->store)) call(BBu8free, brix->store);
    if (Bok(brix->index)) call(FILEunmap, (u8**)brix->index);
    if (Bok(brix->home)) call(Bu8free, brix->home);
    if (Bok(brix->pad)) call(Bu8unmap, brix->pad);
    done;
}

ok64 BRIXpush(BRIX* brix, h60 let) {
    sane(BRIXok(brix));
    aBcpad(u8, brik, FILEmaxpathlen);
    call(BRIKfeedpath, brikidle, brix, let);
    Bu8 buf = {};
    call(SSTu128open, buf, brikdata);
    call(BBu8feed1, brix->store, buf);
    done;
}

fun ok64 BRIXresetpad(BRIX* brix) {
    Breset(brix->pad);
    // TODO if got too big: remap?!
    return OK;
}

ok64 _BRIXget(u8c$ into, BRIX const* brix, u8 rdt, id128 key) {
    sane($ok(into) && brix != nil && TLVlong(rdt));
    aBpad2($u8c, ins, LSM_MAX_INPUTS);
    Bu8$ ssts = BBu8$1(brix->store);
    for (Bu8* p = $head(ssts); p < $term(ssts); ++p) {
        u8 t = rdt;
        $u8c val = {};
        ok64 o = SSTu128get(&t, val, *p, &key);
        if (o == OK) $$u8cfeed1(insidle, val);
    }
    if ($len(insdata) == 1) {
        $mv(into, *$head(insdata));
    } else if ($empty(insdata)) {
        $zero(into);
        fail(BRIXnone);
    } else {
        u8$ idle = Bu8idle(brix->pad);
        a$dup(u8, tmp, idle);
        call(RDXY, idle, insdata);
        $u8sup(tmp, idle);
        $mv(into, tmp);
    }
    done;
}

ok64 BRIXget(u8c$ into, BRIX const* brix, u8 rdt, id128 key) {
    BRIXresetpad((BRIX*)brix);
    return _BRIXget(into, brix, rdt, key);
}

ok64 BRIXinitdeps(SSTu128 sst, BRIX* brix) {
    sane(Bok(sst) && BRIXok(brix));
    if (Bempty(brix->store)) {
        sha256 sha = {};
        a$rawc(sharaw, sha);
        call($u8feedall, Bu8$2(sst), sharaw);
        Bu8eatdata(sst);
    } else {
        fail(notimplyet);
    }
    done;
}

ok64 BRIXenlist(B$u8c heap, u64* roughlen, $u8c rdx) {
    sane(Bok(heap) && $ok(rdx));
    while (!$empty(rdx)) {
        $u8c rec = {};
        call(TLVdrain$, rec, rdx);
        *roughlen += $len(rec);
        if (RDXisPLEX(**rec)) {
            call(HEAP$u8cpushf, heap, &rec, RDXZrevision);
            $u8c id, val;
            u8 rdt;
            call(TLVdrainkv, &rdt, id, val, rec);
            *roughlen -= $len(val);
            call(BRIXenlist, heap, roughlen, val);
        }
    }
    done;
}

ok64 BRIXflatfeed($u8 into, $u8c rdx) {
    sane($ok(into) && $ok(rdx) && RDXisPLEX(**rdx));
    $u8c empty = {};
    aBcpad(u8p, stack, 1);
    u8 rdt;
    $u8c key, val;
    call(TLVdrainkv, &rdt, key, val, rdx);
    call(TLVinitlong, into, rdt, stackbuf);  // TODO adapt
    call($u8feed1, into, $len(key));
    call($u8feedall, into, key);
    while (!$empty(val)) {
        if (RDXisPLEX(**val)) {
            u8 t;
            $u8c k, v;
            call(TLVdrainkv, &t, k, v, val);
            call(TLVfeedkv, into, t, k, empty);
        } else {
            $u8c rec;
            call(TLVdrain$, rec, val);
            call($u8feed, into, rec);
        }
    }
    call(TLVendany, into, rdt, stackbuf);
    done;
}

ok64 _BRIXpatch(h60* let, BRIX* brix, $u8c rdx, B$u8c heap) {
    sane(BRIXok(brix) && $ok(rdx));
    aBcpad(u8p, stack, RDX_MAX_NEST);

    u64 roughlen = 0;
    call(BRIXenlist, heap, &roughlen, rdx);

    SSTu128 sst = {};
    int fd = FILE_CLOSED;
    aBcpad(u8, tmp, FILEmaxpathlen);
    call($u8feedall, tmpidle, Bu8c$1(brix->home));
    call($u8feedall, tmpidle, BRIKtmp);
    size_t size = roundup(roughlen + PAGESIZE, PAGESIZE);
    SKIPu8tab tab = {};
    call(SSTu128init, sst, &fd, tmpdata, size);
    call(BRIXinitdeps, sst, brix);

    while (!Bempty(heap)) {
        $u8c pop = {};
        call(HEAP$u8cpop, &pop, heap);
        call(BRIXflatfeed, Bu8idle(sst), pop);
    }

    call(SSTu128end, sst, &fd, &tab);
    aBusy(u8c, hashed, sst);
    sha256 sha = {};
    SHAsum(&sha, hashed);
    aBcpad(u8, sha, FILEmaxpathlen);
    call(BRIKfeedpath, shaidle, brix, BRIXhashlet(&sha));
    call(FILErename, tmpdata, shadata);

    done;
}

ok64 BRIXpatch(h60* sha, BRIX* brix, $u8c rdx) {
    B$u8c heap = {};
    ok64 o = B$u8calloc(heap, BRIX_MAX_SST0_ENTRIES);
    if (o != OK) return o;
    o = _BRIXpatch(sha, brix, rdx, heap);
    B$u8cfree(heap);
    return o;
}

ok64 BRIXpushrev(BRIX* brix, id128 head) { return notimplyet; }

ok64 BRIXproduceimpl($u8 into, BRIX const* brix, u8 rdt, id128 key,
                     Bu8p stack) {
    sane($ok(into) && BRIXok(brix));
    $u8c got = {};
    call(BRIXget, got, brix, rdt, key);
    if (rdt == 0) rdt = **got & ~TLVaA;
    $u8c body = {};
    call(TLVinitlong, into, rdt, stack);
    while (!$empty(body)) {
        a$dup(u8c, d, body);
        u8 t = 0;
        u128 k = {};
        $u8c v = {};
        call(RDXdrain, &t, &k, v, body);
        if (RDXisPLEX(t)) {
            call(BRIXproduceimpl, into, brix, t, k, stack);
        } else {
            $u8csup(d, body);
            call($u8feedall, into, d);
        }
    }
    call(TLVendany, into, rdt, stack);
    done;
}

ok64 BRIXproduce($u8 into, BRIX const* brix, u8 rdt, id128 key) {
    aBcpad(u8p, stack, RDX_MAX_NEST);
    return BRIXproduceimpl(into, brix, rdt, key, stackbuf);
}
