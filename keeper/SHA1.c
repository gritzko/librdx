//  SHA1: SHA-1 hash via sha1dc (same implementation git uses)
//
#include <string.h>

#include "sha1dc/sha1.h"
#include "SHA1.h"

void SHA1Sum(uint8_t out[20], uint8_t const *data, uint64_t len) {
    SHA1_CTX ctx;
    SHA1DCInit(&ctx);
    SHA1DCSetSafeHash(&ctx, 0);  // disable collision detection for speed
    SHA1DCUpdate(&ctx, (char const *)data, (size_t)len);
    SHA1DCFinal(out, &ctx);
}
