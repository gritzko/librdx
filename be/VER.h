#ifndef LIBRDX_VER_H
#define LIBRDX_VER_H

#include "abc/INT.h"
#include "abc/RON.h"

// Version point: u128 with time in MS word, origin in LS word.
// _64[1] = time ron60 (bits 0-59) | operator (bits 60-63)
// _64[0] = branch/origin ron60
// Pure points (op=0): u128cmp gives chronological ordering.
typedef u128 ron120;
typedef ron120 *ron120p;
typedef ron120 const *ron120cp;
typedef u128s ron120s;
typedef u128cs ron120cs;

// Operator constants (stored in bits 60-63 of _64[1])
#define VER_ANY 0
#define VER_LE  1
#define VER_GT  2
#define VER_EQ  3

#define VER_MAX 16

// --- Inline accessors ---

fun ron120 VERMake(ron60 time, ron60 origin, u8 op) {
    ron120 v = {};
    v._64[1] = (time & 0x0FFFFFFFFFFFFFFFUL) | ((u64)(op & 0xF) << 60);
    v._64[0] = origin;
    return v;
}

fun ron120 VERPoint(ron60 time, ron60 origin) {
    return VERMake(time, origin, VER_ANY);
}

fun ron60 VERTime(ron120cp v) {
    return v->_64[1] & 0x0FFFFFFFFFFFFFFFUL;
}

fun ron60 VEROrigin(ron120cp v) {
    return v->_64[0];
}

fun u8 VEROp(ron120cp v) {
    return (u8)(v->_64[1] >> 60);
}

// --- Functions (VER.c) ---

// Parse single version entry "time-origin" / "time+origin" / "time=origin" / "origin"
// Operators: '-' -> VER_LE, '+' -> VER_GT, '=' -> VER_EQ, none -> VER_ANY
ok64 VERParse(ron120p into, u8cs text);

// Parse formula "branchA&stamp-branchB&stamp+branchC&stamp=branchD"
// Feeds entries into slice, advancing into[0].
ok64 VERFormParse(ron120s into, u8cs query);

// Build formula from parsed branch entries (copies + appends base)
ok64 VERFormFromBranches(ron120s into, int branchc, ron120cp branches);

// Check if waypoint with given stamp+origin matches formula
b8 VERFormMatch(ron120cs form, ron60 time, ron60 origin);

// Encode ron120 to text: time-origin / time+origin / time=origin / origin
ok64 VERutf8Feed(u8s into, ron120cp v);

#endif  // LIBRDX_VER_H
