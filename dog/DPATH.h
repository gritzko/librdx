#ifndef DOG_DPATH_H
#define DOG_DPATH_H

#include "abc/INT.h"

con ok64 DPATHFAIL = 0xd64a7513ca495;
con ok64 DPATHBAD  = 0x35929d44b28d;

// Validate and drain one path segment from a tree entry name.
//
// A segment is a single filename (no slashes — tree objects
// handle directories via nesting).  The ragel grammar accepts
// valid UTF-8 bytes excluding NUL, '/' and '\\', and rejects
// dangerous names:
//
//   .         (current dir)
//   ..        (parent dir escape)
//   .git      (case-insensitive)
//   .dogs     (case-insensitive)
//
// Returns OK and advances input past the segment.
// Returns DPATHBAD on dangerous names or invalid bytes.
// Returns DPATHFAIL on empty input.
ok64 DPATHu8sDrainSeg(u8cs input, u8cs out);

// Verify a tree entry name (single segment, no slashes).
// Returns OK if valid, DPATHBAD/DPATHFAIL otherwise.
fun ok64 DPATHVerify(u8csc name) {
    if ($empty(name)) return DPATHFAIL;
    a_dup(u8c, tmp, name);
    u8cs seg = {};
    ok64 o = DPATHu8sDrainSeg(tmp, seg);
    if (o != OK) return o;
    if (!$empty(tmp)) return DPATHBAD;
    return OK;
}

#endif
