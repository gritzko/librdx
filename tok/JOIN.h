//
// JOIN - Token-level 3-way file merge (git merge driver)
//
// Tokenizes base/ours/theirs, hashes tokens with RAP,
// runs Myers diff on hash arrays, walks lockstep to merge.
//

#ifndef TOK_JOIN_H
#define TOK_JOIN_H

#include "TOK.h"
#include "abc/DIFF.h"
#include "abc/RAP.h"

// Token u32: ms 5 bits = lit tag (A-Z), lower 27 bits = byte offset
#define JOIN_LIT(t) ((t) >> 27)
#define JOIN_OFF(t) ((t) & 0x07FFFFFFU)
#define JOIN_TOK(lit, off) (((u32)(lit) << 27) | ((off) & 0x07FFFFFFU))

// Hash u64 marking: ms 2 bits
// For base hashes:
#define JOIN_RM_O (1ULL << 63)  // removed by ours
#define JOIN_RM_T (1ULL << 62)  // removed by theirs
// For ours/theirs hashes:
#define JOIN_IN (1ULL << 63)  // inserted (not from base)
// Mask and strip:
#define JOIN_MARK (3ULL << 62)
#define JOIN_HASH(h) ((h) & ~JOIN_MARK)

con ok64 JOINFAIL = 0x4d85cf3ca495;
con ok64 JOINBAD = 0x4d85cfb28d;

// Tokenized file ready for merge
typedef struct {
    u8cs data;        // original file content
    u32 *toks[4];     // u32b: token array (lit:5 | off:27)
    u64 *hashes[4];   // u64b: per-token RAPHash values
} JOINfile;

ok64 JOINTokenize(JOINfile *jf, u8csc data, u8csc ext);
ok64 JOINMerge(u8bp out, JOINfile *base, JOINfile *ours, JOINfile *theirs);
void JOINFree(JOINfile *jf);

#endif
