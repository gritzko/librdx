#   SLOG: One-Level Skip Log

A skip log is a stream of TLV records interleaved with backward-pointing
skip entries. This enables logarithmic search in an append-only log without
a separate index. Seek is definitely slower than B-trees allow as a skip log 
can only do binary search. Overheads and complexity are much lower though.
Does not copy the keys, makes no assumptions about ordering.
Intended for use in mmapped streams (no paging).

##  Rationale

Consider a stream of timestamped log records. To find a record by timestamp,
one could scan from the start (O(n)) or maintain a separate index. SLOG
embeds the index directly into the stream: skip entries point backward to
earlier positions, enabling binary-search-like navigation in O(log n) hops.

The overhead is minimal: offsets are marked only when crossing 128-byte
block boundaries, and those marks get flushed periodically into skip records
to keep the stack small (logN sized). In the end of the stream, the final
skip record is appended with all remaining marked offsets. Due to the
logarithmical flushing pattern, the final record has enough offsets to 
start binary search.

##  Record Types

Two TLV record types:

    'k' - intermediate skip record (flushed during writes)
    'i' - close record (final flush, last byte = record length)

Skip record body contains raw u64 offsets (relative to DATA start).
Offsets are stored in order on a stack; flushing pops lower-rank
entries into a 'k' record.

##  Rank Calculation

Block number and rank for an offset:

    block(off) = (off + 0x7f) >> 7      // 128-byte blocks, rounded up
    rank(off)  = ctz(block(off))        // trailing zeros = rank

Offset 0 has block=0, which has infinite rank (never flushed until close).
Higher rank = rarer, covers more ground. A skip entry at offset P can
only contain entries with rank < rank(P).

##  API

```c
// Error codes
con ok64 SLOGNOROOM = 0x7156105d86d8616;
con ok64 SLOGBAD    = 0x1c55840b28d;
con ok64 SLOGMISS   = 0x71561059271c;
con ok64 SLOGEOF    = 0x1c55840e60f;
con ok64 SLOGNONE   = 0x7156105d85ce;

#define SLOG_K 'k'   // intermediate skip record
#define SLOG_I 'i'   // close record

// State is a u64 gauge (stack of offsets), typically from u64bDataIdle()
// Write: stack grows as records are sampled, shrinks on flush
// Read:  stack loaded from close record, shrinks during seek

// --- Write Path ---

// Puts zero on the stack
ok64 SLOGCreate(u64gp stack, u8bp buf);

// Flush a 'k' record if crossing high-rank block boundary.
ok64 SLOGFeed(u64gp stack, u8bp buf);

// Put the current write offset on the stack (higher on the top)
// May flush a 'k' record before that (based on rank calc)
ok64 SLOGMark(u64gp stack, u8bp buf);

// Sample record at the current write offset u8bDataLen()
// May mark the offset if a block has changed
ok64 SLOGSample(u64gp stack, u8bp buf);

// Close stream: flush remaining stack as 'i' record.
// Last byte of 'i' = total length of 'i' record (max 255).
ok64 SLOGClose(u64gp stack, u8bp buf);

// --- Read Path ---

// Load skip list from close record at stream end.
// Returns stack populated with offsets (lower on the top)
ok64 SLOGOpen(u64gp stack, u8csc stream);

// Seek to equal-or-greater marked entry. Uses the stack to navigate
// in a logarithmical number of steps. The offset of the target
// (or greater marked entry) is on the top of the stack on return.
ok64 SLOGSeek(u64gp stack, u8csc stream, $cmpfn cmp, u8csc target);
```

##  Write Action Sequence

1. Allocate stack: `a_pad(u64, mem, 64); u64g stack = {mem, mem, a_end(mem)};`
2. Call `SLOGCreate(stack, buf)` - pushes offset 0 onto stack
3. Write TLV records to buf, call `SLOGSample(stack, buf)` after each:
   - Internally, let `off = u8bDataLen(buf)`:
     a. Compute `r = rank(off)`
     b. If `r <= rank(top)`: do nothing (same or lower rank block)
     c. Else while stack has entries with `rank < r`: pop them
     d. Write popped offsets as 'k' TLV record
     e. Push `off` onto stack
4. After last record, call `SLOGClose(stack, buf)`
   - Flushes all stack entries as 'i' record
   - Appends record length as final byte

