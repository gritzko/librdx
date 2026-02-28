#include "BAST.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// Usage: bast file.ext              → JSON to stdout
//        bast file.ext file.bason   → BASON to file
//        bast file.ext file.json    → JSON to file

ok64 bastcli() {
    sane(1);
    test($arglen >= 2 && $arglen <= 3, BADARG);
    call(FILEInit);

    a$rg(arg, 1);
    an_arg(outarg, 2);

    // Find extension: scan back for '.', stop at '/'
    u8cs ext = {};
    for (u8cp p = arg[1]; p > arg[0]; --p) {
        if (*(p - 1) == '/') break;
        if (*(p - 1) == '.') {
            ext[0] = p - 1;
            ext[1] = arg[1];
            break;
        }
    }
    test(ext[0] != NULL, BADARG);

    // Map input file
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);

    u8bp mapped = NULL;
    call(FILEMapRO, &mapped, path8cgIn(path));
    u8cp i0 = mapped[1], i1 = mapped[2];
    u8cs indata = {i0, i1};

    // Parse to BASON
    size_t buflen = $len(indata) * 16;
    if (buflen < 1024 * 1024) buflen = 1024 * 1024;
    u8b bson = {};
    call(u8bMap, bson, buflen);
    size_t idxlen = buflen / BASON_PAGE + 256;
    u64 *_idx = (u64 *)malloc(idxlen * sizeof(u64));
    test(_idx != NULL, FAILsanity);
    u64b idx = {_idx, _idx, _idx, _idx + idxlen};
    call(BASTParse, bson, idx, indata, ext);

    u8cp o0 = bson[1], o1 = bson[2];
    u8cs bdata = {o0, o1};

    // Determine output format: .json suffix or default (no arg = json to stdout)
    b8 want_json = YES;
    if ($len(outarg) >= 6 &&
        memcmp(outarg[1] - 6, ".bason", 6) == 0)
        want_json = NO;

    // Determine output fd
    int outfd = STDOUT_FILENO;
    if ($len(outarg) > 0) {
        a_pad(u8, opath, FILE_PATH_MAX_LEN);
        call(u8bFeed, opath, outarg);
        u8bFeed1(opath, 0);
        u8bShed1(opath);
        call(FILECreate, &outfd, path8cgIn(opath));
    }

    if (want_json) {
        u8b jbuf = {};
        call(u8bMap, jbuf, buflen * 2);
        u64 _stk[256];
        u64b stk = {_stk, _stk, _stk, _stk + 256};
        call(BASONExportJSON, u8bIdle(jbuf), stk, bdata);
        u8cp j0 = jbuf[1], j1 = jbuf[2];
        u8cs jout = {j0, j1};
        call(FILEFeedall, outfd, jout);
        u8cs nl = $u8str("\n");
        call(FILEFeedall, outfd, nl);
        u8bUnMap(jbuf);
    } else {
        call(FILEFeedall, outfd, bdata);
    }

    if (outfd != STDOUT_FILENO) close(outfd);
    free(_idx);
    u8bUnMap(bson);
    call(FILEUnMap, mapped);
    done;
}

MAIN(bastcli);
