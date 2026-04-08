#ifndef BRO_BRO_H
#define BRO_BRO_H

#include "abc/INT.h"
#include "dog/HUNK.h"

// A displayable hunk (file section, diff region, grep match context)
typedef struct {
    u8cs title;  // hunk header (file name, @@ line info, function context)
    u8cs text;   // source text bytes
    u32cs toks;  // packed tok32: syntax fg, text-relative offsets
    u32cs hili;  // sparse tok32: bg highlights ('I'=INS, 'D'=DEL), text-relative
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

#endif
