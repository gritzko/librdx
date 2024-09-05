#ifndef LIBRDX_PRO_H
#define LIBRDX_PRO_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "OK.h"

extern uint8_t _pro_depth;

// use this with every int main() {...}
#define ABC_INIT uint8_t _pro_depth = 0;

con char *_pro_indent =
    "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

con ok64 badarg = 0xaf6968966;
con ok64 faileq = 0xd69c2d96a;

#define PROindent (_pro_indent + 32 - (_pro_depth & 31))
#define PROind (_pro_indent + 32 - (__depth & 31))

#define pro(name, ...) ok64 name(__VA_ARGS__)

// Procedure return with no finalizations.
#define done              \
    _over:                \
    _pro_depth = __depth; \
    return __;

// Procedure return with finalizations.
#define nedo(...)           \
    _over: { __VA_ARGS__; } \
    _pro_depth = __depth;   \
    return __;

// Procedure fails, skip to finalizations.
#define fail(code)                                                          \
    {                                                                       \
        __ = (code);                                                        \
        trace("%s<%s at %s:%i\n", PROind, ok64str(__), __func__, __LINE__); \
        goto _over;                                                         \
    }

// Skip to procedure return.
#define skip goto _over;

#define failc(code)                                                       \
    {                                                                     \
        __ = (code);                                                      \
        trace("%s<%s errno %i %s at %s:%i\n", PROind, ok64str(__), errno, \
              strerror(errno), __func__, __LINE__);                       \
        goto _over;                                                       \
    }

#define test(c, f) \
    if (!(c)) fail(f);

#define testc(c, f) \
    if (!(c)) failc(f);

// Mandatory sanity checks; might be disabled in Release mode.
#define sane(c)                            \
    trace("%s>%s\n", PROindent, __func__); \
    u8 __depth = _pro_depth;               \
    _pro_depth++;                          \
    ok64 __ = 0;                           \
    if (!(c)) fail(FAILsanity);

#define try(f, ...) \
    { __ = (f(__VA_ARGS__)); }

#define on(f) if (__ == (f))

#define is(f) (__ == f)

#define sure(f) \
    if (__ != f) fail(__);

#define call(f, ...)                                                 \
    {                                                                \
        __ = (f(__VA_ARGS__));                                       \
        if (__ != OK) {                                              \
            trace("%s<%s at %s:%i\n", PROind, ok64str(__), __func__, \
                  __LINE__);                                         \
            goto _over;                                              \
        }                                                            \
    }

#define fwdcall(f, ...)                                              \
    {                                                                \
        __ = (f(__VA_ARGS__));                                       \
        if (__ != OK) {                                              \
            trace("%s<%s at %s:%i\n", PROind, ok64str(__), __func__, \
                  __LINE__);                                         \
        }                                                            \
        goto _over;                                                  \
    }

#define mute(f, o)            \
    {                         \
        __ = (f);             \
        if (__ == o) __ = OK; \
        if (__ != OK) {       \
            goto _over;       \
        }                     \
    }

#define testeq(a, b)               \
    {                              \
        if (!likely((a) == (b))) { \
            fail(faileq)           \
        }                          \
    }

#define testeqv(a, b, fmt)                                                  \
    {                                                                       \
        if (!likely((a) == (b))) {                                          \
            fprintf(stderr, "%sNot equal: " fmt " <> " fmt "\n", PROind, a, \
                    b);                                                     \
            fail(faileq)                                                    \
        }                                                                   \
    }

#define $testeq(a, b)                                                         \
    {                                                                         \
        if (!likely($size(a) == $size(b) && 0 == memcmp(*a, *b, $size(a)))) { \
            fail(faileq)                                                      \
        }                                                                     \
    }

#ifndef ABC_NOTRACE
#define trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define trace(...) ;
#endif

#endif
