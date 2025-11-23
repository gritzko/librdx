//
// Created by gritzko on 10/25/25.
//

#include "BRIX2.h"

#include <threads.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/MMAP.h"
#include "abc/NACL.h"
#include "abc/POL.h"
#include "abc/PRO.h"

a_cstr(BRIX_EXT, ".brik");

thread_local int BRIX_REPO = FILE_CLOSED;
thread_local u8b BRIX_REPO_KEY = {};

ok64 BRIXOpenRepo(path8 path) {
    sane(u8bOK(path));
    call(FILEOpenDir, &BRIX_REPO, path);
    a_path(key, "key");
    call(FILEMapROAt, BRIX_REPO_KEY, BRIX_REPO, key);
    done;
}

ok64 BRIXMakeRepo(path8 path) {
    sane(path8Sane(path));
    struct stat st;
    ok64 o = FILEStat(&st, path);
    if (o == FILEnone) {
        call(FILEMakeDir, path);
    } else if (o != OK) {
        fail(o);
    } else {
        test((st.st_mode & S_IFDIR), FILEbadarg);
    }
    call(FILEOpenDir, &BRIX_REPO, path);
    a_path(key, "key");
    call(FILEMapCreateAt, BRIX_REPO_KEY, BRIX_REPO, key, sizeof(edsec512));
    edpub256 pub;
    edsec512 sec;
    call(NACLed25519create, &pub, &sec);
    a_rawc(secraw, sec);
    call(u8bFeed, BRIX_REPO_KEY, secraw);
    done;
}

ok64 BRIXCloseRepo() {
    sane(1);
    call(FILEClose, &BRIX_REPO);
    call(FILEUnMap, BRIX_REPO_KEY);
    done;
}

ok64 BRIXPath(u8s pad, sha256cp hash) {
    sane($ok(pad) && hash != NULL);
    a_rawcp(raw, hash);
    call(HEXfeed, pad, raw);
    call(u8sFeed, pad, BRIX_EXT);
    done;
}

ok64 BRIXTipPath(path8 pad, u8csc tip) {
    sane($ok(pad) && $ok(tip));
    // todo dir
    call(u8sFeed, u8bIdle(pad), tip);
    call(u8sFeed, u8bIdle(pad), BRIX_EXT);
    done;
}

ok64 BRIXu8bBase(u8b brik, sha256p own) {
    if (u8bPastLen(brik) < sizeof(brikhead128) + sizeof(sha256)) {
        if (unlikely(Blen(brik) < 4)) return BRIXnoopen;
        zerop(own);
    } else {
        sha256mv(own, (sha256cp)(brik[0] + sizeof(brikhead128)));
    }
    return OK;
}

ok64 BRIXu8bOwn(u8b brik, sha256p own) {
    if (u8bPastLen(brik) < sizeof(brikhead128) + sizeof(sha256)) {
        if (unlikely(Blen(brik) < 4)) return BRIXnoopen;
        zerop(own);
    } else {
        sha256mv(own, (sha256cp)(brik[0] + sizeof(brikhead128)));
    }
    return OK;
}

ok64 BRIXu8bOpen(u8bp brik, sha256cp hash) {
    sane(Bok(brik) && BRIXIsRepoOpen() && !sha256empty(hash));
    a_pad(u8, fn, FILENAME_MAX);
    BRIXPath(fn_idle, hash);
    call(FILEMapROAt, brik, BRIX_REPO, fn);
    done;
}

ok64 BRIXu8bbOpen(u8bbp brix, sha256cp hash) {
    sane(Bok(brix) && BRIXIsRepoOpen() && !sha256empty(hash));
    u8b top = {};
    call(BRIXu8bOpen, top, hash);
    sha256 base = {};
    call(BRIXu8bBase, top, &base);
    if (!sha256empty(&base)) {
        call(BRIXu8bbOpen, brix, &base);
    }
    call(u8bbFeed1, brix, top);
    done;
}

ok64 BRIXu8bbCreateTip(u8bbp brix, sha256cp base, u8cs tip) {
    sane(Bok(brix));
    a_path(fn, "");
    call(BRIXTipPath, fn, tip);
    u8b top = {};
    call(FILEMapCreateAt, top, BRIX_REPO, fn, PAGESIZE);
    brikhead128 head = {
        .litS = 'S',
        .litT = 'T',
        .lit_crypto = BRIX_CRYPTO_NOSIG_SHA256,
        .lit_index = BRIX_INDEX_WALTZ_4,
        .meta_len = 32,
        .data_len = 0,
    };
    a_rawc(headraw, head);
    call(u8bFeed, top, headraw);
    a_rawcp(baseraw, base);
    call(u8bFeed, top, baseraw);
    call(u8sAte, u8bData(top));
    // ...
    call(u8bbFeed1, brix, top);
    done;
}

