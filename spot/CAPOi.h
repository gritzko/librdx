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
void CAPOFilterInPlace(Bu32 hashbuf, u64css iter, u64 prefix);
void CAPOFilterInPlaceUnion(Bu32 hashbuf,
                             u64css ia, u64 pa,
                             u64css ib, u64 pb);
u32 CAPOIntersect(u32s a, u32csc b);
int CAPOu32cmp(const void *a, const void *b);

// --- HIT instantiation for u64cs ---

// Manual Swap for u64cs (array type, can't use Sx.h)
fun void u64csSwap(u64cs *a, u64cs *b) {
    u64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*b)[0] = t0;
    (*b)[1] = t1;
}

#define X(M, name) M##u64##name
#include "abc/HITx.h"
#undef X

// --- DIFF template for u64 ---

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
