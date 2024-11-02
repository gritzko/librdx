#ifndef LIBRDX_SHA_H
#define LIBRDX_SHA_H
#include <sodium.h>

#include "INT.h"

typedef struct {
    u8 data[crypto_hash_sha256_BYTES];
} sha256;

typedef crypto_hash_sha256_state SHAstate;

fun void SHAsum(sha256* hash, $cu8c from) {
    crypto_hash_sha256(hash->data, *from, $len(from));
}

fun void SHAopen(SHAstate* state) { crypto_hash_sha256_init(state); }

fun void SHAfeed(SHAstate* state, $cu8c data) {
    crypto_hash_sha256_update(state, *data, $len(data));
}

fun void SHAclose(SHAstate* state, sha256* hash) {
    crypto_hash_sha256_final(state, hash->data);
}

fun int sha256cmp(sha256 const* a, sha256 const* b) {
    return memcmp(a, b, sizeof(sha256));
}

#define X(M, n) M##sha256##n
#include "Bx.h"
#include "HEXx.h"
#undef X

#endif
