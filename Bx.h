#include "$x.h"
#include "B.h"

#define T X(, )

typedef T *const X(B, )[4];
typedef T **X(, B);
typedef T const **X(, cB);
typedef X($, ) * X(B$, )[4];

fun T *const *X(B, past)(X(B, ) buf) { return (X(, ) **)buf + 0; }
fun T **X(B, data)(X(B, ) buf) { return (X(, ) **)buf + 1; }
fun T **X(B, idle)(X(B, ) buf) { return (X(, ) **)buf + 2; }

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

fun T *X(B, at)(X(B, ) buf, size_t ndx) {
    // todo range checks
    return buf[1] + ndx;
}

fun ok64 X(B, alloc)(X(B, ) buf, size_t len) {
    size_t sz = len * sizeof(T);
    ok64 o = Balloc((void **)buf, sz);
    if (o != OK) return o;
    memset(*buf, 0, sz);
    return OK;
}

fun ok64 X(B, free)(X(B, ) buf) { return Bfree((void **)buf); }

fun ok64 X(B, reserve)(X(B, ) buf, size_t len) {
    return Breserve((void *const *)buf, len * sizeof(T));
}

fun ok64 X(B, feedp)(X(B, ) buf, T const *one) {
    ok64 re = X(B, reserve)(buf, 1);
    if (re != OK) return re;
    T **idle = X(B, idle)(buf);
    memcpy(*idle, one, sizeof(T));
    ++*idle;
    return OK;
}

fun ok64 X(B, feed2)(X(B, ) buf, T a, T b) {
    ok64 re = X(B, reserve)(buf, 2);
    if (re != OK) return re;
    T **idle = X(B, idle)(buf);
    memcpy(*idle, &a, sizeof(T));
    ++*idle;
    memcpy(*idle, &b, sizeof(T));
    ++*idle;
    return OK;
}

fun ok64 X(B, feed1)(X(B, ) buf, T one) {
    return X(B, feedp)(buf, (T const *)&one);
}

fun ok64 X(B, feed$)(X(B, ) buf, X($c, c) from) {
    T **into = X(B, idle)(buf);
    if ($len(into) < $len(from)) return Bnoroom;
    X($, copy)(into, from);
    *into += $len(from);
    return OK;
}

fun ok64 X(B, pop)(X(B, ) buf) {
    if (buf[2] <= buf[1]) return Bnodata;
    --*X(B, idle)(buf);
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
    size_t len = $len(buf);
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

#undef T
