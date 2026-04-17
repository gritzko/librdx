#ifndef KEEPER_KEEP_H
#define KEEPER_KEEP_H

//  KEEP: local git object store.
//
//  Stores git pack objects in append-only log files under .dogs/keeper/.
//  Objects are looked up via an LSM index of wh128 entries:
//    key = keepKeyPack(obj_type, hashlet60)  hashlet60[60] | type[4]
//    val = wh64Pack(flags, file_id, offset)  offset[40] | file_id[20] | flags[4]
//
//  On disk (.dogs/keeper/):
//    log/0000000001.pack — append-only pack log (FILEBook'd)
//    idx/0000000001.idx  — sorted wh128 run (LSM)
//    REFS                — URI→URI reflog

#include "abc/INT.h"
#include "abc/KV.h"
#include "abc/URI.h"
#include "dog/CLI.h"
#include "dog/SHA1.h"
#include "dog/WHIFF.h"
#include "dog/DOG.h"

con ok64 KEEPFAIL    = 0x50e3993ca495;
con ok64 KEEPNOROOM  = 0x50e3995d86d8616;
con ok64 KEEPNONE    = 0x50e3995d85ce;

// --- 60-bit hashlet: index key format ---
//
//  key = hashlet60[60] | obj_type[4]
//  Hashlet is big-endian: first SHA byte in MS bits.
//  60 bits = 15 hex chars of SHA prefix.
//  obj_type: 1=commit 2=tree 3=blob 4=tag (git pack types).
//  Sorts by hashlet first, type second.

// Index key uses wh64 with id=0 → 60-bit hashlet spans both fields.
// key = wh64Pack(obj_type, 0, hashlet40) but hashlet's upper 20 bits
// spill into the id field, giving 60 effective hashlet bits.

// Key pack: type in LS 4 bits, 60-bit hashlet above.
// Same as wh64Pack(type, id_hi, off) where hashlet60 = (off << 20) | id_hi.
fun u64 keepKeyPack(u8 type, u64 hashlet60) {
    return ((u64)type & WHIFF_TYPE_MASK) |
           ((hashlet60 & WHIFF_ID_MASK) << WHIFF_ID_SHIFT) |
           (((hashlet60 >> WHIFF_ID_BITS) & WHIFF_OFF_MASK) << WHIFF_OFF_SHIFT);
}
fun u8  keepKeyType(u64 key)    { return wh64Type(key); }
fun u64 keepKeyHashlet(u64 key) {
    return (wh64Off(key) << WHIFF_ID_BITS) | wh64Id(key);
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
    wh128cs runs[KEEP_MAX_LEVELS];  // LSM index runs
    u8bp   run_maps[KEEP_MAX_LEVELS];
    u32    nruns;
    char   dir[1024];               // resolved .dogs/keeper/ path
    Bu8    buf1;                    // working buffer for KEEPGet base inflate
    Bu8    buf2;                    // working buffer for KEEPGet delta apply
    Bu8    buf3;                    // working buffer for keep_resolve base
    Bu8    buf4;                    // working buffer for keep_resolve delta
} keeper;

// --- Public API (DOG 4-fn) ---

//  Open keeper store.  home empty → HOMEFindDogs from cwd.
//  rw=YES creates `.dogs/keeper/` and its subdirs if missing.
ok64 KEEPOpen(keeper *k, u8cs home, b8 rw);

//  Run one CLI invocation — same effect as `keeper ...`.
ok64 KEEPExec(keeper *k, cli *c);

//  Feed a single git object (type + raw uncompressed content) into
//  the store.  Appends to the current pack log + records an index
//  entry.  obj_type uses KEEP_OBJ_* below.  `path` is informational
//  (repo-relative path for blobs, empty for trees/commits/tags).
ok64 KEEPUpdate(keeper *k, u8 obj_type, u8cs blob, u8csc path);

//  Close and unmap everything.
ok64 KEEPClose(keeper *k);

//  Verb + value-flag tables for CLIParse.
extern char const *const KEEP_CLI_VERBS[];
extern char const KEEP_CLI_VAL_FLAGS[];

// Git object types (from packfile format)
#define KEEP_OBJ_COMMIT 1
#define KEEP_OBJ_TREE   2
#define KEEP_OBJ_BLOB   3
#define KEEP_OBJ_TAG    4

//  Retrieve object by hashlet.  Inflates from pack, chases deltas.
//  Returns object body in `out`, type in `*out_type`.
ok64 KEEPGet(keeper *k, u64 hashlet60, size_t hexlen, u8bp out, u8p out_type);

//  Retrieve object by full 20-byte SHA-1.  Scans all hashlet matches,
//  inflates each, verifies SHA-1.  Use when the full hash is known
//  (e.g. tree entries) to avoid hashlet collisions.
ok64 KEEPGetExact(keeper *k, sha1 const *sha, u8bp out, u8p out_type);

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
//  `remote` is the parsed "host /path" form used for the ssh command.
//  `origin_uri` is the original URI the user typed (e.g.
//  "localhost:src/git" or "//localhost/path") and is used as the
//  prefix for remote-tracking entries in the refs reflog. Empty/NULL
//  is allowed — entries will then be recorded only under their local
//  bare names.
ok64 KEEPSync(keeper *k, u8cs remote, u8cs origin_uri,
              char const *const *wants, char const *const *haves);

//  Store a batch of objects (convenience wrapper over Open/Feed/Close).
ok64 KEEPPut(keeper *k, u8csc *objects, wh64 *whiffs, u32 nobjs);

// --- Incremental pack writer ---

#define KEEP_PACK_MAX_OBJS (1u << 20)

typedef struct {
    u8bp     log;                   // FILEBook'd log file
    u32      file_id;               // sequence number
    u32      nobjs;                 // objects written so far
    Bwh128   entries;               // index entries buffer
} keep_pack;

ok64 KEEPPackOpen(keeper *k, keep_pack *p);

//  Feed one object.  sha_out receives the 20-byte git SHA-1.
ok64 KEEPPackFeed(keeper *k, keep_pack *p,
                  u8 type, u8csc content, sha1 *sha_out);

ok64 KEEPPackClose(keeper *k, keep_pack *p);

//  Walk objects in a pack file from a given val position.
typedef ok64 (*keep_cb)(u8 type, u8cs content, u64 hashlet, void *ctx);
ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx);

// --- Tree walk ---

typedef enum {
    KEEP_WALK_DEEP    = 1,  // recurse into subtrees
    KEEP_WALK_BLOBS   = 2,  // yield blob entries
    KEEP_WALK_TREES   = 4,  // yield subtree entries
    KEEP_WALK_CONTENT = 8,  // inflate content for blobs
    KEEP_WALK_ALL     = 6,  // blobs + trees
} KEEP_WALK;

//  Callback for KEEPWalk.  entry URI has path filled with
//  repo-relative filepath, query/authority from the input URI.
//  content is non-empty only if KEEP_WALK_CONTENT is set.
//  Return OK to continue, error to stop.
typedef ok64 (*keep_walk_f)(void0p ctx, uricp entry, u8 obj_type, u8csc content);

//  Walk a git tree.  Resolves target URI:
//    ?ref or #sha → commit → tree, then recurse.
//    //alias?ref  → resolve alias, same.
ok64 KEEPWalk(keeper *k, uricp target, KEEP_WALK mode,
              keep_walk_f cb, void0p ctx);

#endif
