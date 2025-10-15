#include "BRIX.h"

#include <fcntl.h>

#include "abc/01.h"
#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/SHA.h"
#include "abc/SST.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"
#include "dirent.h"
#include "rdx/RDX.h"
#include "rdx/RDXZ.h"
#include "rdx/Y.h"

a$strc(BRIKtmp, ".tmp.brik");
a$strc(BRIKext, ".brik");

fun ok64 BRIKhead(BRIX const* brix, SSTheader const** head, sha256c$ shas,
                  u32 ndx) {
    if (ndx >= Busylen(brix->shas)) return BRIXmiss;
    return SSTu128meta(Bat(brix->ssts, 0), head, (u8c$)shas);
}

ok64 BRIKfilename($u8 into, BRIX const* brix, sha256c* sha) {
    sane($ok(into) && brix != nil && sha != nil);
    a$rawcp(raw, sha);
    call(HEXfeedall, into, raw);
    call($u8feedall, into, BRIKext);
    done;
}

ok64 BRIKpath($u8 into, BRIX const* brix, sha256c* sha) {
    sane($ok(into) && brix != nil);
    call($u8feedall, into, brix->home);
    call(u8sFeed1, into, '/');
    call(BRIKfilename, into, brix, sha);
    done;
}

ok64 BRIXopenrepo(BRIX* brix, u8cs path) {
    sane(brix != nil && !Bok(brix->ssts) && $ok(path));
    call(FILEisdir, path);
    call(Bu8Balloc, brix->ssts, LSM_MAX_INPUTS);
    call(Bsha256alloc, brix->shas, LSM_MAX_INPUTS);
    Bu8 pb = {};
    call(Bu8alloc, pb, $len(path));
    $u8feed(Bu8$2(pb), path);
    $mv(brix->home, Bu8$1(pb));
    call(Bu8map, brix->pad, GB);
    done;
}

// add an SST to the stack, including *missing* dependencies
ok64 BRIXadd(BRIX* brix, sha256c* sha) {
    sane(BRIXok(brix) && sha != nil);
    aBcpad(u8, fn, FILEmaxpathlen);
    call(BRIKpath, fnidle, brix, sha);
    SSTu128 sst = {};
    int fd = FILE_CLOSED;
    call(SSTu128open, sst, fndata);
    $sha256c deps = {};
    call(SSTu128meta, sst, nil, (u8c**)deps);
    test($sha256ok(deps), BRIXbad);
    if (!$empty(deps) && !sha256empty($head(deps))) {  // TODO
        ok64 o = BRIXhave(brix, $head(deps));
        if (o != OK) call(BRIXadd, brix, $head(deps));
    }
    call(u8BBFeed1, brix->ssts, sst);
    call(sha256BFeedP, brix->shas, sha);
    done;
}

// close everything previously open, then
// add an SST to the stack, including its dependencies.
ok64 BRIXopen(BRIX* brix, sha256c* sha) {
    sane(BRIXok(brix) && sha != nil);
    call(BRIXcloseall, brix);
    call(BRIXadd, brix, sha);
    Bu8Beatdata(brix->ssts);
    Bsha256eatdata(brix->shas);
    done;
}

ok64 BRIXcloserepo(BRIX* brix) {
    sane(BRIXok(brix));
    call(BRIXcloseall, brix);
    if (Bok(brix->ssts)) Bu8Bfree(brix->ssts);
    if (Bok(brix->shas)) Bsha256free(brix->shas);
    if ($ok(brix->home)) $u8free(brix->home);
    if (Bok(brix->pad)) Bu8unmap(brix->pad);
    done;
}

// @return OK, BRIXnone
ok64 BRIXhave(BRIX const* brix, sha256c* id) {
    sane(BRIXok(brix) && id != nil);
    aBusy(sha256c, shas, brix->shas);
    $eat(shas) if (sha256eq(*shas, id)) return OK;
    aBusy(SSTu128, ssts, brix->ssts);
    $eat(ssts) {
        $sha256c deps = {};
        call(SSTu128meta, **ssts, nil, (u8c**)deps);
        $eat(shas) if (sha256eq(*shas, id)) return OK;
    }
    // TODO a recurring version, one day
    fail(BRIXnone);
}

