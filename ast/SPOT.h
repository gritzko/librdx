#ifndef LIBRDX_SPOT_H
#define LIBRDX_SPOT_H

#include "json/BASON.h"

con ok64 SPOTEND = 0x1c65874e5cd;
con ok64 SPOTBAD = 0x1c65874b28d;

#define SPOT_MAX_BINDS 52
#define SPOT_MAX_SUBS 32

fun u64 SPOTLogPack(u32 hay, u16 ndl, u16 extra) {
    return ((u64)hay << 32) | ((u64)ndl << 16) | (u64)extra;
}
fun u32 SPOTLogHay(u64 e) { return (u32)(e >> 32); }
fun u16 SPOTLogNdl(u64 e) { return (u16)(e >> 16); }
fun u16 SPOTLogExtra(u64 e) { return (u16)(e & 0xFFFF); }

typedef struct {
    u8cs ndl;
    u8cs hay;
    u64  hstk_store[256];
    u64p hstk[4];
    u64  binds[SPOT_MAX_BINDS];  // byte offsets into haystack data
    u64  bound;                   // bitmask of bound placeholders
    u64  match;                   // match position (byte offset)
    u64  subs[SPOT_MAX_SUBS];    // byte offsets of gap-separated segments
    u8   nsubs;                   // number of segments
    u8   depth;
    b8   exhausted;
    u64p mlog[4];  // match log buffer (caller-provided, NULL = disabled)
    u64p alog[4];  // alias log buffer (caller-provided, NULL = disabled)
} SPOTstate;

// Initialize. Parses needle_src with BAST (ext = file extension).
// ndl_buf/ndl_idx: caller-provided buffers for parsed needle.
// hay: pre-parsed haystack BASON.
ok64 SPOTInit(SPOTstate *st, u8bp ndl_buf, u64bp ndl_idx,
              u8csc needle_src, u8csc ext, u8csc hay);

// Find next match. Returns OK on match, SPOTEND when exhausted.
// On OK: st->match is the byte offset of the matched container,
// st->binds[] holds byte offsets of bound placeholders,
// st->bound is the bitmask of which are bound,
// st->subs[0..nsubs) holds byte offsets of gap-separated segments.
// Caller uses basonSeekTo() to rehydrate any offset.
ok64 SPOTNext(SPOTstate *st);

#endif
