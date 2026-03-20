#include "ast/CSS.h"
#include "ast/BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define FUZZ_BUF 16384

static u8 _pad[FUZZ_BUF];
static u64 _idx[1024];
static u8 _out[FUZZ_BUF];

FUZZ(u8, CSSfuzz) {
    sane(1);
    if ($empty(input) || $len(input) > 4096) done;

    // Split at first newline: selector \n source
    u8cp nl = NULL;
    $for(u8c, p, input) {
        if (*p == '\n') {
            nl = p;
            break;
        }
    }
    if (!nl) done;

    a_head(u8c, sel, input, nl - input[0]);
    a_rest(u8c, src, input, nl + 1 - input[0]);
    if ($empty(sel) || $empty(src)) done;

    // Parse selector — ignore errors, fuzz CSSParse itself
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    u8cs selector = {sel[0], sel[1]};
    CSSParse(qbuf, qidx, selector);
    u8cp qd0 = qbuf[1], qd1 = qbuf[2];
    u8cs query = {qd0, qd1};

    // Parse source as C
    u8b pad = {_pad, _pad, _pad, _pad + FUZZ_BUF};
    u64b idx = {_idx, _idx, _idx, _idx + 1024};
    u8csc source = {src[0], src[1]};
    u8cs ext = $u8str(".c");
    BASTParse(pad, idx, source, ext);
    u8cp bd0 = pad[1], bd1 = pad[2];
    u8cs bason = {bd0, bd1};

    // Match — must not crash
    u8b out = {_out, _out, _out, _out + FUZZ_BUF};
    CSSMatch(u8bIdle(out), bason, query, 0, NO);

    done;
}
