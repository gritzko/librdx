#ifndef SNIFF_SNIFF_H
#define SNIFF_SNIFF_H

//  SNIFF: file path registry + filesystem change log.
//
//  On disk (.dogs/sniff/):
//    paths.log   newline-separated path strings, append-only, Book-mmap'd
//    state.log   flat append-only log of wh64 entries
//
//  Change entry types (wh64: type[4] | id[20] | off[40]):
//    SNIFF_BLOB     (1)  id=path_index, off=blob hashlet (file base)
//    SNIFF_CHECKOUT (2)  id=path_index, off=mtime at checkout
//    SNIFF_CHANGED  (3)  id=path_index, off=mtime observed
//    SNIFF_TREE     (4)  id=path_index, off=tree hashlet (dir base)
//
//  BLOB and TREE occupy the same paired-slot (0) as the "base hashlet"
//  for files and dirs respectively.  Dirs end with '/' in the path
//  string; the empty-path index (0) is reserved for the root tree.
//
//  In RAM:
//    Bu32  offsets   offsets[i] = byte pos of path i in booked paths
//    Bkv64 names     RAPHash(path) → path_index
//    Bkv64 state     aggregated (type|id) → off from changes log

#include "abc/BUF.h"
#include "abc/INT.h"
#include "abc/PATH.h"
#include "dog/CLI.h"
#include "dog/HOME.h"
#include "dog/WHIFF.h"
#include "keeper/KEEP.h"

con ok64 SNIFFFAIL   = 0x1c5d23cf3ca495;
con ok64 SNIFFNOROOM = 0xc5d23cf5d86d8616;
//  Returned by SNIFFOpen when SNIFF is already open, compatible mode.
con ok64 SNIFFOPEN   = 0x1c5d23cf619397;
//  Returned by SNIFFOpen when open ro and caller asked for rw.
con ok64 SNIFFOPENRO = 0xc5d23cf6193976d8;

#define SNIFF_DIR       ".dogs/sniff"
#define SNIFF_PATH_BOOK (256UL << 20)  // 256 MB VA for paths
#define SNIFF_CHG_BOOK  (128UL << 20)  // 128 MB VA for changes
#define SNIFF_HASH_SIZE (1 << 20)      // 1M slots

// --- Entry types ---

#define SNIFF_BLOB      1   // base blob hashlet (files)
#define SNIFF_CHECKOUT  2   // mtime at checkout (clean state)
#define SNIFF_CHANGED   3   // mtime observed (dirty)
#define SNIFF_TREE      4   // base tree hashlet (dirs)

// Key for state hash: low 24 bits of wh64 (type | id<<4)
#define SNIFF_KEY(type, id) \
    (((u64)(type) & WHIFF_TYPE_MASK) | \
     (((u64)(id) & WHIFF_ID_MASK) << WHIFF_ID_SHIFT))

// --- State ---

typedef struct {
    home   *h;              // borrowed
    u8bp    changes;        // FILEBook'd state.log (stable mmap address)
    Bu32    sorted;         // indices sorted by KEEPPath (for POST/DEL)
} sniff;

//  Singleton.  Zero-initialised; populated by SNIFFOpen.
extern sniff SNIFF;

// --- Public API (DOG 4-fn) ---

//  Open .dogs/sniff/ state rooted at h->root.  Internally opens keeper
//  too (singleton: nests naturally; pair with KEEPClose only if the
//  keeper open returns OK, same contract as KEEPOpen).  Returns:
//    OK          I opened &SNIFF; pair with SNIFFClose.
//    SNIFFOPEN   already open compatible mode; use &SNIFF, don't close.
//    SNIFFOPENRO already ro but caller asked for rw.
//    (other)     real error, propagate.
ok64 SNIFFOpen(home *h, b8 rw);

//  Run one CLI invocation — same effect as `sniff ...`.
ok64 SNIFFExec(cli *c);

//  Feed a single git object (blob/tree/commit/tag) into sniff's
//  index.  obj_type uses KEEP_OBJ_* constants from keeper/KEEP.h.
//  `path` is the repo-relative path this object lives at (empty
//  for commits/tags/top-level trees).  `sha` is the caller's
//  pre-computed git-object SHA-1 (may be NULL; currently unused by
//  sniff but carried through for parity with graf/spot).
ok64 SNIFFUpdate(u8 obj_type, sha1 const *sha, u8cs blob, u8csc path);

//  Close singleton.  Idempotent; no-op if not open.
ok64 SNIFFClose(void);

//  Verb + value-flag tables for CLIParse.
extern char const *const SNIFF_VERBS[];
extern char const SNIFF_VAL_FLAGS[];

//  path→index.  Forwards to keeper's registry; appends if new.
u32  SNIFFIntern(u8cs path);

//  index→path string (slice into keeper's paths.log, stable until close).
ok64 SNIFFPath(u8csp out, u32 index);

//  Number of known paths (delegates to keeper).
u32  SNIFFCount(void);

//  Is path a directory? (trailing /)
fun b8 SNIFFIsDir(u32 index) {
    u8cs p = {};
    if (SNIFFPath(p, index) != OK) return NO;
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
u32  SNIFFInternDir(u8cs path);

//  Intern the repo root dir ("/") and return its index.  The root's
//  SNIFF_TREE hashlet is the base tree — the "staged" tree that
//  PUT/DELETE update and POST commits.
u32  SNIFFRootIdx(void);

//  Convenience: current base tree hashlet (0 if unset).
u64  SNIFFBaseTree(void);

//  Build sorted index array (by path string, depth-first).
//  Stores result in SNIFF.sorted.
ok64 SNIFFSort(void);

//  Record a change entry to the log.
ok64 SNIFFRecord(u8 type, u32 index, u64 off);

//  Look up aggregated state for (type, index).
//  Returns the off value, or 0 if not found.
u64 SNIFFGet(u8 type, u32 index);

//  Compact: rewrite state.log, keeping only live entries.
ok64 SNIFFCompact(void);

// --- Parent-commit helpers (shared by POST/DEL/COM) ---

//  Resolve a commit hex prefix (≤15 hex chars) to the root tree SHA.
//  Handles annotated-tag dereference.  Fails if the object isn't a
//  commit (after tag deref).
ok64 SNIFFParentTreeSha(sha1 *tree_out, u8cs parent_hex);

//  Walk the parent tree and populate sha_tab[idx] = entry SHA-1,
//  where idx is obtained via SNIFFInternDir (dirs) or SNIFFIntern
//  (files).  `sha_tab` must be pre-sized to hold at least `capacity`
//  sha1 values and zero-initialized.  Submodules skipped.
ok64 SNIFFCollectParentTree(u8cs parent_hex, sha1 *sha_tab, u32 capacity);

//  Same, but walks the current base tree (root SNIFF_TREE hashlet)
//  instead of a named parent commit.  No-op if base is unset.  Used
//  by PUT/DELETE to seed per-entry SHAs for tree reuse.
ok64 SNIFFCollectBaseTree(sha1 *sha_tab, u32 capacity);

#endif
