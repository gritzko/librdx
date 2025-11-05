//
// Created by gritzko on 10/25/25.
//

#include "BRIX2.h"

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"

a_cstr(BRIX_EXT, ".brik");

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

ok64 BRIXOpenHome(int* home, path8 path) {
    sane(home != NULL && $ok(path));
    struct stat st;
    ok64 o = FILEStat(&st, path);
    if (o == FILEnone) {
        call(FILEMakeDir, path);
    } else if (o != OK) {
        fail(o);
    } else {
        test((st.st_mode & S_IFDIR), FILEbadarg);
    }
    call(FILEOpenDir, home, path);
    done;
}

ok64 BRIXu8bOpen(u8bp brik, int home, sha256cp hash) {
    sane(Bok(brik) && home != FILE_CLOSED && !sha256empty(hash));
    a_pad(u8, fn, FILENAME_MAX);
    BRIXPath(fn_idle, hash);
    call(FILEMapROAt, brik, &home, fn);
    done;
}

ok64 BRIXu8bbOpen(u8bbp brix, int home, sha256cp hash) {
    sane(Bok(brix) && home != FILE_CLOSED && !sha256empty(hash));
    u8b top = {};
    call(BRIXu8bOpen, top, home, hash);
    sha256 base = {};
    call(BRIXu8bBase, top, &base);
    if (!sha256empty(&base)) {
        call(BRIXu8bbOpen, brix, home, &base);
    }
    call(u8bbFeed1, brix, top);
    done;
}

ok64 BRIXu8bbCreateTip(u8bbp brix, int* fd, int home, sha256cp base, u8cs tip) {
    sane(Bok(brix));
    a_path(fn, "");
    call(BRIXTipPath, fn, tip);
    u8b top = {};
    call(FILEMapNew, top, fd, fn, PAGESIZE);
    brikhead128 head = {
        .litS = 'S',
        .litT = 'T',
        .lit_crypto = BRIX_CRYPTO_NOSIG_SHA256,
        .lit_index = BRIX_INDEX_LSMHASH,
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

ok64 BRIXu8bbOpenTip(u8bbp brix, int* fd, int home, u8cs tip) {
    sane(Bok(brix));
    a_pad(u8, fn, FILENAME_MAX);
    call(BRIXTipPath, fn_idle, tip);
    u8b top = {};
    call(FILEMapRW, top, fd, fn);
    size_t pastlen = sizeof(brikhead128) + sizeof(sha256);
    call(u8sFed, u8bIdle(top), pastlen);
    call(u8sAte, u8bData(top));
    // ...
    call(u8bbFeed1, brix, top);
    done;
}

ok64 BRIXu8bCreate(u8bp brik, int home, sha256cs deps) {
    sane(brik != NULL && Bempty(brik));
    brikhead128 head = {
        .litS = 'S',
        .litT = 'T',
        .lit_crypto = BRIX_CRYPTO_ED25519_SHA256,
        .lit_index = BRIX_INDEX_2K,
        .meta_len = $size(deps),
        .data_len = 0,
    };
    a_rawc(headraw, head);
    call(u8bFeed, brik, headraw);
    $feed(u8bIdle(brik), deps);
    u8sAte(u8bData(brik));
    done;
}

ok64 BRIXu8bbSeal(u8bbp brix, int* fd, int home, sha256p result) {
    sane(Bok(brix) && fd != NULL && *fd != FILE_CLOSED && result != NULL);
    a_pad(sha256, deps, 32);
    u8b top = {};
    call(BRIXu8bCreate, top, home, deps_datac);
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
    return brikOK(brik) && BRIXu8bIndexType(brik) == BRIX_INDEX_LSMHASH;
}

ok64 BRIXu8bExtend(u8b brik) { sane(brikOK(brik)) done; }

ok64 BRIXu8bIdle(u8b tip, u8sp idle) {}

ok64 BRIXu8bPut1(u8b tip, rdxcp rec) {
    sane(tipOK(tip) && rec != NULL && RDXisPLEX(rec->type));
    u32sp idx = (u32**)u8bIdle(tip);
    // ...
    u8s idle;
    call(BRIXu8bIdle, tip, idle);
    // ...
    u8b mergeb;
    rdxpsc eqs;
    call(RDXu8bMergeLWW, mergeb, eqs);
    // ...
    a_pad(u8, head, 32);
    // ...
    call(u8bFeed, tip, head_datac);
    call(u8bFeed, tip, rec->plex);

    done;
}

ok64 BRIXu8bAdd(u8b tip, u8csc rec) {
    sane(brikOK(tip));
    rdx p;
    call(rdxInit, &p, rec);
    // POLnow();

    // use the clock
    // make the rec
    u8cs newrec;  // :(
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