ok64 BRIXu8bbOpenTip(u8bbp brix, u8cs tip) {
    sane(Bok(brix));
    a_pad(u8, fn, FILENAME_MAX);
    call(BRIXTipPath, fn_idle, tip);
    u8b top = {};
    call(FILEMapRW, top, fn);
    size_t pastlen = sizeof(brikhead128) + sizeof(sha256);
    call(u8sFed, u8bIdle(top), pastlen);
    call(u8sAte, u8bData(top));
    // ...
    call(u8bbFeed1, brix, top);
    done;
}

ok64 BRIXu8bCreate(u8bp brik, sha256cs deps) {
    sane(brik != NULL && Bempty(brik));
    brikhead128 head = {
        .litS = 'S',
        .litT = 'T',
        .lit_crypto = BRIX_CRYPTO_ED25519_SHA256,
        .lit_index = BRIX_INDEX_OCTAB,
        .meta_len = $size(deps),
        .data_len = 0,
    };
    a_rawc(headraw, head);
    call(u8bFeed, brik, headraw);
    $feed(u8bIdle(brik), deps);
    u8sAte(u8bData(brik));
    done;
}

ok64 BRIXu8bResize2(u8b tip) {
    sane(u8bOK(tip));
    // fixme fd fd fd ?   FILE_BUFS!!!!
    // call(MMAPresize2, tip);
    int fd;
    call(FILEReMap, tip, Bsize(tip) * 2);  // fixme fd ?!!
    done;
}

ok64 BRIXu8bbSeal(u8bbp brix, int* fd, sha256p result) {
    sane(Bok(brix) && fd != NULL && *fd != FILE_CLOSED && result != NULL);
    a_pad(sha256, deps, 32);
    u8b top = {};
    call(BRIXu8bCreate, top, deps_datac);
    u32sp idx = (u32**)u8bIdle(top);
    size_t idxlen;
    a_pad(u8cs, inputs, RDX_MAX_INPUTS);
    u8cssp ins = u8csbData(inputs);
    eats(u32c, i, idx) {
        if (*i != 0) {
            //...
            u8cs rec;
            u8csbFeed1(inputs, rec);
        } else if (!$empty(ins)) {
            call(RDXu8sMerge, u8bIdle(top), ins);
        }
    }
    // ...
    //...
    // u8sCopy(u8bPast(top), headraw);
    // u8sCopy(u8bPast(top), headraw);
    call(FILEClose, fd);

    done;
}

fun b8 tipOK(u8b brik) {
    return brikOK(brik) && BRIXu8bIndexType(brik) <= BRIX_INDEX_WALTZ_4;
}

ok64 BRIXu8bExtend(u8b brik) {
    sane(brikOK(brik));
    done;
}

ok64 BRIXu8bWaltzIndexLen(u8bp tip, u64p len) {
    if (!brikOK(tip)) return badarg;
    u8 t = BRIXu8bIndexType(tip);
    if (t > '9') return badarg;
    u8 shift = t - '0';
    *size = Bsize(tip) >> shift;
    return OK;
}

ok64 BRIXu8bWaltzFind(u8bp brik, rdxs recs, ref128 ref) {
    sane(brikOK(brik) && rdxsOK(recs));
    u64 idxlen = 0;
    call(BRIXu8bWaltzIndexLen, brik, &idxlen);
    test(idxlen * sizeof(u32) >= u8bIdleLen(brik), FAILsanity);
    a_tail(u32, index, u8bIdle(brik), idxlen);
    size_t indx = ref128Hash(&ref) % $len(index);
    u8csp data = u8bDataC(brik);
    while ($at(index, indx) != 0) {
        u32 off = $at(index, indx);
        a_tail(u8c, rec, data, off);
        rdx r;
        call(rdxInit, &r, rec);
        if (ref128Eq(&r.id, &ref)) {
            call(rdxsFeed1, recs, r);
        }
        indx = (indx + 1) % $len(index);
    }
    done;
}

ok64 BRIXu8bOctabFind(u8bp brik, rdxs recs, ref128 ref) { return notimplyet; }

ok64 BRIXu8bBloomFind(u8bp brik, rdxs recs, ref128 ref) { return notimplyet; }

ok64 BRIXu8bFind(u8bp brik, rdxs recs, ref128 ref) {
    u8 t = BRIXu8bIndexType(brik);
    if (t == BRIX_INDEX_OCTAB) {
        return BRIXu8bOctabFind(brik, recs, ref);
    } else if (t <= BRIX_INDEX_WALTZ_MAX) {
        return BRIXu8bWaltzFind(brik, recs, ref);
    } else if (t == BRIX_INDEX_BLOOM) {
        return BRIXu8bBloomFind(brik, recs, ref);
    }
    return notimplyet;
}

ok64 BRIXu8bbFind(u8bbp brix, rdxs recs, ref128 ref) {
    sane(u8bbOK(brix) && rdxsOK(recs));
    eats(u8b, b, u8bbData(brix)) { call(BRIXu8bFind, *b, recs, ref); }
    done;
}

fun ok64 BRIXu8bWaltzLocate(u8b brik, u32sp idx, u8sp idle) {

}

fun u32 BRIXu8bWaltzLen(u8bp brik) { return Bat(brik, 2); }

