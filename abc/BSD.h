// BSD.h — in-memory bsdiff/bspatch (no compression, no malloc)
//
// Based on bsdiff by Colin Percival and libbdiff by William Woodruff.
// Copyright 2003-2005 Colin Percival
// Copyright 2014 William Woodruff
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted providing that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef ABC_BSD_H
#define ABC_BSD_H

#include "INT.h"

con ok64 BSDbadmagic = 0x2e69cb3f49a6d0;
con ok64 BSDcorrupt = 0x2e69ca31d38b8;
con ok64 BSDnoroom = 0x2e69cd8d8616;

// Workspace size in i64 elements needed for BSDDiff.
// Caller must provide 2*(oldlen+1) i64 values.
fun u64 BSDWorkLen(u64 oldlen) { return 2 * (oldlen + 1); }

// Workspace size in bytes.
fun u64 BSDWorkSize(u64 oldlen) { return BSDWorkLen(oldlen) * sizeof(i64); }

// Read newsize from patch header. Returns 0 on invalid patch.
fun u64 BSDPatchNewSize(u8csc patch) {
    if ($len(patch) < 24) return 0;
    u8cp p = patch[0];
    if (memcmp(p, "BSDIFF01", 8) != 0) return 0;
    i64 sz = 0;
    memcpy(&sz, p + 8, 8);
    return (u64)sz;
}

// Compute binary diff: old -> neu, write patch into output slice.
// work: i64 slice of at least BSDWorkLen($len(old)) elements.
ok64 BSDDiff(u8s patch, u8csc old, u8csc neu, i64s work);

// Apply binary patch: old + patch -> neu.
// neu must have BSDPatchNewSize(patch) bytes available.
ok64 BSDPatch(u8s neu, u8csc old, u8csc patch);

#endif  // ABC_BSD_H