// close the added SSTs
ok64 BRIXcloseadded(BRIX* brix) {
    sane(BRIXok(brix));
    a$dup(Bu8, added, Bu8Bdata(brix->ssts));
    $eat(added) SSTu128close(**added);
    $zero(Bsha256data(brix->shas));
    Bu8Bresetdata(brix->ssts);
    Bsha256resetdata(brix->shas);
    done;
}

// close all SSTs
ok64 BRIXcloseall(BRIX* brix) {
    sane(BRIXok(brix));
    aBusy(Bu8, ssts, brix->ssts);
    $eat(ssts) SSTu128close(**ssts);
    Breset(brix->ssts);
    Breset(brix->shas);
    Bzero(brix->shas);
    done;
}

ok64 BRIKhash(sha256* hash, SSTu128 sst) {
    sane(hash != nil && Bok(sst));
    // aBusy(u8c, hashed, sst); strange compiler glitch?
    u8cs hashed = {};
    hashed[0] = sst[0];
    hashed[1] = sst[2];
    SHAsum(hash, hashed);
    done;
}

// Merge the added SSTs, so the newly formed SST replaces them.
ok64 BRIXmerge(sha256* newsha, BRIX* brix) {
    sane(newsha != nil && BRIXok(brix));
    test(Bdatalen(brix->ssts) > 0, BRIXnone);

    aBpad2(u8cs, ins, LSM_MAX_INPUTS);
    aBpad2(sha256, deps, LSM_MAX_INPUTS);

    sha256 base = {};
    sha256$c opened = Bsha256past(brix->shas);
    if (!$empty(opened)) {
        base = *$sha256last(opened);
    }
    sha256BFeed1(depsbuf, base);
    Bsha256eatdata(depsbuf);
    call($sha256feed, Bsha256idle(depsbuf), Bsha256cdata(brix->shas));
    $sha256sort(depsdata);
    Bsha256resetpast(depsbuf);

    a$dup(u8B, news, Bu8Bdata(brix->ssts));
    $eat(news) HEAPu8csPush1Z(insbuf, Bu8cdata(**news), RDXZrevision);

    SSTu128 sst = {};
    int fd = FILE_CLOSED;
    aBcpad(u8, tmp, FILEmaxpathlen);
    call($u8feedall, tmpidle, brix->home);
    call($u8feedall, tmpidle, BRIKtmp);
    size_t sumsz = PAGESIZE;
    a$dup(u8cs, ins, insdata);
    $eat(ins) sumsz += $size(**ins);
    sumsz = roundup(sumsz + PAGESIZE + $size(depsdata), PAGESIZE);
    call(SSTu128init, sst, &fd, tmpdata, sumsz);
    call(u8B_feed$, sst, (u8c$)depsdata);
    Bu8eatdata(sst);

    SKIPu8tab tab = {};
    u8$ sstinto = Bu8idle(sst);
    while (!$empty(insdata)) {
        call(LSMnext, sstinto, insdata, RDXZrevision, Y);
        call(SKIPu8mayfeed, sst, &tab);
    }

    call(SSTu128end, sst, &fd, &tab);
    sha256 sha = {};
    call(BRIKhash, &sha, sst);
    call(SSTu128close, sst);

    aBcpad(u8, path, FILEmaxpathlen);
    call(BRIKpath, pathidle, brix, &sha);
    call(FILErename, tmpdata, pathdata);

    call(BRIXcloseadded, brix);
    call(BRIXopen, brix, &sha);
    *newsha = sha;

    done;
}

ok64 BRIXget($u8 rec, BRIX const* brix, u8 rdt, id128 key) {
    sane(rec != nil && brix != nil && (TLVlong(rdt) || rdt == 0));
    aBpad2(u8cs, ins, LSM_MAX_INPUTS);
    for (Bu8* p = brix->ssts[0]; p < brix->ssts[2]; ++p) {
        u8 t = rdt;
        u8cs rec = {};
        ok64 o = SSTu128getkv(rec, *p, t, &key);
        if (o == OK) u8cssFeed1(insidle, rec);
    }
    if ($len(insdata) == 1) {
        call($u8feedall, rec, *$head(insdata));
    } else if ($empty(insdata)) {
        fail(BRIXnone);
    } else {
        call(Y, rec, insdata);
    }
    done;
}

