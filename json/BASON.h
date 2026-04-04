#ifndef LIBRDX_BASON_H
#define LIBRDX_BASON_H

#include "TLKV.h"
#include "abc/INT.h"

con ok64 BASONEND = 0x1c5584dcf0;
con ok64 BASONBAD = 0x1c5584de8d;

// Container types: vowels are containers, consonants are leaves
fun b8 BASONCollection(u8 type) {
    return type == 'A' || type == 'E' || type == 'I' ||
           type == 'O' || type == 'U' || type == 'Y';
}

// Object containers: children keyed by name (sorted merge diff)
fun b8 BASONObject(u8 type) { return type == 'O'; }

// Array containers: children keyed by position (Myers diff)
fun b8 BASONArray(u8 type) {
    return BASONCollection(type) && !BASONObject(type);
}

// Page size for index sampling (one entry per page)
#define BASON_PAGE 512
// Index record TLV tag
#define BASON_IDX 'X'

// Pack 6-byte key prefix into u64 (big-endian for lexicographic compare).
// Low 16 bits reserved for page-local offset when packed as index entry.
fun u64 BASONKeyPrefix(u8csc key) {
    u64 pfx = 0;
    size_t n = $len(key);
    if (n > 6) n = 6;
    for (size_t i = 0; i < n; i++)
        pfx = (pfx << 8) | key[0][i];
    pfx <<= (6 - n) * 8;
    return pfx;
}

// --- Stack layout ---
// [end₀, end₁, ..., endₙ, cursor]
// Open pushes [data_len, 0].  Into overwrites cursor with end, pushes new cursor.
// Outo pops cursor; the end below becomes the new cursor.

// --- API A: stateless ---
// Stack holds u64 offsets into data buffer.
// Data buffer is never modified by the reader.

ok64 BASONOpen(u64bp stack, u8csc data);
ok64 BASONDrain(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val);
ok64 BASONInto(u64bp stack, u8csc data, u8csc val);
ok64 BASONOuto(u64bp stack);

// Seek within current level using page index.
// Must be called right after BASONInto (cursor == children_start).
ok64 BASONSeek(u64bp stack, u8csc data, u8csc target);

// --- Write helpers: TLKV + page index ---
// idx may be NULL (no index written).

ok64 BASONFeedInto(u64bp idx, u8bp buf, u8 type, u8csc key);
ok64 BASONFeed(u64bp idx, u8bp buf, u8 type, u8csc key, u8csc val);
ok64 BASONFeedOuto(u64bp idx, u8bp buf);

// Sort TLKV children by key inside an open container (between TLKVInto and
// TLKVOuto).  Uses idle space as scratch.  Invalidates idx entries if non-NULL.
ok64 BASONSortChildren(u8bp buf, u64bp idx);

// --- Array key helpers ---

// Minimum RON64 width to represent n sequential keys (0..n-1).
fun u8 BASONKeyWidth(u64 n) {
    u8 w = 1;
    u64 cap = 64;
    while (cap < n && w < 10) { cap *= 64; w++; }
    return w;
}

// Increment a RON64 bigint key, feed result into slice.
// Output has same width as orig. Fails on overflow (all '~').
ok64 BASONFeedInc(u8s into, u8cs orig);

// Compute a midpoint key between left and right for array splicing.
// Ensures room for len insertions with collision probability 1/pcoll.
// Empty left/right means no bound (start/end of keyspace).
ok64 BASONFindMid(u8s into, u8cs left, u8cs right,
                  u64 len, u64 pcoll, u64 random);

// Incremental key generator for sequential parsing (unknown count).
// First digit determines increment position P via width thresholds.
// Subsequent digits use full 0..63 range. Empty prev produces "0".
ok64 BASONFeedInfInc(u8s into, u8cs prev);

// --- JSON ↔ BASON ---
ok64 BASONParseJSON(u8bp buf, u64bp idx, u8cs json);
ok64 BASONExportJSON(u8s out, u64bp stack, u8csc data);
ok64 BASONExportText(u8s out, u64bp stack, u8csc data);

// --- API B: cursor struct, delegates to API A ---

typedef struct bason {
    u8    type;
    u8    ptype;
    u8bp  data;
    u64bp stack;
    u8cs  key;
    u8cs  val;
} bason;

typedef bason* basonp;

