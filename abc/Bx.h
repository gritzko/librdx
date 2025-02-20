#include "$x.h"
#include "B.h"
#include "OK.h"

#define T X(, )

typedef T *const X(B, )[4];
typedef T **X(, B);
typedef T const **X(, cB);
typedef X($, ) * X(B$, )[4];

fun T *const *X(B, past)(X(B, ) buf) { return (T **)buf + 0; }
fun T const *const *X(B, pastc)(X(B, ) buf) {
    return (T const *const *)buf + 0;
}
fun T **X(B, data)(X(B, ) buf) { return (T **)buf + 1; }
fun T **X(B, idle)(X(B, ) buf) { return (T **)buf + 2; }

fun T **X(B, $1)(X(B, ) buf) { return (T **)buf + 1; }
fun T **X(B, $2)(X(B, ) buf) { return (T **)buf + 2; }
fun T const **X(B, c$1)(X(B, ) buf) { return (T const **)buf + 1; }
fun T const **X(B, c$2)(X(B, ) buf) { return (T const **)buf + 2; }

fun void X(B, eat1)(X(B, ) buf) { ((T **)buf)[1] = buf[2]; }
fun void X(B, eat2)(X(B, ) buf) { ((T **)buf)[2] = buf[3]; }
fun void X(B, eatdata)(X(B, ) buf) { ((T **)buf)[1] = buf[2]; }
fun void X(B, eatidle)(X(B, ) buf) { ((T **)buf)[2] = buf[3]; }

fun b8 X(B, hasroom)(X(B, ) buf) { return !$empty(X(B, idle)(buf)); }
fun b8 X(B, hasdata)(X(B, ) buf) { return !$empty(X(B, data)(buf)); }

fun T const *const *X(B, cpast)(X(B, ) buf) {
    return (T const *const *)buf + 0;
}
fun T const **X(B, cdata)(X(B, ) buf) { return (T const **)buf + 1; }
fun T const **X(B, cidle)(X(B, ) buf) { return (T const **)buf + 2; }

fun T *const *X(Bc, past)(X(B, ) buf) { return (T *const *)buf + 0; }
fun T *const *X(Bc, data)(X(B, ) buf) { return (T *const *)buf + 1; }
fun T *const *X(Bc, idle)(X(B, ) buf) { return (T *const *)buf + 2; }

fun T const *const *X(Bc, cpast)(X(B, ) buf) {
    return (T const *const *)buf + 0;
}
fun T const *const *X(Bc, cdata)(X(B, ) buf) {
    return (T const *const *)buf + 1;
}
fun T const *const *X(Bc, cidle)(X(B, ) buf) {
    return (T const *const *)buf + 2;
}
fun b8 X(B, empty)(X(B, ) buf) { return buf[2] == buf[1]; }

fun T *X(B, atp)(X(B, ) buf, size_t ndx) {
    T *p = buf[0] + ndx;
    assert(p < buf[3]);
    return p;
}

fun T *X(B, last)(X(B, ) buf) {
    size_t len = buf[2] - buf[0];
    return X(B, atp)(buf, len - 1);
}

/*fun T X(B, at)(X(B, ) buf, size_t ndx) {
    T *p = buf[0] + ndx;
    assert(p < buf[3]);
    return *p;
}*/

fun ok64 X(B, alloc)(X(B, ) buf, size_t len) {
    size_t sz = len * sizeof(T);
    ok64 o = Balloc((void **)buf, sz);
    if (o != OK) return o;
    memset((void *)*buf, 0, sz);
    return OK;
}

fun ok64 X(B, free)(X(B, ) buf) { return Bfree((void **)buf); }

fun ok64 X(B, reserve)(X(B, ) buf, size_t len) {
    return Breserve((void *const *)buf, len * sizeof(T));
}
/*
fun ok64 X(B, feedp)(X(B, ) buf, T const *one) {
    ok64 re = X(B, reserve)(buf, 1);
    if (re != OK) return re;
    T **idle = X(B, idle)(buf);
    memcpy(*idle, one, sizeof(T));
    ++*idle;
    return OK;
}
*/
fun ok64 X(B, feedp)(X(B, ) buf, T const *one) {
    return X($, feedp)(X(B, idle)(buf), one);
}

