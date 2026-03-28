#ifndef LIBRDX_SPOT_H
#define LIBRDX_SPOT_H

#include "json/BASON.h"

con ok64 SPOTEND = 0x1c65874e5cd;
con ok64 SPOTBAD = 0x1c65874b28d;

#define SPOT_MAX_BINDS 52
#define SPOT_MAX_SUBS 32
#define SPOT_MAX_NTOKS 64

// Flattened needle token
typedef struct {
    u8cs val;
    u8   type;
    b8   skip;    // YES if 2+ space gap before this token
    u32  parent;  // needle parent BASON offset (for bracket constraint)
} SPOTntok;

typedef struct {
    u8cs ndl;
    u8cs hay;
    u8cs source;                  // original source text (set by caller)
    u64  hstk_store[256];
    u64p hstk[4];
    u64  bound;                   // bitmask of bound placeholders
    u64  match;                   // match position (byte offset)
    u64  subs[SPOT_MAX_SUBS];    // byte offsets of gap-separated segments
    u8   nsubs;                   // number of segments
    u8   depth;
    b8   exhausted;
    // Flat matching state
    SPOTntok ntoks[SPOT_MAX_NTOKS];
    int      nntoks;
    u64      src_pos;             // cumulative source byte position
    u32      parents[64];         // parent BASON offset at each depth
    range32  src_rng;             // overall match source range
    match32  bind_matches[SPOT_MAX_BINDS];
    match32p ranges[4];  // caller-provided buffer (NULL = disabled)
} SPOTstate;

// Initialize. Parses needle_src with BAST (ext = file extension).
// ndl_buf/ndl_idx: caller-provided buffers for parsed needle.
// hay: pre-parsed haystack BASON.
ok64 SPOTInit(SPOTstate *st, u8bp ndl_buf, u64bp ndl_idx,
              u8csc needle_src, u8csc ext, u8csc hay);

// Find next match. Returns OK on match, SPOTEND when exhausted.
// On OK: st->match is the byte offset of the matched container,
// st->bind_ranges[] holds source slices for bound placeholders,
// st->bound is the bitmask of which are bound,
// st->subs[0..nsubs) holds byte offsets of gap-separated segments.
ok64 SPOTNext(SPOTstate *st);

// Find source byte range [*lo, *hi) of the BASON element at bson_off.
// Walks leaves under that element; their vals are copies of source text.
ok64 SPOTSourceRange(u8csc hay, u64 bson_off, u64p lo, u64p hi);

// Apply SPOT replacement to all matches in one file.
// source: original file content.
// hay: pre-parsed BASON from BASTParse on source.
// needle_src/replace_src: code pattern / replacement template as text.
// ext: file extension (e.g. ".c").
// out: output buffer (caller-allocated, should be >= 2*source size).
// Returns OK if replacements were made, SPOTEND if no matches.
ok64 SPOTReplace(u8s out, u8csc source, u8csc hay,
                 u8csc needle_src, u8csc replace_src, u8csc ext);

#endif
