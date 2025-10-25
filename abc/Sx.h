#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "01.h"
#include "B.h"
#include "OK.h"
#include "S.h"

#define T X(, )

typedef T const X(, c);
#ifndef ABC_X_$
typedef T *X(, p);
typedef T const *X(, cp);
#endif

typedef T *X($, )[2];
typedef T *const X($c, )[2];
typedef T const *X($, c)[2];
typedef T const *const X($c, c)[2];

typedef T *X(, s)[2];
typedef T *const X(, sc)[2];
typedef T const *X(, cs)[2];
typedef T const *const X(, csc)[2];
typedef T **X(, sp);
typedef T *const *X(, spc);
typedef T const **X(, csp);
typedef X(, cs) * *X(, cssp);
typedef X(, cs) const **X(, cscsp);
typedef T const *const *X(, cspc);
typedef X(, s) * X(, ss)[2];
typedef X(, cs) * X(, css)[2];
typedef X(, p) * X(, ps)[2];

typedef T **X(, $);
typedef T const **X(, c$);
typedef T *const *X(, $c);
typedef T const *const *X(, c$c);
typedef T const *const *X(, $cc);

typedef X($, ) * X($$, )[2];

typedef int (*X(, cmpfn))(const X(, ) *, const X(, ) *);

typedef ok64 (*X(, x))(X(, p) a, X(, cp) b);
typedef ok64 (*X(, y))(X(, p) a, X(, cs) b);
typedef ok64 (*X(, z))(X(, cp) a, X(, cp) b);

typedef b8 (*X(, isfn))(const X(, ) *);

fun size_t X(, sLen)(X(, sc) data) { return data[1] - data[0]; }
fun size_t X(, csLen)(X(, csc) data) { return data[1] - data[0]; }

fun T *X($, last)(X($c, ) data) {
    assert(!$empty(data));
    return data[1] - 1;
}

fun T const *X($, lastc)(X($c, c) data) {
    assert(!$empty(data));
    return data[1] - 1;
}

fun void X($, sort)(X($c, ) data) { $sort(data, X(, cmp)); }

fun void X($, sortfn)(X($c, ) data, X(, cmpfn) fn) { $sort(data, fn); }

fun T *X($, bsearch)(T const *p, X($c, c) data) {
    return (T *)$bsearch(p, data, X(, cmp));
}

// Find the first entry >= needle or $term if none TODO test
fun T const *X($, findge)(X($c, c) haystack, T const *needle) {
    size_t b = 0, e = $len(haystack);
    if (e == 0) return haystack[1];
    while (e > b + 1) {
        size_t m = (b + e) >> 1;
        int c = X(, cmp)($atp(haystack, m), needle);
        if (c < 0) {
            b = m;
        } else {
            e = m;
        }
    }
    int c = X(, cmp)($atp(haystack, b), needle);
    return $atp(haystack, c < 0 ? e : b);
}

#ifndef ABC_X_$
fun T X(, sAt)(X($, ) s, size_t pos) { return s[0][pos]; }
#endif

fun T *X(, sAtP)(X($, ) s, size_t pos) { return s[0] + pos; }

fun ok64 X($, eat1)(X($, ) s) {
    if (s[0] >= s[1]) return $nodata;
    ++s[0];
    return OK;
}

