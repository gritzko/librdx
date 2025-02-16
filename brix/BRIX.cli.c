#include "BRIX.h"

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"

enum {
    BRIXargver = 1 << 0,
    BRIXargobj = 1 << 1,
    BRIXargsst = 1 << 2,
    BRIXargpath = 1 << 3,
};

typedef struct {
    id128 version;
    id128 object;
    sha256 sst;
    $u8c path;
} BRIXarg;

fun u8 BRIXargbits(BRIXarg const* args) {
    u8 ret = 0;
    if (!id128empty(args->version)) ret |= BRIXargver;
    if (!id128empty(args->object)) ret |= BRIXargobj;
    if (!sha256empty(&args->sst)) ret |= BRIXargsst;
    if (!$empty(args->path)) ret |= BRIXargpath;
    return ret;
}

enum {
    BRIXcmdnone,
    BRIXcmdinit,
    BRIXcmdsee,
    BRIXcmdlen,
};
$u8c BRIX_CMDS[] = {
    $u8str(""),
    $u8str("init"),
    $u8str("see"),
};

BRIX brix = {};

ok64 BRIXparseCLI(BRIXarg* subj, int* verb, BRIXarg* obj) {
    sane(1);
    BRIXarg* n = subj;
    int i = 1;
    while (i < $arglen) {
        a$rg(a, i);
        if ($empty(a)) fail(badarg);
        if (**a == '#') {
            ++*a;
            call(RDXid128drain, &n->object, a);
        } else if (**a == '@') {
            ++*a;
            call(RDXid128drain, &n->version, a);
        } else if (n == subj) {
            int v = 1;
            while (v < BRIXcmdlen && !$eq(BRIX_CMDS[v], a)) ++v;
            if (v < BRIXcmdlen) {
                *verb = v;
                n = obj;
            } else {
                $mv(n->path, a);
            }
        } else {
            $mv(n->path, a);
        }
        ++i;
    }
    done;
}

ok64 BRIXdoinit(BRIXarg const* subj, BRIXarg const* obj) {
    sane(1);
    a$strc(path, ".");
    if (!$empty(obj->path)) $mv(path, obj->path);
    call(BRIXinit, &brix, path);
    done;
}

ok64 BRIXdosee(sha256* sha, BRIXarg const* subj, BRIXarg const* obj) {
    sane(1);
    u8 sb = BRIXargbits(subj);
    u8 ob = BRIXargbits(obj);
    u8 b = (sb << 4) | ob;
    switch (b) {
        case 0x8:   // see file.rdx
        case 0x18:  // @branch see file.jdr
        {
            Bu8 rdx = {};
            int fd = FILE_CLOSED;
            call(FILEmapro, rdx, &fd, obj->path);
            if (sb) {
                call(BRIXpushrev, &brix, subj->version);
            }
            call(BRIXpatch, sha, &brix, Bu8cdata(rdx));
            call(FILEclose, &fd);
            call(FILEunmap, rdx);
            break;
        }
        case 0x11: {  // @branch see @other
            break;
        }
        default:
            fail(notimplyet);
    }
    done;
}

ok64 BRIXcli() {
    sane(1);
    BRIXarg subj = {};  // TODO
    int verb = 0;
    BRIXarg obj = {};
    sha256 res = {};
    a$strc(path, ".");
    call(BRIXparseCLI, &subj, &verb, &obj);
    if (verb != BRIXcmdinit) {
        call(BRIXopen, &brix, path);
    }
    switch (verb) {
        case BRIXcmdnone:  // subject
            break;
        case BRIXcmdinit:
            return BRIXdoinit(&subj, &obj);
        case BRIXcmdsee:
            return BRIXdosee(&res, &subj, &obj);
        default:
            fail(notimplyet);
    }
    if (!sha256empty(&res)) {
        // TODO print
    }
    call(BRIXclose, &brix);
    done;
}

MAIN(BRIXcli);
