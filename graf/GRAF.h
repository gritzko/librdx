#ifndef GRAF_GRAF_H
#define GRAF_GRAF_H

#include "abc/INT.h"
#include "dog/CLI.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "graf/DAG.h"

#define GRAF_ARENA_SIZE (1UL << 24)   // 16MB

// Hunk → bytes serializer: HUNKu8sFeed (TLV) or HUNKu8sFeedText (plain).
typedef ok64 (*graf_emit_fn)(u8s into, hunk const *hk);

// --- graf control struct (per DOG.md rule 8) ---

typedef struct {
    home        *h;          // borrowed
    int          lock_fd;    // flock on .dogs/graf/.lock; -1 = none
    Bu8          arena;      // hunk staging buffer
    int          out_fd;     // output fd (-1 = uninitialized)
    graf_emit_fn emit;       // serializer (TLV or plain text)
    dag_stack    idx;        // DAG index (LSM sorted runs)
} graf;

// --- Public API (DOG 4-fn) ---

//  Open graf state.  `h` is borrowed; provides root/arena/config/rw.
//  rw=YES creates `.dogs/graf/` if missing.
ok64 GRAFOpen(graf *g, home *h, b8 rw);

//  Run one CLI invocation — same effect as `graf ...`.
ok64 GRAFExec(graf *g, cli *c);

//  Feed a single git object into graf's DAG index.
//  obj_type uses KEEP_OBJ_* constants from keeper/KEEP.h.
//  `path` is the repo-relative path (for blobs) or empty.
ok64 GRAFUpdate(graf *g, u8 obj_type, u8cs blob, u8csc path);

//  Close and free resources.
ok64 GRAFClose(graf *g);

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

// Token-level blame (reads blobs from keeper).
ok64 GRAFBlame(keeper *k, u8cs filepath, u8cs reporoot);

// Weave diff between two commits (reads blobs from keeper).
ok64 GRAFWeaveDiff(keeper *k, u8cs filepath, u8cs reporoot,
                   u8cs from, u8cs to);

#endif
