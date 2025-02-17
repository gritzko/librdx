#include "SST.h"

#include <limits.h>
#include <unistd.h>

#include "01.h"
#include "BUF.h"
#include "FILE.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

#define u128pack ZINTu128feed
#define u128unpack ZINTu128drain

#define X(M, name) M##u8##name
#include "SKIPx.h"
#undef X
#define X(M, name) M##u128##name
#include "SSTx.h"
#undef X

fun int alpha($cu8c* a, $cu8c* b) {
    a$dup(u8c, aa, *a);
    a$dup(u8c, bb, *b);
    u8 ta, tb;
    $u8c keya, keyb, vala, valb;
    TLVdrainkv(&ta, keya, vala, aa);
    TLVdrainkv(&tb, keyb, valb, bb);
    int c = $cmp(keya, keyb);
    return c;
}

fun ok64 latest($u8 into, $$u8c from) {
    u8 ta = 0;
    $u8c max = {};
    for (int i = 0; i < $len(from); ++i) {
        $u8c rec;
        TLVdrain$(rec, $at(from, i));
        if (*$last(rec) > ta) $mv(max, rec);
    }
    $u8feed(into, max);
    return OK;
}

con ok64 SrcAlice = 0x299edc0a;

pro(SST0) {
    sane(1);
#define ITER 10000
    Bu8 sst = {};
    int fd = FILE_CLOSED;
    a$strc(path, "/tmp/SST0.sst");
    SSTab tab = {};
    call(SSTu128init, sst, &fd, path, roundup(ITER * 32, PAGESIZE));
    for (u64 n = 0; n < ITER; ++n) {
        u128 id = {SrcAlice, n};
        a$rawc(val, n);
        call(SSTu128feed, sst, &tab, 'E', &id, val);
    }
    call(SSTu128end, sst, &fd, &tab);
    call(SSTu128close, sst);
    call(SSTu128open, sst, path);
    for (u64 n = 0; n < ITER; ++n) {
        u128 id = {SrcAlice, n};
        $u8c val = {}, fact = {};
        u8 t = 0;
        call(SSTu128get, &t, val, sst, &id);
        want(t == 'E');
        a$rawc(nval, n);
        $testeq(val, nval);
    }
    done;
}

fun ok64 nomerge($u8 into, $$u8c from) { return $u8feedall(into, **from); }

pro(SSTtest) {
    sane(1);
    call(SST0);
    done;
}

TEST(SSTtest);
