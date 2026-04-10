#ifndef KEEPER_KEEP_H
#define KEEPER_KEEP_H

//  KEEP: local git object store.
//
//  Stores git packfiles as numbered .packs files under .dogs/keeper/.
//  Objects are looked up via an LSM index of kv64 entries:
//    key = hash64 = sha1_prefix[60] | hash_type[4]
//    val = offset[40] | file_id[20] | flags[4]
//
//  On disk (.dogs/keeper/):
//    0000000001.packs  — concatenated packfiles (append-only)
//    0000000001.idx    — sorted kv64 run (LSM)
//    ...

#include "abc/INT.h"
#include "abc/KV.h"
#include "dog/WHIFF.h"

con ok64 KEEPFAIL    = 0x11c53ca495;
con ok64 KEEPNOROOM  = 0x11c55d86d8616;
con ok64 KEEPNONE    = 0x11c55d85ce;

// --- hash64: 60-bit hash prefix + 4-bit hash type ---

typedef u64 hash64;

//  hash64 layout: type[4] | sha1_prefix[60]
//                 bits 63-60   bits 59-0
//
//  Type in the HIGH 4 bits so short hex prefixes (≤14 chars)
//  lose no information.  On little-endian, the u64's low bytes
//  hold the FIRST bytes of SHA-1 — matching git's hex prefix
//  order.  Type goes in the top nibble of byte[7], which is
//  zero for prefixes shorter than 15 hex chars.

#define HASH_SHA1       0   // git SHA-1 (only type for now)
#define HASH_TYPE_SHIFT 60
#define HASH_TYPE_MASK  (0xfULL << HASH_TYPE_SHIFT)
#define HASH_PREFIX_MASK (~HASH_TYPE_MASK)
#define HASH_MIN_HEX   6

// Construct hash64 from raw SHA-1 bytes (first 8 bytes)
fun hash64 hash64FromSha1(u8cp sha, u8 type) {
    u64 h = 0;
    memcpy(&h, sha, 8);
    return (h & HASH_PREFIX_MASK) | ((u64)(type & 0xf) << HASH_TYPE_SHIFT);
}

// Construct hash64 from hex prefix (6-15 chars).
// Parses hex into SHA-1 byte order (first hex char = high nibble of byte 0).
fun hash64 hash64FromHex(u8cs hex) {
    u8 bin[8] = {};
    size_t hlen = $len(hex);
    if (hlen > 15) hlen = 15;
    for (size_t i = 0; i < hlen; i++) {
        u8 c = hex[0][i];
        u8 nib = 0;
        if (c >= '0' && c <= '9') nib = c - '0';
        else if (c >= 'a' && c <= 'f') nib = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nib = c - 'A' + 10;
        if (i & 1) bin[i / 2] |= nib;
        else bin[i / 2] = nib << 4;
    }
    u64 h = 0;
    memcpy(&h, bin, 8);
    return (h & HASH_PREFIX_MASK) | ((u64)HASH_SHA1 << HASH_TYPE_SHIFT);
}

// Write hash64 as hex into a u8s slice (up to 15 hex chars).
fun ok64 hash64ToHex(u8s out, hash64 h) {
    u8 bin[8] = {};
    u64 prefix = h & HASH_PREFIX_MASK;
    memcpy(bin, &prefix, 8);
    size_t maxhex = $len(out);
    if (maxhex > 15) maxhex = 15;
    for (size_t i = 0; i < maxhex; i++) {
        u8 nib = (i & 1) ? (bin[i/2] & 0xf) : (bin[i/2] >> 4);
        u8sFeed1(out, "0123456789abcdef"[nib]);
    }
    return OK;
}

// Trim trailing zero hex chars to find the effective prefix length.
fun size_t hash64HexLen(hash64 h) {
    u8 bin[8] = {};
    u64 prefix = h & HASH_PREFIX_MASK;
    memcpy(bin, &prefix, 8);
    size_t len = 15;
    while (len > HASH_MIN_HEX && bin[(len - 1) / 2] == 0) len -= 2;
    if (len > HASH_MIN_HEX && (bin[(len - 1) / 2] & 0xf) == 0) len--;
    return len;
}

fun u8  hash64Type(hash64 h)   { return (u8)((h >> HASH_TYPE_SHIFT) & 0xf); }
fun u64 hash64Prefix(hash64 h) { return h & HASH_PREFIX_MASK; }

// --- val is w64: type[4] | file_id[20] | offset[40] ---
//  type 0 = concatenated git packfiles
#define KEEP_PACK 0

// --- Keeper state ---

#include "abc/MSET.h"

#define KEEP_MAX_FILES  256
#define KEEP_MAX_LEVELS MSET_MAX_LEVELS
#define KEEP_DIR        ".dogs/keeper"
#define KEEP_PACK_EXT   ".packs"
#define KEEP_IDX_EXT    ".idx"
#define KEEP_SEQNO_W    10

typedef struct {
    u8bp   packs[KEEP_MAX_FILES];   // mmap'd pack files
    u32    npacks;
    kv64cs runs[KEEP_MAX_LEVELS];   // LSM index runs
    u8bp   run_maps[KEEP_MAX_LEVELS];
    u32    nruns;
    u8cs   root;                    // repo root
} keeper;

// --- Public API ---

//  Open keeper store.  reporoot empty → HOMEFindDogs from cwd.
ok64 KEEPOpen(keeper *k, u8cs reporoot);

//  Close and unmap everything.
ok64 KEEPClose(keeper *k);

//  Retrieve object by hash.  Inflates from pack, chases deltas.
//  Returns object content in `out` (caller provides mapped buf).
ok64 KEEPGet(keeper *k, hash64 h, u8bp out);

//  Check if object exists in the store.
ok64 KEEPHas(keeper *k, hash64 h);

//  Raw index lookup: hash64 → val (offset|file|flags).
ok64 KEEPLookup(keeper *k, hash64 h, u64p val);

//  Fetch missing objects from remote.
ok64 KEEPSync(keeper *k, u8cs remote);

//  Walk objects in a pack file from a given val position.
typedef ok64 (*keep_cb)(u8 type, u8cs content, hash64 h, void *ctx);
ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx);

#endif