ok64 _BRIXgetc(u8c$ rec, BRIX const* brix, u8 rdt, id128 key) {
    sane($nil(rec) && brix != nil && (TLVlong(rdt) || rdt == 0));
    aBpad2(u8cs, ins, LSM_MAX_INPUTS);
    for (Bu8* p = brix->ssts[0]; p < brix->ssts[2]; ++p) {
        u8 t = rdt;
        u8cs rec = {};
        ok64 o = SSTu128getkv(rec, *p, t, &key);
        if (o == OK) u8cssFeed1(insidle, rec);
    }
    if ($len(insdata) == 1) {
        $mv(rec, *$head(insdata));
    } else if ($empty(insdata)) {
        fail(BRIXnone);
    } else {
        u8$ idle = Bu8idle(brix->pad);
        a$dup(u8, tmp, idle);
        call(Y, idle, insdata);
        $u8sup(tmp, idle);
        $mv(rec, tmp);
    }
    done;
}

// Get a record (TLKV, ZINT u128 key, RDX body).
ok64 BRIXgetc(u8c$ rec, BRIX const* brix, u8c rdt, id128 key) {
    Breset(brix->pad);
    return _BRIXgetc(rec, brix, rdt, key);
}

ok64 _BRIXreget($u8 into, BRIX const* brix, u8 rdt, id128 key, Bu8p stack) {
    sane($ok(into) && BRIXok(brix) && Bdatalen(stack) < RDX_MAX_NEST);
    u8cs got = {};
    call(_BRIXgetc, got, brix, rdt, key);
    u8 t = 0;
    u8cs k = {}, v = {}, body = {};
    call(TLVDrainKeyVal, &t, k, body, got);
    call(TLVinitlong, into, t, stack);
    call(u8sFeed1, into, $len(k));
    call($u8feedall, into, k);
    while (!$empty(body)) {
        a$dup(u8c, d, body);
        u8 erdt = 0;
        u8cs ekey = {};
        u8cs eval = {};
        id128 eid = {};
        call(TLVDrainKeyVal, &erdt, ekey, eval, body);
        if (RDXisPLEX(erdt) && !$empty(ekey)) {
            call(ZINTu128drain, &eid, ekey);
            if (id128src(eid) != 0) {
                call(_BRIXreget, into, brix, erdt, eid, stack);
                continue;
            }
        }
        u8cssup(d, body);
        call($u8feedall, into, d);
    }
    call(TLVendany, into, rdt, stack);
    done;
}

// Same as BRIXget, but recursively produces a document following all
// the references, e.g. `{@rec-1 1 2 [@rec-3] }` rec-1 refers to rec-3
// as an element. if `[@rec-3 "one" "two"]` then the combined result is
// `{@rec-1 1 2 [@rec-3 "one" "two"] }`
ok64 BRIXreget($u8 into, BRIX const* brix, u8c rdt, id128 key) {
    sane($ok(into) && BRIXok(brix));
    Breset(brix->pad);
    aBcpad(u8p, nest, RDX_MAX_NEST);
    call(_BRIXreget, into, brix, rdt, key, nestbuf);
    done;
}

ok64 BRIXisentry($cu8c rdx) {
    if ($empty(rdx) || !RDXisPLEX(**rdx)) return BRIXnone;
    u128 id = {};
    u8 t = 0;
    u8cs val = {};
    a$dup(u8c, dup, rdx);
    ok64 o = RDXdrain(&t, &id, val, dup);
    if (o == OK && id128src(id) == 0) o = BRIXnone;
    return o;
}

ok64 BRIXenlist(u8csb heap, u64* roughlen, $cu8c allrdx) {
    sane(Bok(heap) && $ok(allrdx));
    a$dup(u8c, rdx, allrdx);
    while (!$empty(rdx)) {
        u8cs rec = {};
        call(TLVDrain$, rec, rdx);
        *roughlen += $len(rec);
        if (RDXisPLEX(**rec)) {
            if (BRIXisentry(rec) == OK) {
                call(HEAPu8csPushZ, heap, &rec, RDXZrevision);
            }
            u8cs id, val;
            u8 rdt;
            call(TLVDrainKeyVal, &rdt, id, val, rec);
            *roughlen -= $len(val);
            call(BRIXenlist, heap, roughlen, val);
        }
    }
    done;
}

