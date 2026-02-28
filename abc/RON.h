#ifndef ABC_RON_H
#define ABC_RON_H

#include "01.h"
#include <time.h>

typedef u64 ron60;
typedef const ron60 ron60c;
typedef ron60* ron60p;
typedef const ron60 * ron60cp;

#define ron60Max ((1UL << 60) - 1)

// RON Base64 character set
extern const char* RON64_CHARS;
extern const u8 RON64_REV[256];

typedef enum {
    RON_0 = 0, RON_1, RON_2, RON_3, RON_4, RON_5, RON_6, RON_7, RON_8, RON_9,
    RON_A, RON_B, RON_C, RON_D, RON_E, RON_F, RON_G, RON_H, RON_I, RON_J,
    RON_K, RON_L, RON_M, RON_N, RON_O, RON_P, RON_Q, RON_R, RON_S, RON_T,
    RON_U, RON_V, RON_W, RON_X, RON_Y, RON_Z,
    RON__,
    RON_a, RON_b, RON_c, RON_d, RON_e, RON_f, RON_g, RON_h, RON_i, RON_j,
    RON_k, RON_l, RON_m, RON_n, RON_o, RON_p, RON_q, RON_r, RON_s, RON_t,
    RON_u, RON_v, RON_w, RON_x, RON_y, RON_z,
    RON_TILDE,
} RON64;

#define ok64sub(o, u) ((o<<6)|(u&63))

// Extract 6-bit value from ok64 at given index
fun u8 ok64Lit(ok64 o, u8 ndx) { return (o >> (ndx * 6)) & 63; }

// Encode u64 to RON base64 string
ok64 RONutf8sFeed(u8** into, ok64 val);

// Decode RON base64 string to u64
ok64 RONutf8sDrain(ok64* o, u8c* const* from);

// Convert struct tm to RON60 timestamp (months are 1-based in RON60)
ok64 RONOfTime(ron60* r, struct tm* t);

// Convert RON60 timestamp to struct tm (months are 1-based in RON60)
ok64 RONToTime(ron60 r, struct tm* t);

con ok64 RONBAD = 0xe2a2a61cf1;

// Verify string contains only valid RON64 characters (non-empty)
ok64 RONVerify(u8c** txt);

// Fixed-width zero-padded RON64 encoding (big-endian, left-padded with '0')
ok64 RONu8sFeedPad(u8** into, ok64 val, u8 width);

// Lex-sortable variable-width RON64 encoding.
// Top digit encodes width: 0-31 → w1, 32-47 → w2, 48-55 → w3, ...
ok64 RONu8sFeedInc(u8** into, u32 val);

// Compute random base offset and key width for a splice of n elements
ok64 RONSpliceBase(ok64 *base, u8 *width, u64 rand, u64 prob, ok64 n);

// Left-align a short ron60 to fill the 60-bit space.
// ron60Z(0x25_01) = 0x25_01_00_00_00_00_00_00_00  ("a1" -> "a100000000")
fun ron60 ron60Norm(ron60 r) {
    u8 digits = (64 - clz64(r|1) + 5) / 6;
    return r << ((10 - digits) * 6);
}

// Right-align a normalized ron60, stripping trailing zero digits.
// ron60DeNorm("a100000000") = "a1"
fun ron60 ron60DeNorm(ron60 r) {
    if (r == 0) return 0;
    u8 tail = ctz64(r) / 6;
    return r >> (tail * 6);
}

fun ron60 ron60NormInc(ron60 r) {
   ron60 shift = (r >> 54) >> 3;
   return r + (1UL<<(shift*6));
}

fun ron60 ron60Inc(ron60 r) {
  ron60 norm = ron60Norm(r);
  norm = ron60NormInc(norm);
  return ron60DeNorm(norm);
}

fun ron60 ron60NormInk(ron60 r) {
   ron60 shift = (r >> 54) >> 3;
   return r + (1UL << ((9 - shift) * 6));
}

fun ron60 ron60Ink(ron60 r) {
  ron60 norm = ron60Norm(r);
  norm = ron60NormInk(norm);
  return ron60DeNorm(norm);
}

fun b8 ron60Z(ron60cp a, ron60cp b) {
    ron60 aa = ron60Norm(*a);
    ron60 bb = ron60Norm(*b);
    return aa < bb;
}

#endif
