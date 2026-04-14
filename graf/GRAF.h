#ifndef GRAF_GRAF_H
#define GRAF_GRAF_H

#include "abc/INT.h"
#include "dog/HUNK.h"
#include "graf/DAG.h"

#define GRAF_ARENA_SIZE (1UL << 24)   // 16MB

// Hunk → bytes serializer: HUNKu8sFeed (TLV) or HUNKu8sFeedText (plain).
typedef ok64 (*graf_emit_fn)(u8s into, hunk const *hk);

// --- graf control struct (per DOG.md rule 8) ---

typedef struct {
    Bu8          arena;      // hunk staging buffer
    int          out_fd;     // output fd (-1 = uninitialized)
    graf_emit_fn emit;       // serializer (TLV or plain text)
    dag_stack    idx;        // DAG index (LSM sorted runs)
    char         dir[1024];  // resolved .dogs/graf/ path
} graf;

//  Open graf state.  dogsroot empty → HOMEFindDogs from cwd.
ok64 GRAFOpen(graf *g, u8cs dogsroot);

//  Close and free resources.
ok64 GRAFClose(graf *g);

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
