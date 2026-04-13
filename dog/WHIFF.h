#ifndef DOG_WHIFF_H
#define DOG_WHIFF_H

//  WHIFF: 64-bit tagged word with offset[40] | id[20] | type[4].
//
//  Hashlet in the MS 40 bits so entries sort by hashlet first.
//  When id=0, the hashlet effectively spans 60 bits (id bits
//  become additional hashlet bits).
//
//  Hashlets are big-endian: first SHA byte in the top bits.
//  Construct via flip64(memcpy 8 SHA bytes) >> shift.
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

// --- SHA-1 hashlet helpers ---
//
// Two widths:
//   40-bit (10 hex chars): for vals where id field is used separately
//   60-bit (15 hex chars): for keys where id=0 (hashlet spans both fields)
//
// Both are big-endian: first SHA byte in the most significant bits.

// 40-bit hashlet: first 5 bytes of SHA (10 hex chars)
fun u64 wh64Hashlet(u8csc sha) {
    u64 h = 0;
    memcpy(&h, sha[0], 8);
    return (flip64(h) >> 24) & WHIFF_OFF_MASK;
}

// 60-bit hashlet: first 7.5 bytes of SHA (15 hex chars)
#define WHIFF_HASHLET60_BITS  60
#define WHIFF_HASHLET60_MASK  ((1ULL << 60) - 1)

fun u64 wh64Hashlet60(u8csc sha) {
    u64 h = 0;
    memcpy(&h, sha[0], 8);
    return (flip64(h) >> 4) & WHIFF_HASHLET60_MASK;
}


// --- Hex conversion (works for both 40 and 60 bit) ---

// Hashlet → hex string. nchars: up to 10 for 40-bit, 15 for 60-bit.
// Top nibble is at bit (width-4), extracted via >> (width-4 - i*4).
fun void wh64HashletHex(char *out, u64 hashlet, size_t nchars, size_t width) {
    if (nchars > width / 4) nchars = width / 4;
    for (size_t i = 0; i < nchars; i++) {
        u8 nib = (u8)((hashlet >> (width - 4 - i * 4)) & 0xf);
        out[i] = "0123456789abcdef"[nib];
    }
    out[nchars] = 0;
}

// Convenience: 40-bit hex (10 chars max)
fun void wh64Hex40(char *out, u64 hashlet, size_t nchars) {
    wh64HashletHex(out, hashlet, nchars, 40);
}

// Convenience: 60-bit hex (15 chars max)
fun void wh64Hex60(char *out, u64 hashlet, size_t nchars) {
    wh64HashletHex(out, hashlet, nchars, 60);
}

// Hex prefix → hashlet (zero-padded in low bits).
// width: 40 or 60.
fun u64 wh64HashletFromHex(u8csc hex, size_t width) {
    size_t max = width / 4;
    size_t nchars = u8csLen(hex);
    if (nchars > max) nchars = max;
    u64 h = 0;
    $for(u8c, p, hex) {
        if ((size_t)(p - hex[0]) >= nchars) break;
        u8 c = *p;
        u8 nib = 0;
        if (c >= '0' && c <= '9') nib = c - '0';
        else if (c >= 'a' && c <= 'f') nib = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nib = c - 'A' + 10;
        h = (h << 4) | nib;
    }
    h <<= (max - nchars) * 4;
    return h;
}

// Convenience: 40-bit from hex
fun u64 wh64FromHex40(u8csc hex) {
    return wh64HashletFromHex(hex, 40);
}

// Convenience: 60-bit from hex
fun u64 wh64FromHex60(u8csc hex) {
    return wh64HashletFromHex(hex, 60);
}

// Hex prefix → 40-bit hashlet (convenience for sniff state)
fun u64 wh64HexHashlet(u8csc hex) {
    return wh64HashletFromHex(hex, 40);
}

// Hex prefix → 60-bit hashlet (convenience for keeper index)
fun u64 wh64HexHashlet60(u8csc hex) {
    return wh64HashletFromHex(hex, 60);
}

// Compare hashlet against hex prefix of any length.
fun b8 wh64HashletMatch(u64 hashlet, u8csc hex, size_t width) {
    char full[16];
    wh64HashletHex(full, hashlet, width / 4, width);
    size_t nchars = u8csLen(hex);
    if (nchars > width / 4) nchars = width / 4;
    return memcmp(full, hex[0], nchars) == 0;
}

#endif
