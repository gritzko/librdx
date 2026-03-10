#include "BIFF.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// biff: diff two files.
// Usage: biff <old> <new>            → JSON patch to stdout
//        biff <old> <new> <patch>    → BASON patch to file, colored to stdout
// Input: JSON or BASON (auto-detected).

#define BIFF_BUF_LEN (4 * 1024 * 1024)

static b8 biffIsJSON(u8csc data) {
    u8cp p = data[0];
    while (p < data[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= data[1]) return NO;
    return *p == '{' || *p == '[';
}

static ok64 biffLoadFile(u8bp bson, u64bp idx,
                         u8bp *mapped, u8cs arg) {
    sane(bson != NULL);
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);
    call(FILEMapRO, mapped, path8cgIn(path));
    u8cp d0 = (*mapped)[1];
    u8cp d1 = (*mapped)[2];
    u8cs data = {d0, d1};
    if (biffIsJSON(data)) {
        call(BASONParseJSON, bson, idx, data);
    } else {
        call(u8bFeed, bson, data);
    }
    done;
}

ok64 biffcli() {
    sane(1);
    test($arglen >= 3 && $arglen <= 4, BADARG);

    call(FILEInit);

    u8b obuf = {};
    call(u8bMap, obuf, BIFF_BUF_LEN);
    u64 _oidx[256];
    u64b oidx = {_oidx, _oidx, _oidx, _oidx + 256};
    u8bp omap = NULL;
    a$rg(oarg, 1);
    call(biffLoadFile, obuf, oidx, &omap, oarg);

    u8b nbuf = {};
    call(u8bMap, nbuf, BIFF_BUF_LEN);
    u64 _nidx[256];
    u64b nidx = {_nidx, _nidx, _nidx, _nidx + 256};
    u8bp nmap = NULL;
    a$rg(narg, 2);
    call(biffLoadFile, nbuf, nidx, &nmap, narg);

    u8b out = {};
    call(u8bMap, out, BIFF_BUF_LEN);
    u64 _didx[256];
    u64b didx = {_didx, _didx, _didx, _didx + 256};
    u64 _ostk[256];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 256};
    u64 _nstk[256];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 256};

    u8cp od0 = obuf[1], od1 = obuf[2];
    u8cp nd0 = nbuf[1], nd1 = nbuf[2];
    u8cs od = {od0, od1};
    u8cs nd = {nd0, nd1};
    call(BASONDiff, out, didx, ostk, od, nstk, nd, NULL);

    u8cp df0 = out[1], df1 = out[2];
    u8cs diff = {df0, df1};

    if ($arglen == 4) {
        // Write BASON patch to file
        a$rg(parg, 3);
        a_pad(u8, ppath, FILE_PATH_MAX_LEN);
        call(u8bFeed, ppath, parg);
        u8bFeed1(ppath, 0);
        u8bShed1(ppath);
        int pfd;
        call(FILECreate, &pfd, path8cgIn(ppath));
        if ($len(diff) > 0)
            call(FILEFeedall, pfd, diff);
        close(pfd);

        // Render colored diff to stdout
        u8b rbuf = {};
        call(u8bMap, rbuf, BIFF_BUF_LEN * 2);
        u64 _rs1[256], _rs2[256];
        u64b rs1 = {_rs1, _rs1, _rs1, _rs1 + 256};
        u64b rs2 = {_rs2, _rs2, _rs2, _rs2 + 256};
        call(BASONDiffRender, u8bIdle(rbuf), rs1, od, rs2, diff);
        u8cp r0 = rbuf[1], r1 = rbuf[2];
        u8cs rout = {r0, r1};
        call(FILEFeedall, STDOUT_FILENO, rout);
        u8bUnMap(rbuf);
    } else {
        // JSON patch to stdout
        if ($len(diff) == 0) {
            u8cs empty = $u8str("{}\n");
            call(FILEFeedall, STDOUT_FILENO, empty);
        } else {
            u8b jbuf = {};
            call(u8bMap, jbuf, BIFF_BUF_LEN);
            u64 _jstk[256];
            u64b jstk = {_jstk, _jstk, _jstk, _jstk + 256};
            call(BASONExportJSON, u8bIdle(jbuf), jstk, diff);
            u8cp j0 = jbuf[1], j1 = jbuf[2];
            u8cs jout = {j0, j1};
            call(FILEFeedall, STDOUT_FILENO, jout);
            u8cs nl = $u8str("\n");
            call(FILEFeedall, STDOUT_FILENO, nl);
            u8bUnMap(jbuf);
        }
    }

    call(FILEUnMap, omap);
    call(FILEUnMap, nmap);
    u8bUnMap(obuf);
    u8bUnMap(nbuf);
    u8bUnMap(out);
    done;
}

MAIN(biffcli);