fun ok64 X($, eatall)(X($, ) s) {
    s[0] = s[1];
    return OK;
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

fun ok64 X($, retract)(X($, c) from, size_t len) {
    if ($len(from) < len) return $miss;
    from[1] -= len;
    return OK;
}

fun ok64 X(, sSup)(X(, s) full, X(, s) consumed) {
    if (unlikely(full[1] != consumed[1] || full[0] > consumed[0])) return $miss;
    full[1] = consumed[0];
    return OK;
}

fun ok64 X(, scSup)(X(, cs) full, X(, cs) consumed) {
    if (unlikely(full[1] != consumed[1] || full[0] > consumed[0])) return $miss;
    full[1] = consumed[0];
    return OK;
}

/*
fun ok64 X($, last)(X($, c) into, X($c, c) from, size_t len) {
    if ($len(from) < len) return $miss;
    into[0] = from[1] - len;
    into[1] = from[1];
    return OK;
}
*/
fun ok64 X($, part)(X($, c) into, X($c, c) orig, size_t from, size_t till) {
    if ($len(orig) < till || from > till) return $miss;
    into[0] = orig[0] + from;
    into[1] = orig[0] + till;
    return OK;
}

fun size_t X(, sCopy)(X(, sc) into, X(, csc) from) {
    size_t l = $minlen(into, from);
    memcpy((void *)*into, (void *)*from, l * sizeof(T));
    return l;
}

fun ok64 X($, alloc)(X($, ) what, size_t len) {
    T *m = (T *)malloc(len * sizeof(T));
    if (m == NULL) return noroom;
    what[0] = m;
    what[1] = m + len;
    return OK;
}

fun b8 X($, ok)(X($, c) orig) {
    return $ok(orig) && $size(orig) % sizeof(T) == 0;
}

fun ok64 X($, dup)(X($, ) copy, X($, c) orig) {
    ok64 o = X($, alloc)(copy, $len(orig));
    if (o != OK) return o;
    memcpy((void *)*copy, (void *)*orig, $size(orig));
    return OK;
}

fun ok64 X($, free)(X($, c) what) {
    if (what[0] == NULL) return $badarg;
    free((void *)what[0]);
    what[0] = what[1] = NULL;
    return OK;
}

fun u64 X(, cs_len)(X(, cs) s) {
    if (s == NULL || s[0] == NULL || s[1] <= s[0]) return 0;
    return s[1] - s[0];
}

fun u64 X(, s_len)(X(, s) s) { return X(, cs_len)((X(, csp))s); }

fun ok64 X(, s_feed)(X(, s) into, X(, cs) from) {
    if (unlikely(X(, cs_len)(from) > X(, s_len)(into))) return $noroom;
    memcpy((void *)*into, (void *)*from, $size(from));
    *into += $len(from);
    return OK;
}
/*
fun ok64 X(, sFeed1)(X(, s) into, X(, ) from) {
    if (1 > X(, s_len)(into)) return $noroom;
    memcpy((void *)*into, (void *)&from, sizeof(T));
    *into += 1;
    return OK;
}
*/
fun ok64 X($, feed)(X($, ) into, X($c, c) from) {
    if (unlikely(!$ok(from) || !$ok(into))) return $badarg;
    if (unlikely($size(from) > $size(into))) return $noroom;
    memcpy((void *)*into, (void *)*from, $size(from));
    *into += $len(from);
    return OK;
}

fun ok64 X(, sFeed)(X(, s) into, X(, csc) from) {
    return X($, feed)(into, from);
}

fun ok64 X($, feedall)(X($, ) into, X($c, c) from) {
    if (unlikely(!$ok(from) || !$ok(into))) return $badarg;
    if (unlikely($size(from) > $size(into))) return $noroom;
    memcpy((void *)*into, (void *)*from, $size(from));
    *into += $len(from);
    return OK;
}

fun ok64 X($, drain)(X($, ) into, X($, c) from) {
    size_t len = $len(into) < $len(from) ? $len(into) : $len(from);
    memcpy((void *)*into, (void *)*from, len * sizeof(T));
    *into += len;
    *from += len;
    return OK;
}

fun ok64 X($, take)(X(, c$) prefix, X($, c) from, size_t len) {
    if ($len(from) < len) return $nodata;
    prefix[0] = from[0];
    from[0] += len;
    prefix[1] = from[0];
    return OK;
}

fun ok64 X($, move)(X($, ) into, X($, c) from) {
    if ($len(into) < $len(from)) return $noroom;
    memmove((void *)*into, (void *)*from, $size(from));
    return OK;
}

fun b8 X(, csOK)(X(, csc) s) {
    return s != NULL && s[1] >= s[0] &&
           (((u8c *)s[1] - (u8c *)s[0]) % sizeof(T) == 0);
}

fun b8 X(, sOK)(X(, sc) s) { return X(, csOK)((T const **)s); }

fun void X(, mv)(T *into, T const *from) { Ocopy(into, from); }

fun void X(, Move)(X(, p) into, X(, cp) from) { Ocopy(into, from); }

fun ok64 X(, sFeed1)(X(, s) into, T what) {
    if ($empty(into)) return $noroom;
#ifndef ABC_X_$
    X(, mv)(*into, (T const *)&what);
#else
    memcpy((void *)*into, (void *)what, sizeof(T));
#endif
    ++*into;
    return OK;
}

fun ok64 X(, sFed1)(X(, s) into) {
    if (unlikely(into[0] >= into[1])) return $noroom;
    ++*into;
    return OK;
}

fun ok64 X(, sDrain1)(X(, cs) from, T *into) {
    if ($empty(from)) return $nodata;
#ifndef ABC_X_$
    X(, mv)(into, *from);
#else
    Ocopy(into, *from);
#endif
    ++*from;
    return OK;
}
fun ok64 X(, sFeed2)(X($, ) into, T a, T b) {
    if ($len(into) < 2) return $noroom;
    X(, mv)(*into, (T const *)&a);
    ++*into;
    X(, mv)(*into, (T const *)&b);
    ++*into;
    return OK;
}

fun ok64 X(, s_feed3)(X(, s) into, T a, T b, T c) {
    if ($len(into) < 3) return $noroom;
    X(, sFeed1)(into, a);
    X(, sFeed1)(into, b);
    X(, sFeed1)(into, c);
    return OK;
}
fun ok64 X(, sFeedP)(X(, s) into, T const *what) {
    if ($empty(into)) return $noroom;
    X(, mv)(*into, what);
    ++*into;
    return OK;
}

fun b8 X($, empty)(X($, ) s) { return $empty(s); }
fun b8 X($, cempty)(X($, c) s) { return $empty(s); }

fun void X($, drop)(X($, ) into, T const *from) {
    X(, mv)(*into, from);
    ++*into;
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

fun void X(, csDup)(X(, csp) a, X(, cspc) b) {
    a[0] = b[0];
    a[1] = b[1];
}

fun void X(, sDup)(X(, sp) a, X(, spc) b) {
    a[0] = b[0];
    a[1] = b[1];
}

fun void X(, Swap)(T *a, T *b) {
    u8 c[sizeof(T)];
    Ocopy(&c, a);
    Ocopy(a, b);
    Ocopy(b, &c);
}

fun ok64 X(, sSwap)(X(, s) s, size_t a, size_t b) {
    size_t l = X(, sLen)(s);
    if (unlikely(a >= l || b >= l)) return badarg;
    X(, Swap)(*s + a, *s + b);
    return OK;
}

fun void X(, s_purge)(X($, ) s, X(, isfn) f) {
    for (int i = 0; i < $len(s); ++i) {
        T *p = X(, sAtP)(s, i);
        if (f(p)) {
            X(, Swap)(p, $last(s));
            --$term(s);
        }
    }
}

fun void X($, str0)(X($, c) s, T const *a) {
    u8 v0[sizeof(T)] = {};
    s[0] = a;
    size_t i = 0;
    while (X(, cmp)((T const *)&v0, a + i) != 0) ++i;
    s[1] = a + i;
}

static const u8 X(, zero)[sizeof(T)] = {};

fun b8 X($, is0)(X($, ) s, size_t ndx) {
    return X(, cmp)((T const *)X(, zero), X(, sAtP)(s, ndx)) == 0;
}

fun b8 X(, csHasSuffix)(X(, cs) line, X(, cs) suffix) {
    size_t l = $len(suffix);
    size_t s = $size(suffix);
    return l <= $len(line) && 0 == memcmp(line[1] - l, suffix[0], s);
}

#undef T
