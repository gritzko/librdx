#ifndef ABC_NUM_H
#define ABC_NUM_H

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/OK.h"

// Number-to-English-words conversion, 64-bit ready

con u8c* NUM_ONES[] = {
    (u8c*)"", (u8c*)"one", (u8c*)"two", (u8c*)"three", (u8c*)"four",
    (u8c*)"five", (u8c*)"six", (u8c*)"seven", (u8c*)"eight", (u8c*)"nine",
    (u8c*)"ten", (u8c*)"eleven", (u8c*)"twelve", (u8c*)"thirteen",
    (u8c*)"fourteen", (u8c*)"fifteen", (u8c*)"sixteen", (u8c*)"seventeen",
    (u8c*)"eighteen", (u8c*)"nineteen"};

con u8c* NUM_TENS[] = {
    (u8c*)"", (u8c*)"", (u8c*)"twenty", (u8c*)"thirty", (u8c*)"forty",
    (u8c*)"fifty", (u8c*)"sixty", (u8c*)"seventy", (u8c*)"eighty",
    (u8c*)"ninety"};

typedef struct {
    u64 div;
    u8c* name;
} NUMscale;

con NUMscale NUM_SCALES[] = {
    {1000000000000000000ULL, (u8c*)"quintillion"},
    {1000000000000000ULL, (u8c*)"quadrillion"},
    {1000000000000ULL, (u8c*)"trillion"},
    {1000000000ULL, (u8c*)"billion"},
    {1000000ULL, (u8c*)"million"},
    {1000ULL, (u8c*)"thousand"},
    {0, NULL}
};

fun ok64 NUMu8sFeedStr(u8s into, u8c* s) {
    while (*s) {
        if (*into >= into[1]) return NOROOM;
        **into = *s++;
        ++*into;
    }
    return OK;
}

fun ok64 NUMu8sFeedSep(u8s into) {
    if (*into >= into[1]) return NOROOM;
    **into = ' ';
    ++*into;
    return OK;
}

// Feed number < 1000 as words
fun ok64 NUMu8sFeedSmall(u8s into, u64 n) {
    if (n >= 100) {
        ok64 o = NUMu8sFeedStr(into, (u8c*)NUM_ONES[n / 100]);
        if (o != OK) return o;
        o = NUMu8sFeedStr(into, (u8c*)" hundred");
        if (o != OK) return o;
        n %= 100;
        if (n > 0) {
            o = NUMu8sFeedSep(into);
            if (o != OK) return o;
        }
    }
    if (n >= 20) {
        ok64 o = NUMu8sFeedStr(into, (u8c*)NUM_TENS[n / 10]);
        if (o != OK) return o;
        if (n % 10) {
            o = NUMu8sFeedSep(into);
            if (o != OK) return o;
            o = NUMu8sFeedStr(into, (u8c*)NUM_ONES[n % 10]);
            if (o != OK) return o;
        }
    } else if (n > 0) {
        ok64 o = NUMu8sFeedStr(into, (u8c*)NUM_ONES[n]);
        if (o != OK) return o;
    }
    return OK;
}

// Feed u64 as English words into slice
fun ok64 NUMu8sFeed(u8s into, u64 n) {
    if (n == 0) return NUMu8sFeedStr(into, (u8c*)"zero");

    b8 need_sep = NO;
    for (const NUMscale* sc = NUM_SCALES; sc->div; sc++) {
        if (n >= sc->div) {
            if (need_sep) {
                ok64 o = NUMu8sFeedSep(into);
                if (o != OK) return o;
            }
            ok64 o = NUMu8sFeedSmall(into, n / sc->div);
            if (o != OK) return o;
            o = NUMu8sFeedSep(into);
            if (o != OK) return o;
            o = NUMu8sFeedStr(into, sc->name);
            if (o != OK) return o;
            n %= sc->div;
            need_sep = YES;
        }
    }
    if (n > 0) {
        if (need_sep) {
            ok64 o = NUMu8sFeedSep(into);
            if (o != OK) return o;
        }
        ok64 o = NUMu8sFeedSmall(into, n);
        if (o != OK) return o;
    }
    return OK;
}

// Returns length of number in words (without writing)
fun u64 NUMLen(u64 n) {
    u8 buf[256];
    u8s s = {buf, buf + 256};
    NUMu8sFeed(s, n);
    return *s - buf;
}

#endif
