//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_NACL_H
#define ABC_NACL_H

#include "SHA.h"
#include "sodium.h"

typedef u512 edsec512;
typedef u256 edpub256;
typedef u512 edsig512;

#define edpub256cmp u256cmp
#define edsig512cmp u512cmp

con ok64 NACLbad = 0x2896654c297;
con ok64 NACLfail = 0xc2d96a54c297;
con ok64 NACLfail0 = 0xc2d96a54c297;

fun ok64 NACLed25519create(edpub256 *publicKey, edsec512 *secretKey) {
    int ret = crypto_sign_ed25519_keypair((u8 *)publicKey, (u8 *)secretKey);
    return ret == 0 ? OK : NACLfail0 + ret;
}

typedef unsigned long long int nacl_size_t;

fun ok64 NACLed25519sign(edsig512 *sign, const sha256 *hash,
                         const edsec512 *seckey) {
    nacl_size_t slen;
    int ret = crypto_sign_detached((u8 *)sign, &slen, (u8 *)hash,
                                   sizeof(sha256), (u8 *)seckey);
    must(slen == sizeof(edsig512));
    return ret == 0 ? OK : NACLfail0 + ret;
}

fun ok64 NACLed25519verify(const edsig512 *signature, const sha256 *hash,
                           const edpub256 *pubkey) {
    int ret = crypto_sign_ed25519_verify_detached(
        (const u8 *)signature, (const u8 *)hash, sizeof(sha256),
        (const u8 *)pubkey);
    return ret == 0 ? OK : NACLbad;
}

#endif  // DW_ED_H
