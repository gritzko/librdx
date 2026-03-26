#include "ast/SPOT.h"
#include "ast/BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define FUZZ_BUF 16384

static u8 _hpad[FUZZ_BUF];
static u64 _hidx[1024];
static u8 _npad[FUZZ_BUF];
static u64 _nidx[256];

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

    // Parse haystack as C
    u8b hpad = {_hpad, _hpad, _hpad, _hpad + FUZZ_BUF};
    u64b hidx = {_hidx, _hidx, _hidx, _hidx + 1024};
    u8csc source = {src[0], src[1]};
    u8cs ext = $u8str(".c");
    ok64 ho = BASTParse(hpad, hidx, source, ext);
    if (ho != OK) done;
    u8cp hd0 = hpad[1], hd1 = hpad[2];
    u8cs hay = {hd0, hd1};
    if ($empty(hay)) done;

    // Init SPOT — needle parsed internally
    u8b npad = {_npad, _npad, _npad, _npad + FUZZ_BUF};
    u64b nidx = {_nidx, _nidx, _nidx, _nidx + 256};
    u8csc needle = {ndl[0], ndl[1]};
    SPOTstate st = {};
    ok64 so = SPOTInit(&st, npad, nidx, needle, ext, hay);
    if (so != OK) done;

    // Drain all matches — must not crash
    for (int i = 0; i < 64; i++) {
        ok64 o = SPOTNext(&st);
        if (o != OK) break;
    }

    done;
}
