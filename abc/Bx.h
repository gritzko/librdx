#include "B.h"
#include "OK.h"
#include "Sx.h"

#define T X(, )

typedef T *X(B, )[4];
typedef X($, ) * X(B$, )[4];
#ifndef ABC_X_$
typedef X($, c) * X(B$, c)[4];
#endif

typedef T *const X(, b)[4];
typedef T const *const X(, cb)[4];
typedef T *const *X(, bp);
typedef T const *const *X(, cbp);
typedef X($, ) *const X(, sb)[4];
typedef X($, c) *const X(, csb)[4];

fun T *const *X(B, past)(X(B, ) buf) { return (T **)buf + 0; }
fun T const *const *X(B, pastc)(X(B, ) buf) {
    return (T const *const *)buf + 0;
}

fun T **X(B, $1)(X(B, ) buf) { return (T **)buf + 1; }
fun T **X(B, $2)(X(B, ) buf) { return (T **)buf + 2; }
fun T const **X(B, c$1)(X(B, ) buf) { return (T const **)buf + 1; }
fun T const **X(B, c$2)(X(B, ) buf) { return (T const **)buf + 2; }

fun void X(B, eat$1)(X(B, ) buf) { ((T **)buf)[1] = buf[2]; }
fun void X(B, eat$2)(X(B, ) buf) { ((T **)buf)[2] = buf[3]; }
fun void X(B, eatdata)(X(B, ) buf) { ((T **)buf)[1] = buf[2]; }
fun void X(B, eatidle)(X(B, ) buf) { ((T **)buf)[2] = buf[3]; }

fun void X(B, resetpast)(X(B, ) buf) { ((T **)buf)[1] = buf[0]; }
fun void X(B, resetdata)(X(B, ) buf) { ((T **)buf)[2] = buf[1]; }

fun T const **X(, cbPast)(X(, cb) buf) { return (T const **)buf + 0; }
fun T const **X(, cbData)(X(, cb) buf) { return (T const **)buf + 1; }
fun T const **X(, cbIdle)(X(, cb) buf) { return (T const **)buf + 2; }
fun T const **X(, bPastC)(X(, b) buf) { return (T const **)buf + 0; }
fun T const **X(, bDataC)(X(, b) buf) { return (T const **)buf + 1; }
fun T const **X(, bIdleC)(X(, b) buf) { return (T const **)buf + 2; }
fun T **X(, bPast)(X(, b) buf) { return (T **)buf + 0; }
fun T **X(, bData)(X(, b) buf) { return (T **)buf + 1; }
fun T **X(, bIdle)(X(, b) buf) { return (T **)buf + 2; }

fun size_t X(, bLen)(X(, b) buf) { return ((T *)buf[3]) - ((T *)buf[0]); }
fun size_t X(, bBusyLen)(X(, b) buf) { return ((T *)buf[2]) - ((T *)buf[0]); }
fun size_t X(, bPastLen)(X(, b) buf) { return $len((T **)buf + 0); }
fun size_t X(, bDataLen)(X(, b) buf) { return $len((T **)buf + 1); }
fun size_t X(, bIdleLen)(X(, b) buf) { return $len((T **)buf + 2); }

fun size_t X(, cbPastLen)(X(, cb) buf) { return $len((T **)buf + 0); }
fun size_t X(, cbDataLen)(X(, cb) buf) { return $len((T **)buf + 1); }
fun size_t X(, cbIdleLen)(X(, cb) buf) { return $len((T **)buf + 2); }

fun b8 X(, bHasRoom)(X(, bp) buf) { return !$empty(X(, bIdle)(buf)); }
fun b8 X(, bHasData)(X(, bp) buf) { return !$empty(X(, bData)(buf)); }

fun b8 X(, bOK)(const X(, bp) buf) { return Bok(buf); }
fun b8 X(, cbOK)(const X(, cbp) buf) { return Bok(buf); }

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

fun T *X(, bAtP)(X(, b) buf, size_t ndx) {
    T *p = buf[0] + ndx;
    assert(p < buf[3]);
    return p;
}

#ifndef ABC_X_$
fun T X(, bAt)(X(, b) buf, size_t ndx) { return *X(, bAtP)(buf, ndx); }
#endif

fun void X(B, eat)(X(B, ) buf) {
    T const **b = (T const **)buf;
    b[1] = b[2];
}

fun ok64 X(, bFed)(X(, b) buf, size_t len) {
    return X(, sFed)(X(, bIdle)(buf), len);
}

fun ok64 X(, bFed1)(X(, b) buf) { return X(, sFed)(X(, bIdle)(buf), 1); }

