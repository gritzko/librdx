#include "BRIX.h"

#include <unistd.h>

#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "abc/SST.h"
#include "abc/TLV.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"
#include "rdx/RDXC.h"
#include "rdx/RDXY.h"
#include "rdx/RDXZ.h"

ok64 JDRdrainargs($u8 into, $$u8c jdr) {
    a$dup($u8c, j, jdr);
    id128 id128zero = {};
    while (!$empty(j)) {
        a$dup(u8, dup, into);
        a$dup(u8c, next, **j);
        ok64 o = JDRdrain(into, next);
        if (ok64is(o, noroom)) return o;
        if (o != OK) {
            $mv(into, dup);
            o = RDXfeed(into, RDX_STRING, id128zero, **j);
            if (o != OK) return o;
        }
        ++*j;
    }
    return OK;
}

extern $u8c BRIKtmp;
ok64 BRIKfeedpath($u8 into, BRIX const* brix, h60 let);
ok64 BRIKhash(sha256* hash, SSTu128 sst);

// ...........................

// [x] init
// [x] init "path"
ok64 BRIX_init(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane($ok(args));
    a$strc(path, ".");
    if (1) {
        id128 id = {};
        call(RDXCdrainS, path, &id, args);
    }
    call(BRIXinit, brix, path);
    done;
}

ok64 BRIKhash(sha256* hash, SSTu128 sst) {
    sane(hash != nil && Bok(sst));
    // aBusy(u8c, hashed, sst); strange compiler glitch?
    $u8c hashed = {};
    hashed[0] = sst[0];
    hashed[1] = sst[2];
    SHAsum(hash, hashed);
    done;
}

con ok64 b0b = 0x26026;

static const u32 rdxmagic =
    (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) | ((u32)'x' << 24);

// [ ] add  (stdin)
// [x] add "file.rdx"
// [ ] add:jdr "file.jdr"
// [ ] add:txt "file.txt"
ok64 BRIX_add(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane($ok(args));
    $u8c path = {};
    id128 _;
    aRDXid(clock, 0, b0b);
    if (RDXrdt(args) == RDX_STRING) {
        call(RDXCdrainS, path, &_, args);
    }

    int fd = FILE_CLOSED;
    Bu8 rdx = {};
    SSTu128 tmp = {};
    SSTheader head = {.magic = rdxmagic, .metalen = sizeof(head)};
    a$rawc(rawhead, head);

    call(FILEmapro, rdx, &fd, path);

    then try(Bu8map, tmp, roundup(Busylen(rdx) * 2, PAGESIZE));
    then try($u8feed, Bu8idle(tmp), rawhead);
    then Bu8eatdata(tmp);
    // TODO !!! stamping
    then try(BRIXfromRDX, Bu8idle(tmp), &clock, Bu8cdata(rdx));
    sha256 hash = {};
    BRIKhash(&hash, tmp);
    then try(BBu8feed1, brix->ssts, tmp);  // at this point, becomes effective
    then try(Bu64feed1, brix->ids, BRIXhashlet(&hash));
    // TODO hash

    FILEunmap(rdx);
    FILEclose(&fd);
    nedo Bu8unmap(tmp);
    done;
}

// [x] get b0b-101
// [ ] get b0b-101:field:123
// [ ] get:all b0b-101
// [ ] get:txt b0b-101
// [ ] get:jdr b0b-101
// [ ] get:dir b0b-44
ok64 BRIX_get(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    $u8c rec = {};
    if (!id128empty(id)) {
        call(BRIXget, rec, brix, 0, id);
        call(FILEfeed, STDOUT_FILENO, rec);
    }
    while (!$empty(args) && RDXrdt(args) != RDX_TERM) {
        u8 rdt = RDXrdt(args);
        if (rdt == RDX_REF) {
            id128 ref, _;
            call(RDXCdrainR, &ref, &_, args);
            call(BRIXget, rec, brix, 0, ref);
            call(FILEfeed, STDOUT_FILENO, rec);
        } else {
            fail(notimplyet);
        }
    }
    done;
}

