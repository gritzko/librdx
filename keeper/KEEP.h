#ifndef KEEPER_KEEP_H
#define KEEPER_KEEP_H

//  KEEP: local git object store.
//
//  Stores git pack objects in append-only log files under .dogs/keeper/.
//  Objects are looked up via an LSM index of kv64 entries:
//    key = keepKeyPack(obj_type, hashlet60)  hashlet60[60] | type[4]
//    val = wh64Pack(flags, file_id, offset)  offset[40] | file_id[20] | flags[4]
//
//  On disk (.dogs/keeper/):
//    log/0000000001.pack — append-only pack log (FILEBook'd)
//    idx/0000000001.idx  — sorted kv64 run (LSM)
//    REFS                — URI→URI reflog

#include "abc/INT.h"
#include "abc/KV.h"
#include "dog/WHIFF.h"

con ok64 KEEPFAIL    = 0x11c53ca495;
con ok64 KEEPNOROOM  = 0x11c55d86d8616;
con ok64 KEEPNONE    = 0x11c55d85ce;

// --- 60-bit hashlet: index key format ---
//
//  key = hashlet60[60] | obj_type[4]
//  Hashlet is big-endian: first SHA byte in MS bits.
//  60 bits = 15 hex chars of SHA prefix.
//  obj_type: 1=commit 2=tree 3=blob 4=tag (git pack types).
//  Sorts by hashlet first, type second.

#define KEEP_HASHLET_BITS  60
#define KEEP_HASHLET_MASK  ((1ULL << 60) - 1)
#define KEEP_TYPE_BITS     4
#define KEEP_TYPE_MASK     0xfULL

fun u64 keepKeyPack(u8 type, u64 hashlet60) {
    return (hashlet60 << KEEP_TYPE_BITS) | ((u64)type & KEEP_TYPE_MASK);
}
fun u8  keepKeyType(u64 key)    { return (u8)(key & KEEP_TYPE_MASK); }
fun u64 keepKeyHashlet(u64 key) { return (key >> KEEP_TYPE_BITS) & KEEP_HASHLET_MASK; }

// Extract 60-bit hashlet from 20-byte SHA (big-endian, first byte on top)
fun u64 keepHashlet60(u8cp sha) {
    u64 h = 0;
    memcpy(&h, sha, 8);
    return (flip64(h) >> 4) & KEEP_HASHLET_MASK;
}

// 60-bit hashlet → hex prefix (up to 15 chars)
fun void keepHashlet60Hex(char *out, u64 hashlet60, size_t nchars) {
    if (nchars > 15) nchars = 15;
    for (size_t i = 0; i < nchars; i++) {
        u8 nib = (u8)((hashlet60 >> (56 - i * 4)) & 0xf);
        out[i] = "0123456789abcdef"[nib];
    }
    out[nchars] = 0;
}

// Hex prefix → 60-bit hashlet (zero-padded in low bits)
fun u64 keepHashlet60FromHex(char const *hex, size_t nchars) {
    if (nchars > 15) nchars = 15;
    u64 h = 0;
    for (size_t i = 0; i < nchars; i++) {
        u8 c = (u8)hex[i];
        u8 nib = 0;
        if (c >= '0' && c <= '9') nib = c - '0';
        else if (c >= 'a' && c <= 'f') nib = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nib = c - 'A' + 10;
        h = (h << 4) | nib;
    }
    h <<= (15 - nchars) * 4;
    return h;
}

// Compare hashlet against hex prefix of any length
fun b8 keepHashlet60Match(u64 hashlet60, char const *hex, size_t nchars) {
    char full[16];
    keepHashlet60Hex(full, hashlet60, 15);
    if (nchars > 15) nchars = 15;
    return memcmp(full, hex, nchars) == 0;
}

// --- Val format: same wh64 layout ---
//  val = wh64Pack(flags, file_id, offset)
//  flags[4] | file_id[20] | offset[40]

