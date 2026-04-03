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
int CAPOFormatTitle(char *out, size_t outsz,
                     const char *filepath, const char *funcname);
void CAPOGrepCtx(u8csc source, u32 match_pos, u32 nctx,
                  u32 *lo, u32 *hi);

// --- Trigram query helpers ---
ok64 CAPOCollectPaths(u64css iter, u64 tri_prefix, u32 *hashes,
                      u32p nhashes, u32 maxhashes);
u32 CAPOIntersect(u32 *a, u32 na, u32 *b, u32 nb, u32 *out);
int CAPOu32cmp(const void *a, const void *b);

// --- MSET/DIFF template instantiations for u64 ---

#define X(M, name) M##u64##name
#include "abc/MSETx.h"
#undef X

#define X(M, name) M##u64##name
#include "abc/DIFFx.h"
#undef X

#define CAPO_MAX_HLS 64
#define HUNK_MAX 64

#endif
