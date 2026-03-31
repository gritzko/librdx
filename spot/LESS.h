#ifndef SPOT_LESS_H
#define SPOT_LESS_H

#include "abc/INT.h"

// Per-byte annotation: bottom 6 bits = tag/color index, top 2 bits = diff flags
#define LESS_INS 0x80  // bit 7: inserted
#define LESS_DEL 0x40  // bit 6: deleted
#define LESS_TAG 0x3F  // bits 0-5: tag index

// A displayable hunk (file section, diff region, grep match context)
typedef struct {
    u8cs title;  // hunk header (file name, @@ line info, function context)
    u8cs text;   // source text bytes
    u32cs toks;  // packed u32 tokens (for tok-aware rendering)
    u8cs lits;   // per-byte: tag + INS/DEL flags, parallel to text
} LESShunk;

// Interactive pager: displays hunks with syntax colors, diff highlighting,
// status bar, and search. Falls back to plain fprintf when !isatty.
ok64 LESSRun(LESShunk const *hunks, u32 nhunks);

// --- LESS arena: shared scratch space for grep/diff/spot/cat ---
#define LESS_ARENA_SIZE (1UL << 27)   // 128MB
#define LESS_MAX_HUNKS 4096
#define LESS_MAX_MAPS 1024

extern Bu8 less_arena;
extern LESShunk less_hunks[LESS_MAX_HUNKS];
extern u8bp less_maps[LESS_MAX_MAPS];
extern Bu32 less_toks[LESS_MAX_MAPS];
extern u32 less_nhunks;
extern u32 less_nmaps;

ok64 LESSArenaInit(void);
void LESSArenaCleanup(void);
u8p LESSArenaWrite(void const *data, size_t len);
u8p LESSArenaAlloc(size_t len);
void LESSDefer(u8bp mapped, Bu32 toks);

#endif
