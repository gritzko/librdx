#ifndef ABC_BUF_H
#define ABC_BUF_H

#include "01.h"
#include "B.h"
#include "S.h"

fun int u8cmp(const u8 *a, const u8 *b) { return (int)*a - (int)*b; }

fun int u8cpcmp(u8 const *const *a, u8 const *const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int u8pcmp(u8 *const *a, u8 *const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int u8csmp(u8 const *const *a, u8 const *const *b) { return $cmp(a, b); }

fun int u8csZ($cu8c a, $cu8c b) { return $cmp(a, b); }

#define X(M, name) M##u8##name
#include "Bx.h"
#undef X

#define X(M, name) M##u8p##name
#include "Bx.h"
#undef X

#define X(M, name) M##u8cp##name
#include "Bx.h"
#undef X

typedef void *voidp;
typedef void const *voidcp;

fun int voidpcmp(void *const *a, void *const *b) {
    if (*a < *b) return -1;
    if (*a > *b) return 1;
    return 0;
}

#define X(M, name) M##voidp##name
#include "Bx.h"
#undef X

fun int u8cscmp(u8cs const *a, u8cs const *b) { return $cmp(*a, *b); }

typedef u8cs const *$u8ccp;
typedef u8cs const *u8cscp;

#define X(M, name) M##u8cs##name
#define ABC_X_$
#include "Bx.h"
#undef ABC_X_$
#undef X

fun int u8Bcmp(Bu8 const *a, Bu8 const *b) {
    return u8cpcmp((u8cp *)((*a)[0]), (u8cp *)((*b)[0]));
}

typedef u8B const *u8Bcp;

#define X(M, name) M##u8B##name
#define ABC_X_$
#include "Bx.h"
#undef ABC_X_$
#undef X

#define $u8raw(v) \
    { (u8 *)&(v), (u8 *)(&v) + sizeof(v) }

#define a$raw(n, v) $u8 n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}
#define a$rawc(n, v) u8cs n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}
#define a$rawp(n, p) $u8 n = {(u8 *)(p), (u8 *)(p) + sizeof(*p)}
#define a$rawcp(n, p) u8cs n = {(u8 const *)(p), (u8 const *)(p) + sizeof(*p)}

#define a$u8c(n, ...)            \
    u8c __##n[] = {__VA_ARGS__}; \
    u8cs n = {__##n, __##n + sizeof(__##n)};

#define an$u8(n, l, ...)         \
    u8 __##n[l] = {__VA_ARGS__}; \
    $u8 n = {__##n, __##n + sizeof(__##n)};

#define an$u8c(n, l, ...)         \
    u8c __##n[l] = {__VA_ARGS__}; \
    u8cs n = {__##n, __##n + sizeof(__##n)};

fun b8 Bitat(Bu8 buf, size_t ndx) {
    size_t thebyte = ndx >> 3;
    size_t thebit = ndx & 7;
    must(thebyte < Bsize(buf));
    return (Bat(buf, thebyte) >> thebit) & 1;
}

fun void Bitset(Bu8 buf, size_t ndx) {
    size_t thebyte = ndx >> 3;
    size_t thebit = ndx & 7;
    must(thebyte < Bsize(buf));
    Bat(buf, thebyte) |= 1 << thebit;
}

fun void Bitunset(Bu8 buf, size_t ndx) {
    size_t thebyte = ndx >> 3;
    size_t thebit = ndx & 7;
    must(thebyte < Bsize(buf));
    Bat(buf, thebyte) |= ~(1 << thebit);
}

#define a$$pad(n, l, ll)                                \
    u8 _##n[(l)];                                       \
    Bu8 n##buf = {_##n, _##n, _##n, _##n + (l)};        \
    u8$ n##idle = Bu8idle(n##buf);                      \
    u8$ n##data = Bu8data(n##buf);                      \
    u8cs _$##n[(l)];                                    \
    u8csB n##$buf = {_$##n, _$##n, _$##n, _$##n + (l)}; \
    u8cssp n##$idle = Bu8csidle(n##$buf);               \
    u8cssp n##$data = u8csB_data(n##$buf);

#define $$call(fn, n, ...)               \
    {                                    \
        u8cs _s = {n##idle[0]};          \
        call(fn, n##idle, __VA_ARGS__);  \
        _s[1] = n##idle[0];              \
        call(u8cssFeed1, n##$idle, _s); \
    }

con ok64 Badtemplte = 0x2e5a38a71d30e29;

fun ok64 $$feedf($u8 into, u8cs tmpl, u8css args) {
    a$dup(u8c, t, tmpl);
    ok64 o = OK;
    while (!$empty(t) && o == OK) {
        if (**t != '$') {
            o = u8sFeed1(into, **t);
            ++*t;
            continue;
        }
        ++*t;
        if ($empty(t)) return Badtemplte;
        if (**t < '1' || **t > '9') return Badtemplte;
        int n = **t - '1';
        ++*t;
        if (n >= $len(args)) return Bnodata;
        o = $u8feedall(into, $at(args, n));
    }
    return $empty(t) ? OK : Bnoroom;
}

fun ok64 $u8feedstr($u8 into, const char *str) {
    int l = strlen(str);
    if ($len(into) < l) return $noroom;
    memcpy(*into, str, l);
    *into += l;
    return OK;
}

fun ok64 $u8feedn($u8 into, u8c *what, size_t n) {
    if (unlikely($len(into) < n)) return $noroom;
    memcpy(*into, what, n);
    *into += n;
    return OK;
}

fun ok64 $u8feedcn($u8 into, u8 what, size_t n) {
    if ($len(into) < n) return Bnoroom;
    memset(*into, what, n);
    *into += n;
    return OK;
}

#endif