fun ok64 basonOpen(basonp x) {
    u8cs d = {u8bDataHead(x->data), u8bIdleHead(x->data)};
    return BASONOpen(x->stack, d);
}

fun ok64 basonDrain(basonp x) {
    x->ptype = x->type;
    u8cs d = {u8bDataHead(x->data), u8bIdleHead(x->data)};
    return BASONDrain(x->stack, d, &x->type, x->key, x->val);
}

#define basonNext basonDrain

fun range32 basonRange(basonp x) {
    u8cp base = u8bDataHead(x->data);
    return (range32){(u32)(x->val[0] - base), (u32)(x->val[1] - base)};
}

fun ok64 basonInto(basonp x) {
    if (!BASONCollection(x->type)) return BASONBAD;
    u8cs d = {u8bDataHead(x->data), u8bIdleHead(x->data)};
    ok64 o = BASONInto(x->stack, d, x->val);
    if (o == OK) x->ptype = x->type;
    return o;
}

fun ok64 basonOuto(basonp x) {
    return BASONOuto(x->stack);
}

fun ok64 basonSeek(basonp x, u8csc target) {
    u8cs d = {u8bDataHead(x->data), u8bIdleHead(x->data)};
    return BASONSeek(x->stack, d, target);
}

// Descend one level toward byte offset pos within data.
// Drains elements at current level until finding a container that
// spans pos, then enters it.  Caller can inspect x->type/key/val
// to see each ancestor.  Returns OK on descent, BASONEND when the
// cursor is at the target level (ready to basonDrain the element).
fun ok64 basonSeekStep(basonp x, u64 pos) {
    u8cp base = u8bDataHead(x->data);
    ok64 o;
    while ((o = basonDrain(x)) == OK) {
        if (!BASONCollection(x->type)) continue;
        u64 vstart = (u64)(x->val[0] - base);
        u64 vend = (u64)(x->val[1] - base);
        if (pos >= vstart && pos < vend)
            return basonInto(x);
    }
    *u64bLast(x->stack) = pos;
    x->type = 0;
    x->ptype = 0;
    x->key[0] = x->key[1] = NULL;
    x->val[0] = x->val[1] = NULL;
    return BASONEND;
}

// Seek to byte offset pos from root, descending level by level.
// On return the stack holds the full ancestor chain and the cursor
// is at pos, ready for basonDrain.
fun ok64 basonSeekTo(basonp x, u64 pos) {
    u64 dlen = u8bDataLen(x->data);
    if (pos > dlen) return BASONBAD;
    u64bReset(x->stack);
    ok64 o = u64bFeed1(x->stack, dlen);
    if (o != OK) return o;
    o = u64bFeed1(x->stack, (u64)0);
    if (o != OK) return o;
    x->type = 0;
    x->ptype = 0;
    x->key[0] = x->key[1] = NULL;
    x->val[0] = x->val[1] = NULL;
    while ((o = basonSeekStep(x, pos)) == OK) {}
    return (o == BASONEND) ? OK : o;
}

// Position cursor at byte offset pos without descending from root.
// Stack is reset to [data_len, pos]; parent type is unknown (0).
// Ready for basonDrain from that offset.
fun ok64 basonBlindSeek(basonp x, u64 pos) {
    u64 dlen = u8bDataLen(x->data);
    if (pos > dlen) return BASONBAD;
    u64bReset(x->stack);
    ok64 o = u64bFeed1(x->stack, dlen);
    if (o != OK) return o;
    o = u64bFeed1(x->stack, pos);
    if (o != OK) return o;
    x->type = 0;
    x->ptype = 0;
    x->key[0] = x->key[1] = NULL;
    x->val[0] = x->val[1] = NULL;
    return OK;
}

// --- Write path wrappers ---

fun ok64 basonFeedInto(basonp x, u8 type, u8csc key) {
    return BASONFeedInto(x->stack, x->data, type, key);
}

fun ok64 basonFeed(basonp x, u8 type, u8csc key, u8csc val) {
    return BASONFeed(x->stack, x->data, type, key, val);
}

fun ok64 basonFeedOuto(basonp x) {
    return BASONFeedOuto(x->stack, x->data);
}

fun ok64 basonParseJSON(basonp x, u8cs json) {
    return BASONParseJSON(x->data, x->stack, json);
}

#endif