// fixme hash, block, off  CtrlC
fun ok64 BRIXrdxsWaltzScan(rdxs entries, u32csc idx, u8csc data, ref128 id) {}

// [ ] waltz length, not 1/k
// [ ] replace-then-clean!  shift backwards
// [ ] flush buckets
// [ ] append or replace
// [ ] salt?
// [ ] fixme old maps / seal (never walz ptrs) / idoff! (up4)
// [ ] state
// [ ] put1 always y
// [ ] ladder
// [ ] rdxs ins/outs
// [ ] revscan, no TS
// [ ] u64 idx, u16 idxoff
ok64 BRIXu8bPut1(u8b brik, rdxcp put) {
    sane(tipOK(brik) && put != NULL && RDXisPLEX(put->type));
    u64 idxlen = 0;
    call(BRIXu8bWaltzIndexLen, brik, &idxlen);
    test(idxlen * sizeof(u32) >= u8bIdleLen(brik), FAILsanity);
    a_tail(u32, index, u8bIdle(brik), idxlen);
    a_head(u8, idle, u8bIdle(brik), idlelen);
    size_t indx = ref128Hash(&put->id) % $len(index);
    u8csp data = u8bDataC(brik);
    a_pad(rdxp, ins, RDX_MAX_INPUTS);
    rdxpbFeed1(ins, (rdxp)put);
    BRIX_INDEX_WALTZ_MAX;  // todo 'z' len
    // scan till 0/128  *dyn*
    //   find matches (limit to 4?)
    //   rdxps 8b * MAX_STACK 256b
    //   u32s  4b 128b
    //   rdxs  64b * MAX_STACK 2K
    //   ow? ate, break
    // have ins? add, merge
    //  else feed
    // ow
    // clear the merged
    while ($at(index, indx) != 0) {
        u32 off = $at(index, indx);
        a_tail(u8c, rec, data, off);
        rdx r;
        call(rdxInit, &r, rec);
        if (ref128Eq(&r.id, &put->id)) {
            if (r.reclen <= put->reclen) {  // mere
                rdxbFeed1(ins, r);
            }
        }
        indx = (indx + 1) % $len(index);
    }
    if (rdxpbDataLen(ins) != 0) {
        a_bs(u8, mergeb, idle);
        call(RDXu8bMergeLWW, mergeb, eqs);
        // clean-up
    } else {
        u32 off = u8bDataLen(brik);
        u8cs rec;
        rdxRecord(rdxcp, rec);
        call(u8sFeed, idle, rec);
        $at(index, indx) = off;
    }

    u64 idxlen = 0;
    call(BRIXu8bWaltzIndexLen, tip, &idxlen);
    a_tail(u32, index, u8bIdle(tip), idxlen);
    size_t indx = ref128Hash(&rec->id) % $len(index);

    a_pad(rdx, inputs, RDX_MAX_INPUTS);
    a_pad(rdxp, eqs, RDX_MAX_INPUTS);
    while ($at(index, indx) != 0) {
        u8cs in;
        rdx* h = $head(inputs_idle);
        call(rdxInit, h, in);
        if (ref128Eq(&h->id, &rec->id)) {
            call(rdxpbFeed1, eqs, h);
            rdxsFed1(inputs_idle);
        }
        indx = (indx + 1) % $len(index);
    }

    u8b mergeb;
    rdxpsc eqs;

    // ...
    a_pad(u8, head, 32);
    // ...
    call(u8bFeed, tip, head_datac);
    call(u8bFeed, tip, rec->plex);

    $at(idx, indx) = 0;
    done;
}

ok64 BRIXu8bGet(u8b brik, ref128 key, u8csp val) {
    sane(brikOK(brik) && u8csOK(val));
    u8 t = BRIXu8bIndexType(brik);
    if (t == BRIX_INDEX_OCTAB) {
    } else if (t <= BRIX_INDEX_WALTZ_4) {
    } else {
        fail(notimplyet);
    }
    done;
}

ok64 BRIXu8bAdd(u8b tip, u8csc rec) {
    sane(brikOK(tip) && BRIXIsRepoOpen());
    rdx p;
    call(rdxInit, &p, rec);
    call(BRIXRepoTime, &p.id);
    call(BRIXu8bPut1, tip, &p);
    done;
}

ok64 BRIXu8bClose(u8bp b) { return FILEUnMap(b); }

ok64 BRIXu8bbClose(u8bbp bx) {
    sane(Bok(bx));
    eats(u8b, b, bx) { call(BRIXu8bClose, *b); }
    Breset(bx);
    done;
}

ok64 BRIXu8bbCloseTip(u8bbp bx, int* fd) {
    sane(Bok(bx) && fd != NULL && *fd != FILE_CLOSED);
    call(FILEClose, fd);
    call(BRIXu8bbClose, bx);
    done;
}

ok64 BRIXu8bAppend(u8b tip, u8csb recs) { return notimplyet; }

ok64 BRIXu8bbGets(u8bb brix, ref128 key, u8css intos) { return notimplyet; }

ok64 BRIXu8bbGet(u8bb brix, ref128 key, u8s into) { return notimplyet; }
