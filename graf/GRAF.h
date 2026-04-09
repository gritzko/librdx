#ifndef GRAF_GRAF_H
#define GRAF_GRAF_H

#include "abc/INT.h"
#include "dog/HUNK.h"

// Producer-side staging for graf — same shape as spot/LESS.h.
// Build hunks in `graf_arena`, emit them via `graf_emit` and write
// the resulting bytes to `graf_out_fd`.

#define GRAF_ARENA_SIZE (1UL << 24)   // 16MB

extern Bu8 graf_arena;

// File descriptor for outgoing hunks.  STDOUT_FILENO when piped, else
// the write end of a pipe to bro.  -1 = uninitialized.
extern int graf_out_fd;

// Hunk → bytes serializer: HUNKu8sFeed (TLV) or HUNKu8sFeedText (plain).
typedef ok64 (*graf_emit_fn)(u8s into, hunk const *hk);
extern graf_emit_fn graf_emit;

ok64 GRAFArenaInit(void);
void GRAFArenaCleanup(void);

// Serialize one hunk via graf_emit and write to graf_out_fd.  Used as
// a HUNKcb by graf's diff/merge wrappers.
ok64 GRAFHunkEmit(hunk const *hk, void *ctx);

// User-facing diff entry: maps two files, calls dog/DIFF, streams
// hunks via GRAFHunkEmit.
ok64 GRAFDiff(u8cs old_path, u8cs new_path, u8cs name,
              u8cs old_mode, u8cs new_mode);

// 3-way merge entry: writes resolved bytes to outpath (or stdout if
// outpath is empty).
ok64 GRAFMerge(u8cs base_path, u8cs ours_path, u8cs theirs_path,
               u8cs outpath);

// Install graf as git's diff/merge driver in the given workspace.
ok64 GRAFInstall(u8cs reporoot);

#endif