ok64 BRIXflatfeed($u8 into, u8cs rdx) {
    sane($ok(into) && $ok(rdx) && RDXisPLEX(**rdx));
    u8cs empty = {};
    aBcpad(u8p, stack, 1);
    u8 rdt = 0;
    u8cs key = {}, body = {};
    call(TLVDrainKeyVal, &rdt, key, body, rdx);
    call(TLVinitlong, into, rdt, stackbuf);  // TODO adapt
    call(u8sFeed1, into, $len(key));
    call($u8feedall, into, key);
    while (!$empty(body)) {
        u8cs e = {};
        call(TLVDrain$, e, body);
        if (BRIXisentry(e) == OK) {
            call(BRIXflatfeed, into, e);
        } else {
            call($u8feedall, into, e);
        }
    }
    call(TLVendany, into, rdt, stackbuf);
    done;
}

ok64 _BRIXaddpatch(sha256* sha, BRIX* brix, u8cs rdx, u8csb heap) {
    sane(sha != nil && BRIXok(brix) && $ok(rdx));

    u64 roughlen = 0;
    call(BRIXenlist, heap, &roughlen, rdx);

    SSTu128 sst = {};
    int fd = FILE_CLOSED;
    aBcpad(u8, tmp, FILEmaxpathlen);
    call($u8feedall, tmpidle, brix->home);
    call($u8feedall, tmpidle, BRIKtmp);
    size_t size = roundup(roughlen + PAGESIZE, PAGESIZE);
    SKIPu8tab tab = {};
    call(SSTu128init, sst, &fd, tmpdata, size);

    while (!Bempty(heap)) {
        u8cs pop = {};
        call(HEAPu8csPop, &pop, heap);
        call(BRIXflatfeed, Bu8idle(sst), pop);
    }

    call(SSTu128end, sst, &fd, &tab);
    call(BRIKhash, sha, sst);
    call(SSTu128close, sst);

    aBcpad(u8, fn, FILEmaxpathlen);
    call(BRIKpath, fnidle, brix, sha);
    call(FILErename, tmpdata, fndata);

    done;
}

// Converts a nested RDX document (as produced by BRIXreget) into BRIX
// key-value form.
// Makes a *patch* SST for it (no deps), puts it on the stack.
ok64 BRIXaddpatch(sha256* sha, BRIX* brix, u8cs rdx) {
    sane(sha != nil && BRIXok(brix) && $ok(rdx));
    u8csb heap = {};
    try(Bu8csalloc, heap, BRIX_MAX_SST0_ENTRIES);
    then try(_BRIXaddpatch, sha, brix, rdx, heap);
    then try(BRIXadd, brix, sha);
    // TODO rm file on fail
    Bu8csfree(heap);
    done;
}

ok64 BRIXfind(sha256* sha, BRIX const* brix, u8cs part) {
    sane($ok(part) && BRIXok(brix) && sha != nil &&
         $len(part) <= sizeof(sha256) * 2);
    DIR* d = opendir((char*)*brix->home);
    testc(d != NULL, BRIXnone);
    for (struct dirent* de = readdir(d); de != NULL; de = readdir(d)) {
        a$strc(name, de->d_name);
        if ($len(name) != $len(BRIKext) + sizeof(sha256) * 2) continue;
        a$last(u8c, factext, name, $len(BRIKext));
        a$head(u8c, factsha, name, sizeof(sha256) * 2);
        if (!$eq(factext, BRIKext)) continue;
        u8 ndx = 0;
        while (ndx < $len(part) && $at(part, ndx) == $at(name, ndx)) ++ndx;
        if (ndx == $len(part)) {
            closedir(d);
            a$rawp(raw, sha);
            call(HEXdrain, raw, factsha);
            done;
        }
    }
    closedir(d);
    fail(BRIXnone);
}
