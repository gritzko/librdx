#ifndef LIBRDX_TEST_H
#define LIBRDX_TEST_H

#define ABC_TRACE 1

#include <assert.h>

#include "PRO.h"
#include "trace.h"

#define TEST(f)                                                          \
    int main(int argn, char **args) {                                    \
        ok64 ret = f();                                                  \
        if (ret != OK)                                                   \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(ret), __func__, \
                  __LINE__);                                             \
        return ret;                                                      \
    }

#define fuzz(T, n)                                                 \
    ok64 n($##T##c input);                                         \
    int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) { \
        T const *p = (T const *)Data;                              \
        $##T##c data = {p, p + (Size / sizeof(T))};                \
        ok64 o = n(data);                                          \
        assert(o == OK);                                           \
        return 0;                                                  \
    }                                                              \
    pro(n, $##T##c input)
#endif
