#ifndef KEEPER_PSTR_H
#define KEEPER_PSTR_H

//  PSTR: streaming pack-stitcher.
//
//  Builds a single valid git packfile from an ordered list of
//  byte-segments residing inside one or more open keeper log files.
//  Per `keeper/LOG.md`, on-disk packs in keeper log files are stored
//  *stripped* (no PACK header, no SHA-1 trailer); the bytes between
//  the header and the trailer of every original pack are appended
//  verbatim into the dir's log file.  PSTR re-frames an ordered
//  series of those stripped pack bodies into one wire-shaped git
//  packfile:
//
//    PACK | ver=2 | sum_of_counts                 (12 bytes header)
//    seg_0_bytes seg_1_bytes ... seg_{n-1}_bytes  (concatenated)
//    sha1_over_all_of_the_above                   (20 bytes trailer)
//
//  The encoder does no object scanning, no inflation, no varint
//  fiddling — the per-segment object count is supplied by the caller
//  (read out of the pack bookmark val by `keepPackBmCount`).  Bytes
//  flow segment_fd --pread--> scratch buf --SHA1Feed--> SHA1state +
//  --write--> out_fd.  No splice/sendfile (we have to hash inline).
//
//  Sums of `count` overflow into 64 bits internally; if the total
//  does not fit u32 the whole call returns PSTRFAIL.
//
//  See `keeper/WIRE.md` Phase 2 for the surrounding plan.

#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/S.h"

con ok64 PSTRFAIL = 0x65c75b3ca495;

typedef struct {
    int  fd;       // open keeper log file (read by pread on this fd)
    u64  offset;   // segment start (byte offset within fd)
    u64  length;   // segment byte length
    u32  count;    // object count contained in segment
} pstr_seg;

typedef pstr_seg const  *pstr_segcp;
typedef pstr_segcp pstr_segcs[2];   // const slice of segments [head, term)

//  Stitch `segs` into one valid git packfile written to `out_fd`.
//
//    1. Sum `segs[i].count` into a u32 (caller-checked overflow ->
//       PSTRFAIL).
//    2. Write a 12-byte git PACK header (magic + version=2 +
//       count_be32) to out_fd, feed the same bytes through SHA1.
//    3. For each segment: pread `length` bytes from `fd` starting at
//       `offset` in chunks of PSTR_BUF (64 KiB), feeding each chunk
//       through SHA1 and writing it to out_fd.
//    4. Append the 20-byte SHA1 digest to out_fd.
//
//  Caller owns every `segs[i].fd` and `out_fd`; PSTR does not close
//  or seek.  Empty `segs` is valid and produces a 32-byte packfile
//  (header + trailer, count = 0).
ok64 PSTRWrite(int out_fd, pstr_segcs segs);

#endif
