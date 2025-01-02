#include "VLT.h"

#include "RDX.h"
#include "abc/$.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "abc/ZINT.h"

con u64 RDX_PLEX_BITS = (1 << (RDX_TUPLE - 'A')) | (1 << (RDX_LINEAR - 'A')) |
                        (1 << (RDX_EULER - 'A')) | (1 << (RDX_MULTIX - 'A'));

pro(VLTtest1) {
    sane(1);
    must(sizeof(backmark) == 32);
    aBpad(u8, pad, PAGESIZE);
    aBpad(u256, stack, RDX_MAX_NEST);

    aBcpad(u8, id, 16);
    u128 id1 = {._64 = {0xc, 0x123}};
    call(ZINTu128feed, ididle, id1);

    call(VLTopen, pad, stack, RDX_TUPLE);
    call(VLTsetid, pad, stack, iddata);
    call(VLTopen, pad, stack, RDX_INT);
    call($u8feed1, Bu8idle(pad), 0x42);
    call(VLTsetid, pad, stack, iddata);
    call(VLTclose, pad, stack, RDX_INT);
    call(VLTclose, pad, stack, RDX_TUPLE);

    u8 correct[] = {'p', 0xd, 0x4, 0xc, 0x0,  0x23, 0x1, 'i',
                    0x6, 0x4, 0xc, 0x0, 0x23, 0x1,  0x42};
    a$raw(cor, correct);

    aBcpad(u8, fed, sizeof(correct));
    call(VLTfeedTLV, fedidle, Bu8cdata(pad), RDX_PLEX_BITS);
    $testeq(cor, feddata);

    call(VLTreverse, pad, RDX_PLEX_BITS);

    $testeq(cor, Bu8data(pad));

    done;
}

pro(VLTtest) {
    sane(1);
    call(VLTtest1);
    done;
}

TEST(VLTtest);
