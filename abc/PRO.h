#ifndef LIBRDX_PRO_H
#define LIBRDX_PRO_H

// It is not recommended to include this header from
// public .h files. It pollutes the namespace for non-ABC code.
// PRO.h use implies use of MAIN(), TEST() or fuzz() macros in executables.

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "BUF.h"
#include "OK.h"

extern uint8_t _pro_depth;

// use this with every int main() {...}
#define ABC_INIT uint8_t _pro_depth = 0;

con char *_pro_indent =
    "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

#define PROindent (_pro_indent + 32 - (_pro_depth & 31))

// Mandatory sanity checks; might be disabled in Release mode.
#ifndef ABC_INSANE
#define sane(c)                                                           \
    trace("%s>%s\n", PROindent, __func__);                                \
    if (!(c)) {                                                           \
        trace("%s<FAILSANITY at %s:%i\n", PROindent, __func__, __LINE__); \
        return FAILSANITY;                                                \
    }                                                                     \
    ok64 __ = OK;
#else
#define sane(c) ok64 __ = OK;
#endif

#define call(f, ...)                                                    \
    {                                                                   \
        u8 __depth = _pro_depth++;                                      \
        __ = (f(__VA_ARGS__));                                          \
        _pro_depth = __depth;                                           \
        if (__ != OK) {                                                 \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(__), __func__, \
                  __LINE__);                                            \
            return __;                                                  \
        }                                                               \
    }

// e.g. scan(u64sDrain1, numbers, &i) { ... }  seen(NODATA);
#define scan(f, ...) while (OK == (__ = f(__VA_ARGS__)))

#define try(f, ...)                                                     \
    {                                                                   \
        u8 __depth = _pro_depth++;                                      \
        __ = (f(__VA_ARGS__));                                          \
        _pro_depth = __depth;                                           \
        if (__ != OK) {                                                 \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(__), __func__, \
                  __LINE__);                                            \
        }                                                               \
    }

#define then if (__ == OK)

#define nedo if (__ != OK)

#define on(status) if (__ == status)

#define callsafe(fcall, ffail)                                          \
    {                                                                   \
        u8 __depth = _pro_depth++;                                      \
        __ = (fcall);                                                   \
        _pro_depth = __depth;                                           \
        if (__ != OK) {                                                 \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(__), __func__, \
                  __LINE__);                                            \
            ffail;                                                      \
            return __;                                                  \
        }                                                               \
    }

// Procedure return with no finalizations.
#define done return __;

// Procedure fails, skip to finalizations.
#define fail(code)                                                    \
    {                                                                 \
        trace("%s<%s at %s:%i\n", PROindent, ok64str(code), __func__, \
              __LINE__);                                              \
        return (code);                                                \
    }

#define failc(code)                                                            \
    {                                                                          \
        trace("%s<%s errno %i %s at %s:%i\n", PROindent, ok64str(code), errno, \
              strerror(errno), __func__, __LINE__);                            \
        return (code);                                                         \
    }

#define test(c, f) \
    if (!likely(c)) fail(f);

#define testsafe(c, f, cleanup) \
    if (!(c)) {                 \
        __ = f;                 \
        cleanup;                \
        fail(__);               \
    }

#define testc(c, f) \
    if (!(c)) failc(f);

#define otry(f, ...) __ = (f(__VA_ARGS__))

#define ofix(f) if (__ == (f) && OK == (__ = OK))

#define orly if (__ == OK)

#define oops if (__ != OK)

#define ocry(...)    \
    if (__ != OK) {  \
        __VA_ARGS__; \
        fail(__);    \
    }

#define is(f) (__ == f)

#define sure(f) \
    if (__ != f) fail(__);

#define seen(f)    \
    if (__ != f) { \
        fail(__);  \
    } else {       \
        __ = OK;   \
    }

#define mute(f, o)            \
    {                         \
        ok64 __ = (f);        \
        if (__ == o) __ = OK; \
        if (__ != OK) {       \
            fail(__);         \
        }                     \
    }