Example write sequence (G=7, blocks are 128 bytes):
```
SLOGCreate         -> stack: [0]
write rec, Sample  -> off=0x50, block=0, rank=inf, same block, no-op
                      stack: [0]
write rec, Sample  -> off=0x90, block=1, rank=0, new block, push
                      stack: [0, 0x90]
write rec, Sample  -> off=0x180, block=3, rank=0, push
                      stack: [0, 0x90, 0x180]
write rec, Sample  -> off=0x200, block=4, rank=2 > 0, flush [0x90, 0x180] as 'k'
                      stack: [0, 0x1F0] (0x1F0 = 'k' record offset)
write rec, Sample  -> off=0x280, block=5, rank=0, push
                      stack: [0, 0x1F0, 0x280]
SLOGClose          -> flush all as 'i': [0, 0x1F0, 0x280, reclen]
```

##  Read Action Sequence

1. Allocate stack, call `SLOGOpen(stack, stream)`:
   - Reads last byte to get 'i' record length
   - Parses 'i' record, pushes offsets in reverse (lower on top)
   - Stack now has: `[highest_off, ..., 0]` (0 on top)
2. To seek for `target` using comparator `cmp`:
   - Call `SLOGSeek(stack, stream, cmp, target)`
   - Internally:
     a. Pop top offset, read record at that position
     b. Compare record with target using `cmp`
     c. If record < target: continue popping (skip past it)
     d. If record >= target: found, leave offset on top, return
     e. If stack has 'k' record offset: parse it, push its offsets
   - On return, top of stack is offset of equal-or-greater record
3. Repeat `SLOGSeek` to continue iteration from current position

##  Close Record Format

The 'i' close record:
```
[lit='i'][len][off0][off1]...[offN][reclen]
          ^    <-- offsets as u64 -->  ^
          |                            +-- 1 byte: total TLV record length
          +-- TLV length field (1 or 4 bytes)
```

The final byte `reclen` allows parsing from stream end: read last byte,
back up `reclen` bytes, parse TLV header, extract offsets. Offsets are
stored in stack order (lower ranks first, offset 0 last).

##  Marking Rate Analysis

Marking is deterministic and position-based: a record gets marked when it
crosses into a new 128-byte block compared to the previous marked position.
This differs from probabilistic skip lists (coin-flip promotion).

The marking rate depends on record size:

| Record size | Records/block | Marking rate | Notes                    |
|-------------|---------------|--------------|--------------------------|
| ~10 bytes   | ~12           | ~8%          | Many unmarked records    |
| ~50 bytes   | ~2.5          | ~40%         | Moderate clustering      |
| ~128 bytes  | ~1            | ~100%        | Nearly every record      |
| >128 bytes  | <1            | 100%         | Every record marked      |

### Small Records Stream

For streams with small records (e.g., 10-byte TLVs):
- Only ~1 in 12 records gets marked
- Multiple records cluster within a single 128-byte block
- Seek uses O(log M) hops where M = marked count, not total count
- Final hop requires **linear scan** through unmarked records in the last block
- Empirical: SLOG4 test with 1M 12-byte records shows avg 16 pops for seek,
  implying ~65K marked positions (~6.5% marking rate)

The linear scan fallback is essential: when all stack entries point to records
less than target, unmarked records between them must be scanned sequentially.
Worst case adds ~12 comparisons for 10-byte records (one block worth).

### Large Records Stream

For streams with large records (e.g., 200+ byte TLVs):
- Every record crosses a block boundary, so 100% get marked
- No linear scan needed; pure logarithmic seek
- Higher index overhead: more 'k' records flushed, larger 'i' record
- Stack may need more slots (still bounded by log₂(stream_size/128))

### Space Overhead

The index overhead is bounded:
- Stack slots: O(log₂(N)) where N = stream size in blocks
- 'k' record frequency: one per rank-increase event
- 'i' record size: stack_depth × 8 bytes + 3 bytes header

For a 1GB stream with 10-byte records (~100M records, ~8M blocks):
- ~23 stack slots maximum (log₂(8M))
- 'i' record: ~186 bytes
- Total 'k' records: O(blocks) but amortized over stream

##  TODO

- [ ] Delta + blocked varint encoding for offsets
- [ ] Streaming read (load 'k' records on demand)
- [ ] Multiple metrics (e.g., timestamp + sequence number)
