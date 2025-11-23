//
// Created by gritzko on 11/17/25.
//

#ifndef RDX_RDX_LSM_H
#define RDX_RDX_LSM_H
#include "RDX.h"

#include "abc/NACL.h"

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
    RDX_LSM_INDEX_OCTAB = 'K',
    RDX_LSM_INDEX_BLOOM = 'B',
} RDX_LSM_INDEX;

#define RDX_LSM_INDEX_WALTZ_BUCKET_LEN 128
#define RDX_LSM_FACTOR 8

typedef enum {
    RDX_LSM_COMPRESS_NONE = '_',
    RDX_LSM_COMPRESS_DEFLATE = 'z',
} RDX_LSM_COMPRESS;



con ok64 RDX_LSMnoopen = 0x2db4a1cb3cf4a72;
con ok64 RDX_LSMbadtip = 0x2db4a19a5a38b74;



#endif  // RDX_RDX_LSM_H
