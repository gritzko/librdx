#include "BASON.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// bason: convert JSON->BASON or BASON->JSON depending on input.
// If input starts with '{' or '[' (after whitespace), it's JSON -> BASON.
// Otherwise it's BASON -> JSON.
// Usage: bason <file>

#define BASON_BUF_LEN (4 * 1024 * 1024)

static b8 basonIsJSON(u8csc data) {
    u8cp p = data[0];
    while (p < data[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= data[1]) return NO;
    return *p == '{' || *p == '[';
}

ok64 basoncli() {
    sane(1);
    test($arglen == 2, BADARG);

    call(FILEInit);

    a$rg(arg, 1);
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);

    u8bp mapped = NULL;
    call(FILEMapRO, &mapped, path8cgIn(path));
    u8cp i0 = mapped[1], i1 = mapped[2];
    u8cs indata = {i0, i1};

    if (basonIsJSON(indata)) {
        u8b bson = {};
        call(u8bMap, bson, BASON_BUF_LEN);
        u64 _idx[256];
        u64b idx = {_idx, _idx, _idx, _idx + 256};
        call(BASONParseJSON, bson, idx, indata);
        u8cp o0 = bson[1], o1 = bson[2];
        u8cs out = {o0, o1};
        call(FILEFeedall, STDOUT_FILENO, out);
        u8bUnMap(bson);
    } else {
        u8b jbuf = {};
        call(u8bMap, jbuf, BASON_BUF_LEN);
        u64 _stk[256];
        u64b stk = {_stk, _stk, _stk, _stk + 256};
        call(BASONExportJSON, u8bIdle(jbuf), stk, indata);
        u8cp o0 = jbuf[1], o1 = jbuf[2];
        u8cs out = {o0, o1};
        call(FILEFeedall, STDOUT_FILENO, out);
        u8cs nl = $u8str("\n");
        call(FILEFeedall, STDOUT_FILENO, nl);
        u8bUnMap(jbuf);
    }

    call(FILEUnMap, mapped);
    done;
}

MAIN(basoncli);
