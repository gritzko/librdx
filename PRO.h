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

#define otry(f, ...) __ = (f(__VA_ARGS__))

#define ofix(f) if (__ == (f) && OK == (__ = OK))

#define orly if (__ == OK)

#define oops if (__ != OK)

#define ocry \
    if (__ != OK) fail(__);

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
            fail(faileq);          \
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

extern $u8c _STD_ARGS[];
extern $u8c *STD_ARGS[];

fun void _parse_args(int argn, char **args) {
    for (int i = 0; i < argn; ++i) {
        u8c *s[2] = $u8str(args[i]);
        memcpy(_STD_ARGS + i, s, sizeof($u8c));
    }
    STD_ARGS[0] = STD_ARGS[1] = _STD_ARGS;
    STD_ARGS[2] = STD_ARGS[3] = _STD_ARGS + argn;
}

#define $arglen $len(B$u8cdata(STD_ARGS))

#define $arg(i) (*$$u8catp(B$u8cdata(STD_ARGS), i))

#define a$rg(name, i) \
    $u8c name = {};   \
    $mv(name, *$$u8catp(B$u8cdata(STD_ARGS), i));

#define MAIN(f)                                                          \
    uint8_t _pro_depth = 0;                                              \
    $u8c _STD_ARGS[64] = {};                                             \
    $u8c *STD_ARGS[4] = {};                                              \
    int main(int argn, char **args) {                                    \
        _parse_args(argn, args);                                         \
        ok64 ret = f();                                                  \
        if (ret != OK)                                                   \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(ret), __func__, \
                  __LINE__);                                             \
        return ret;                                                      \
    }

#endif
