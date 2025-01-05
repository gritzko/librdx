#include "VLT.h"

#include "RDX.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

pro(VLTtest1) {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    aBpad(u64, stack, RDX_MAX_NEST);
    Bzero(padbuf);

    aBcpad(u8, id, 16);
    u128 id1 = {._64 = {0xc, 0x123}};
    call(ZINTu128feed, ididle, id1);
    call($u8feed1, ididle, $len(iddata));

    call(VLTopen, padbuf, stack, RDX_TUPLE);
    call(VLTopen, padbuf, stack, RDX_INT);
    call($u8feed1, padidle, 0x42);
    call($u8feed, padidle, iddata);
    call(VLTclose, padbuf, stack, RDX_INT);
    call($u8feed, padidle, iddata);
    call(VLTclose, padbuf, stack, RDX_TUPLE);

    a$u8c(cor, 'p', 0xd, 0x4, 0xc, 0x0, 0x23, 0x1, 'i', 0x6, 0x4, 0xc, 0x0,
          0x23, 0x1, 0x42);

    aBcpad(u8, fed, $len(cor));
    Bzero(fedbuf);
    call(VLTfeedTLKV, fedidle, paddata, RDX_PLEX_BITS);
    $testeq(cor, feddata);

    call(VLTreverseTLKV, padbuf, RDX_PLEX_BITS);

    $testeq(cor, paddata);

    done;
}

pro(VLTtest2) {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    aBpad(u64, stack, RDX_MAX_NEST);
    Bzero(padbuf);

    call(VLTopen, padbuf, stack, RDX_TUPLE);
    for (int i = 0; i < 256; ++i) {
        id128 id = {i * 2, i * 3};
        aBcpad(u8, idb, 32);
        ZINTu128feed(idbidle, id);
        call(VLTopen, padbuf, stack, RDX_INT);
        call($u8feed1, padidle, i);
        call($u8feed, padidle, idbdata);
        call($u8feed1, padidle, $len(idbdata));
        call(VLTclose, padbuf, stack, RDX_INT);
    }
    call($u8feed1, padidle, 0);
    call(VLTclose, padbuf, stack, RDX_TUPLE);

    aBcpad(u8, fact, PAGESIZE);
    Bzero(factbuf);
    call(VLTfeedTLKV, factidle, paddata, RDX_PLEX_BITS);

    u8 t;
    $u8c key, value;
    call(TLVdrainkv, &t, key, value, factdata);
    want(t == RDX_TUPLE);
    for (int i = 0; i < 256; ++i) {
        $u8c k, body;
        call(TLVdrainkv, &t, k, body, value);
        want(t == RDX_INT);
    }

    done;
}

pro(VLTtest) {
    sane(1);
    call(VLTtest1);
    call(VLTtest2);
    done;
}

TEST(VLTtest);
