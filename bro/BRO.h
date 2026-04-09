#ifndef BRO_BRO_H
#define BRO_BRO_H

#include "abc/B.h"
#include "abc/INT.h"
#include "dog/HUNK.h"

#define BRO_NONE UINT32_MAX

// A displayable hunk (file section, diff region, grep match context)
typedef struct {
    u8cs title;  // hunk header (file name, @@ line info, function context)
    u8cs text;   // source text bytes
    u32cs toks;  // packed tok32: syntax fg, text-relative offsets
    u32cs hili;  // sparse tok32: bg highlights ('I'=INS, 'D'=DEL), text-relative
    u8cs path;   // repo-relative file path (never trimmed)
} BROhunk;

// --- BRO arena: scratch space for cat-mode hunk staging ---
#define BRO_ARENA_SIZE (1UL << 27)   // 128MB
#define BRO_MAX_HUNKS  4096
#define BRO_MAX_MAPS   1024

extern b8      BRO_COLOR;
extern Bu8     bro_arena;
extern BROhunk bro_hunks[BRO_MAX_HUNKS];
extern u8bp    bro_maps[BRO_MAX_MAPS];
extern Bu32    bro_toks[BRO_MAX_MAPS];
extern u32     bro_nhunks;
extern u32     bro_nmaps;

ok64 BROArenaInit(void);
void BROArenaCleanup(void);
u8p  BROArenaWrite(void const *data, size_t len);
ok64 BROArenaAlloc(u8s out, size_t len);
void BRODefer(u8bp mapped, Bu32 toks);

// Bump bro_nhunks after the caller has filled bro_hunks[bro_nhunks].
void BROHunkAdd(void);

// Interactive pager: displays hunks with syntax colors, diff highlighting,
// status bar, and search. Falls back to plain output when !isatty.
ok64 BRORun(BROhunk const *hunks, u32 nhunks);

// Pager event loop: reads TLV hunks from pipefd, displays incrementally.
ok64 BROPipeRun(int pipefd);

// Cat mode: syntax-highlight one or more files via the pager.
ok64 BROCat(u8css files, u8csc reporoot);

// --- Navigation primitives (exposed for testing) ---
//
// The line index `lines[]` is the array built by BROBuildIndex: each
// entry is a range32{lo=hunk_idx, hi=byte_offset_in_hunk_text}, with
// hi=UINT32_MAX marking a title separator line. All functions return
// BRO_NONE when no qualifying line exists.
//
// "Hili range" = one tok in hk->hili whose tag is non-neutral
// (anything other than 'A' or 0). Adjacent toks of different
// non-neutral kinds (e.g. INS then DEL) count as two distinct ranges.

// Next hunk start strictly after line `from`.
u32 BROHunkNextLine(range32 const *lines, u32 nlines, u32 from);

// Start of current hunk if `from` is not already on it; else previous
// hunk start. (vim's [[ semantics)
u32 BROHunkPrevLine(range32 const *lines, u32 nlines, u32 from);

// Total number of hunks represented in lines[0..nlines).
u32 BROHunkCount(range32 const *lines, u32 nlines);

// 1-based index of the hunk that contains line `at`; 0 if nlines==0.
u32 BROHunkIndexAt(range32 const *lines, u32 nlines, u32 at);

// Total non-neutral hili ranges across hunks[0..nhunks).
u32 BROHiliCount(BROhunk const *hunks, u32 nhunks);

// 1-based index of the latest hili range whose first line <= `at`;
// 0 if none precede `at`.
u32 BROHiliIndexAt(BROhunk const *hunks, u32 nhunks,
                   range32 const *lines, u32 nlines, u32 at);

// First line containing a hili range whose first line is strictly
// greater than `mid`.
u32 BROHiliNextLine(BROhunk const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid);

// Last line containing a hili range whose first line is strictly
// less than `mid`.
u32 BROHiliPrevLine(BROhunk const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid);

#endif
