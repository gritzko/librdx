#  keeper — git object store + compat layer

Parsers for git wire protocol (pkt-line, packfile) and git objects
(blob, tree, commit), plus keeper's append-only pack log and LSM
index.  Uses zlib for pack decompression and OpenSSL for SHA-1
object IDs.

Store layout is **sharded by branch directory** (see `README.md`
§"Storage layout" and `LOG.md`): each branch dir holds its own
`NNNNN.keeper` + `NNNNN.idx` files plus `REFS` and optional `WT`.
`file_id`s are store-global sequential.  Object resolution walks
the dir chain child → parent → root; REF_DELTA bases are constrained
to the same dir or an ancestor.

##  Headers

### GIT.h — git object parsers

Types: none (output via slices).

  - `GITu8sDrainTree`    drain one tree entry (mode+name, 20-byte SHA1)
  - `GITu8sDrainCommit`  drain one commit header; empty field = body
  - `GITu8sCommitTree`   extract the tree SHA-1 from a commit body

### PKT.h — pkt-line framing

  - `PKTu8sDrain`      drain one pkt-line; returns PKTFLUSH/PKTDELIM for specials
  - `PKTu8sFeed`       feed one pkt-line (4-hex prefix + payload)
  - `PKTu8sFeedFlush`  feed a flush packet (0000)

### PACK.h — packfile parser

Types: `pack_hdr` (version, count), `pack_obj` (type, size, delta ref).

Object types: COMMIT=1, TREE=2, BLOB=3, TAG=4, OFS_DELTA=6, REF_DELTA=7.

  - `PACKDrainHdr`     parse PACK magic + version + count
  - `PACKDrainObjHdr`  parse object type/size varint + delta base
  - `PACKInflate`      zlib-inflate compressed object data

### IGNO.h — .gitignore parser/matcher

Types: `igno_pat` (pattern + flags), `igno` (up to 256 patterns).

  - `IGNOLoad`   load .gitignore from directory
  - `IGNOFree`   free resources
  - `IGNOMatch`  check if relative path should be ignored

### SHA1.h — SHA-1 hash (OpenSSL wrapper)

  - `SHA1Sum`  compute 20-byte SHA-1 (isolated from ABC types)

### ZINF.h — zlib inflate/deflate wrapper

  - `ZINFInflate(u8s into, u8cs zipped)`  decompress zlib data
  - `ZINFDeflate(u8s into, u8cs plain)`  compress data


##  Implementation files

  - `GIT.c`     tree/commit drain parsers (~75 lines)
  - `PKT.c`     pkt-line framing (~77 lines)
  - `PACK.c`    packfile header/object/inflate (~101 lines)
  - `IGNO.c`    gitignore glob matching (~233 lines)
  - `SHA1.c`    SHA-1 via OpenSSL EVP (~24 lines)
  - `ZINF.c`    zlib inflate/deflate (~63 lines)
  - `WALK.c`    KEEP-backed tree walker (eager + lazy)

### KEEP.h — branch-aware Open + per-shard state

`KEEPOpenBranch(home *h, u8cs branch, b8 rw)` registers `branch` on
the home singleton via `HOMEOpenBranch` and delegates to `KEEPOpen`.
Phase 0 accepts only the trunk (canonical branch = empty slice); a
non-trunk branch returns `KEEPNOBR`.

Per-shard state is extracted into `keeper_shard`: `branch`,
`lock_fd`, `packs[KEEP_MAX_FILES]`, `npacks`, `runs[KEEP_MAX_LEVELS]`,
`run_maps[]`, `nruns`.  The singleton `keeper` carries
`shards[KEEP_MAX_SHARDS]` + `nshards`, the home pointer, the paths
registry, and scratch buffers.  Phase 1a opens exactly one trunk
shard.

Phase 1b flattens the on-disk layout and drops the `keeper/` subdir
inside `.dogs/`: trunk pack logs live at `.dogs/NNNNN.keeper` and
index runs at `.dogs/NNNNN.idx`, where `NNNNN` is the 5-hex-char
wh64 `file_id` (20 bits).  `KEEP_PACK_EXT` = `.keeper`,
`KEEP_SEQNO_W` = 5, `KEEP_DIR` = `.dogs`.

