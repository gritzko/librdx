#include "BRIX.h"

#include <unistd.h>

#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "abc/TLV.h"
#include "rdx/CLI.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"
#include "rdx/RDXC.h"
#include "rdx/RDXZ.h"
#include "rdx/Y.h"

a$strc(BRIXhome, ".rdx/brix");

static const ok64 SUBsst = 0x37df8;
static const ok64 b0b = 0x26026;
static const ok64 SUBdeps = 0xa29d37;
static const ok64 SUBjdr = 0x2ea36;

ok64 BRIKfeedpath($u8 into, BRIX const* brix, h60 let);
ok64 BRIKhash(sha256* hash, SSTu128 sst);

static const u32 rdxmagic =
    (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) | ((u32)'x' << 24);

// [ ] add  (stdin)
// [x] add "file.rdx"
ok64 BRIX_add(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    $u8c path = {};
    id128 _;
    sha256 sha = {};
    aRDXid(clock, 0, b0b);
    if (RDXrdt(args) == RDX_STRING) {
        call(RDXCdrainS, path, &_, args);
        fail(notimplyet);
    } else if (RDXrdt(args) == RDX_TERM) {
        call(RDXCdrainT, path, &_, args);
        call(BRIXfind, &sha, brix, path);
    } else {
        fail(notimplyet);
    }

    call(BRIXadd, brix, &sha);
    done;
}

ok64 BRIX_repo(BRIX* brix) {
    sane(1);
    if (BRIXok(brix)) done;
    a$strc(path1, ".rdx");
    a$strc(path2, ".rdx/brix");
    if (FILEisdir(path1) != OK) call(FILEmakedir, path1);
    if (FILEisdir(path2) != OK) call(FILEmakedir, path2);
    call(BRIXopenrepo, brix, path2);
    done;
}

ok64 BRIX_patch(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    call(BRIX_repo, brix);

    aBcpad(Bu8, b, LSM_MAX_INPUTS);
    Bzero(bbuf);
    call(RDXingestall, bbuf, args);

    $eat(bdata) {  // TODO bundle routine
        sha256 sha = {};
        try(BRIXaddpatch, &sha, brix, Bu8cdata(**bdata));
    }

    $for(Bu8, b, bdata) Bu8unmap(*b);

    done;
}

// [x] get b0b-101
// [x] get:jdr b0b-101
// [ ] get b0b-101:field:123
ok64 BRIX_get(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    while (!$empty(args) && RDXrdt(args) != RDX_TERM) {
        u8 rdt = RDXrdt(args);
        if (rdt == RDX_REF) {
            id128 ref, _;
            $u8c rec = {};
            call(RDXCdrainR, &ref, &_, args);
            call(BRIXgetc, rec, brix, 0, ref);
            /*if (sub == SUBjdr) {
                Beat(brix->pad);
                call(JDRfeed, Bu8idle(brix->pad), rec);
                $mv(rec, Bu8data(brix->pad));
            }*/
            call(FILEfeed, STDOUT_FILENO, rec);
        } else {
            fail(notimplyet);
        }
    }
    done;
}

ok64 BRIX_reget(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    Bu8 gig = {};
    call(Bu8map, gig, GB);  // TODO pump
    while (!$empty(args) && RDXrdt(args) != RDX_TERM) {
        u8 rdt = RDXrdt(args);
        if (rdt == RDX_REF) {
            id128 ref, _;
            call(RDXCdrainR, &ref, &_, args);
            call(BRIXreget, Bu8idle(gig), brix, 0, ref);
            call(FILEfeedall, STDOUT_FILENO, Bu8cdata(gig));
        } else {
            fail(notimplyet);
        }
    }
    Bu8unmap(gig);
    done;
}

/*
// [ ] seal
ok64 BRIX_seal(BRIX* brix, $u8c args) {
    sane(1);
    test(Bdatalen(brix->ssts) > 0, BRIXnone);

    aBcpad($u8c, ins, LSM_MAX_INPUTS);
    aBpad2(sha256, deps, LSM_MAX_INPUTS);

    u64 base = {};
    if (Bpastlen(brix->ssts) > 0) {
        base = Blast(Bpast(brix->shas));
    }
    sha256 dummy = {};  // FIXME
    Bsha256feed1(depsbuf, dummy);
    Bsha256eatdata(depsbuf);
    // FIXME Bsha256feed$(depsidle, Bsha256cdata(brix->ids));
    $sha256sort(depsdata);

    a$dup(Bu8, news, BBu8data(brix->ssts));
    $eat(news) B$u8cfeed1(insbuf, Bu8cdata(**news));

    SSTu128 sst = {};
    int fd = FILE_CLOSED;
    aBcpad(u8, tmp, FILEmaxpathlen);
    call($u8feedall, tmpidle, Bu8c$1(brix->home));
    call($u8feedall, tmpidle, BRIKtmp);
    size_t sumsz = PAGESIZE;
    a$dup($u8c, ins, B$u8cdata(insbuf));
    $eat(ins) sumsz += $size(**ins);
    call(SSTu128init, sst, &fd, tmpdata, sumsz);
    SKIPu8tab tab = {};

    call(LSMmerge, Bu8idle(sst), B$u8cdata(insbuf), RDXZrevision, Y);

    call(SSTu128end, sst, &fd, &tab);
    sha256 sha = {};
    call(BRIKhash, &sha, sst);
    call(SSTu128close, sst);
    aBcpad(u8, sha, FILEmaxpathlen);
    h60 let = BRIXhashlet(&sha);
    call(BRIKfeedpath, shaidle, brix, let);
    call(FILErename, tmpdata, shadata);
    call(BRIKfeedpath, shaidle, brix, let);

    done;
}*/

