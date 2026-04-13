#ifndef BRO_BRO_H
#define BRO_BRO_H

#include "abc/B.h"
#include "abc/INT.h"
#include "abc/URI.h"
#include "dog/FRAG.h"
#include "dog/HUNK.h"

#define BRO_NONE UINT32_MAX

// Extract path from a hunk's URI (strip leading /, stop at ? or #).
// Returns a slice into the original URI data.
fun void BROHunkPath(u8csp out, hunkc const *hk) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(hk->uri)) return;
    uri u = {};
    $mv(u.data, hk->uri);
    if (URILexer(&u) != OK || $empty(u.path)) return;
    $mv(out, u.path);
    if (*out[0] == '/') u8csFed(out, 1);
}

// Extract title (symbol name) from a hunk's URI fragment.
fun void BROHunkTitle(u8csp out, hunkc const *hk) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(hk->uri)) return;
    uri u = {};
    $mv(u.data, hk->uri);
    if (URILexer(&u) != OK || $empty(u.fragment)) return;
    frag fr = {};
    if (FRAGu8sDrain(u.fragment, &fr) != OK) return;
    if (fr.type == FRAG_IDENT) $mv(out, fr.body);
}

// Extract line number from a hunk's URI fragment. Returns 0 if absent.
fun u32 BROHunkLine(hunkc const *hk) {
    if ($empty(hk->uri)) return 0;
    uri u = {};
    $mv(u.data, hk->uri);
    if (URILexer(&u) != OK || $empty(u.fragment)) return 0;
    frag fr = {};
    if (FRAGu8sDrain(u.fragment, &fr) != OK) return 0;
    return fr.line;
}
#define BRO_TITLE_LINE UINT32_MAX

// --- BRO arena: scratch space for cat-mode hunk staging ---
#define BRO_ARENA_SIZE (1UL << 27)   // 128MB
#define BRO_MAX_HUNKS  4096
#define BRO_MAX_MAPS   1024

extern b8      BRO_COLOR;
extern Bu8     bro_arena;
extern hunk    bro_hunks[BRO_MAX_HUNKS];
extern u8bp    bro_maps[BRO_MAX_MAPS];
extern Bu32    bro_toks[BRO_MAX_MAPS];
extern u32     bro_nhunks;
extern u32     bro_nmaps;

ok64 BROArenaInit(void);
void BROArenaCleanup(void);
u8p  BROArenaWrite(void const *data, size_t len);
void BRODefer(u8bp mapped, Bu32 toks);

// Bump bro_nhunks after the caller has filled bro_hunks[bro_nhunks].
void BROHunkAdd(void);

// Tokenize source in hk->text using the extension from pathslice.
// Allocates toks buffer on success (caller must u32bUnMap).
// Sets hk->toks. Returns YES if tokenized, NO otherwise.
b8 BROTokenize(Bu32 toks, hunk *hk, u8csc pathslice);

// Interactive pager: displays hunks with syntax colors, diff highlighting,
// status bar, and search. Falls back to plain output when !isatty.
ok64 BRORun(hunkc const *hunks, u32 nhunks);

// Pager event loop: reads TLV hunks from pipefd, displays incrementally.
ok64 BROPipeRun(int pipefd);

// --- Navigation primitives (exposed for testing) ---

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
u32 BROHiliCount(hunkc const *hunks, u32 nhunks);

// 1-based index of the latest hili range whose first line <= `at`.
u32 BROHiliIndexAt(hunkc const *hunks, u32 nhunks,
                   range32 const *lines, u32 nlines, u32 at);

// First line containing a hili range whose first line > `mid`.
u32 BROHiliNextLine(hunkc const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid);

// Last line containing a hili range whose first line < `mid`.
u32 BROHiliPrevLine(hunkc const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid);

#endif
