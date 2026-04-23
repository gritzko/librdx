#ifndef GRAF_GRAF_H
#define GRAF_GRAF_H

#include "abc/INT.h"
#include "dog/CLI.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "dog/SHA1.h"
#include "graf/DAG.h"

#define GRAF_ARENA_SIZE (1UL << 24)   // 16MB

// Hunk → bytes serializer: HUNKu8sFeed (TLV) or HUNKu8sFeedText (plain).
typedef ok64 (*graf_emit_fn)(u8s into, hunk const *hk);

// Forward decl for transient ingest state (see graf/DAG.c).
typedef struct dag_ingest dag_ingest;

// --- graf control struct (per DOG.md rule 8) ---

typedef struct {
    home        *h;          // borrowed
    int          lock_fd;    // flock on .dogs/graf/.lock; -1 = none
    Bu8          arena;      // hunk staging buffer
    int          out_fd;     // output fd (-1 = uninitialized)
    graf_emit_fn emit;       // serializer (TLV or plain text)
    dag_stack    idx;        // DAG index (LSM sorted runs)
    dag_ingest  *ing;        // lazily allocated on first GRAFUpdate
} graf;

//  Singleton.  Zero-initialised; populated by GRAFOpen.
extern graf GRAF;

// --- Internal helpers used by GRAFUpdate (implemented in DAG.c) ---
ok64 GRAFDagUpdate(u8 obj_type, sha1 const *sha, u8cs blob, u8csc path);
ok64 GRAFDagFinish(void);

// --- Error / sentinel codes ---

con ok64 GRAFFAIL    = 0x1c4a993ca495;
con ok64 GRAFOPEN    = 0x41b28f619397;
con ok64 GRAFOPENRO  = 0x41b28f6193976d8;
//  GRAFOpenBranch: branch outside the Phase-3-supported set (trunk only).
con ok64 GRAFNOBR    = 0x41b28f5d82db;

// --- Public API (DOG 4-fn, singleton) ---

//  Open graf state.  Returns OK (I opened), GRAFOPEN (already open
//  compatible), GRAFOPENRO (ro/rw conflict), or a real error.
ok64 GRAFOpen(home *h, b8 rw);

//  Branch-aware Open (Phase 3 surface).  Normalizes `branch` via
//  DPATHBranchNormFeed and registers it on the home singleton via
//  HOMEOpenBranch before delegating to GRAFOpen.  Phase 3 accepts
//  only the trunk (canonical form = empty); other branches return
//  GRAFNOBR.  Mirrors `KEEPOpenBranch`.
ok64 GRAFOpenBranch(home *h, u8cs branch, b8 rw);

//  Run one CLI invocation.
ok64 GRAFExec(cli *c);

//  Feed a single object (commit/tree/blob) into graf's DAG index.
//  obj_type uses KEEP_OBJ_* constants; `path` is repo-relative
//  (blobs only; trees/commits pass an empty path).
//
//  `sha` is the git-object SHA-1 of the object.  May be NULL when
//  the caller does not have it pre-computed (e.g. the manual reindex
//  path); graf then falls back to hashing `blob` itself.  On the hot
//  UNPKIndex → keeper_indexer_fanout path, UNPK passes its already-
//  resolved SHA, skipping a full SHA1DC pass per object.
ok64 GRAFUpdate(u8 obj_type, sha1 const *sha, u8cs blob, u8csc path);

//  Close singleton; idempotent.
ok64 GRAFClose(void);

//  Verb + value-flag tables for CLIParse.
extern char const *const GRAF_CLI_VERBS[];
extern char const GRAF_CLI_VAL_FLAGS[];

// --- Legacy globals (used by existing diff/merge code) ---

extern Bu8          graf_arena;
extern int          graf_out_fd;
extern graf_emit_fn graf_emit;

ok64 GRAFArenaInit(void);
void GRAFArenaCleanup(void);

// Serialize one hunk via graf_emit and write to graf_out_fd.
ok64 GRAFHunkEmit(hunk const *hk, void *ctx);

// User-facing diff entry.
ok64 GRAFDiff(u8cs old_path, u8cs new_path, u8cs name,
              u8cs old_mode, u8cs new_mode);

// 3-way merge entry.
ok64 GRAFMerge(u8cs base_path, u8cs ours_path, u8cs theirs_path,
               u8cs outpath);

// Drive a full streaming ingest from keeper: iterate every object in
// the keeper store (commits, trees, blobs), replay DOG.md §8 updates
// into graf's DAG, and finalize with PATH_VER emission.  Idempotent.
ok64 GRAFIndex(keeper *k);

// Token-level blame (reads blobs from keeper).
//   tip_h: 40-bit commit hashlet bounding the history (0 = no filter).
ok64 GRAFBlame(keeper *k, u8cs filepath, u64 tip_h, u8cs reporoot);

// Weave diff between two commits (reads blobs from keeper).
ok64 GRAFWeaveDiff(keeper *k, u8cs filepath, u8cs reporoot,
                   u8cs from, u8cs to);

// Deterministic URI-driven blob/tree merge (see graf/GET.md).
//
// URI grammar: `path?sha1&sha2&...&shaN`.  Trailing `/` on the path
// selects tree mode (future task).  Reaches keeper via &KEEP and graf
// state via &GRAF — callers must have both singletons open.
ok64 GRAFGet(u8b into, u8csc uri);

#endif