#define testeq(a, b)               \
    {                              \
        if (!likely((a) == (b))) { \
            fail(FAILEQ);          \
        }                          \
    }

#define testeqv(a, b, fmt)                                                     \
    {                                                                          \
        if (!likely((a) == (b))) {                                             \
            fprintf(stderr, "%sNot equal: " fmt " <> " fmt "\n", PROindent, a, \
                    b);                                                        \
            fail(FAILEQ)                                                       \
        }                                                                      \
    }

#define must(cond, msg)                                                 \
    if (!(cond)) {                                                      \
        fprintf(stderr, "%s assert fail %s at %s:%i\n", PROindent, msg, \
                __func__, __LINE__);                                    \
        __builtin_trap();                                               \
    }

#define $testeq(a, b)                                                         \
    {                                                                         \
        if (!likely($size(a) == $size(b) && 0 == memcmp(*a, *b, $size(a)))) { \
            fail(FAILEQ)                                                      \
        }                                                                     \
    }

#ifdef ABC_TRACE
#define trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define trace(...) ;
#endif

extern u8cs _STD_ARGS[];
extern u8cs *STD_ARGS[];

fun void _parse_args(int argn, char **args) {
    for (int i = 0; i < argn; ++i) {
        u8c *s[2] = $u8str(args[i]);
        memcpy(_STD_ARGS + i, s, sizeof(s));
    }
    STD_ARGS[0] = STD_ARGS[1] = _STD_ARGS;
    STD_ARGS[2] = STD_ARGS[3] = _STD_ARGS + argn;
}

#define $arglen $len(u8csbData(STD_ARGS))

#define $arg(i) (*u8cssAtP(u8csbData(STD_ARGS), i))

#define a$rg(name, i) \
    u8cs name = {};   \
    $mv(name, *u8cssAtP(u8csbData(STD_ARGS), i));

#define an_arg(name, i) \
    u8cs name = {};     \
    if ($arglen > (i)) $mv(name, *u8cssAtP(u8csbData(STD_ARGS), i));

// Redirect stderr to file for tracing
// If ABC_TRACE env var is set to a filename, use that; otherwise
// /tmp/name-pid.err Prints path to stderr before redirecting, returns OK on
// success
fun ok64 PROStderrToFile(char const *name) {
    char const *env = getenv("ABC_TRACE");
    char path[256];
    if (env && *env && *env != '1') {
        // ABC_TRACE=filename - use as-is
        int n = snprintf(path, sizeof(path), "%s", env);
        if (n < 0 || n >= (int)sizeof(path)) return NOROOM;
    } else {
        // Default: /tmp/basename-pid.err
        char const *base = name;
        for (char const *p = name; *p; p++) {
            if (*p == '/') base = p + 1;
        }
        int n = snprintf(path, sizeof(path), "/tmp/%s-%d.err", base, getpid());
        if (n < 0 || n >= (int)sizeof(path)) return NOROOM;
    }
    fprintf(stderr, "stderr: %s\n", path);
    if (!freopen(path, "w", stderr)) return FAILSANITY;
    return OK;
}

#ifdef ABC_TRACE
#define _PRO_TRACE_INIT(name) PROStderrToFile(name)
#else
#define _PRO_TRACE_INIT(name)
#endif

#define MAIN(f)                                                          \
    uint8_t _pro_depth = 0;                                              \
    u8cs _STD_ARGS[64] = {};                                             \
    u8cs *STD_ARGS[4] = {};                                              \
    int main(int argn, char **args) {                                    \
        _PRO_TRACE_INIT(args[0]);                                        \
        _parse_args(argn, args);                                         \
        ok64 ret = f();                                                  \
        if (ret != OK) {                                                 \
            trace("%s<%s at %s:%i\n", PROindent, ok64str(ret), __func__, \
                  __LINE__);                                             \
            fprintf(stderr, "Error: %s\n", ok64str(ret));                \
        }                                                                \
        return ret;                                                      \
    }

#endif
