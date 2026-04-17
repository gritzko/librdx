#ifndef SNIFF_SNIFF_H
#define SNIFF_SNIFF_H

//  SNIFF: file path registry + filesystem change log.
//
//  On disk (.dogs/sniff/):
//    paths.log   newline-separated path strings, append-only, Book-mmap'd
//    state.log   flat append-only log of wh64 entries
//
//  Change entry types (wh64: type[4] | id[20] | off[40]):
//    SNIFF_HASHLET  (1)  id=path_index, off=sha hashlet (base version)
//    SNIFF_CHECKOUT (2)  id=path_index, off=mtime at checkout
//    SNIFF_CHANGED  (3)  id=path_index, off=mtime observed
//
//  Dirs also get path indices; type=0 carries tree object hashlets.
//
//  In RAM:
//    Bu32  offsets   offsets[i] = byte pos of path i in booked paths
//    Bkv64 names     RAPHash(path) → path_index
//    Bkv64 state     aggregated (type|id) → off from changes log

#include "abc/BUF.h"
#include "abc/INT.h"
#include "abc/PATH.h"
#include "dog/CLI.h"
#include "dog/WHIFF.h"
#include "keeper/KEEP.h"

con ok64 SNIFFFAIL   = 0x1c5d23cf3ca495;
con ok64 SNIFFNOROOM = 0xc5d23cf5d86d8616;

#define SNIFF_DIR       ".dogs/sniff"
#define SNIFF_PATH_BOOK (256UL << 20)  // 256 MB VA for paths
#define SNIFF_CHG_BOOK  (128UL << 20)  // 128 MB VA for changes
#define SNIFF_HASH_SIZE (1 << 20)      // 1M slots

// --- Entry types ---

#define SNIFF_HASHLET   1   // base object hashlet
#define SNIFF_CHECKOUT  2   // mtime at checkout (clean state)
#define SNIFF_CHANGED   3   // mtime observed (dirty)

// Key for state hash: low 24 bits of wh64 (type | id<<4)
#define SNIFF_KEY(type, id) \
    (((u64)(type) & WHIFF_TYPE_MASK) | \
     (((u64)(id) & WHIFF_ID_MASK) << WHIFF_ID_SHIFT))

// --- State ---

typedef struct {
    u8bp  paths;          // FILEBook'd paths.log (stable mmap address)
    u8bp  changes;        // FILEBook'd state.log (stable mmap address)
    char  head_path[1024];
    char  head[256];
    Bu8cs past;           // sorted u8cs slices into paths (checkout portion)
    Bu8cs data;           // unsorted u8cs slices (post-checkout new paths)
    Bu32  sorted;         // merged sorted index (for POST/DEL)
} sniff;

// --- Public API (DOG 4-fn) ---

//  Open .dogs/sniff/ state rooted at `home` (repo root).
ok64 SNIFFOpen(sniff *s, u8cs home, b8 rw);

//  Run one CLI invocation — same effect as `sniff ...`.
ok64 SNIFFExec(sniff *s, cli *c);

//  Feed a single git object (blob/tree/commit/tag) into sniff's
//  index.  obj_type uses KEEP_OBJ_* constants from keeper/KEEP.h.
//  `path` is the repo-relative path this object lives at (empty
//  for commits/tags/top-level trees).
ok64 SNIFFUpdate(sniff *s, u8 obj_type, u8cs blob, u8csc path);

//  Close and unmap everything.
ok64 SNIFFClose(sniff *s);

//  Verb + value-flag tables for CLIParse.
extern char const *const SNIFF_VERBS[];
extern char const SNIFF_VAL_FLAGS[];

//  path→index.  Appends to paths log if new.
u32  SNIFFIntern(sniff *s, u8cs path);

//  index→path string (pointer into booked mmap, stable until close).
ok64 SNIFFPath(u8csp out, sniff const *s, u32 index);

//  Number of known paths.
fun u32 SNIFFCount(sniff const *s) {
    return (u32)(u8csbDataLen(s->past) + u8csbDataLen(s->data));
}

//  Is path a directory? (trailing /)
fun b8 SNIFFIsDir(sniff const *s, u32 index) {
    u8cs p = {};
    if (SNIFFPath(p, s, index) != OK) return NO;
    return (!$empty(p) && *$last(p) == '/');
}

//  Build absolute path: reporoot/rel.
fun ok64 SNIFFFullpath(path8b out, u8cs reporoot, u8cs rel) {
    a_cstr(sep, "/");
    u8bFeed(out, reporoot);
    u8bFeed(out, sep);
    u8bFeed(out, rel);
    return PATHu8bTerm(out);
}

//  Intern a directory path (appends / if missing).
u32  SNIFFInternDir(sniff *s, u8cs path);

//  Build sorted index array (by path string, depth-first).
//  Stores result in s->sorted.
ok64 SNIFFSort(sniff *s);

//  Record a change entry to the log.
ok64 SNIFFRecord(sniff *s, u8 type, u32 index, u64 off);

//  Look up aggregated state for (type, index).
//  Returns the off value, or 0 if not found.
u64 SNIFFGet(sniff const *s, u8 type, u32 index);

//  Compact: rewrite paths.log sorted, state.log with paired entries.
//  Rebuilds past/data arrays.  Called after checkout.
ok64 SNIFFCompact(sniff *s);

//  Read HEAD into out (ref name or hex SHA).  Points into s->head.
//  Empty slice if no HEAD.
fun void SNIFFHead(u8csp out, sniff const *s) {
    size_t len = strlen(s->head);
    out[0] = (u8cp)s->head;
    out[1] = (u8cp)s->head + len;
}

//  Write HEAD.  val is either "refs/heads/main" or 40-char hex.
ok64 SNIFFSetHead(sniff *s, u8cs val);

// --- Parent-commit helpers (shared by POST/DEL/COM) ---

//  Resolve a commit hex prefix (≤15 hex chars) to the root tree SHA.
//  Handles annotated-tag dereference.  Fails if the object isn't a
//  commit (after tag deref).
ok64 SNIFFParentTreeSha(sha1 *tree_out, keeper *k, u8cs parent_hex);

//  Walk the parent tree and populate sha_tab[idx] = entry SHA-1,
//  where idx is obtained via SNIFFInternDir (dirs) or SNIFFIntern
//  (files).  `sha_tab` must be pre-sized to hold at least `capacity`
//  sha1 values and zero-initialized.  Submodules skipped.
ok64 SNIFFCollectParentTree(sniff *s, keeper *k, u8cs parent_hex,
                             sha1 *sha_tab, u32 capacity);

#endif
