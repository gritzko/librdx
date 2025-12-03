//
// Created by gritzko on 11/17/25.
//

#ifndef RDX_RDX_LSM_H
#define RDX_RDX_LSM_H

#include "RDX.h"

#define RDX_LSM_MAX_STACK 32

typedef enum {
    RDX_LSM_CRYPTO_NOSIG_SHA256 = '2',
    RDX_LSM_CRYPTO_NOSIG_SHA3 = '3',
    RDX_LSM_CRYPTO_ED25519_SHA256 = 'E',
    RDX_LSM_CRYPTO_LAMPORT = 'L',
} RDX_LSM_CRYPTO;

typedef enum {
    RDX_LSM_INDEX_NONE = '_',
    RDX_LSM_INDEX_WALTZ_4 = '2',
    RDX_LSM_INDEX_WALTZ_8 = '3',
    RDX_LSM_INDEX_WALTZ_G = '4',
    RDX_LSM_INDEX_WALTZ_MAX = '9',
    RDX_LSM_INDEX_KILO = 'K',
    RDX_LSM_INDEX_OCTAB = 'O',
    RDX_LSM_INDEX_BLOOM = 'B',
} RDX_LSM_INDEX;

#define RDX_LSM_INDEX_WALTZ_BUCKET_LEN 128
#define RDX_LSM_FACTOR 8

typedef enum {
    RDX_LSM_COMPRESS_NONE = '_',
    RDX_LSM_COMPRESS_DEFLATE = 'z',
} RDX_LSM_COMPRESS;

#define RDX_LSM_PAGE_BIT_LEN 11
#define RDX_LSM_PAGE_LEN (1 << RDX_LSM_PAGE_BIT_LEN)

// The most basic LSM index ever, as if provided by the lowest bidder.
// Pages either fit 1KB or contain 1 object only.
typedef struct {
    u64 key;  // seq(54)+src(10)
    w64 off;  // zip(8)+sz(8)+off(48)
} idx128;

fun int idx128cmp(idx128 const* a, idx128 const* b) {
    return u64cmp(&a->key, &b->key);
}

#define X(M, name) M##idx128##name
#include "abc/Bx.h"
#undef X

con ok64 RDXFILEBAD = 0x6cd84f49538b28d;

fun u64 id128Key(id128 id) {
    u64 mask = u64max << 10;
    u64 key = ((id.seq << 4) & mask) | ((id.src >> 50) & 1023);
    return key;
}

#endif  // RDX_RDX_LSM_H
