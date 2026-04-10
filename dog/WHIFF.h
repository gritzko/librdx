#ifndef DOG_WHIFF_H
#define DOG_WHIFF_H

//  WHIFF: 64-bit tagged word with hashlet[40] | id[20] | type[4].
//
//  Hashlet in the MS 40 bits so entries sort by hashlet first.
//  Hashlets are big-endian: first SHA byte in the top bits.
//  Construct via flip64(memcpy 8 SHA bytes) >> 24.
//
//  Canonical layout used across dogs:
//    keeper: hashlet=sha_prefix, id=file_number, type=pack_format
//    graf:   hashlet=sha_prefix, id=generation,  type=entry_kind
//    sniff:  off=mtime,          id=path_index,  type=flags

#include "abc/INT.h"

typedef u64 wh64;

#define WHIFF_TYPE_BITS   4
#define WHIFF_TYPE_MASK   0xfULL
#define WHIFF_ID_SHIFT    WHIFF_TYPE_BITS       // bits 4-23
#define WHIFF_ID_BITS     20
#define WHIFF_ID_MASK     ((1ULL << WHIFF_ID_BITS) - 1)
#define WHIFF_OFF_SHIFT   (WHIFF_TYPE_BITS + WHIFF_ID_BITS)  // bits 24-63
#define WHIFF_OFF_BITS    40
#define WHIFF_OFF_MASK    ((1ULL << WHIFF_OFF_BITS) - 1)

fun wh64 wh64Pack(u8 type, u32 id, u64 off) {
    return ((u64)type & WHIFF_TYPE_MASK) |
           (((u64)id & WHIFF_ID_MASK) << WHIFF_ID_SHIFT) |
           ((off & WHIFF_OFF_MASK) << WHIFF_OFF_SHIFT);
}

fun u8  wh64Type(wh64 v) { return (u8)(v & WHIFF_TYPE_MASK); }
fun u32 wh64Id(wh64 v)   { return (u32)((v >> WHIFF_ID_SHIFT) & WHIFF_ID_MASK); }
fun u64 wh64Off(wh64 v)  { return (v >> WHIFF_OFF_SHIFT) & WHIFF_OFF_MASK; }

// --- SHA-1 hashlet helpers (big-endian, first SHA byte on top) ---

fun u64 wh64Hashlet(u8cp sha) {
    u64 h = 0;
    memcpy(&h, sha, 8);
    return (flip64(h) >> 24) & WHIFF_OFF_MASK;
}

// Hashlet → hex prefix string (up to 10 chars).
fun void wh64HashletHex(char *out, u64 hashlet, size_t nchars) {
    if (nchars > 10) nchars = 10;
    for (size_t i = 0; i < nchars; i++) {
        u8 nib = (u8)((hashlet >> (36 - i * 4)) & 0xf);
        out[i] = "0123456789abcdef"[nib];
    }
    out[nchars] = 0;
}

// Hex prefix → hashlet (zero-padded in low bits).
fun u64 wh64HashletFromHex(char const *hex, size_t nchars) {
    if (nchars > 10) nchars = 10;
    u64 h = 0;
    for (size_t i = 0; i < nchars; i++) {
        u8 c = (u8)hex[i];
        u8 nib = 0;
        if (c >= '0' && c <= '9') nib = c - '0';
        else if (c >= 'a' && c <= 'f') nib = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nib = c - 'A' + 10;
        h = (h << 4) | nib;
    }
    h <<= (10 - nchars) * 4;  // zero-pad remaining
    return h & WHIFF_OFF_MASK;
}

// Compare hashlet against a hex prefix of any length (up to 10 chars).
fun b8 wh64HashletMatch(u64 hashlet, char const *hex, size_t nchars) {
    if (nchars > 10) nchars = 10;
    u64 prefix = wh64HashletFromHex(hex, nchars);
    u64 mask = WHIFF_OFF_MASK << ((10 - nchars) * 4);  // wrong approach
    // Simpler: compare hex output
    char full[12];
    wh64HashletHex(full, hashlet, 10);
    return memcmp(full, hex, nchars) == 0;
}

#endif
