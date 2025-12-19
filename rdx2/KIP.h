#include "abc/INT.h"

con ok64 SKIPBAD = 0x1c51264b28d;
con ok64 SKIPNONE = 0x7144995d85ce;
con u8 SKIP_LIT = 'K';

// Low overhead backward pointing skip lists (skip logs)
// make append-only data streams searchable.
// This implementation is decoupled from the actual byte
// stream and its particularities. Uses 256 byte blocks.
typedef struct {
    u64 pos;      // pos in the byte stream
    u64 cur;      // current skip roadsign pos
    u8 offs[48];  // roadsign offsets per se
} skip512;

fun int skip512cmp(skip512 const *a, skip512 const *b) {
    return u64cmp(&a->pos, &b->pos);
}

#define X(M, name) M##skip512##name
#include "abc/Bx.h"
#undef X

fun u64 skip512blk(u64 pos) { return (pos >> 8) + 1; }

fun u64 skip512Block(skip512cp skip) { return skip512blk(skip->cur); }

fun u8 skip512Tallness(skip512cp skip) {
    return 64 - clz64(skip512Block(skip));
}

fun b8 skip512IsNext(skip512cp skip) {
    u64 next_block = (skip->pos >> 8) + 1;
    return skip512Block(skip) < next_block;
}

// Given the new pos in the stream, plant a new skip record (or not, SKIPNONE)
// Sets cur to pos, offs should be saved to the stream
fun ok64 skip512Next(skip512p skip, u8csp offs) {
    u64 next_block = skip512blk(skip->pos);
    u64 last_block = skip512Block(skip);
    if (last_block >= next_block) return SKIPNONE;
    u8 next_off = skip->pos & 0xff;
    u8 last_off = skip->cur & 0xff;
    u8 next_hi = ctz64(next_block);
    u8 last_hi = ctz64(last_block);
    for (u8 h = 0; h <= last_hi; ++h) {
        skip->offs[h] = last_off;
    }
    if (next_block > last_block + 1) {
        u8 topflip = 64 - clz64(next_block ^ last_block);
        memset(skip->offs, 0xff, topflip);
    }
    offs[0] = skip->offs;
    offs[1] = offs[0] + next_hi + 1;
    skip->cur = skip->pos;
    return OK;
}

// Plant a full-height skip record
fun ok64 skip512TallNext(skip512p skip, u8csp offs) {
    ok64 o = skip512Next(skip, offs);  // NONE?
    u8 top = skip512Tallness(skip);
    offs[1] = offs[0] + top;
    return o;
}

// Skip back one step, but not overstepping pos:  [  pos  cur2<---cur ]
// Sets cur to the earlier record's pos, offs should be loaded from the stream
fun ok64 skip512Back(skip512p skip, u8sp offs) {
    u64 block = skip512Block(skip);
    u8 top = skip512Tallness(skip);
    u64 cur = 0;
    for (int hi = top - 1; hi >= 0; hi--) {
        u64 mask = (1 << hi) - 1;
        u64 back_block = (block - 1) & ~mask;
        u8 back_off = skip->offs[hi];
        if (back_off == 0xff) continue;
        u64 pos = ((back_block - 1) << 8) + back_off;
        if (pos <= skip->pos) continue;
        cur = pos;
        break;
    }
    if (cur) {
        skip->cur = cur;
        offs[0] = skip->offs;
        offs[1] = offs[0] + skip512Tallness(skip);
        return OK;
    } else {
        offs[0] = offs[1] = 0;
        return SKIPNONE;
    }
}
