//  SHA1: SHA-1 hash via OpenSSL, isolated from ABC type system
//
#include <string.h>

#include <openssl/evp.h>

#include "SHA1.h"

void SHA1Sum(uint8_t out[20], uint8_t const *data, uint64_t len) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        memset(out, 0, 20);
        return;
    }
    if (EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, data, len) != 1) {
        memset(out, 0, 20);
        EVP_MD_CTX_free(ctx);
        return;
    }
    unsigned int olen = 20;
    EVP_DigestFinal_ex(ctx, out, &olen);
    EVP_MD_CTX_free(ctx);
}
