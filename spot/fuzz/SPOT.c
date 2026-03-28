#include "spot/SPOT.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define FUZZ_BUF 16384

static u32 _htoks[FUZZ_BUF];
static u32 _ntoks[4096];

FUZZ(u8, SPOTfuzz) {
    sane(1);
    if ($empty(input) || $len(input) > 4096) done;

    // Split at first newline: needle \n haystack
    u8cp nl = NULL;
    $for(u8c, p, input) {
        if (*p == '\n') {
            nl = p;
            break;
        }
    }
    if (!nl) done;

    a_head(u8c, ndl, input, nl - input[0]);
    a_rest(u8c, src, input, nl + 1 - input[0]);
    if ($empty(ndl) || $empty(src)) done;

    // Tokenize haystack
    u8csc source = {src[0], src[1]};
    u8cs ext = $u8str(".c");
    u32b htoks = {_htoks, _htoks, _htoks, _htoks + FUZZ_BUF};
    ok64 ho = SPOTTokenize(htoks, source, ext);
    if (ho != OK) done;
    u32cp hd = htoks[1], hi = htoks[2];
    u32cs ht = {(u32cp)hd, (u32cp)hi};
    if ($empty(ht)) done;

    // Init SPOT — needle parsed internally
    u32b ntoks = {_ntoks, _ntoks, _ntoks, _ntoks + 4096};
    u8csc needle = {ndl[0], ndl[1]};
    SPOTstate st = {};
    ok64 so = SPOTInit(&st, ntoks, needle, ext, ht, source);
    if (so != OK) done;

    // Drain all matches — must not crash
    for (int i = 0; i < 64; i++) {
        ok64 o = SPOTNext(&st);
        if (o != OK) break;
    }

    done;
}
