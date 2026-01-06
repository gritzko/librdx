#include "RDX.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/NACL.h"
#include "abc/PRO.h"
#include "abc/S.h"

a_cstr(USAGE, "Usage: rdx verb inputs\n");

a_cstr(EXT_JDR, ".jdr");
a_cstr(EXT_TLV, ".tlv");
a_cstr(EXT_SKIL, ".tlv");

u8 RDX_FMT_DEFAULT = RDX_FMT_JDR;

ok64 AddFileInput(voidp arg, path8p path) {
    sane(arg && path8Sane(path));
    rdxbp inputs = (rdxbp)arg;
    rdxp n = 0;
    call(rdxbFedP, inputs, &n);
    if (u8csHasSuffix(arg, EXT_JDR)) {
        n->format = RDX_FMT_JDR;
    } else {
        done;
    }
    int fd = FILE_CLOSED;
    u8bp buf = {};
    call(FILEMapRO, buf, path);
    $mv(n->data, u8bData(buf));
    done;
}

ok64 AddInput(rdxbp inputs, u8csp arg) {
    sane(rdxbOK(inputs) && u8csOK(arg));
    a_path(path, "");
    call(u8bFeed, path, arg);
    struct stat s = {};

    if (OK == FILEStat(&s, path)) {
        if ((s.st_mode & S_IFMT) == S_IFDIR) {
            call(FILEScanFiles, path, AddFileInput, (voidp)inputs);
        } else {
            call(AddFileInput, (voidp)inputs, path);
        }
    } else {
        rdxp n = 0;
        call(rdxbFedP, inputs, &n);
        zerop(n);
        n->format = RDX_FMT_JDR;
        $mv(n->data, arg);
    }

    done;
}

ok64 CmdY(rdxg inputs, u8 fmt) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    rdx in = {};
    if (rdxgLeftLen(inputs) == 1) {
        in = rdxsAt(rdxgLeft(inputs), 0);
    } else {
        in.format = RDX_FMT_Y;
        rdxgMv(in.ins, inputs);
    }
    rdx out = {.format = fmt | RDX_FMT_WRITE};
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));  // FIXME
    try(rdxCopyF, &out, &in, (voidf)FILEFlush, &fd);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);  // FIXME
    then try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

a_cstr(VERB_JDR, "jdr");
a_cstr(VERB_MERJ, "merj");
ok64 CmdJDR(rdxg inputs) { return CmdY(inputs, RDX_FMT_JDR); }

a_cstr(VERB_Q, "q");
a_cstr(VERB_QUERY, "query");
ok64 CmdQuery(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    done;
}

a_cstr(VERB_HASH, "hash");
ok64 CmdHash(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    rdx in = {};
    if (rdxgLeftLen(inputs) == 1) {
        in = rdxsAt(rdxgLeft(inputs), 0);
    } else {
        in.format = RDX_FMT_Y;
        rdxgMv(in.ins, inputs);
    }
    blake256 blake = {};
    call(rdxHashBlake, &in, &blake);
    a_rawc(hash, blake);
    a_pad(u8, hex, 65);
    HEXFeed(hex_idle, hash);
    u8sFeed1(hex_idle, 0);
    printf("Simple BLAKE256: %s\n", *hex_data);
    done;
}

a_cstr(VERB_Y, "y");
a_cstr(VERB_MERGE, "merge");
a_cstr(VERB_TLV, "tlv");
a_cstr(VERB_STRIP, "strip");
a_cstr(VERB_NOW, "now");
a_cstr(VERB_NORM, "norm");
a_cstr(VERB_CAT, "cat");

ok64 CmdNorm(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_DEFAULT | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxMerge, &out, inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ok64 CmdCat(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxMerge, &out, inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ok64 CmdStrip(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_DEFAULT | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxStrip, &out, *inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ron60 RONNow();
ok64 CmdNow(rdxg inputs) {
    ron60 now = RONNow();
    printf("%s\n", ok64str(now));
    return OK;
}

ok64 rdxcli() {
    sane(1);
    u8cssp args = u8csbData(STD_ARGS);
    if (u8cssLen(args) < 2) {
        FILEerr(USAGE);
        fail(BADARG);
    }
    call(FILEInit);

    if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_JDR)) {
        RDX_FMT_DEFAULT = RDX_FMT_JDR;
    } else if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_TLV)) {
        RDX_FMT_DEFAULT = RDX_FMT_TLV;
    } else if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_SKIL)) {
        RDX_FMT_DEFAULT = RDX_FMT_SKIL;
    }

    // rdx merge|hash|strip|jdr|tlv|etc inputs*

    u8csp verb = *u8csbAtP(STD_ARGS, 1);

    a_pad0(rdx, inputs, 64);
    a_rest(u8cs, inn, u8csbData(STD_ARGS), 2);
    $for(u8cs, arg, inn) call(AddInput, inputs, *arg);

    rdxgp din = rdxbDataIdle(inputs);

    if ($eq(verb, VERB_JDR)) {
        call(CmdY, din, RDX_FMT_JDR);
    } else if ($eq(verb, VERB_Q)) {
        call(CmdQuery, din);
    } else if ($eq(verb, VERB_HASH)) {
        call(CmdHash, din);
    } else if ($eq(verb, VERB_QUERY)) {
        call(CmdQuery, din);
    } else if ($eq(verb, VERB_MERGE)) {
        call(CmdY, din, RDX_FMT_DEFAULT);
    } else if ($eq(verb, VERB_TLV)) {
        call(CmdY, din, RDX_FMT_TLV);
    } else if ($eq(verb, VERB_MERJ)) {
        call(CmdY, din, RDX_FMT_JDR);
    } else if ($eq(verb, VERB_NOW)) {
        call(CmdNow, 0);
    } else if ($eq(verb, VERB_NORM)) {
        call(CmdNorm, din);
    } else if ($eq(verb, VERB_STRIP)) {
        call(CmdStrip, din);
    } else if ($eq(verb, VERB_CAT)) {
        call(CmdCat, din);
    } else {
        fprintf(stderr, "Unknown command %s.\n%s", *verb, *USAGE);
    }

    done;
}

MAIN(rdxcli);
