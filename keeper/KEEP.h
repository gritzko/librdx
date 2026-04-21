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
#include "dog/HOME.h"
#include "dog/SHA1.h"
#include "dog/WHIFF.h"
#include "dog/DOG.h"

con ok64 KEEPFAIL    = 0x50e3993ca495;
con ok64 KEEPNOROOM  = 0x50e3995d86d8616;
con ok64 KEEPNONE    = 0x50e3995d85ce;
//  Returned by KEEPOpen when KEEP is already open with a compatible
//  mode.  The caller gets a usable &KEEP but must NOT call KEEPClose.
con ok64 KEEPOPEN    = 0x50e399619397;
//  Returned by KEEPOpen when caller requests rw on a ro-opened keeper.
con ok64 KEEPOPENRO  = 0x50e3996193976d8;
//  Intra-pack object order violation: commit→tree→blob→tag required.
con ok64 ORDERBAD = 0x61b34e6cb28d;

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
    home  *h;                       // borrowed; owns root/arena/config/rw
    int    lock_fd;                 // flock on .dogs/keeper/.lock; -1 = none
    u8bp   packs[KEEP_MAX_FILES];   // mmap'd log files
    u32    npacks;
    wh128cs runs[KEEP_MAX_LEVELS];  // LSM index runs
    u8bp   run_maps[KEEP_MAX_LEVELS];
    u32    nruns;
    Bu8    buf1;                    // working buffer for KEEPGet base inflate
    Bu8    buf2;                    // working buffer for KEEPGet delta apply
    Bu8    buf3;                    // working buffer for keep_resolve base
    Bu8    buf4;                    // working buffer for keep_resolve delta
    //  Path registry: .dogs/keeper/paths.log (newline-separated),
    //  offsets[i] → start-byte of path i, hash(path) → i for dedup.
    //  Dir paths end with '/'.  Index 0 is reserved for the empty path.
    u8bp   paths_log;
    Bu32   paths_offs;
    Bkv64  paths_hash;
} keeper;

// Relative ".dogs/keeper" slice.  Call sites compose the full dir via
//   a_path(dir, u8bDataC(k->h->root), KEEP_DIR_S);
// and use $path(dir) wherever a u8csc is needed.
extern u8c *const KEEP_DIR_S[2];

// --- Public API (DOG 4-fn) ---

//  Singleton keeper state.  Zero-initialised; population happens in
//  KEEPOpen.  All helpers that need keeper reach this symbol directly
//  rather than threading a pointer through every call chain.
extern keeper KEEP;

//  Open keeper store at `h`.  Returns:
//    OK         I opened it; caller must pair with KEEPClose.
//    KEEPOPEN   already open, compatible mode; use &KEEP, do NOT close.
//    KEEPOPENRO already open ro but caller asked for rw — real conflict;
//               propagate.  Caller's outer scope must re-architect.
//    (other)    real error — propagate, no KEEPClose.
ok64 KEEPOpen(home *h, b8 rw);

//  Run one CLI invocation — same effect as `keeper ...`.
ok64 KEEPExec(keeper *k, cli *c);

//  Feed a single git object (type + raw uncompressed content) into
//  the store.  Appends to the current pack log + records an index
//  entry.  obj_type uses KEEP_OBJ_* below.  `path` is informational
//  (repo-relative path for blobs, empty for trees/commits/tags).
ok64 KEEPUpdate(keeper *k, u8 obj_type, u8cs blob, u8csc path);

//  Close and unmap everything.
ok64 KEEPClose(void);

//  Verb + value-flag tables for CLIParse.
extern char const *const KEEP_CLI_VERBS[];
extern char const KEEP_CLI_VAL_FLAGS[];

// Git object types (from packfile format)
#define KEEP_OBJ_COMMIT 1
#define KEEP_OBJ_TREE   2
#define KEEP_OBJ_BLOB   3
#define KEEP_OBJ_TAG    4

//  Pack bookmark index type.  Index entries whose key carries this
//  type are bookmarks for whole packs, not objects:
//    key = keepKeyPack(KEEP_TYPE_PACK, pack_hashlet60)
//    val = wh64Pack(flags, file_id, offset_of_first_object)
//  pack_hashlet60 is derived from the pack's trailing SHA-1 (git
//  convention — hash covers PACK header + object bytes).  Sits
//  outside the 1..4 object-type range so range queries for
//  objects never see it.  See keeper/LOG.md.
#define KEEP_TYPE_PACK  0xF

