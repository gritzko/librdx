# sniff staging packs

Staging packs are per-branch scratch space for object bytes that
have been **staged but not committed**.  They live **inside the
branch directory itself**, alongside the branch's canonical pack
logs:

    <store>/<branch-dir>/
        NNNNN.keeper          canonical pack logs (append-only)
        NNNNN.idx             canonical LSM index runs
        REFS                  branch reflog
        WT                    abspath of the wt on this branch
        stage.sniff           staging pack (tree + blob only)
        stage.idx             staging index (pack bookmarks + objects)
        stage.lock            flock for single-writer guarantee

`<branch-dir>` is the path-like branch name (e.g. `heads/feature/`,
`feature/fix1/`, or trunk = `<store>/` itself).  Staging files are
created lazily on the first `be put` / `be delete` and unlinked on
`be post`.  Only branches that can host a worktree may stage; tag
dirs and remote-only branches are rejected at the API.

Because each branch dir holds at most one worktree
(see `sniff/AT.md`), staging state is naturally per-branch,
per-worktree — no extra namespacing needed.

## Why a separate pack from the branch's canonical log

Staged objects are strictly local scratch — no commit points at
them yet, peers can't use them, and they will be **replaced** by a
fresh canonical pack on `be post`.  Keeping them in `stage.sniff`
rather than appending to the tail `NNNNN.keeper` means:

  * Canonical packs never contain dead staging objects.
  * `be post` promotes the committed tree into a fresh canonical
    pack and deletes the staging files atomically — no holes in the
    main log.
  * Dog-to-dog sync (`keeper/WIRE.md`) ships canonical packs only;
    `stage.*` files are skipped by filename.

## Pack format inside a staging pack

A staging pack follows the same stripped-framing rules as a
canonical pack (see `keeper/LOG.md` §"Stripped git pack framing"):
concatenated object records, no PACK header, no trailing SHA-1.
Packs are appended back-to-back in one file.

Differences from canonical packs:

  * **No commit objects.**  Each staging pack holds trees + blobs
    only.  The first object of a staging pack is a tree.
  * **Intra-pack type order still holds**: trees before blobs.
  * **Cross-pack references MUST use REF_DELTA (hash), never
    OFS_DELTA (offset).**  Staging is repacked/discarded on post;
    byte offsets to neighbouring packs are not stable.  Within one
    pack, OFS_DELTA remains fine.
  * REF_DELTA bases follow the same **delta-dependency DAG** as
    canonical packs (see `keeper/LOG.md` §"Delta-dependency DAG"):
    the base must live in this staging pack, in this branch's
    canonical log, or in an **ancestor** dir's canonical log.
    Never in a sibling or descendant — materialize the base raw
    instead.

## Staging index

Reuses keeper's on-disk index schema — wh128 pack bookmarks plus
per-object entries (see `keeper/LOG.md` §"Pack bookmarks").  Stored
in `stage.idx` so `be post` can drop it atomically.  `file_id` in
staging bookmarks is a reserved local constant (not drawn from the
store-wide canonical sequence) — callers distinguish "is this the
staging idx or a canonical idx" by which file answered the lookup.

The idx answers "is this blob/tree already staged" in O(log N)
during `be put` — cheaper than scanning packs.

## Lifecycle

**`be put <file>` / `be delete <file>`** (driven by `sniff/PUT.c`,
`sniff/DEL.c`):

  1. Compute the new blobs and rebuild the affected subtrees.
  2. Open the branch staging pack (create `stage.sniff` + `stage.idx`
     + `stage.lock` if missing).
  3. Append one pack (tree objects, then blob objects) and emit its
     bookmark + object entries into `stage.idx`.
  4. Update sniff's root `SNIFF_TREE` hashlet to the new tree.

**`be post`** (driven by `sniff/POST.c`):

  1. Read sniff's root tree hashlet (the *current* staged tree).
  2. Walk reachable trees + blobs via `stage.idx`, materializing
     any cross-dir bases as required by the DAG invariant.
  3. Build a fresh canonical pack: new commit object (with parent
     from the branch's `REFS` tail), then trees in order, then
     blobs in order.  No intermediate/obsolete staged objects are
     copied.
  4. Feed the pack into the **same branch dir's** canonical log via
     `KEEPPackFeed` — full checks (including the type-order
     invariant) and indexing.  The pack gets the next store-wide
     sequential `file_id`; the file lands at
     `<store>/<branch-dir>/NNNNN.keeper`.
  5. On success, unlink `stage.sniff`, `stage.idx`, `stage.lock`,
     and append the new commit sha to the branch's `REFS`.

**Unstage** = sniff rewrites its root `SNIFF_TREE` hashlet to exclude
the path.  Dead objects in the staging pack remain; they are purged
when the next `be post` drops the staging files wholesale.

**Branch switch** moves the worktree's `.dogs` pointer to another
branch dir.  Each branch's own `stage.sniff` is untouched;
switching back resumes staging where it was.

## Crash safety

The staging pack is append-only; a torn append is detected by
`stage.idx` ↔ `stage.sniff` mismatch on next open and truncated.
On `be post`:

  1. Repacked canonical pack is appended + fsync'd to the branch
     dir's tail `NNNNN.keeper`.
  2. Canonical bookmarks + object entries are emitted to the
     branch's index runs.
  3. `stage.sniff` + `stage.idx` + `stage.lock` are unlinked.
  4. Branch `REFS` line appended + fsync'd.

A crash between (2) and (4) leaves a duplicate commit in the
canonical log (harmless — dedup by hash) and a still-live staging
file set (harmless — next `be post` re-repacks the same tree).  A
crash between (1) and (2) leaves an object-bytes-only tail in the
canonical log with no index entries — recoverable by re-indexing
the last pack on keeper open.

## Replication

Staging files never participate in the wire protocol.  `keeper/WIRE.c`
ships canonical packs only; `stage.*` names are excluded by convention.

## Concurrency

Single-writer per branch dir, enforced by `stage.lock` (flock).
Different branches have independent lock files.  The "one
worktree per branch" rule (see `sniff/AT.md`) makes concurrent
same-branch staging from two wts structurally impossible.

## Current code vs. this spec

As of 2026-04-22, staging lives at `.dogs/sniff/<branch-path>/pack`
+ `idx` (under sniff's private dir, separate from keeper's store).
Moving staging **into** the branch dir alongside canonical packs
is the next refactor once the sharded keeper layout lands
(see `keeper/LOG.md` §"Current code vs. this spec").  `sniff/PUT.c`
continues to feed blobs + trees into `KEEPPackFeed` with the
`ORDERBAD` check disabled; re-enabling it in the canonical path
depends on this move.
