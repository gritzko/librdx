#ifndef LIBRDX_TEST_H
#define LIBRDX_TEST_H

#define ABC_TRACE 1

#include <assert.h>

#include "OK.h"
#include "PRO.h"

con ok64 TESTfail = 0xc2d96a75c39d;
con ok64 TESTfaileq = 0xd69c2d96a75c39d;

#define want(cond) \
    if (!(cond)) fail(TESTfail);

#define same(a, b) \
    if ((a) != (b)) fail(TESTfaileq);

#define TEST(f)                                                          \
    uint8_t _pro_depth = 0;                                              \
    int main(int argn, char **args) {                                    \
        ok64 ret = f();                                                  \
        if (ret != OK) {                                                 \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(ret), __func__, \
                  __LINE__);                                             \
        }                                                                \
        return ret;                                                      \
    }

#define fuzz(T, n)                                                 \
    ok64 n($##T##c input);                                         \
    uint8_t _pro_depth = 0;                                        \
    int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) { \
        T const *p = (T const *)Data;                              \
        $##T##c data = {p, p + (Size / sizeof(T))};                \
        ok64 o = n(data);                                          \
        assert(o == OK);                                           \
        return 0;                                                  \
    }                                                              \
    pro(n, $##T##c input)
#endif
