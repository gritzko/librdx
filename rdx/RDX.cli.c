#include "RDX.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"

a_cstr(USAGE, "Usage: rdx-fmt dir/ 'rdx path'\n");

ok64 AddInput(path8p path, rdxbp inputs) {
    sane(rdxbOK(inputs) && path8Sane(path));

    // TODO ext
    int fd = FILE_CLOSED;
    u8bp buf = {};
    call(FILEMapRO, buf, path);

    rdxp n = 0;
    call(rdxbFedP, inputs, &n);
    n->format = RDX_FMT_JDR;
    $mv(n->data, u8bData(buf));

    done;
}

ok64 OpenRepo(rdxbp inputs, path8 path) {
    sane(rdxbOK(inputs) && path8Sane(path));
    if (!FILEisdir(path)) {
        call(AddInput, path, inputs);
    } else {
        call(FILEScanFiles, path, (path8f)AddInput, (voidp)inputs);
    }
    done;
}

ok64 FindPath(rdxsp inputs, rdxs locus, rdxp path) {
    sane(rdxsOK(inputs) && path);
    ok64 o = rdxNext(path);
    if (o == END) {
        call(rdxsFeed, locus, (rdxcsp)inputs);
        done;
    } else if (o != OK) {
        fail(o);
    }
    a_pad(rdx, pad, 64);
    $for(rdx, p, inputs) {
        rdxp n = 0;
        call(rdxbFedP, pad, &n);
        ok64 o = rdxInto(n, p);
        if (o != OK) rdxbShed1(pad);
    }
    test(rdxbDataLen(pad), NODATA);
    return FindPath(rdxbData(pad), locus, path);
}

ok64 rdxcli() {
    sane(1);
    u8cssp args = u8csbData(STD_ARGS);
    if (u8cssLen(args) < 2) {
        FILEerr(USAGE);
        fail(BADARG);
    }
    a_pad0(rdx, inputs, 64);
    a_pad0(rdx, locus, 64);
    rdx qpath = {};
    rdx in = {.format = RDX_FMT_Y};
    rdx out = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};

    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);

    a_pad(u8, path, 256);
    call(u8bFeed, path, *u8csbAtP(args, 1));
    call(OpenRepo, inputs, path);
    call(FindPath, rdxbData(inputs), rdxbIdle(locus), &qpath);
    rdxgMv(in.ins, rdxbDataIdle(locus));
    call(rdxCopyF, &out, &in, (voidf)FILEFlush, &fd);
    call(FILEFlushAll, &fd);

    call(u8bUnMap, FILE_BUFS[fd]);

    done;
}

MAIN(rdxcli);
