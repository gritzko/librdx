#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "$.h"
#include "B.h"
#include "PRO.h"

#define T X(, )

typedef T const X(, c);
typedef T const *X(, cp);

typedef T *X($, )[2];
typedef T *const X($c, )[2];
typedef T const *X($, c)[2];
typedef T const *const X($c, c)[2];

typedef T **X(, $);
typedef T const **X(, c$);
typedef T const *const *X(, $cc);

typedef X($, ) * X($$, )[2];

typedef int (*X(, cmpfn))(const X(, ) *, const X(, ) *);

fun void X($, sort)(X($c, ) data) { $sort(data, X(, cmp)); }

fun T *X($, bsearch)(T const *p, X($c, ) data) {
    return (T *)$bsearch(p, data, X(, cmp));
}

fun T const *X($, find)(X($c, c) haystack, T const *needle) {
    for (T const *p = haystack[0]; p < haystack[1]; p++)
        if (memcmp(p, needle, sizeof(T)) == 0) return p;
    return NULL;
}

fun ok64 X($, tail)(X($, c) into, X($c, c) from, size_t off) {
    if ($len(from) < off) return $miss;
    into[0] = from[0] + off;
    into[1] = from[1];
    return OK;
}

fun ok64 X($, last)(X($, c) into, X($c, c) from, size_t len) {
    if ($len(from) < len) return $miss;
    into[0] = from[1] - len;
    into[1] = from[1];
    return OK;
}

fun ok64 X($, part)(X($, c) into, X($c, c) orig, size_t from, size_t till) {
    if ($len(orig) < till || from > till) return $miss;
    into[0] = orig[0] + from;
    into[1] = orig[0] + till;
    return OK;
}

fun size_t X($, copy)(X($c, ) into, X($c, c) from) {
    size_t l = $minlen(into, from);
    memcpy((void *)*into, (void *)*from, l * sizeof(T));
    return l;
}

fun ok64 X($, feed)(X($, ) into, X($c, c) from) {
    if (unlikely(!$ok(from) || !$ok(into))) return $badarg;
    if ($size(from) > $size(into)) return $noroom;
    $feed(into, from);
    return OK;
}

fun ok64 X($, drain)(X($, ) into, X($, c) from) {
    size_t len = $len(into) < $len(from) ? $len(into) : $len(from);
    memcpy(*into, *from, len * sizeof(T));
    *into += len;
    *from += len;
    return OK;
}

fun void X($, move)(X($, ) into, X($, c) from) { $drain(into, from); }

fun void X(, mv)(T *into, T const *from) { memcpy(into, from, sizeof(T)); }

fun ok64 X($, feed1)(X($, ) into, T what) {
    if ($empty(into)) return $noroom;
#ifndef ABC_X_$
    X(, mv)(*into, (T const *)&what);
#else
    memcpy(*into, what, sizeof(T));
#endif
    ++*into;
    return OK;
}

fun ok64 X($, feed2)(X($, ) into, T a, T b) {
    if ($len(into) < 2) return $noroom;
    X(, mv)(*into, (T const *)&a);
    ++*into;
    X(, mv)(*into, (T const *)&b);
    ++*into;
    return OK;
}

fun ok64 X($, feed3)(X($, ) into, T a, T b, T c) {
    if ($len(into) < 3) return $noroom;
    X(, mv)(*into, (T const *)&a);
    ++*into;
    X(, mv)(*into, (T const *)&b);
    ++*into;
    X(, mv)(*into, (T const *)&c);
    ++*into;
    return OK;
}
fun ok64 X($, feedp)(X($, ) into, T const *what) {
    if ($empty(into)) return $noroom;
    X(, mv)(*into, what);
    ++*into;
    return OK;
}
fun void X($, drop)(X($, ) into, T const *from) {
    X(, mv)(*into, from);
    ++*into;
}

fun ok64 X($, take)(X($, c) prefix, X($, c) body, size_t len) {
    if (len > $len(body)) return $noroom;
    prefix[0] = body[0];
    prefix[1] = body[0] + len;
    body[0] += len;
    return OK;
}

fun ok64 X($, drainn)(X($, ) into, X($, c) from, size_t len) {
    if (len > $len(into) || len > $len(from)) {
        return len > $len(into) ? Bnoroom : Bnodata;
    }
    memcpy((void *)*into, (void *)*from, len * sizeof(T));
    *from += len;
    *into += len;
    return OK;
}

fun int X(, memcmp)(T const *a, T const *b) { return memcmp(a, b, sizeof(T)); }

fun size_t X($, prefix)(X($, c) common, X($c, c) a, X($c, c) b) {
    size_t lim = $len(a);
    if ($len(b) < lim) lim = $len(b);
    size_t l = 0;
    while (l < lim && X(, memcmp)(*a + l, *b + l) == 0) l++;
    common[0] = a[0];
    common[1] = a[0] + l;
    return l;
}

fun size_t X($, offset)(X($c, c) outer, X($c, c) inner) {
    assert(outer[0] <= inner[0] && outer[1] >= inner[1]);
    return inner[0] - outer[0];
}

fun void X(, swap)(T *a, T *b) {
    T c;
    memcpy(&c, a, sizeof(T));
    memcpy(a, b, sizeof(T));
    memcpy(b, &c, sizeof(T));
}

fun void X($, str0)(X($, c) s, T const *a) {
    T zero = {};
    s[0] = a;
    size_t i = 0;
    while (X(, cmp)((T const *)&zero, a + i) != 0) ++i;
    s[1] = a + i;
}

#undef T
