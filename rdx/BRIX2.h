//
// Created by gritzko on 10/25/25.
//

#ifndef RDX_BRIX_H
#define RDX_BRIX_H
#include "RDX2.h"
#include "abc/FILE.h"
#include "abc/SHA.h"

#define BRIX_MAX_STACK 32

typedef enum {
    BRIX_CRYPTO_NOSIG_SHA256 = '2',
    BRIX_CRYPTO_ED25519_SHA256 = 'e',
} BRIX_CRYPTO;

typedef enum {
    BRIX_INDEX_NONE = '_',
    BRIX_INDEX_LSMHASH_4 = '9',
    BRIX_INDEX_LSMHASH_8 = '8',
    BRIX_INDEX_2K = 'K',
    BRIX_INDEX_BLOOM = 'B',
} BRIX_INDEX;

typedef enum {
    BRIX_COMPRESS_NONE = '_',
    BRIX_COMPRESS_DEFLATE = 'z',
} BRIX_COMPRESS;

// PAST: header, meta
// DATA: data
// IDLE: index
typedef struct {
    struct {
        u8 litS;
        u8 litT;
        u8 lit_index;
        u8 lit_crypto;
    };
    // a. base
    // b. own
    // b. inputs
    // c. pubkey
    // c. signature
    u32 meta_len;
    u64 data_len;
} brikhead128;

con ok64 BRIXnoopen = 0x2db4a1cb3cf4a72;
con ok64 BRIXbadtip = 0x2db4a19a5a38b74;

fun b8 brikOK(u8b brik) {
    return Bok(brik) && u8bPastLen(brik) >= sizeof(brikhead128);
}

fun u8 BRIXu8bIndexType(u8b brik) { return Bat(brik, 2); }
fun u8 BRIXu8bCryptoType(u8b brik) { return Bat(brik, 2); }
ok64 BRIXu8bBase(u8b brik, sha256p own);
ok64 BRIXu8bOwn(u8b brik, sha256p own);

ok64 BRIXTipPath(path8 pad, u8csc tip);
ok64 BRIXOpenHome(int* home, path8 path);

ok64 BRIXu8bbCreateTip(u8bbp brix, int home, sha256cp base, u8cs tip);
ok64 BRIXu8bCreate(u8bp brik, int home, sha256cs deps);
ok64 BRIXu8bResize2(u8b tip);

ok64 BRIXu8bOpen(u8bp brik, int home, sha256cp hash);
ok64 BRIXu8bbOpen(u8bbp brix, int home, sha256cp hash);
ok64 BRIXu8bbOpenTip(u8bbp brix, int home, u8cs tip);

ok64 BRIXu8bbSeal(u8bbp brix, int* fd, int home, sha256p result);

ok64 BRIXu8bbMerge(u8bb brix, int home, int height, sha256p result);

ok64 BRIXu8bAdd(u8b tip, u8csc rec);
ok64 BRIXu8bAppend(u8b tip, u8csb recs);
fun ok64 BRIXu8bbAdd(u8bb brix, u8csc rec) {
    if (unlikely(brix == NULL || Bempty(brix))) return BRIXnoopen;
    return BRIXu8bAdd(Blast(brix), rec);
}

ok64 BRIXu8bGet(u8b b, ref128 key, u8csp val);
ok64 BRIXu8bbGets(u8bb brix, ref128 key, u8css intos);
ok64 BRIXu8bbGet(u8bb brix, ref128 key, u8s into);

ok64 BRIXu8bPick(u8b b, ref128 key, rdxcp sub, u8csp val);
ok64 BRIXu8bbPicks(u8bb brix, ref128 key, rdxcp sub, u8css intos);
ok64 BRIXu8bbPick(u8bb brix, ref128 key, rdxcp sub, u8s into);

ok64 BRIXu8bPut1(u8b tip, rdxcp rec);
ok64 BRIXu8bPut(u8b tip, u8csc rec);
ok64 BRIXu8bbPut(u8bb brix, u8csc rec);

ok64 BRIXu8bClose(u8bp b);
ok64 BRIXu8bbClose(u8bbp bx);
ok64 BRIXu8bbCloseTip(u8bbp bx, int* fd);

#endif  // RDX_BRIX_H
