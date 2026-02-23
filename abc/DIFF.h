//
// DIFF - Linear-space Myers diff for slices
//

#ifndef ABC_DIFF_H
#define ABC_DIFF_H

#include "OK.h"

typedef u32 e32;
typedef e32 const e32c;
typedef e32 *e32s[2];
typedef e32 *e32g[3];
typedef e32 const *e32cs[2];

#define DIFF_EQ 0
#define DIFF_DEL 1
#define DIFF_INS 2

#define DIFF_OP(e) ((e) >> 30)
#define DIFF_LEN(e) ((e) & 0x3FFFFFFF)
#define DIFF_ENTRY(op, len) (((e32)(op) << 30) | ((len) & 0x3FFFFFFF))

con ok64 DIFFNOROOM = 0x3523cf5d86d8616;

static inline u64 DIFFWorkSize(u64 alen, u64 blen) {
    return 2 * (2 * (alen + blen) + 1);
}

static inline u64 DIFFEdlMaxEntries(u64 alen, u64 blen) {
    return alen + blen;
}

#endif
