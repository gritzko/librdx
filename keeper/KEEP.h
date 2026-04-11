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

// --- Keeper uses whiff (wh64) for both keys and vals ---
//
//  Index kv64 entry:
//    key = wh64Pack(HASH_SHA1, 0, hashlet)   hashlet from wh64Hashlet()
//    val = wh64Pack(KEEP_PACK, file_id, offset)
//
//  Hashlet: 40-bit big-endian SHA prefix (first byte on top).
//  Sort on key groups same hashlet together (type in LS bits).

#define HASH_SHA1   0   // hash type for git SHA-1
#define KEEP_PACK   0   // val type for concatenated git packfiles
#define HASH_MIN_HEX 6

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
ok64 KEEPGet(keeper *k, u64 hashlet, size_t hexlen, u8bp out, u8p out_type);

//  Check if object exists in the store (hexlen = prefix length).
ok64 KEEPHas(keeper *k, u64 hashlet, size_t hexlen);

//  Raw index lookup: hashlet → val wh64 (type|file|offset).
//  hexlen: prefix length for partial matching (10 = full 40-bit).
ok64 KEEPLookup(keeper *k, u64 hashlet, size_t hexlen, u64p val);

//  Verify object by full SHA-1: get from store, recompute hash,
//  compare.  For commits: also verifies tree.  For trees: verifies
//  all children recursively.  Reports errors to stderr.
ok64 KEEPVerify(keeper *k, u8cs hex_sha);

//  Import a git packfile into the store.
ok64 KEEPImport(keeper *k, u8cs pack_path);

//  Fetch objects from remote via git-upload-pack.
//  wants/haves: arrays of 40-char hex SHA strings (NULL-terminated).
//  If wants is NULL, wants all advertised refs.
//  If haves is NULL, sends no haves (full clone).
ok64 KEEPSync(keeper *k, u8cs remote,
              char const *const *wants, char const *const *haves);

//  Walk objects in a pack file from a given val position.
typedef ok64 (*keep_cb)(u8 type, u8cs content, u64 hashlet, void *ctx);
ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx);

#endif
