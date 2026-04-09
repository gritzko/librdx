#ifndef DOG_SHA1_H
#define DOG_SHA1_H

//  SHA1: git object ID type for the ABC type system.
//  No hashing — we take SHA-1 values from git.

#include "abc/INT.h"

typedef struct {
    u8 data[20];
} sha1;

fun b8 sha1empty(sha1 const *s) {
    u64c *w = (u64c *)s->data;
    u32c *t = (u32c *)(s->data + 16);
    return (w[0] | w[1] | *t) == 0;
}

fun int sha1cmp(sha1 const *a, sha1 const *b) {
    return memcmp(a->data, b->data, 20);
}

fun b8 sha1eq(sha1 const *a, sha1 const *b) {
    return memcmp(a->data, b->data, 20) == 0;
}

fun b8 sha1Z(sha1 const *a, sha1 const *b) {
    return memcmp(a->data, b->data, 20) < 0;
}

#define X(M, n) M##sha1##n
#include "abc/Bx.h"
#include "abc/HEXx.h"
#undef X

#endif
