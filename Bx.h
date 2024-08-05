#include "$x.h"
#include "B.h"

#define T X(, )

typedef T *const X(B, )[4];

fun T *const *X(B, past)(X(B, ) buf) { return (X(, ) **)buf + 0; }
fun T **X(B, data)(X(B, ) buf) { return (X(, ) **)buf + 1; }
fun T **X(B, idle)(X(B, ) buf) { return (X(, ) **)buf + 2; }

fun T const *const *X(B, cpast)(X(, c$) buf) {
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
    return Balloc((void **)buf, len * sizeof(T));
}

fun ok64 X(B, free)(X(B, ) buf) { return Bfree((void **)buf); }

fun ok64 X(B, reserve)(X(B, ) buf, size_t len) {
    if ($len(X(B, idle)(buf)) >= len) return OK;
    return notimplyet;
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

fun ok64 X(B, feed1)(X(B, ) buf, T one) { return X(B, feedp)(buf, &one); }

fun ok64 X(B, feed$)(X(B, ) buf, X($c, c) from) {
    T **into = X(B, idle)(buf);
    if ($len(into) < $len(from)) return Bnoroom;
    X($, copy)(into, from);
    *into += $len(from);
    return OK;
}

fun void X(B, reset)(X(B, ) buf, size_t past, size_t data) {
    T **b = (T **)buf;
    if (past > Blen(buf)) past = Blen(buf);
    if (past + data > Blen(buf)) data = Blen(buf) - past;
    b[1] = b[0] + past;
    b[2] = b[0] + past + data;
}

fun void X(B, restart)(X(B, ) buf) { X(B, reset)(buf, 0, 0); }

fun ok64 X(B, pop)(X(B, ) buf) {
    if (buf[2] <= buf[1]) return Bnodata;
    --*X(B, idle)(buf);
    return OK;
}

#undef T