fun b8 X(, bDataEmpty)(const X(, cbp) buf) { return X(, cbDataLen)(buf) == 0; }

fun b8 X(, bEmpty)(const X(, bp) buf) { return buf[2] == buf[0]; }

fun T *X(, bLast)(X(, b) buf) {
    assert(buf[2] > buf[1]);
    return buf[2] - 1;
}

/*fun T X(B, at)(X(B, ) buf, size_t len) {
    T *p = buf[0] + len;
    assert(p < buf[3]);
    return *p;
}*/

fun ok64 X(, bAllocate)(X(, bp) buf, size_t len) {
    size_t sz = len * sizeof(T);
    ok64 o = Balloc((void **)buf, sz);
    if (o != OK) return o;
    memset((void *)*buf, 0, sz);
    return OK;
}

fun ok64 X(, bFree)(X(, bp) buf) { return Bfree((void **)buf); }

fun ok64 X(, bReserve)(X(, bp) buf, size_t len) {
    return Breserve((void *const *)buf, len * sizeof(T));
}
/*
fun ok64 X(B, feedp)(X(B, ) buf, T const *one) {
    ok64 re = X(B, reserve)(buf, 1);
    if (re != OK) return re;
    T **idle = X(,bIdle)(buf);
    memcpy(*idle, one, sizeof(T));
    ++*idle;
    return OK;
}
*/

fun ok64 X(, bPush)(X(, bp) buf, X(, cp) one) {
    return X(, sFeedP)(X(, bIdle)(buf), one);
}

fun ok64 X(, bFeedP)(X(, bp) buf, T const *one) {
    return X(, sFeedP)(X(, bIdle)(buf), one);
}

fun ok64 X(, bFeed2)(X(, bp) buf, T a, T b) {
    // ok64 re = X(B, reserve)(buf, 2);
    // f (re != OK) return re;
    T **idle = X(, bIdle)(buf);
    if ($len(idle) < 2) return Bnoroom;
    memcpy((void *)*idle, &a, sizeof(T));
    ++*idle;
    memcpy((void *)*idle, &b, sizeof(T));
    ++*idle;
    return OK;
}

fun ok64 X(, bFeed1)(X(, bp) buf, T one) {
    return X(, sFeed1)(X(, bIdle)(buf), one);
}

fun ok64 X(, bFeed)(X(, b) buf, X(, csc) from) {
    T **into = X(, bIdle)(buf);
    if ($len(into) < $len(from)) return Bnoroom;
    X(, sCopy)(into, from);
    *into += $len(from);
    return OK;
}

fun ok64 X(B, mark)(X(B, ) const buf, range64 *range) {
    range->from = buf[1] - buf[0];
    range->till = buf[2] - buf[0];
    return OK;
}

fun void X(B, reset)(X(, b) buf) {
    T **b = (T **)buf;
    b[1] = b[0];
    b[2] = b[0];
}

fun void X(B, Ate)(X(, b) buf) { Bate(buf); }

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

fun T const *X(B, top)(X(B, ) buf) {
    assert(buf[2] > buf[1]);
    return buf[2] - 1;
}

fun ok64 X(, bPop)(X(, b) buf) {
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
    if (unlikely(buf == NULL || *buf == NULL)) return FAILsanity;
    if (-1 == munmap((void *)buf[0], Bsize(buf))) return Bmapfail;
    void **b = (void **)buf;
    b[0] = b[1] = b[2] = b[3] = NULL;
    return OK;
}

fun ok64 X(, bShift)(X(, b) buf, size_t pastlen) {
    if (unlikely(!Bok(buf))) return FAILsanity;
    size_t datalen = $len(Bdata(buf));
    if (unlikely(pastlen + datalen > Blen(buf))) return Bnoroom;
    T *to = buf[0] + pastlen;
    memmove((void *)(to), (void *)(buf[1]), BDataSize(buf));
    ((T **)buf)[1] = to;
    ((T **)buf)[2] = to + datalen;
    return OK;
}

fun ok64 X(, bSplice)(X(, bp) buf, size_t off, size_t cut, X(, csc) paste) {
    if (!Bok(buf) || !X(, csOK)(paste)) return FAILsanity;
    if (X(, bDataLen)(buf) < off + cut ||
        X(, bIdleLen)(buf) + cut < X(, csLen)(paste))
        return Bmiss;
    u8 *b = ((u8 **)buf)[1];
    memmove(b + off + $len(paste), b + off + cut,
            X(, bDataLen)(buf) - off - cut);
    memmove(b + off, paste[0], $len(paste));
    ((T **)buf)[2] = buf[2] + $len(paste) - cut;
    return OK;
}

#undef T
