#include "BRIX.h"

#include "abc/PRO.h"
#include "rdx/JDR.h"

enum {
    BRIXargver = 1 << 1,
    BRIXargobj = 1 << 2,
    BRIXargsst = 1 << 3,
    BRIXargpath = 1 << 4,
};

typedef struct {
    id128 version;
    id128 object;
    sha256 sst;
    $u8c path;
} BRIXarg;

enum {
    BRIXcmdnone,
    BRIXcmdinit,
    BRIXcmdlen,
};
$u8c BRIX_CMDS[] = {
    $u8str(""),
    $u8str("init"),
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
            call(RDXid128drain, &subj->object, a);
        } else if (**a == '@') {
            ++*a;
            call(RDXid128drain, &subj->version, a);
        } else if (n == subj) {
            int v = 1;
            while (v < BRIXcmdlen && !$eq(BRIX_CMDS[v], a)) ++v;
            if (v < BRIXcmdlen) {
                *verb = v;
                n = obj;
            } else {
                $mv(subj->path, a);
            }
        } else {
            $mv(subj->path, a);
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

ok64 BRIXcli() {
    sane(1);
    BRIXarg subj = {};  // TODO
    int verb = 0;
    BRIXarg obj = {};
    call(BRIXparseCLI, &subj, &verb, &obj);
    switch (verb) {
        case BRIXcmdnone:  // subject
            break;
        case BRIXcmdinit:
            return BRIXdoinit(&subj, &obj);
        default:
            fail(notimplyet);
    }
    done;
}

MAIN(BRIXcli);
