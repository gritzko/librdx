#include "BASON.h"

#include "abc/PRO.h"

ok64 BASONOpen(u64bp stack, u8csc data) {
    sane(stack != NULL && $ok(data));
    call(u64bFeed2, stack, (u64)$len(data), (u64)0);
    done;
}

ok64 BASONNext(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val) {
    sane(stack != NULL && $ok(data) && type != NULL);
    test(u64bDataLen(stack) >= 2, BASONBAD);
    u64 *top = u64bLast(stack);
    u64 cursor = *top;
    u64 end = *(top - 1);
    if (cursor >= end) return BASONEND;
    u8cs from = {data[0] + cursor, data[0] + end};
    call(TLKVDrain, from, type, key, val);
    *top = (u64)(from[0] - data[0]);
    done;
}

ok64 BASONInto(u64bp stack, u8csc data, u8csc val) {
    sane(stack != NULL && $ok(data));
    call(u64bFeed1, stack, (u64)(val[0] - data[0]));
    done;
}

ok64 BASONOuto(u64bp stack) {
    sane(stack != NULL);
    test(u64bDataLen(stack) >= 3, BASONBAD);
    call(u64bPop, stack);
    done;
}