fun ok64 X(B, feed2)(X(B, ) buf, T a, T b) {
    // ok64 re = X(B, reserve)(buf, 2);
    // f (re != OK) return re;
    T **idle = X(B, idle)(buf);
    if ($len(idle) < 2) return Bnoroom;
    memcpy((void *)*idle, &a, sizeof(T));
    ++*idle;
    memcpy((void *)*idle, &b, sizeof(T));
    ++*idle;
    return OK;
}

fun ok64 X(B, feed1)(X(B, ) buf, T one) {
    return X($, feed1)(X(B, idle)(buf), one);
}

fun ok64 X(B, feed$)(X(B, ) buf, X($c, c) from) {
    T **into = X(B, idle)(buf);
    if ($len(into) < $len(from)) return Bnoroom;
    X($, copy)(into, from);
    *into += $len(from);
    return OK;
}

fun ok64 X(B, mark)(X(B, ) const buf, range64 *range) {
    range->from = buf[1] - buf[0];
    range->till = buf[2] - buf[0];
    return OK;
}

fun void X(B, reset)(X(B, ) buf) {
    T **b = (T **)buf;
    b[1] = b[0];
    b[2] = b[0];
}

fun ok64 X(B, rewind)(X(B, ) buf, range64 range) {
    size_t len = Blen(buf);
    if (range.till < range.from || range.till > len) return $miss;
    T **b = (T **)buf;
    b[1] = b[0] + range.from;
    b[2] = b[0] + range.till;
    return OK;
}

// fun void X(B, reset)(X(B, ) buf) { X(B, rewind)(buf, 0, 0); }

fun ok64 X(B, mark$)(X(B, ) const buf, X($, ) slice, range64 *range) {
    if (!Bwithin(buf, slice)) return Bmiss;
    range->from = slice[0] - buf[0];
    range->till = slice[1] - buf[0];
    return OK;
}

fun ok64 X($, mark)(X($, c) const host, X($, c) const slice, range64 *range) {
    if (!$within(host, slice)) return $miss;
    range->from = slice[0] - host[0];
    range->till = slice[1] - host[0];
    return OK;
}

fun ok64 X($, rewind)(X($c, c) host, X($, c) slice, range64 range) {
    size_t len = $len(host);
    if (range.till < range.from || range.till > len) return $miss;
    slice[0] = host[0] + range.from;
    slice[1] = host[0] + range.till;
    return OK;
}

fun ok64 X(B, rewind$)(X(B, ) buf, X($, ) slice, range64 range) {
    size_t len = Blen(buf);
    if (range.till < range.from || range.till > len) return Bmiss;
    slice[0] = buf[0] + range.from;
    slice[1] = buf[0] + range.till;
    return OK;
}

fun ok64 X(B, push)(X(B, ) buf, const T *val) { return X(B, feedp)(buf, val); }

fun T const *X(B, top)(X(B, ) buf) {
    assert(buf[2] > buf[1]);
    return buf[2] - 1;
}

fun ok64 X(B, pop)(X(B, ) buf) {
    if (buf[2] <= buf[1]) return Bnodata;
    T const **b = (T const **)buf;
    --b[2];
    return OK;
}

fun ok64 X(B, map)(X(B, ) buf, size_t len) {
    size_t size = len * sizeof(T);
    T *map = (T *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == MAP_FAILED) {
        return Bmapfail;
    }
    T **b = (T **)buf;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += len;
    return OK;
}

fun ok64 X(B, unmap)(X(B, ) buf) {
    if (unlikely(buf == nil || *buf == nil)) return FAILsanity;
    if (-1 == munmap((void *)buf[0], Bsize(buf))) return Bmapfail;
    void **b = (void **)buf;
    b[0] = b[1] = b[2] = b[3] = nil;
    return OK;
}

#undef T
