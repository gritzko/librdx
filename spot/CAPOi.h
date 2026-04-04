#ifndef SPOT_CAPOi_H
#define SPOT_CAPOi_H

#include "CAPO.h"
#include "abc/DIFF.h"
#include "abc/FILE.h"
#include "spot/LESS.h"

// --- Helper: find file extension ---

#define CAPOFindExt(ext, path, len)                      \
    do {                                                 \
        (ext)[0] = NULL; (ext)[1] = NULL;                \
        for (size_t _i = (len); _i > 0; _i--) {         \
            if ((path)[_i - 1] == '/') break;            \
            if ((path)[_i - 1] == '.') {                 \
                (ext)[0] = (u8cp)(path) + _i - 1;       \
                (ext)[1] = (u8cp)(path) + (len);         \
                break;                                   \
            }                                            \
        }                                                \
    } while (0)

// --- Display helpers ---
void CAPOProgress(const char *line);
b8 CAPOExtIs(u8csc ext, const char *a, const char *b);
void CAPOFindFunc(u8csc source, u32 pos, u8csc ext,
                   char *out, size_t outsz);
ok64 CAPOFormatTitle(u8s into,
                     const char *filepath, const char *funcname);
void CAPOGrepCtx(u8csc source, u32 match_pos, u32 nctx,
                  u32 *lo, u32 *hi);

// --- Trigram query helpers ---
ok64 CAPOCollectPaths(u64css iter, u64 tri_prefix, u32g hashes);
u32 CAPOIntersect(u32s a, u32csc b);
int CAPOu32cmp(const void *a, const void *b);

// --- HIT/DIFF template instantiations for u64 ---

// Manual Swap for u64cs (array type, can't use Sx.h)
fun void u64csSwap(u64cs *a, u64cs *b) {
    u64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*b)[0] = t0;
    (*b)[1] = t1;
}
typedef b8 (*u64csz)(u64cs const *, u64cs const *);
typedef ok64 (*u64csx)(u64cs *, u64cs const *);
typedef ok64 (*u64csy)(u64cs *, u64css);

#define HIT_ENTRY_IS_SLICE
#define X(M, name) M##u64cs##name
#include "abc/HITx.h"
#undef X
#undef HIT_ENTRY_IS_SLICE

// Compare u64cs entries by head value
fun b8 u64csHeadZ(u64cs const *a, u64cs const *b) {
    return *(*a)[0] < *(*b)[0];
}

// Advance past equal u64 values
fun ok64 u64csNextX(u64cs *a, u64cs const *b) {
    u64 val = *(*b)[0];
    while (!$empty(*a) && *(*a)[0] == val) ++(*a)[0];
    return $empty(*a) ? NODATA : OK;
}

// Binary search within entry
fun ok64 u64csSeekX(u64cs *a, u64cs const *b) {
    u64c *const run[2] = {(*a)[0], (*a)[1]};
    u64c *pos = $u64findge(run, (*b)[0]);
    (*a)[0] = pos;
    return $empty(*a) ? NODATA : OK;
}

// Seek with raw u64 key
fun ok64 CAPOHITSeek(u64css heap, u64 key, u64csz z) {
    u64cs keyentry = {&key, &key + 1};
    return HITu64csSeekXZ(heap, &keyentry, u64csSeekX, z);
}

// Step heap: advance top entry by one, eject if exhausted
fun void CAPOHITStep(u64css heap, u64csz z) {
    ++(*heap[0])[0];
    if ($empty(*heap[0])) {
        HITu64csEject(heap, 0);
    }
    if (!$empty(heap)) HITu64csDownYZ(heap, 0, z);
}

#define X(M, name) M##u64##name
#include "abc/DIFFx.h"
#undef X

#define CAPO_MAX_HLS 64
#define HUNK_MAX 64

// --- Hunk building (shared by SPOT/GREP/DIF2) ---
ok64 CAPOBuildHunk(u8csc source, u32cs htoks, u32 ctx_lo, u32 ctx_hi,
                   range32 const *hls, int nhl,
                   u8csc file_ext, const char *filepath,
                   b8 needs_title, b8 *first_hunk);

#endif
