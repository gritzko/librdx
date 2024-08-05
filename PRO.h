#ifndef LIBRDX_PRO_H
#define LIBRDX_PRO_H

#include <errno.h>
#include <stdint.h>

#include "OK.h"
#include "trace.h"

static uint8_t _pro_depth = 0;

con char *_pro_indent =
    "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

con ok64 badarg = 0xaf6968966;

#define PROindent (_pro_indent + 32 - (_pro_depth & 31))
#define PROind (_pro_indent + 32 - (__depth & 31))

#define pro(name, ...)                      \
    ok64 name(__VA_ARGS__) {                \
        trace("%s>%s\n", PROindent, #name); \
        u8 __depth = _pro_depth;            \
        _pro_depth++;                       \
        ok64 __ = 0;

#define done              \
    }                     \
    _over:                \
    _pro_depth = __depth; \
    return __;

#define nedo(...)           \
    _over: { __VA_ARGS__; } \
    }                       \
    return __;

#define fail(code)                                                          \
    {                                                                       \
        __ = (code);                                                        \
        trace("%s<%s at %s:%i\n", PROind, ok64str(__), __func__, __LINE__); \
        goto _over;                                                         \
    }

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

#define sane(c) \
    if (!(c)) fail(FAILsanity);

#define call(f, ...)                                                 \
    {                                                                \
        __ = (f(__VA_ARGS__));                                       \
        if (__ != OK) {                                              \
            trace("%s<%s at %s:%i\n", PROind, ok64str(__), __func__, \
                  __LINE__);                                         \
            goto _over;                                              \
        }                                                            \
    }

#define mute(f, o)            \
    {                         \
        __ = (f);             \
        if (__ == o) __ = OK; \
        if (__ != OK) {       \
            goto _over;       \
        }                     \
    }

#endif