//  Retrieve object by hashlet.  Inflates from pack, chases deltas.
//  Returns object body in `out`, type in `*out_type`.
ok64 KEEPGet(keeper *k, u64 hashlet60, size_t hexlen, u8bp out, u8p out_type);

//  Retrieve object by full 20-byte SHA-1.  Scans all hashlet matches,
//  inflates each, verifies SHA-1.  Use when the full hash is known
//  (e.g. tree entries) to avoid hashlet collisions.
ok64 KEEPGetExact(keeper *k, sha1 const *sha, u8bp out, u8p out_type);

//  Check if object exists in the store (hexlen = prefix length).
ok64 KEEPHas(keeper *k, u64 hashlet60, size_t hexlen);

//  Compute the canonical git object SHA-1 of a body: SHA1("<type>
//  <size>\0" + body).  `type` must be one of DOG_OBJ_*.
void KEEPObjSha(sha1 *out, u8 type, u8csc content);

//  Raw index lookup: hashlet → val wh64 (flags|file|offset).
//  hexlen: prefix length for partial matching (15 = full 60-bit).
ok64 KEEPLookup(keeper *k, u64 hashlet60, size_t hexlen, u64p val);

//  Verify object by full SHA-1: get from store, recompute hash,
//  compare.  For commits: also verifies tree.  For trees: verifies
//  all children recursively.  Reports errors to stderr.
ok64 KEEPVerify(keeper *k, u8cs hex_sha);

//  Import a git packfile into the store.
ok64 KEEPImport(keeper *k, u8cs pack_path);

//  Ingest a keeper-native stripped pack file (one SYNC.md Q body):
//  whole log file bytes = PACK header (12 B) + concatenated object
//  records, no git trailer.  Writes a new log/NNN.pack, UNPK-indexes
//  it, emits one pack bookmark at offset 12 (covering the whole
//  file), writes idx/NNN.idx, maps both, and extends k->packs /
//  k->runs.  Caller holds no resources beyond the `bytes` slice.
ok64 KEEPIngestFile(keeper *k, u8csc bytes);

//  Push one new commit object to `host:path` via git-receive-pack.
//  Spawns `ssh <host> git-receive-pack <path>` (no shell).
//  `ref` is the full remote ref name, e.g. "refs/heads/master".
//  `old_hex`/`new_hex` are 40-char SHA-1 hex slices.  old_hex may be
//  "000...0" (40 zeros) to create a new ref.
//  `commit_body` is the raw commit object bytes that correspond to
//  new_hex; KEEPPush packs and sends exactly this one object.
ok64 KEEPPush(keeper *k, u8csc host, u8csc path, char const *ref,
              u8csc old_hex, u8csc new_hex, u8csc commit_body);

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
    u8bp     log;                   // FILEBook'd log file (may already hold earlier packs)
    u32      file_id;               // sequence number
    u32      nobjs;                 // objects written so far by THIS pack
    u64      pack_offset;           // byte offset in log where THIS pack's first object starts
    u8       last_type;             // for commit->tree->blob->tag ordering check
    b8       strict_order;          // enforce commit->tree->blob->tag (ORDERBAD)
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

//  Retrieve a single blob by URI.  Dispatch:
//    URI has host     → materialize from remote (TODO: not yet wired),
//    URI has query    → historical lookup via ref + path descent,
//    URI has fragment → hex SHA prefix (object lookup) + path descent,
//    otherwise        → KEEPFAIL (caller should use the filesystem).
//  `out` must be an allocated u8bp; the blob body is written into it.
ok64 KEEPGetByURI(keeper *k, uricp target, u8bp out);

//  Resolve a target URI to a root tree SHA-1 (20 bytes).
//  Accepted forms:
//    target.fragment = hex SHA prefix of a tree or commit object.
//    target.query    = ref name ("HEAD", "refs/tags/X", …) — looked up
//                      via REFS, tag-dereferenced if needed.
//  Returns KEEPNONE when the ref is unknown, KEEPFAIL on bad input.
ok64 KEEPResolveTree(keeper *k, uricp target, sha1 *tree_out);

//  KEEPLsFiles is declared in keeper/WALK.h (takes a walk_tree_fn).
//  Path registry (KEEPIntern / KEEPPath / KEEPPathCount) is declared
//  in keeper/PATHS.h.  Its backing fields (paths_log, paths_offs,
//  paths_hash) live on the `keeper` struct above.

#endif
