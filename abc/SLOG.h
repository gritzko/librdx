#ifndef ABC_SLOG_H
#define ABC_SLOG_H

#include "abc/B.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/S.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

// Error codes
con ok64 SLOGNOROOM = 0x7156105d86d8616;
con ok64 SLOGBAD = 0x1c55840b28d;
con ok64 SLOGMISS = 0x71561059271c;
con ok64 SLOGEOF = 0x1c55840e60f;
con ok64 SLOGNONE = 0x7156105d85ce;

#define SLOG_K 'k'  // intermediate skip record (written to stream)
#define SLOG_C 'c'  // close record (written to stream)

// Normalized versions (after TLVu8sDrain removes TLVaA bit)
#define SLOG_K_N 'K'
#define SLOG_C_N 'C'

#define SLOG_G 7  // block granularity: 128 bytes

// Statistics counters
enum {
    SLOG_STAT_POPS,    // stack pops (seek jumps)
    SLOG_STAT_KEXP,    // 'k' records expanded
    SLOG_STAT_KSKIP,   // 'k' records skipped
    SLOG_STAT_CMP,     // data record comparisons
    SLOG_STAT_KWRITE,  // 'k' records written
    SLOG_STAT_COUNT
};

extern u64 SLOG_STATS[SLOG_STAT_COUNT];

fun void SLOGStatsReset(void) {
    for (int i = 0; i < SLOG_STAT_COUNT; ++i) SLOG_STATS[i] = 0;
}

// Block number for offset (offset 0 is block 0, then 128-byte blocks)
fun u64 SLOGBlock(u64 off) { return (off + 0x7f) >> SLOG_G; }

// Rank = trailing zeros of block number (higher = rarer)
fun u8 SLOGRank(u64 off) {
    u64 blk = SLOGBlock(off);
    return blk == 0 ? 64 : ctz64(blk);
}

// --- Write Path ---

// Puts zero on the stack (marks stream start)
ok64 SLOGCreate(u64gp stack, u8gp data);

// Flush a 'k' record if crossing high-rank block boundary
ok64 SLOGFeed(u64gp stack, u8gp data);

// Put the current write offset on the stack (higher on top)
// May flush a 'k' record before that (based on rank calc)
ok64 SLOGMark(u64gp stack, u8gp data);

// Sample record at current write offset u8gLeftLen()
// May mark the offset if block has changed
ok64 SLOGSample(u64gp stack, u8gp data);

// Close stream: flush remaining stack as 'c' record
// Last byte of 'c' = total length of 'c' record
ok64 SLOGClose(u64gp stack, u8gp data);

// --- Read Path ---

// Load skip list from close record at stream end
// Returns stack populated with offsets (lower on top)
ok64 SLOGOpen(u64gp stack, u8csc stream);

// Seek to equal-or-greater entry using less-comparator
// On return, top of stack is offset of target (or greater) entry
ok64 SLOGSeek(u64gp stack, u8csc stream, u8zs less, u8csc target);

// --- Skip record encoding (delta + ZINT compressed) ---

// Write offsets as delta-compressed ZINT TLV record
// data = gauge [container_start, write_pos, buf_end]
// If trailen is true, appends reclen byte (for close records)
// If count > 64, excess entries are flushed as 'k' records
ok64 SLOGu8sFeedSkips(u8gp data, u8 lit, u64csc offs, b8 trailen);

// Read delta-compressed ZINT offsets into gauge
ok64 SLOGu8bDrainSkips(u64gp into, u8csc from);

#endif  // ABC_SLOG_H
