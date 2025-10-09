//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_BINS_HPP
#define ABC_BINS_HPP

#include "01.h"
#include "S.h"

/**
 * Numbering for (aligned) logarithmical bins.
 *
 * Each number stands for an interval
 *   [layer_offset * 2^layer, (layer_offset + 1) * 2^layer).
 *
 * The following value is called as base_offset:
 *   layer_offset * 2^layer -- is called
 *
 * Bin numbers in the tail111 encoding: meaningless bits in
 * the tail are set to 0111...11, while the head denotes the offset.
 * bin = 2 ^ (layer + 1) * layer_offset + 2 ^ layer - 1
 *
 * Thus, 1101 is the bin at layer 1, offset 3 (i.e. fourth).
 *
 *
 *                  +-----------------00111-----------------+
 *                  |                                       |
 *        +-------00011-------+                   +-------01011-------+
 *        |                   |                   |                   |
 *   +--00001--+         +--00101--+         +--01001--+         +--01101--+
 *   |         |         |         |         |         |         |         |
 * 00000     00010     00100     00110     01000     01010     01100     1110
 *
 *
 *
 *               7
 *           /       \
 *       3              11
 *     /   \           /  \
 *   1       5       9     13
 *  / \     / \     / \    / \
 * 0   2   4   6   8  10  12 14
 *
 *  * The comment is from 2009; this layout was later described in RFC7574.
 *    See also a post with nice pictures by Russ Cox.
 */
typedef u64 bin64;

#define bin64empty 0xffffffffffffffffUL
#define bin64top 0x7fffffffffffffffUL

#define T bin64
#define X(D, V) D##bin64##V

#define bin64fmt "(%lu@%u)"
#define bin64fmtd(x) bin64offset(x), (u32)bin64level(x)

fun h64 X(, hash)(T a) { return mix64(a); }

#include "B.h"

#undef T
#undef X

fun bin64 bin64of(u8 level, u64 ndx) {
    return (ndx << (level + 1)) | ((1UL << level) - 1);
}

#define aBIN(name, lev, ndx) bin64 name = bin64of(lev, ndx)

fun u8 bin64level(bin64 bin) { return ctz64(~bin); }

fun u64 bin64level_bits(bin64 bin) { return (bin + 1) ^ bin; }

fun u64 bin64head(bin64 bin) { return (bin & (bin + 1)) >> 1; }

fun u64 bin64size(bin64 bin) { return 1UL << bin64level(bin); }

fun u64 bin64term(bin64 b) { return bin64head(b) + bin64size(b); }

fun u64 bin64$len(u64 len) { return (len << 1) - 1; }

fun bin64 bin64sibling(bin64 b) { return b ^ (bin64level_bits(b) + 1); }

fun b8 bin64is_adjacent(bin64 a, bin64 b) {
    return bin64term(a) == bin64head(b);
}

fun bin64 bin64parent(bin64 b) {
    u64 lbs = bin64level_bits(b);
    i64 nlbs = -2 - lbs;
    return (u64)(b | lbs) & nlbs;
}

fun bin64 bin64ancestor(bin64 descendant, u8 levels) {
    if (!levels) return descendant;
    u64 b = bin64level_bits(descendant) + 1;
    u64 s = (b << levels) - 1;
    return (descendant & ~s) | (s >> 1);
}

fun bin64 bin64descendant(bin64 ancestor, u8 levels) {
    if (!levels) return ancestor;
    u64 bits = bin64level_bits(ancestor);
    u64 offset = ancestor & ~bits;
    return offset | (bits >> (levels + 1));
}

fun u64 bin64zero_bit(bin64 b) { return (bin64level_bits(b) + 1) >> 1; }

fun bin64 bin64daughter(bin64 b) { return b - (bin64zero_bit(b) >> 1); }

fun bin64 bin64son(bin64 b) {
    u64 zero = bin64zero_bit(b);
    return (b - (zero >> 1)) | zero;
}

fun b8 bin64contains(bin64 a, bin64 b) {
    if (a == bin64empty) return NO;

    u64 a_bits = bin64level_bits(a);
    u64 b_bits = bin64level_bits(b);

    return (a_bits >= b_bits) && ((a | a_bits) == (b | a_bits));
}

fun bin64 bin64find_peak(u64 base_bin, u64 length) {
    must(base_bin < length);
    u64 x = base_bin ^ length;
    u64 m = 64 - (clz64(x) + 1);
    return bin64of(m, base_bin >> m);
}

fun u64 bin64offset(bin64 b) {
    u8 lev = bin64level(b);
    return b >> (lev + 1);
}

fun b8 bin64is_son(bin64 b) { return bin64offset(b) & 1; }

fun b8 bin64is_base(bin64 b) { return !(b & 1); }

fun b8 bin64patch_midpeak(bin64 *midpeak, u64 len, u64 newlen) {
    must(newlen > len);
    u64 flips = len ^ newlen;
    u64 b = clz64(flips);
    u8 level = 63 - b;
    u64 changed = (1UL << level) - 1;
    if (!(changed & len)) return NO;
    u64 offset = len >> level;
    *midpeak = bin64of(level, offset);
    return YES;
}

/** all new peaks at newlen compared to oldlen, ordered low to high
fun void bin64patch_peaks(Bins& bins, u64 oldlen, u64 newlen) {
}*/

fun bin64 bin64patch_prev(u64 *len, u64 oldlen) {
    must(oldlen < *len);
    bin64 b = bin64of(0, *len - 1);
    while (bin64is_son(b)) {
        u64 p = bin64parent(b);
        if (bin64head(p) <= oldlen) break;
        b = p;
    }
    *len = bin64head(b);
    return b;
}

fun bin64 bin64patch_next(u64 *len, u64 newlen) {
    must(newlen > *len);
    bin64 b = bin64of(0, *len);
    while (!bin64is_son(b)) {  // TODO formula
        u64 p = bin64parent(b);
        if (bin64term(p) > newlen) break;
        b = p;
    }
    *len = bin64term(b);
    return b;
}

fun bin64 bin64next(bin64 b) { return b + (bin64level_bits(b) + 1); }

fun int bin64cmp(bin64 const *a, bin64 const *b) {
    if (*a < *b) {
        return -1;
    } else if (*a > *b) {
        return 1;
    } else {
        return 0;
    }
}

#define X(M, name) M##bin64##name
#include "Bx.h"
#undef X

fun ok64 BINpeaks($bin64 bins, size_t len) {
    // 00101 -> 001000, 000011
    ok64 o = OK;
    while (len && o == OK) {
        u64 l = ctz64(len);
        u64 zeros = (1UL << (l + 1)) - 1;
        u64 ones = (1UL << l) - 1;
        len &= ~zeros;
        u64 bin = (len << 1) | ones;
        o = bin64sFeed1(bins, bin);
    }
    return o;
}

fun ok64 BINpath($bin64 bins, size_t len, bin64 b) {
    ok64 o = OK;
    while (bin64term(b) < len && o == OK) {
        bin64 sibling = bin64sibling(b);
        o = bin64sFeed1(bins, sibling);
        b = bin64parent(b);
    }
    return o;
}

#endif  // RON_BINS_HPP
