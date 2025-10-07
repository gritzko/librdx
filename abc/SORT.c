#include "SORT.h"

#include "Y.h"

ok64 SORTu64($u64 into, $u64 from) {
    if ($len(into) < $len(from)) return SORTnoroom;
    aBpad2(u8cs, chunks, Y_MAX_INPUTS);
    ok64 o = OK;
    size_t clen = 1;
    b8 dir = NO;
    while (clen < $len(from)) {
        dir = !dir;
        $u8 into8;
        $u64 from64;
        if (dir) {
            $mv(into8, (u8$)into);
            $mv(from64, from);
        } else {
            $mv(into8, (u8$)from);
            $mv(from64, into);
        }
        u64* p = from64[0];
        while (p < from64[1] && o == OK) {
            while (!$empty(chunksidle) && p < from64[1]) {
                u8cs chunk;
                chunk[0] = (u8c*)p;
                p += clen;
                chunk[1] = (u8c*)p;
                HEAPu8cspush1f(chunksbuf, chunk, SORTu64z);
            }
            u8c$ last = *$last(chunksdata);
            if (last[1] > (u8c*)from64[1]) {
                last[1] = (u8c*)from64[1];
            }
            while (o == OK && !$empty(chunksdata)) {
                o = SORTu64next(into8, chunksdata);
            }
        }
        clen *= Y_MAX_INPUTS;
    }
    if (dir == NO) {
        $u64feedall(into, (u64c$c)from);
    } else {
        *into += $len(from);
    }
    return o;
}
