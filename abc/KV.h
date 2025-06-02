#ifndef ABC_KV_H
#define ABC_KV_H
#include "01.h"

typedef struct kv32 {
    u32 key;
    u32 val;
} kv32;

fun u64 kv32hash(kv32 const *v) { return mix32(v->key); }

fun int kv32z(const kv32 *a, const kv32 *b) {
    if (a->key < b->key) {
        return -1;
    } else if (a->key > b->key) {
        return 1;
    } else {
        return 0;
    }
}

#define X(M, name) M##kv32##name
#include "Bx.h"
#undef X

typedef struct kv64 {
    u64 key;
    u64 val;
} kv64;

fun u64 kv64hash(kv64 const *v) { return mix64(v->key); }

fun int kv64z(const kv64 *a, const kv64 *b) {
    if (a->key < b->key) {
        return -1;
    } else if (a->key > b->key) {
        return 1;
    } else {
        return 0;
    }
}

#define X(M, name) M##kv64##name
#include "Bx.h"
#undef X

#endif
