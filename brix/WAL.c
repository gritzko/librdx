#include "WAL.h"

#include <limits.h>

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/SHA.h"
#include "abc/SST.h"
#include "abc/TLV.h"
#include "rdx/RDX.h"
#include "rdx/Y.h"

a$strc(WALmagic, "RDXWAL00");

ok64 WALcreate(WAL* wal, $cu8c filename, u64 logsz) {
    sane(wal != nil && !WALok(wal) && logsz <= (1UL << 32));
    aBcpad(u8, wp, FILEmaxpathlen);
    int fd = FILE_CLOSED;
    logsz = roundup(logsz, FILEminlen);
    call(FILEmapnew, wal->log, &fd, filename, logsz);
    call(Bu8feed$, wal->log, WALmagic);
    Bu8eat$1(wal->log);
    call(FILEclose, &fd);
    call(Bfly256map, wal->idx, logsz / sizeof(fly256));
    Bfly256eat$2(wal->idx);
    done;
}

ok64 WALscan(WAL* wal) {
    sane(WALok(wal) && $len(Bu8data(wal->log)) == 0);
    u8c$ log = (u8c**)Bu8idle(wal->log);
    u64 loglen = $len(log);
    fly256$ idx = Bfly256data(wal->idx);
    while (!$empty(log)) {
        if (!RDXisFIRST(**log) && !RDXisPLEX(**log)) break;
        u8 t = 0;
        fly256 rec = {};
        $u8c val = {};
        u64 at = loglen - $len(log) + $len(WALmagic);
        call(RDXdrain, &t, &rec.id, val, log);
        mute(HASHnone, HASHfly256get, &rec, idx);
        int i = 0;
        while (i < 4 && rec.recs[i] != 0) ++i;
        if (i < 4) {
            rec.recs[i] = at;
        } else {
            zero(rec.recs);
            rec.recs[0] = at;
        }
        call(HASHfly256put, idx, &rec);
    }
    done;
}

ok64 WALopen(WAL* wal, $cu8c filename) {  // TODO ro
    sane(wal != nil && !WALok(wal));
    aBcpad(u8, wp, FILEmaxpathlen);
    int fd = FILE_CLOSED;
    call(FILEopen, &fd, filename, O_RDWR);
    u64 logsz = 0;
    call(FILEsize, &logsz, &fd);
    test(0 == (logsz & (FILEminlen - 1)), WALbadlen);
    call(FILEmap, wal->log, &fd, PROT_READ | PROT_WRITE);
    call(FILEclose, &fd);
    Breset(wal->log);
    call($u8eat, Bu8idle(wal->log), $len(WALmagic));
    call($u8eat, Bu8data(wal->log), $len(WALmagic));
    test($eq(Bu8past(wal->log), WALmagic), WALbad);
    call(Bfly256map, wal->idx, logsz);
    Bfly256eat$2(wal->idx);
    call(WALscan, wal);
    done;
}

ok64 WALadd1(WAL* wal, $cu8c rdx) {
    sane(WALok(wal));
    u64 at = Busylen(wal->log);
    fly256$ idx = Bfly256data(wal->idx);
    u8$ idle = Bu8idle(wal->log);
    fly256 rec = {};
    u8 t = **rdx & ~TLVaA;
    test(RDXisFIRST(t) || RDXisPLEX(t), RDXbad);
    call(RDXid, &rec.id, rdx);
    mute(HASHnone, HASHfly256get, &rec, idx);
    int i = 0;
    while (i < 4 && rec.recs[i] != 0) ++i;
    if (i < 4) {
        call($u8feedall, idle, rdx);
        rec.recs[i] = at;
    } else {
        aBpad2($u8c, ins, 5);
        call(WALget, insidle, wal, rec.id);
        u64 at = Busylen(wal->log);
        a$dup(u8c, dup, rdx);
        call($$u8cfeed1, insidle, dup);
        call(Y, idle, insdata);
        zero(rec.recs);
        rec.recs[0] = at;
        if (!$empty(idle)) *$head(idle) = 0;
    }
    call(HASHfly256put, idx, &rec);
    done;
}

// may return XYZnoroom if either the log or the index overfill
ok64 WALadd(WAL* wal, $u8c rdx) {
    sane(WALok(wal));
    a$dup(u8c, dup, rdx);
    while (!$empty(rdx)) {
        $u8c next = {};
        u8 t = 0;
        call(RDXdrain$, &t, next, dup);
        call(WALadd1, wal, next);
        $mv(rdx, dup);
    }
    done;
}

ok64 WALget($$u8c ins, WAL const* wal, id128 id) {
    sane($ok(ins) && WALok(wal));
    fly256$ idx = Bfly256data(wal->idx);
    fly256 ir = {.id = id};
    call(HASHfly256get, &ir, idx);
    for (int i = 0; i < 4 && ir.recs[i] != 0; ++i) {
        u64 off = ir.recs[i] - $len(WALmagic);
        a$tail(u8c, tail, Bu8data(wal->log), off);
        $u8c rec = {};
        call(TLVdrain$, rec, tail);
        call($$u8cfeed1, ins, rec);
    }
    done;
}

ok64 WALget1($u8 res, WAL const* wal, id128 id) {
    sane($ok(res) && wal != nil && !id128empty(id));
    aBpad2($u8c, ins, LSM_MAX_INPUTS);
    call(WALget, insidle, wal, id);
    /*size_t sz = 0;
    $for($cu8c, p, insdata) sz += $len(p);
    sz = roundup(sz, PAGESIZE);*/
    call(Y, res, insdata);
    done;
}

a$strc(WALtmp, ".wal.brik");
ok64 BRIKhash(sha256* hash, SSTu128 sst);

ok64 WAL2brick(sha256* sha, sha256c* top, WAL* wal, $cu8c home) {
    sane(sha != nil && WALok(wal));
    aBpad2($u8c, ins, LSM_MAX_INPUTS);
    int fd = FILE_CLOSED;
    SSTu128 sst = {};
    aBcpad(u8, tmp, FILEmaxpathlen);
    call($u8feedall, tmpidle, home);
    call($u8feedall, tmpidle, WALtmp);
    u64 sstlen = Bu8len(wal->log) + sizeof(sha256) + sizeof(SSTheader);
    call(SSTu128init, sst, &fd, tmpdata, sstlen);
    Bu128 ids = {};
    call(Bu128map, ids, Bfly256len(wal->idx));
    // TODO fails
    a$rawcp(sharaw, sha);
    call($u8feedall, Bu8idle(sst), sharaw);
    Bu8eat$1(sst);
    $for(fly256, f, Bfly256data(wal->idx)) if (!id128empty(f->id))
        Bu128feed1(ids, f->id);
    $u128sortfn(Bu128data(ids), id128cmp);
    SKIPu8tab tab = {};
    $for(id128, p, Bu128data(ids)) {
        call(WALget1, Bu8idle(sst), wal, *p);
        call(SKIPu8mayfeed, sst, &tab);
    }
    call(Bu128unmap, ids);
    call(BRIKhash, sha, sst);
    call(SSTu128end, sst, &fd, &tab);

    aBcpad(u8, path, FILEmaxpathlen);
    call($u8feedall, pathidle, home);
    a$rawc(raw, *sha);
    call(HEXfeedall, pathidle, raw);
    call($u8feedall, pathidle, BRIKext);
    call(FILErename, tmpdata, pathdata);

    done;
}

ok64 WALclose(WAL* wal) {
    sane(WALok(wal));
    call(Bu8unmap, wal->log);
    call(Bfly256unmap, wal->idx);
    done;
}
