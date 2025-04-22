//
// Created by gritzko on 5/10/24.
//

#ifndef LIBRDX_$_H
#define LIBRDX_$_H
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "01.h"

#define $head(s) ((s)[0])
#define $term(s) ((s)[1])
#define $last(s) ((s)[1] - 1)
#define $len(s) ($term(s) - $head(s))
#define $size(s) (((uint8_t *)$term(s)) - ((uint8_t *)$head(s)))
#define $at(s, n) (*($head(s) + (n)))
#define $atp(s, n) (($head(s) + (n)))
#define $in(s, p) (p - s[0])
#define $in$(s, ins) (ins[0] - s[0])
#define $nil(s) (s != NULL && s[0] == NULL && s[1] == NULL)

#define $empty(s) ($head(s) >= $term(s))
#define $nospace(s, l) ($head(s) + (l) > $term(s))
#define $ok(s) (s != nil && *s != nil && s[1] >= s[0])
#define $within(h, n) (n[0] >= h[0] && n[1] <= h[1])

#define a$(T, n, a) T *n[2] = {(a), (a) + (sizeof(a) / sizeof(T))};

#define $off(s, o) ((s[0] + (o) < s[1]) ? (s[0] + (o)) : s[1])

#define a$dup(T, n, s) T *n[2] = {(s)[0], (s)[1]}
#define $dup(s) {(s)[0], (s)[1]}

#define a$tail(T, n, s, off) \
    $##T n = {(off) > $len(s) ? s[1] : s[0] + (off), s[1]};
#define a$last(T, n, s, len) \
    $##T n = {(len) > $len(s) ? s[0] : s[1] - (len), s[1]};
#define a$head(T, n, s, l) \
    T *n[2] = {s[0], ((l) > $len(s) ? s[1] : (s[0] + l))};

#define a$part(T, n, s, off, len) T *n[2] = {*s + off, *s + off + len};
#define a$lpart(T, n, s, off) T *n[2] = {(s)[0], (s)[0] + off}
#define a$rpart(T, n, s, off) T *n[2] = {(s)[0] + off, (s)[1]}
#define $set(a, b)   \
    {                \
        a[0] = b[0]; \
        a[1] = b[1]; \
    }

#define $put(s, p)                  \
    {                               \
        memcpy(*s, p, sizeof(**s)); \
        ++*s;                       \
    }

#define $mv(s1, s2)                           \
    {                                         \
        (s1)[0] = (s2)[0], (s1)[1] = (s2)[1]; \
    }

// produce a subslice given an offset
#define $mvnil(s1)                \
    {                             \
        s1[0] = nil, s1[1] = nil; \
    }

#define $sub(s, o) {$off(s, o), s[1]}

/** produce a subslice given offset and length */
#define $cut(s, o, l) {$off(s, o), $off(s, o + l)}

#define $trim2(s, v) \
    if ($len(s) > 0 && *$last(s) == v) --s[1];

#define $shift(s) (*(s[0]++))

#define $for(T, n, s) for (T *n = s[0]; (n + 1) <= s[1]; ++n)
#define $rof(T, n, s) for (T *n = s[1] - 1; n >= s[0]; --n)
#define $next(s) ((*s)++)
#define $eat(s) for (; s[0] < s[1]; s[0]++)
#define $eat2(s) for (; s[0] < s[1]; s[0] += 2)

#define $reverse(s)                                                    \
    {                                                                  \
        size_t sz = sizeof(**s);                                       \
        uint8_t tmp[sz];                                               \
        for (void *f = s[0], *t = s[1] - 1; f < t; f += sz, t -= sz) { \
            memcpy(tmp, f, sz);                                        \
            memcpy(f, t, sz);                                          \
            memcpy(t, tmp, sz);                                        \
        }                                                              \
    }

typedef int (*fncmp)(const void *, const void *);
#define $sort(s, cmp) qsort((void *)s[0], $len(s), sizeof(**s), (fncmp)cmp)
#define $bsearch(p, s, cmp) bsearch(p, s[0], $len(s), sizeof(**s), (fncmp)cmp)

#define $minlen(a, b) ($len(a) > $len(b) ? $len(b) : $len(a))
#define $minsize(a, b) ($size(a) > $size(b) ? $size(b) : $size(a))

