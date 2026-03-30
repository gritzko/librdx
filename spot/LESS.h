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

#endif
