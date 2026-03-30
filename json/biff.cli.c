#include "BIFF.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// biff: diff two files.
// Usage: biff <old> <new> [patch]
// Input: JSON or BASON (auto-detected).
// Output format by patch extension: .json .bason .txt
// No patch arg: JSON diff to stdout.

#define BIFF_BUF_LEN (16 * 1024 * 1024)

static b8 biffIsJSON(u8csc data) {
    u8cp p = data[0];
    while (p < data[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= data[1]) return NO;
    return *p == '{' || *p == '[';
}

// 0=bason, 1=json, 2=txt
static u8 biffOutFmt(u8csc name) {
    size_t n = $len(name);
    if (n >= 5 && memcmp(name[1] - 5, ".json", 5) == 0) return 1;
    if (n >= 6 && memcmp(name[1] - 6, ".bason", 6) == 0) return 0;
    if (n >= 4 && memcmp(name[1] - 4, ".txt", 4) == 0) return 2;
    return 0;  // default: binary bason
}

static ok64 biffLoadFile(u8bp bson, u64bp idx,
                         u8bp *mapped, u8cs arg) {
    sane(bson != NULL);
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);
    call(FILEMapRO, mapped, PATHu8cgIn(path));
    u8cp d0 = u8bDataHead(*mapped);
    u8cp d1 = u8bIdleHead(*mapped);
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
    if ($arglen < 3 || $arglen > 4) {
        a_cstr(u1, "Usage: biff <old> <new>          diff to stdout (JSON)\n");
        a_cstr(u2, "       biff <old> <new> <patch>  patch to file (.json/.bason/.txt)\n");
        a_cstr(u3, "Input: JSON or BASON (auto-detected).\n");
        FILEerr(u1);
        FILEerr(u2);
        FILEerr(u3);
        fail(BADARG);
    }

    call(FILEInit);

    u8b obuf = {};
    call(u8bMap, obuf, BIFF_BUF_LEN);
    u64 _oidx[4096];
    u64b oidx = {_oidx, _oidx, _oidx, _oidx + 4096};
    u8bp omap = NULL;
    a$rg(oarg, 1);
    call(biffLoadFile, obuf, oidx, &omap, oarg);

    u8b nbuf = {};
    call(u8bMap, nbuf, BIFF_BUF_LEN);
    u64 _nidx[4096];
    u64b nidx = {_nidx, _nidx, _nidx, _nidx + 4096};
    u8bp nmap = NULL;
    a$rg(narg, 2);
    call(biffLoadFile, nbuf, nidx, &nmap, narg);

    u8b out = {};
    call(u8bMap, out, BIFF_BUF_LEN);
    u64 _didx[4096];
    u64b didx = {_didx, _didx, _didx, _didx + 4096};
    u64 _ostk[256];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 256};
    u64 _nstk[256];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 256};

    u8cp od0 = u8bDataHead(obuf), od1 = u8bIdleHead(obuf);
    u8cp nd0 = u8bDataHead(nbuf), nd1 = u8bIdleHead(nbuf);
    u8cs od = {od0, od1};
    u8cs nd = {nd0, nd1};
    call(BASONDiff, out, didx, ostk, od, nstk, nd, NULL);

    u8cp df0 = u8bDataHead(out), df1 = u8bIdleHead(out);
    u8cs diff = {df0, df1};

    if ($arglen == 4) {
        a$rg(parg, 3);
        u8 ofmt = biffOutFmt(parg);

        if (ofmt == 0) {
            // Binary BASON patch to file
            a_pad(u8, ppath, FILE_PATH_MAX_LEN);
            call(u8bFeed, ppath, parg);
            u8bFeed1(ppath, 0);
            u8bShed1(ppath);
            int pfd;
            call(FILECreate, &pfd, PATHu8cgIn(ppath));
            if ($len(diff) > 0)
                call(FILEFeedall, pfd, diff);
            close(pfd);
        } else {
            // JSON or text export to file
            u8b xbuf = {};
            call(u8bMap, xbuf, BIFF_BUF_LEN);
            u64 _xstk[256];
            u64b xstk = {_xstk, _xstk, _xstk, _xstk + 256};
            if ($len(diff) == 0) {
                if (ofmt == 1) {
                    u8cs empty = $u8str("{}");
                    call(u8sFeed, u8bIdle(xbuf), empty);
                }
            } else if (ofmt == 1) {
                call(BASONExportJSON, u8bIdle(xbuf), xstk, diff);
            } else {
                call(BASONExportText, u8bIdle(xbuf), xstk, diff);
            }
            u8cp x0 = u8bDataHead(xbuf), x1 = u8bIdleHead(xbuf);
            u8cs xout = {x0, x1};
            a_pad(u8, ppath, FILE_PATH_MAX_LEN);
            call(u8bFeed, ppath, parg);
            u8bFeed1(ppath, 0);
            u8bShed1(ppath);
            int pfd;
            call(FILECreate, &pfd, PATHu8cgIn(ppath));
            call(FILEFeedall, pfd, xout);
            if (ofmt == 1) {
                u8cs nl = $u8str("\n");
                call(FILEFeedall, pfd, nl);
            }
            close(pfd);
            u8bUnMap(xbuf);
        }

        // Render colored diff to stdout
        u8b rbuf = {};
        call(u8bMap, rbuf, BIFF_BUF_LEN * 2);
        u64 _rs1[256], _rs2[256];
        u64b rs1 = {_rs1, _rs1, _rs1, _rs1 + 256};
        u64b rs2 = {_rs2, _rs2, _rs2, _rs2 + 256};
        call(BASONDiffRender, u8bIdle(rbuf), rs1, od, rs2, diff);
        u8cp r0 = u8bDataHead(rbuf), r1 = u8bIdleHead(rbuf);
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
            u8cp j0 = u8bDataHead(jbuf), j1 = u8bIdleHead(jbuf);
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