// [ ] seal
ok64 BRIX_seal(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    test(Bdatalen(brix->ssts) > 0, BRIXnone);

    aBcpad($u8c, ins, LSM_MAX_INPUTS);
    aBpad2(sha256, deps, LSM_MAX_INPUTS);

    u64 base = {};
    if (Bpastlen(brix->ssts) > 0) {
        base = Blast(Bpast(brix->ids));
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

    call(LSMmerge, Bu8idle(sst), B$u8cdata(insbuf), RDXZrevision, RDXY);

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
}

// [ ] merge Branch
// [ ] merge Branch-seq
// [ ] merge Hash1e7
// [ ] merge:branch Branch
// [ ] merge:version Branch-seq
// [ ] merge:sst Hash1e7
// [ ] merge:sst "./path/file.sst"
ok64 BRIX_merge(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    done;
}

ok64 BRIKfeedpath($u8 into, BRIX const* brix, h60 let);

// [ ] open Branch
// [ ] open:branch Branch
// [ ] open Branch-101
// [ ] open:version Branch-101
// [ ] open Hash1e7
// [ ] open:sst Hash1e7
ok64 BRIX_open(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    while (Busylen(brix->ssts)) {
        Bu8* last = Blastp(brix->ssts);
        Bu8unmap(*last);
        BBu8pop(brix->ssts);
        Bu64pop(brix->ids);
    }

    con ok64 sst = 0x37df8;
    if (sub == 0 || sub == sst) {
        $u8c s = {};
        id128 _;
        u8 t;
        u64 hashlet = 0;
        call(RDXdrain, &t, &_, s, args);
        test(t == RDX_TERM, BRIXbadarg);
        call(RONdrain64, &hashlet, s);
        SSTu128 sst = {};
        aBcpad(u8, path, FILEmaxpathlen);
        call(BRIKfeedpath, pathidle, brix, hashlet);
        call(SSTu128open, sst, pathdata);
        call(BBu8feed1, brix->ssts, sst);
        call(Bu64feed1, brix->ids, hashlet);

        BBu8eatdata(brix->ssts);
        Bu64eatdata(brix->ids);
    } else {
        fail(notimplyet);
    }
    done;
}

// [ ] fork
// [ ] fork NewBranch
ok64 BRIX_fork(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    kv64B idx = (kv64B)brix->index;
    u64 name = (random()) & u60max;
    u64 head = 0;
    if (1 && TLVup(**args) == RDX_REF) {
    }
    kv64 headrec = {name | BRIX_BRANCH_HEAD, head};
    call(HASHkv64put, idx, &headrec);
    done;
}

// [x] list
// [x] list:sst
// [ ] list:rdx
// [ ] list:jdr
// [ ] list:head
ok64 BRIX_list(BRIX* brix, id128 id, ok64 sub, $u8c args) {
    sane(1);
    con ok64 sst = 0x37df8;
    if (sub == 0 || sub == sst) {
        aBcpad(u8, out, LSM_MAX_INPUTS * sizeof(sha256) * 3);
        a$strc(line, "...head...\n");
        for (u64* p = brix->ids[0]; p < brix->ids[2]; ++p) {
            if (p == brix->ids[1]) {
                call($u8feed, outidle, line);
            }
            a$rawcp(raw, p);
            call(RONfeed64, outidle, *p);
            call($u8feed1, outidle, '\t');
            // TODO call(HEXfeed, outidle, raw);
            call($u8feed1, outidle, '\n');
        }
        call(FILEfeedall, STDOUT_FILENO, outdata);
    } else {
        fail(BRIXbadarg);
    }
    /*
        Breset(brix->pad);
        a$dup(Bu8, chunks, BBu8data(brix->store));
        while (!$empty(chunks)) {
            u8B chunk = (u8B) * --(chunks[1]);
            a$dup(u8c, rdx, Bu8c$1(chunk));
            call(JDRfeed, Bu8idle(brix->pad), rdx);
            // TODO piecemeal
        }
        call(FILEfeed, STDOUT_FILENO, Bu8c$1(brix->pad));
        */
    done;
}

typedef ok64 (*cmdfn)(BRIX* brix, id128 id, ok64 sub, $u8c args);

typedef struct {
    $u8c name;
    cmdfn fn;
} cmd_t;

cmd_t COMMANDS[] = {
    {$u8str("init"), BRIX_init},    //
    {$u8str("open"), BRIX_open},    //
    {$u8str("merge"), BRIX_merge},  //
    {$u8str("add"), BRIX_add},      //
    {$u8str("commit"), BRIX_seal},  //
    {$u8str("seal"), BRIX_seal},    //
    {$u8str("fork"), BRIX_fork},    //
    {$u8str("get"), BRIX_get},      //
    {$u8str("list"), BRIX_list},    //
    {$u8str(""), nil},
};

ok64 BRIXcli() {
    sane(1);
    BRIX brix = {};
    a$dup($u8c, stdargs, B$u8cdata(STD_ARGS));
    ++*stdargs;  // program name
    aBcpad(u8, cmds, PAGESIZE);
    call(JDRdrainargs, cmdsidle, stdargs);
    u8c$ cmds = cmdsdata;
    a$strc(path, ".");

    if (!$empty(cmds) && TLVup(**cmds) == RDX_STRING) {
        id128 id = {};
        call(RDXCdrainS, path, &id, cmds);
    }

    while (!$empty(cmds)) {
        u8 t = 0;
        id128 id = {};
        $u8c val = {};
        $u8c verb = {};
        u64 sub = 0;
        call(RDXdrain, &t, &id, val, cmds);
        if (t == RDX_TUPLE) {
            id128 _;
            test(!$empty(val), BRIXbadarg);
            call(RDXdrain, &t, &_, verb, val);
            test(t == RDX_TERM, BRIXnoverb);
            if (!$empty(val)) {
                $u8c s = {};
                call(RDXdrain, &t, &_, s, val);
                test(t == RDX_TERM, BRIXbadarg);
                call(RONdrain64, &sub, s);
            }
        } else if (t == RDX_TERM) {
            $mv(verb, val);
        } else {
            fail(BRIXnoverb);
        }
        int v = 0;
        while (!$eq(COMMANDS[v].name, verb)) {
            ++v;
            test(!$empty(COMMANDS[v].name), BRIXnoverb);
        }
        if (v != 0 && Bempty(brix.home)) {
            call(BRIXopen, &brix, path);
        }

        call(COMMANDS[v].fn, &brix, id, sub, cmds);
    }
    test($empty(cmds), BRIXbadarg);

    done;
}

MAIN(BRIXcli);