#define KEEP_VAL_FLAGS 1  // flags value for pack objects

#define HASH_MIN_HEX 4

// --- Keeper state ---

#include "abc/MSET.h"

#define KEEP_MAX_FILES  256
#define KEEP_MAX_LEVELS MSET_MAX_LEVELS
#define KEEP_DIR        ".dogs/keeper"
#define KEEP_LOG_DIR    "log"
#define KEEP_IDX_DIR    "idx"
#define KEEP_PACK_EXT   ".pack"
#define KEEP_IDX_EXT    ".idx"
#define KEEP_SEQNO_W    10

typedef struct {
    u8bp   packs[KEEP_MAX_FILES];   // mmap'd log files
    u32    npacks;
    kv64cs runs[KEEP_MAX_LEVELS];   // LSM index runs
    u8bp   run_maps[KEEP_MAX_LEVELS];
    u32    nruns;
    char   dir[1024];               // resolved .dogs/keeper/ path
    Bu8    buf1;                    // working buffer for KEEPGet base inflate
    Bu8    buf2;                    // working buffer for KEEPGet delta apply
    Bu8    buf3;                    // working buffer for keep_resolve base
    Bu8    buf4;                    // working buffer for keep_resolve delta
} keeper;

// --- Public API ---

//  Open keeper store.  reporoot empty → HOMEFindDogs from cwd.
ok64 KEEPOpen(keeper *k, u8cs reporoot);

//  Close and unmap everything.
ok64 KEEPClose(keeper *k);

// Git object types (from packfile format)
#define KEEP_OBJ_COMMIT 1
#define KEEP_OBJ_TREE   2
#define KEEP_OBJ_BLOB   3
#define KEEP_OBJ_TAG    4

//  Retrieve object by hashlet.  Inflates from pack, chases deltas.
//  Returns object body in `out`, type in `*out_type`.
ok64 KEEPGet(keeper *k, u64 hashlet60, size_t hexlen, u8bp out, u8p out_type);

//  Check if object exists in the store (hexlen = prefix length).
ok64 KEEPHas(keeper *k, u64 hashlet60, size_t hexlen);

//  Raw index lookup: hashlet → val wh64 (flags|file|offset).
//  hexlen: prefix length for partial matching (15 = full 60-bit).
ok64 KEEPLookup(keeper *k, u64 hashlet60, size_t hexlen, u64p val);

//  Verify object by full SHA-1: get from store, recompute hash,
//  compare.  For commits: also verifies tree.  For trees: verifies
//  all children recursively.  Reports errors to stderr.
ok64 KEEPVerify(keeper *k, u8cs hex_sha);

//  Import a git packfile into the store.
ok64 KEEPImport(keeper *k, u8cs pack_path);

//  Fetch objects from remote via git-upload-pack.
ok64 KEEPSync(keeper *k, u8cs remote,
              char const *const *wants, char const *const *haves);

//  Store a batch of objects (convenience wrapper over Open/Feed/Close).
ok64 KEEPPut(keeper *k, u8csc *objects, wh64 *whiffs, u32 nobjs);

// --- Incremental pack writer ---

#define KEEP_PACK_MAX_OBJS (1u << 20)

typedef struct {
    u8bp     log;                   // FILEBook'd log file
    u32      file_id;               // sequence number
    u32      nobjs;                 // objects written so far
    Bkv64    entries;               // index entries buffer
} keep_pack;

ok64 KEEPPackOpen(keeper *k, keep_pack *p);

//  Feed one object.  sha_out receives the 20-byte git SHA-1.
ok64 KEEPPackFeed(keeper *k, keep_pack *p,
                  u8 type, u8csc content, u8 sha_out[20]);

ok64 KEEPPackClose(keeper *k, keep_pack *p);

//  Walk objects in a pack file from a given val position.
typedef ok64 (*keep_cb)(u8 type, u8cs content, u64 hashlet, void *ctx);
ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx);

#endif
