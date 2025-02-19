#include "BRIX.h"

#include <unistd.h>

#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "abc/TLV.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"
#include "rdx/RDXC.h"

enum {
    BRIXargver = 1 << 0,
    BRIXargobj = 1 << 1,
    BRIXargsst = 1 << 2,
    BRIXargpath = 1 << 3,
};

enum {
    BRIXcmdnone,
    BRIXcmdinit,
    BRIXcmdsee,
    BRIXcmdshow,
    BRIXcmddump,
    BRIXcmdget,
    BRIXcmdlen,
};

$u8c BRIX_CMDS[] = {
    $u8str(""),     $u8str("init"), $u8str("see"),
    $u8str("show"), $u8str("dump"), $u8str("get"),
};

con ok64 BRIXbadverb = 0xb6d2866968ea9da6;

ok64 JDRdrain$($u8 into, $$u8c jdr) {
    a$dup($u8c, j, jdr);
    id128 id128zero = {};
    while (!$empty(j)) {
        a$dup(u8, dup, into);
        u8c$ next = **j;
        ++*j;
        ok64 o = JDRdrain(into, next);
        if (ok64is(o, noroom)) return o;
        if (o != OK) {
            $mv(into, dup);
            o = RDXfeed(into, RDX_STRING, id128zero, next);
            if (o != OK) return o;
        }
    }
    return OK;
}

ok64 BRIXdoinit(h60* let, BRIX* brix, $$u8c args) {
    sane(1);
    a$strc(path, ".");
    if (!$empty(args)) $mv(path, **args);
    call(BRIXinit, brix, path);
    done;
}

// @branch see @version
// see file.rdx
ok64 BRIXdosee(h60* let, BRIX* brix, $$u8c args) {
    sane(1);
    while (!$empty(args)) {
        u8c$ a = **args;
        ++*args;
        if ($empty(a)) {
            continue;
        }

        if (**a == '@') {
            ++*a;
            id128 id = {};
            call(RDXid128drain, &id, a);
            fail(notimplyet);
            continue;
        }

        sha256 hash = {};
        a$raw(raw, hash);
        ok64 ho = HEXdrain(raw, a);
        if (ho == OK) {  // hash
            fail(notimplyet);
            continue;
        }

        Bu8 rdx = {};
        int fd = FILE_CLOSED;
        call(FILEmapro, rdx, &fd, a);
        call(FILEclose, &fd);
        call(BRIXpatch, let, brix, Bu8cdata(rdx));
        call(FILEunmap, rdx);
    }

    done;
}

ok64 BRIXdoget(h60* let, BRIX* brix, $u8c args) {
    sane(1);
    while (!$empty(args)) {
        $u8c arg;
        call(TLVdrain$, arg, args);
        u8 rdt = **arg & ~TLVaA;
        switch (rdt) {
            case RDX_REF: {
                id128 id = {}, v = {};
                call(RDXCdrainR, &id, &v, arg);
                $u8c got = {};
                call(BRIXget, got, brix, 0, id);
                call(FILEfeedall, STDOUT_FILENO, got);
                break;
            }
            default:
                fail(notimplyet);
        }
    }

    done;
}
ok64 BRIXdoshow(h60* let, BRIX* brix, $$u8c args) {
    sane(1);
    while (!$empty(args)) {
        u8c$ a = **args;
        ++*args;
        if ($empty(a)) {
            continue;
        }

        if (**a == '@') {
            ++*a;
            id128 id = {};
            call(RDXid128drain, &id, a);
            aBcpad(u8, tmp, PAGESIZE);
            call(BRIXproduce, tmpidle, brix, 0, id);
            call(FILEfeedall, STDOUT_FILENO, tmpdata);
            continue;
        } else {
            fail(notimplyet);
        }
    }

    done;
}

ok64 BRIXdodump(h60* let, BRIX* brix, $$u8c args) {
    sane(1);
    Breset(brix->pad);
    a$dup(Bu8, chunks, BBu8data(brix->store));
    while (!$empty(chunks)) {
        u8B chunk = (u8B) * --(chunks[1]);
        a$dup(u8c, rdx, Bu8c$1(chunk));
        call(JDRfeed, Bu8idle(brix->pad), rdx);
        // TODO piecemeal
    }
    call(FILEfeed, STDOUT_FILENO, Bu8c$1(brix->pad));
    done;
}

ok64 BRIXobject() {
    sane(1);
    BRIX brix = {};
    $u8c$ args = B$u8cdata(STD_ARGS);
    ++*args;  // program
    h60 res = {};
    a$strc(path, ".");
    if (!$empty(args) && !$empty(**args) &&
        (****args == '.' || ****args == '/')) {
        $mv(path, **args);
        ++*args;
    }
    int verb = 0;

    call(BRIXopen, &brix, path);

    while (!$empty(args)) {
        u8c$ a = **args;
        ++*args;
        if ($empty(a)) {
            ;
        } else if (**a == '@') {
            ++*a;
            id128 id = {};
            call(RDXid128drain, &id, a);
            if (id128src(id) == 0) {  // @branch
                u64swap(id._64, id._64 + 1);
            }
            call(BRIXpushrev, &brix, id);
        } else if (**a == '*') {
            ++*a;
            h60 hashlet = {};
            a$raw(raw, hashlet);
            call(RONdrain64, &hashlet, a);
            call(BRIXpush, &brix, hashlet);
        } else {
            int v = 1;
            while (v < BRIXcmdlen && !$eq(BRIX_CMDS[v], a)) ++v;
            if (v < BRIXcmdlen) {
                verb = v;
                break;
            }
            if (v == BRIXcmdlen) fail(BRIXbadverb);
        }
    }

    a$dup($u8c, args2, args);
    aBcpad(u8, rdx, PAGESIZE);
    call(JDRdrain$, rdxidle, args2);

    switch (verb) {
        case BRIXcmdnone:  // subject
            break;
        case BRIXcmdinit:
            return BRIXdoinit(&res, &brix, args);
        case BRIXcmdsee:
            return BRIXdosee(&res, &brix, args);
        case BRIXcmdshow:
            return BRIXdoshow(&res, &brix, args);
        case BRIXcmddump:
            return BRIXdodump(&res, &brix, args);
        case BRIXcmdget:
            return BRIXdoget(&res, &brix, rdxdata);
        default:
            fail(notimplyet);
    }

    if (res != 0) {
        aBcpad(u8, hash, 16);
        BRIXfeedh60(hashidle, res);
        $println(hashdata);
    }
    call(BRIXclose, &brix);
    done;
}

MAIN(BRIXobject);