Phase 1c adds the DAG-invariant scaffolding and the drop-a-dir
surface:

  * `keep_pack` gains `shard_idx` (always 0 in Phase 1c) — tags the
    write shard on every emitted pack.
  * `KEEPPackFeed`'s REF_DELTA path consults `keep_shard_visible`
    before emitting; a base that isn't in the write shard or one of
    its ancestors silently falls through to raw encoding.  nshards=1
    keeps the check trivially passing until Phase 2 opens siblings.
  * `KEEPBranchDrop(keeper *k, u8cs branch)` enforces the top-level
    preconditions: trunk aliases (`""`, `main`, `master`, `trunk`,
    plus their `heads/` forms) return `KEEPTRUNK`; unknown branches
    return `KEEPNOBR`.  The full "refuse on live descendant /
    live staging pack / live WT" semantics (`KEEPDIRTY`) will
    light up once Phase 2 can actually open a sibling to drop.

### WALK.h — git object graph traversal

Types: `walk` (walker state), `walk_fn` (visitor callback).

  - `WALKOpen`         open walker on a branch dir (mmaps that dir's
                       logs + index runs; ancestor dirs resolved lazily
                       via the keeper-wide lookup path)
  - `WALKClose`        close walker, unmap everything
  - `WALKGet`          get object by hashlet
  - `WALKGetSha`       get object by raw 20-byte SHA-1
  - `WALKTree`         DFS tree walk over KEEP — eager (blobs resolved), path-aware visitor
  - `WALKTreeLazy`     DFS tree walk over KEEP — lazy (blobs empty, pulled on demand)
  - `WALKu8sModeKind`  classify git tree-entry mode → `WALK_KIND_*`
  - Commit-graph traversal lives in `graf/`, not here.

### DELT.h — git delta instruction applier + encoder

  - `DELTApply`   apply delta instructions (copy/insert) to base object
  - `DELTEncode`  produce a git delta instruction stream for
                  (base, target).  4-byte hash index over `base` with
                  forward + bounded-backward extension.  Returns
                  DELTFAIL when the delta is no smaller than the raw
                  target (caller should emit raw instead).
                  Exercised end-to-end via `test/DELTA_ROUND.c`:
                  feeds a chain of blob versions with a hashlet60
                  hint to `KEEPPackFeed`, splices the log into a git
                  packfile, reads each version back via `git cat-file`.
                  `KEEPPackFeed` emits OFS_DELTA when the base is a
                  raw object in the same in-progress pack, else
                  REF_DELTA against whatever `KEEPGet` resolves from
                  committed runs (delta chains chased transparently).

##  CLI

  - `git-dl.cli.c`  download repo via git-upload-pack, save packfile
                     + kv64 hash index (truncated SHA-1 → offset).
                     Resolves OFS_DELTA and REF_DELTA chains.

##  Build

Library `gitcompat` (static): GIT.c PKT.c PACK.c DELT.c ZINF.c SHA1.c IGNO.c.
Library `keeplib` (static): KEEP.c KEEP.exe.c REFS.c WALK.c.
Links: abc, ZLIB, OpenSSL::Crypto.

##  Tests

  - `test/GIT.c`    tree/commit parser tests (6 cases)
  - `test/PKT.c`    pkt-line drain/feed tests (8 cases)
  - `test/PACK.c`   packfile header/varint/inflate tests (7 cases)
  - `test/DELT.c`   DELTEncode + DELTApply round-trip
  - `test/DELTA_ROUND.c`  KEEPPackFeed with delta hints → valid git
                           packfile → `git cat-file` per version
  - `test/IGNO.c`   gitignore pattern matching tests (3 cases)
  - `test/ZINF.c`   deflate/inflate round-trip chain (20 versions)
  - `test/FETCH.c`  treadmill: clone repo via ssh git-upload-pack,
                     unpack packfile, write loose objects, verify with git
  - `test/WALK.c`   WALKu8sModeKind table + WALKTree/WALKTreeLazy on synthetic KEEP
  - `test/ROUND.c`  full round-trip: create bare repo, clone via ssh,
                     edit+commit, push back, verify files match
  - `test/POST.c`   `keeper post ssh://…` — synthesize a commit and
                     push it via git-receive-pack; verify remote HEAD