ok64 BRIX_merge(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    sha256 sha = {};
    call(BRIXmerge, &sha, brix);

    aBcpad(u8, out, 128);
    a$rawc(raw, sha);
    call(HEXfeed, outidle, raw);
    call($u8feed1, outidle, '\n');
    call(FILEfeedall, STDOUT_FILENO, outdata);
    done;
}

ok64 BRIKfeedpath($u8 into, BRIX const* brix, h60 let);

// [x] open 40260b
// [ ] open "./.rdx/40260b...brik"
ok64 BRIX_open(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    call(BRIX_repo, brix);
    $u8c p = {};
    id128 _;
    sha256 sha = {};
    if (TLVup(**args) == RDX_STRING) {
        fail(notimplyet);
        call(RDXCdrainS, p, &_, args);
        if (BRIXok(brix)) {  // strip the path
        } else {             // open
        }
    } else if (TLVup(**args) == RDX_TERM) {
        if (!BRIXok(brix)) call(BRIXopenrepo, brix, BRIXhome);
        call(RDXCdrainT, p, &_, args);
        if ($len(p) == sizeof(sha256) * 2) {
            a$raw(raw, sha);
            call(HEXdrain, raw, p);
        } else if ($len(p) < sizeof(sha256) * 2) {
            call(BRIXfind, &sha, brix, p);
        } else {
            fail(BRIXbadarg);
        }
    } else {
    }
    call(BRIXopen, brix, &sha);
    done;
}

// [x] list
// [ ] list:deps
ok64 BRIX_list(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    BRIX* brix = (BRIX*)ctx;
    ok64 sub = 0;
    if (!$empty(args)) {
        $u8c base = {};
        id128 _ = {};
        call(RDXCdrainT, base, &_, args);
        call(RONdrain64, &sub, base);
    }
    if (sub == 0 || sub == SUBsst || sub == SUBdeps) {
        aBcpad(u8, out, LSM_MAX_INPUTS * sizeof(sha256) * 2 * 8);  // TODO
        aBusy(sha256, shas, brix->shas);
        sha256c* head = nil;
        sha256$c opened = Bsha256past(brix->shas);
        if (!$empty(opened)) head = $sha256last(opened);
        for (int i = 0; i < $len(shas); ++i) {
            sha256c* p = $atp(shas, i);
            a$rawcp(raw, p);
            u8 flag = (p == head) ? '*' : ' ';
            call($u8feed1, outidle, flag);
            call(HEXfeed, outidle, raw);
            call($u8feed1, outidle, '\n');
            if (sub == SUBdeps) {
                SSTu128* sst = Batp(brix->ssts, i);
                $sha256c deps = {};
                call(SSTu128meta, *sst, nil, (u8c$)deps);
                u8 flag = '>';
                $eat(deps) {
                    a$rawcp(depraw, *deps);
                    call($u8feed2, outidle, '\t', flag);
                    call(HEXfeed, outidle, depraw);
                    call($u8feed1, outidle, '\n');
                    flag = ' ';
                }
            }
        }
        call(FILEfeedall, STDOUT_FILENO, outdata);
    } else {
        fail(BRIXbadarg);
    }
    done;
}

CLIcmd COMMANDS[] = {
    {$u8str("open"), BRIX_open,
     $u8str("open a brix stack by its top brick hash")},  //
    {$u8str("merge"), BRIX_merge,
     $u8str("merge top of the stack into a brick")},  //
    {$u8str("add"), BRIX_add,
     $u8str("add more bricks on top of the stack")},  //
    {$u8str("patch"), BRIX_patch,
     $u8str("make a brick out of an RDX patch")},  //
    {$u8str("get"), BRIX_get,
     $u8str("get a flat RDX record by its key (id)")},  //
    {$u8str("reget"), BRIX_reget,
     $u8str("retrieve a tree by its root record id")},  //
    {$u8str("list"), BRIX_list,
     $u8str("list the stack's contents, bricks etc")},  //
    {$u8str(""), nil, nil},
};

ok64 BRIXcli() {
    sane(1);
    BRIX brix = {};
    a$dup($u8c, stdargs, B$u8cdata(STD_ARGS));
    aBcpad(u8, cmds, PAGESIZE);
    call(JDRdrainargs, cmdsidle);
    u8c$ cmds = cmdsdata;
    a$dup(u8c, path, BRIXhome);

    if (!$empty(cmds) && TLVup(**cmds) == RDX_STRING) {
        id128 id = {};
        call(RDXCdrainS, path, &id, cmds);
    }

    call(CLI, COMMANDS, &brix);

    // call(BRIXopenrepo, &brix, path);

    done;
}

MAIN(BRIXcli);
