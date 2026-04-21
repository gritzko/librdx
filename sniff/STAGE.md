# keeper staging logs

Staging logs are per-branch, per-worktree scratch space for object
bytes that have been **staged but not committed**.  They live under
sniff's private (non-symlinked) dir:

    .dogs/sniff/<branch-path>/pack      # stripped-framed packs
    .dogs/sniff/<branch-path>/idx       # wh128 bookmarks + object entries

`<branch-path>` is the ref path minus the leading `refs/`, e.g. the
branch `refs/heads/main` maps to `.dogs/sniff/heads/main/`.  The
directory is created lazily on the first `be put`/`be delete` against
that branch and unlinked on `be post`.  Only `refs/heads/*` may host
staging; `tags/*` and remote-attributed refs are rejected at the API.

## Why separate from keeper's main log

`.dogs/keeper/` is shared across worktrees (symlinked) and is the
source of truth replicated by SYNC (see `SYNC.md`).  Staged objects
are strictly local scratch — they have no commit pointing at them,
they are meaningless to peers, and they will be *replaced* by a fresh
canonical pack on `be post`.  Putting them in a separate store keeps
keeper's replicated stream pure and lets any worktree stage
concurrently without cross-worktree interference.

## Pack format inside a staging log

A staging pack follows the same stripped-framing rules as a main-log
pack (see `LOG.md` §"Stripped git pack framing"): concatenated object
records, no PACK header, no trailing SHA-1.  Packs are appended
back-to-back in one file.

Differences from main-log packs:

  * **No commit objects.**  Each staging pack holds trees + blobs
    only.  The first object of a staging pack is a tree.
  * **Intra-pack type order still holds**: trees before blobs.
  * **Cross-pack references MUST use REF_DELTA (hash), never
    OFS_DELTA (offset).**  A staging log is repacked/discarded on
    post; byte offsets to neighbouring packs are not stable.  Within
    one pack, OFS_DELTA remains fine.
  * Delta bases may resolve into (a) the same staging pack
    (OFS_DELTA), (b) an earlier staging pack in the same staging log
    (REF_DELTA → staging idx), or (c) keeper's main log (REF_DELTA →
    main idx).  Resolvers try staging idx first, then main.

## Staging idx

Reuses keeper's on-disk index schema — wh128 pack bookmarks plus
per-object entries (see `LOG.md` §"Pack bookmarks").  Stored in a
separate file so that `be post` can drop it atomically.  `file_id`
in staging bookmarks is local to the staging log (sequence within
the branch dir) and is disjoint from main-log file_ids; callers
distinguish by which idx answered the lookup.

The idx answers the "is this blob/tree already in the staging log"
question in O(log N) during `be put` — cheaper than scanning packs.

## Lifecycle

**`be put <file>` / `be delete <file>`** (driven by `sniff/PUT.c`,
`sniff/DEL.c`):

  1. Compute the new blobs and rebuild the affected subtrees.
  2. Open the branch staging log (create dir if missing).
  3. Append one pack (tree objects, then blob objects) and emit its
     bookmark + object entries into the staging idx.
  4. Update sniff's root `SNIFF_TREE` hashlet to the new tree.

**`be post`** (driven by `sniff/POST.c`):

  1. Read sniff's root tree hashlet (the *current* staged tree).
  2. Walk reachable trees + blobs via the staging idx.
  3. Build a fresh canonical pack: new commit object (with parent
     from `sniff/at.log` tail), then trees in order, then blobs in
     order.  No intermediate/obsolete staged objects are copied.
  4. Feed the pack into keeper's main log via the same
     `KEEPPackFeed` path remote packs use — full checks (including
     the re-enabled `ORDERBAD` type-order invariant) and indexing.
  5. On success, unlink the staging pack + idx, rmdir the branch
     dir, and append the new commit sha to `sniff/at.log`.

**Unstage** = sniff rewrites its root `SNIFF_TREE` hashlet to exclude
the path.  Dead objects in the staging log remain; they are purged
when the next `be post` discards the whole dir.

**Branch switch** (`be get ?<other>`) does *not* touch the current
branch's staging dir.  Switching back resumes staging where it was.

## Crash safety

The staging log is append-only; a torn append is detected by idx
<-> pack mismatch on next open and truncated.  On `be post`:

  1. Repacked canonical pack is appended + fsync'd to keeper's tail
     log file.
  2. Main-log bookmarks + object entries are emitted.
  3. Staging pack file is unlinked; its idx is unlinked; the branch
     dir is rmdir'd.
  4. `sniff/at.log` line appended + fsync'd.

A crash between (2) and (4) leaves a duplicate commit in the main
log (harmless — dedup by hash) and a still-live staging dir
(harmless — next `be post` re-repacks the same tree).  A crash
between (1) and (2) leaves an object-bytes-only tail in the main log
with no index entries — recoverable by re-indexing the last pack on
keeper open.

## Replication

Staging logs never participate in SYNC.  `keeper/SYNC.c` sees only
`.dogs/keeper/`.

## Concurrency

Single-writer per branch dir.  Concurrent `be put` on the same
branch from the same worktree must serialise on a `.dogs/sniff/<branch-path>/lock`
file (flock).  Different branches are independent.  Different
worktrees have separate sniff dirs and cannot collide.

## Current code vs. this spec

As of this writing, `sniff/PUT.c` feeds blobs and trees directly
into `keeper/KEEP.c`'s main-log `KEEPPackFeed`, which is why
`LOG.md` §"Intra-pack object order" says the `ORDERBAD` check is
disabled (blobs can precede trees in a mid-staging pack).  Routing
staging through this separate log is the prerequisite for turning
`ORDERBAD` back on in the main-log path.
