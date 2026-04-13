#  keeper — local git object store

Keeper stores git objects in append-only pack logs with LSM-style
indexes. Packs are git-compatible and trivially exchangeable with
git and git-compatible systems.

Objects are addressed by 60-bit hashlets (15 hex chars of SHA-1
prefix); variable-length prefixes from 4 to 40 chars work for
lookups, matching git's short-hash convention.

Keeper is one of the git dogs and it follows DOG.md conventions,
and integrates with sniff (file tracking), graf (commit graph), 
and spot (code search) through whiff/URI conventions.
The URI convention lets you address remotes, refs, and
objects uniformly: `//host/path` to sync, `?refname` to resolve a
ref, `#hashprefix` to cat an object.

##  Usage

```sh
# clone a repo (fetch all refs)
keeper //localhost/home/user/src/linux

# clone via alias
keeper --alias linux //localhost/home/user/src/linux
keeper //linux

# fetch a specific ref
keeper //linux?refs/tags/v6.0

# resolve a ref to SHA
keeper '?refs/heads/master'

# cat an object by hash prefix (7 chars = git default)
keeper '#abc1234'

# list known refs
keeper --refs

# import an existing git packfile
keeper -i path/to/pack.pack

# show store stats
keeper -s

# verify a commit and all reachable objects
keeper --verify abc123def456789...

# incremental pack writer (C API)
keep_pack p = {};
KEEPPackOpen(&k, &p);
KEEPPackFeed(&k, &p, KEEP_OBJ_BLOB, content, sha_out);  // SHA returned
// use sha_out to build tree entries...
KEEPPackFeed(&k, &p, KEEP_OBJ_TREE, tree_content, tree_sha);
KEEPPackClose(&k, &p);
```

##  Storage layout

```
.dogs/keeper/
    log/                    append-only pack logs (FILEBook'd)
        0000000001.pack
        0000000002.pack
    idx/                    LSM index (sorted kv64 runs)
        0000000001.idx
        0000000002.idx
    REFS                    URI->URI reflog (append-only text)
```

##  Pack log files

Each log file is a valid git packfile: PACK header, objects, trailing
SHA-1.  On clone/fetch the received pack is saved verbatim.  On local
writes (`KEEPPackFeed`), objects are deflated and appended.  Normally
we append to the highest-numbered log; a new file starts on explicit
request.  Objects are never moved or rewritten; offsets are stable.

```
PACK v2 N       12 bytes: magic, version, object count
obj 0           varint(type+size) + zlib(content|delta)
obj 1
...
obj N-1
SHA-1           20 bytes: checksum of entire pack
```

##  Index entries (kv64)

Each index entry is 16 bytes: u64 key + u64 val.

```
key = hashlet60[60] | obj_type[4]
val = offset[40] | file_id[20] | flags[4]

obj_type   meaning
────────   ───────
0001       commit
0010       tree
0011       blob
0100       tag
```

A **hashlet** is the first 60 bits of the SHA-1 in big-endian order
(first byte on top).  The low 4 bits carry the git object type, so
entries sort by hashlet first, type second.  Lookups by hash prefix
span all types via range query.

The **val** uses the wh64 layout: `offset[40]` is the byte position
within the log file, `file_id[20]` identifies which log file,
`flags[4]` reserved.

##  Index management

Index files in `idx/` are numbered sorted runs of kv64 entries:

-   **Write**: sort new entries, flush to `SEQNO.idx`
-   **Read**: mmap all runs, binary search each
-   **Compact**: merge runs when LSM invariant violated
-   **Lookup**: range query `[hashlet_prefix << 4, hashlet_prefix << 4 | 0xf]`

##  Object resolution

1.  Compute 60-bit hashlet from hex prefix (zero-pad short prefixes).
2.  Range query the LSM index; first match gives `(file_id, offset)`.
3.  Read pack object header at that offset in the log file.
4.  Base type (commit/tree/blob/tag): inflate directly.
5.  OFS_DELTA: base is at `offset - delta` in the same log.
6.  REF_DELTA: look up base by hashlet in the index.
7.  Chase delta chain, apply `DELTApply` bottom-up.

##  REFS

The `REFS` file is an append-only URI-to-URI mapping:

```
<ron60-timestamp>\t<from-uri>\t<to-uri>\n
```

Aliases: `//github` -> `https://github.com/torvalds/linux.git`
Refs: `?refs/heads/master` -> `?<hex-sha>`

Resolved iteratively.  Compacted by keeping latest per key.

##  Dependencies

-   `abc/KV.h` — kv64 type and sorted-run operations
-   `abc/FILE.h` — FILEBook (growable mmap), file I/O
-   `abc/URI.h` — URI parsing for CLI
-   `abc/RON.h` — ron60 timestamps, base64 sequence numbers
-   `keeper/PACK.h` — packfile header/object parsing
-   `keeper/DELT.h` — delta apply (and encode)
-   `keeper/ZINF.h` — zlib inflate/deflate
-   `keeper/SHA1.h` — SHA-1 hashing
-   `keeper/GIT.h` — commit/tree/blob parsing
-   `keeper/REFS.h` — URI reflog
-   `dog/WHIFF.h` — wh64 tagged-word packing (val format)
