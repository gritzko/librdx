#include "INT.h"

#include "PRO.h"

ok64 i64decdrain(i64 *i, u8csc tok) {
    sane(i != NULL && $ok(tok) && !$empty(tok));
    a_dup(u8c, dec, tok);
    if (**tok == '-' || **tok == '+') ++*dec;
    u64 x;
    ok64 o = u64decdrain(&x, dec);
    u64 lim = INT64_MAX;
    if (**tok == '-') {
        test(x <= I64_MIN_ABS, INTBAD);
        *i = -x;
        lim = I64_MIN_ABS;
    } else {
        test(x <= I64_MAX, INTBAD);
        *i = x;
    }
    test(x <= lim, INTBAD);
    return OK;
}
