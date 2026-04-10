#  BELT — ersatz git-compatible repository format

BELT stores git objects in an append-only log with LSM-style indexes.
No attempt at bug-for-bug git compatibility; we read/write standard
packfiles over the wire but store locally in our own format.

##  Storage layout

```
.belt/
    objects.log         append-only packfile stream
    objects.idx/        LSM index directory (kv64 sorted runs)
        0000000001.idx
        0000000002.idx
        ...
    HEAD                current head commit hash (hex)
    refs/               named references
```

##  objects.log

Multiple packfiles concatenated into one append-only log file.
Each packfile segment has the standard format:

```
┌─ PACK hdr ─┐
│ "PACK" v2 N│   12 bytes: magic, version, object count
├─ objects ──┤
│ obj 0      │   varint(type+size) + zlib(content|delta)
│ obj 1      │
│ ...        │
│ obj N-1    │
├────────────┤
│ SHA-1 (20) │   checksum of this segment
└────────────┘
│            │   next segment follows immediately...
```

On clone we write the received packfile verbatim.  On fetch we
append the new packfile to the same log.  Objects are never moved
or rewritten; offsets are stable.

##  objects.idx — entry format

Each index entry is a `kv64` (16 bytes: u64 key + u64 val).
Low 4 bits of both key and val encode the entry type:

```
key = (hashlet << 4) | type
val = (gen << 44) | (offset << 4) | flags

type  key payload       val layout                    meaning
────  ───────────       ──────────                    ───────
0000  offset in log     —                             pack segment boundary
0001  hashlet (blob)    (0:20 | offset:40 | 0:4)      blob object location
0010  hashlet (tree)    (0:20 | offset:40 | 0:4)      tree object location
0011  hashlet (commit)  (gen:20 | offset:40 | 0:4)    commit object location
0100  hashlet (tag)     (0:20 | offset:40 | 0:4)      tag object location
```

A **hashlet** is the upper 60 bits of the object's SHA-1 (first
7.5 bytes).  The low 4 bits carry the object type, so entries sort
by hashlet first, type second.  This means lookups by hash naturally
cluster.

An **offset** in the val is the byte position of the object entry
within `objects.log` (the varint header, before zlib data).  The
type tag tells you it's a file offset, not a hash.

Type 0000 entries mark pack segment boundaries — the key payload
is the log offset where a new PACK header starts.  This lets us
enumerate segments without scanning.

A **generation number** is `1 + max(parent gens)` for commits (root
commits get gen=1).  Stored in the upper 20 bits of val (bits 44-63),
it enables O(depth-difference) graph operations: common ancestor,
reachability checks, and BFS early termination.  20 bits covers up
to ~1M generations.  Non-commit types always have gen=0.

##  Index management

Index files live in `objects.idx/` as numbered sorted runs of kv64
entries (same pattern as `spot/`'s trigram index):

-   **Write**: sort new entries, flush to `SEQNO.idx`
-   **Read**: mmap all runs, merge-iterate via `HITkv64`
-   **Compact**: merge youngest runs when LSM 1/8 invariant violated
-   **Lookup**: seek in the merged HIT to the target hashlet

The HIT (Heap of ITerators) machinery from `abc/HITx.h` provides
O(N log K) merge of K sorted runs and O(log N) seek within.

##  Object resolution

To read object `sha1`:

1.  Compute hashlet: `(sha1_trunc >> 4) << 4 | obj_type` — but we
    may not know the type.  So seek to `(sha1_trunc << 4)` and scan
    the 4-5 entries with matching hashlet.
2.  Index gives the log offset.  Read the pack object header there.
3.  If base type (commit/tree/blob/tag): inflate directly.
4.  If OFS_DELTA: base is at `offset - ofs_delta` in the same log.
5.  If REF_DELTA: look up the base by its hashlet in the index.
6.  Chase the delta chain, apply `DELTApply` bottom-up.

##  Operations

**Clone** (git-dl):
1.  Connect to git-upload-pack, negotiate want/done
2.  Receive packfile, append to `objects.log`
3.  Scan pack: for each object, compute SHA-1 (resolving deltas),
    emit kv64 entry `(hashlet|type, offset|0)`, sort, flush to idx

**Fetch**:
1.  Send have/want (our HEAD vs remote HEAD)
2.  Receive incremental packfile, append to `objects.log`
3.  Index new objects into a fresh idx run
4.  Compact if needed

**Push**:
1.  Negotiate common ancestor
2.  Enumerate reachable objects from our HEAD not reachable from common
3.  Build packfile from those objects (inflate from log, re-pack)
4.  Send via git-receive-pack

**Common ancestor** (for push/fetch negotiation):
1.  Walk commit parents from both heads via index lookups
2.  Each step: look up commit by hashlet, inflate, parse parents
3.  Meet point = common ancestor

##  Dependencies

-   `abc/KV.h` — kv64 type
-   `abc/HITx.h` — sorted-run merge iteration
-   `abc/FILE.h` — mmap, file I/O
-   `git/PACK.h` — packfile parsing
-   `git/DELT.h` — delta application
-   `git/ZINF.h` — zlib inflate/deflate
-   `git/SHA1.h` — SHA-1 hashing
-   `git/GIT.h` — commit/tree parsing

##  Design notes

-   No loose objects — everything goes through the log.
-   No repack/GC — the log only grows.  Compaction is index-only.
-   60-bit hashlets: collision probability negligible at <2^30 objects.
    On collision, verify full SHA-1 by inflating both candidates.
-   Delta objects in the log are never resolved to base form on disk.
    Resolution happens on read via `DELTApply`.
-   The val encodes generation number (upper 20 bits) for commits.
    Lower 4 bits (flags) reserved for future use.