#define $copy(into, from) \
    memcpy((void *)*into, (void *)*from, $minsize(into, from))

#define $fed(s) \
    {           \
        ++*s;   \
    }

#define $feed1(s, v) \
    {                \
        **(s) = (v); \
        ++*s;        \
    }

#define $feed(into, from)                 \
    {                                     \
        size_t sz = $minsize(into, from); \
        memcpy(*into, *from, sz);         \
        *(uint8_t **)into += sz;          \
    }

#define $drain(into, from)                         \
    {                                              \
        $copy(into, from);                         \
        *(uint8_t **)into += $minsize(into, from); \
        *(uint8_t **)from += $minsize(into, from); \
    }

fun int $memcmp($cc a, $cc b) {
    size_t sza = $size(a), szb = $size(b);
    size_t sz = sza < szb ? sza : szb;
    int ret = memcmp(*a, *b, sz);
    if (ret == 0 && sza != szb) {
        ret = sza < szb ? -1 : 1;
    }
    return ret;
}

typedef int (*$cmpfn)($cc a, $cc b);

#define $cmp(a, b) $memcmp((void const *const *)(a), (void const *const *)b)

#define $eq(a, b) (0 == $cmp(a, b))

#define $printf(into, fmt, ...)                                         \
    {                                                                   \
        int l = snprintf((char *)*into, $size(into), fmt, __VA_ARGS__); \
        *into += $size(into) < l ? $size(into) : l;                     \
    }

#define a$str(n, s)          \
    char const *__##n = (s); \
    u8 const *n[2] = {(u8 *)__##n, (u8 *)__##n + strlen(__##n)};

#define a$strc(n, c) u8 const *n[2] = {(u8c *)(c), (u8c *)((c) + strlen(c))}

fun ok64 $feedf(u8 **into, u8 const *const *tmpl, ...) {
    va_list ap;
    size_t l = 0;
    ok64 o = OK;
    u8 const **sarg = nil;
    a$dup(u8 const, p, tmpl);
    va_start(ap, tmpl);
    while (!$empty(p) && !$empty(into)) {
        if (**p != '$') {
            **into = **p;
            ++*into;
            ++*p;
            continue;
        }
        ++*p;
        if ($empty(p)) return $badarg;
        switch (**p) {
            case 's':
                sarg = va_arg(ap, u8 const **);
                $feed(into, sarg);
                ++*p;
                break;
            case 'u':
                $printf(into, "%lu", va_arg(ap, u64));  // TODO
                ++*p;
                break;
            case 'f':
                $printf(into, "%lf", va_arg(ap, double));
                ++*p;
                break;
            case '$':
                if ($len(into) < 2) return $noroom;
                **into = '$';
                ++*into;
                ++*p;
                break;
            default:
                if ($len(into) < 1) return $noroom;
                **into = '$';
                ++*into;
        }
    }
    va_end(ap);
    return $empty(p) ? OK : $noroom;
}

#define $tailshift(s, off, rm)                             \
    {                                                      \
        must($len(s) >= (off) + (rm));                     \
        memmove(*(s) + (off), *(s) + (off) + (rm),         \
                ($len(s) - (off) - (rm)) * sizeof(**(s))); \
    }

#define zero(s) memset((void *)&(s), 0, sizeof(s))
#define zerop(s) memset((void *)s, 0, sizeof(*s))
#define $zero(s)     \
    {                \
        s[0] = NULL; \
        s[1] = NULL; \
    }

#define a$findif(T, p, s, cond)          \
    T *p = s[0];                         \
    {                                    \
        while (p < s[1] && !(cond)) ++p; \
    }

#define $rm(s, off, len)                                                  \
    {                                                                     \
        memmove(s[0] + (off), s[0] + (off) + (len), (len) * sizeof(**s)); \
        s[1] -= len;                                                      \
    }

#define $rm1(s, off)                                          \
    {                                                         \
        memmove(s[0] + (off), s[0] + (off) + 1, sizeof(**s)); \
        s[1] -= 1;                                            \
    }

#define $rm1p(s, p)                                          \
    {                                                        \
        memmove(p, p + 1, ((u8c *)s[1]) - ((u8c *)(p + 1))); \
        s[1] -= 1;                                           \
    }

#endif  // LIBRDX_$_H
