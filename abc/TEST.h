#ifndef LIBRDX_TEST_H
#define LIBRDX_TEST_H

#include <assert.h>

#include "OK.h"
#include "PRO.h"

con ok64 TESTFAIL = 0x74e71d3ca495;
con ok64 TESTFAILEQ = 0x74e71d3ca495a75;

#define want(cond) \
    if (!(cond)) fail(TESTFAIL);

#define same(a, b) \
    if ((a) != (b)) fail(TESTFAILEQ);

#define TEST(f)                                                       \
    uint8_t _pro_depth = 0;                                           \
    u8cs _STD_ARGS[64] = {};                                          \
    u8cs *STD_ARGS[4] = {};                                           \
    int main(int argn, char **args) {                                 \
        _PRO_TRACE_INIT(args[0]);                                     \
        ok64 ret = f();                                               \
        if (ret != OK) {                                              \
            fprintf(stderr, "%s<%s at %s:%i\ntest fail\n", PROindent, \
                    ok64str(ret), __func__, __LINE__);                \
        }                                                             \
        return ret;                                                   \
    }

#define FUZZ(T, n)                                                 \
    ok64 n($##T##c input);                                         \
    uint8_t _pro_depth = 0;                                        \
    int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) { \
        T const *p = (T const *)Data;                              \
        $##T##c data = {p, p + (Size / sizeof(T))};                \
        ok64 o = n(data);                                          \
        assert(o == OK);                                           \
        return 0;                                                  \
    }                                                              \
    ok64 n($##T##c input)
#endif
