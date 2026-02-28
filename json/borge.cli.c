#include "BIFF.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// borge: merge two files (right-wins).
// Input: JSON or BASON (auto-detected). Output: JSON to stdout.
// Usage: borge <left> <right>

#define BORGE_BUF_LEN (4 * 1024 * 1024)

static b8 borgeIsJSON(u8csc data) {
    u8cp p = data[0];
    while (p < data[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= data[1]) return NO;
    return *p == '{' || *p == '[';
}

static ok64 borgeLoadFile(u8bp bson, u64bp idx,
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
    if (borgeIsJSON(data)) {
        call(BASONParseJSON, bson, idx, data);
    } else {
        call(u8bFeed, bson, data);
    }
    done;
}

ok64 borgecli() {
    sane(1);
    test($arglen == 3, BADARG);

    call(FILEInit);

    u8b lbuf = {};
    call(u8bMap, lbuf, BORGE_BUF_LEN);
    u64 _lidx[256];
    u64b lidx = {_lidx, _lidx, _lidx, _lidx + 256};
    u8bp lmap = NULL;
    a$rg(larg, 1);
    call(borgeLoadFile, lbuf, lidx, &lmap, larg);

    u8b rbuf = {};
    call(u8bMap, rbuf, BORGE_BUF_LEN);
    u64 _ridx[256];
    u64b ridx = {_ridx, _ridx, _ridx, _ridx + 256};
    u8bp rmap = NULL;
    a$rg(rarg, 2);
    call(borgeLoadFile, rbuf, ridx, &rmap, rarg);

    u8b out = {};
    call(u8bMap, out, BORGE_BUF_LEN);
    u64 _oidx[256];
    u64b oidx = {_oidx, _oidx, _oidx, _oidx + 256};
    u64 _lstk[256];
    u64b lstk = {_lstk, _lstk, _lstk, _lstk + 256};
    u64 _rstk[256];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 256};

    u8cp ld0 = lbuf[1], ld1 = lbuf[2];
    u8cp rd0 = rbuf[1], rd1 = rbuf[2];
    u8cs ld = {ld0, ld1};
    u8cs rd = {rd0, rd1};
    call(BASONMerge, out, oidx, lstk, ld, rstk, rd);

    u8b jbuf = {};
    call(u8bMap, jbuf, BORGE_BUF_LEN);
    u64 _jstk[256];
    u64b jstk = {_jstk, _jstk, _jstk, _jstk + 256};
    u8cp m0 = out[1], m1 = out[2];
    u8cs merged = {m0, m1};
    call(BASONExportJSON, u8bIdle(jbuf), jstk, merged);
    u8cp j0 = jbuf[1], j1 = jbuf[2];
    u8cs jout = {j0, j1};
    call(FILEFeedall, STDOUT_FILENO, jout);
    u8cs nl = $u8str("\n");
    call(FILEFeedall, STDOUT_FILENO, nl);

    call(FILEUnMap, lmap);
    call(FILEUnMap, rmap);
    u8bUnMap(lbuf);
    u8bUnMap(rbuf);
    u8bUnMap(out);
    u8bUnMap(jbuf);
    done;
}

MAIN(borgecli);
