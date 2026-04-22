# keeper pack log

Pack logs are numbered append-only files holding concatenated
*packs*.  They are the sole storage for object bytes; nothing is
ever mutated in place.  The pack format is based on git packfiles.

Logs are sharded **by branch directory**; each branch dir holds its
own sequence of log files plus its own indexes, reflog and (if the
branch is checked out) a worktree back-pointer:

    <store>/                    trunk (heads/main|master|trunk → "")
        00001.keeper
        00003.keeper
        00001.idx
        REFS
        WT
        feature/
            00002.keeper
            00004.keeper
            00001.idx
            REFS

File numbering (`NNNNN`) is **store-wide and sequential** — pack
logs in different branch dirs never share a number.  The physical
file sits in whichever branch dir the pack was written to.

## Append-only pack logs

One log file holds **many packs** appended back-to-back.  Small
packs (e.g. a single local commit) MUST be appended to that dir's
current tail log file — never open a new file just to store one
pack.  A new log file is only started when the current one crosses
a size threshold.  A file has the git packfile header but no
trailing checksum.

Pack boundaries inside a log file are discoverable only via the
index (see pack bookmarks below).  The raw bytes carry no
self-synchronising markers (git's limitation, kept for backward
compatibility).

## Intra-pack object order

Within a single pack, objects MUST appear in type order:

    1. commits   (type 1)
    2. trees     (type 2)
    3. blobs     (type 3)
    4. tags      (type 4)

Git relies on this ordering implicitly (`git repack` produces
type-grouped packs for delta-base locality); keeper makes it an
**explicit invariant** — writers should enforce it, readers may
rely on it.

**Current status (2026-04-22):** enforcement in `KEEPPackFeed` is
**live** for canonical packs.  `keep_pack.strict_order` opts in;
`KEEPPackOpen` sets it automatically.  Sniff staging packs
(see `sniff/STAGE.md`) leave it off — they accumulate tree/blob in
DFS order and are repacked canonically on `be post` (commit first,
then trees via staging walk, then blobs).  A handful of legacy
tests that hand-roll non-canonical packs clear the flag explicitly.

Consequences:

  * A pack's first object is always a commit (or empty / tag-only
    in degenerate cases).  Sync and graf can cheaply identify
    commits by scanning pack prefixes.
  * Trees never reference commits; blobs never reference trees or
    commits; delta bases therefore point earlier in the same pack
    or into an earlier pack.  This rules out forward references
    within a pack.
  * Incoming git packfiles already satisfy the ordering when
    produced by stock git; `KEEPImport` must verify and reject
    packs that violate it rather than silently reorder.

## Stripped git pack framing

Packs are stored with the git packfile header (12 B: `PACK` magic
+ version + object count) and trailer (20 B SHA-1) **stripped**,
except in the beginning of the file. What lands in the log is only
the concatenation of object records (varint header + zlib body,
plus deltas).

This means the stored bytes are not directly a valid git packfile,
but can cheaply be made one. Dog-to-dog sync (see SYNC.md) ships
stripped packs directly — both sides store the same way.

Object count for reconstruction is recovered by walking the
stripped byte range on demand (varint scan of object headers).
Git-compat reconstruction is rare (only `KEEPPush` today), so the
cost is paid per push, not per read.  No count is stored in the
index. File/pack checksums are only kept as 60-bit *hashlets*.

## Delta-dependency DAG

OFS_DELTA is **pack-local**: the base sits earlier in the same
pack at a known offset.  A pack file can therefore be copied
verbatim between stores (dog-to-dog sync, branch-dir moves)
without rewriting deltas.

REF_DELTA resolves by hash lookup.  Keeper constrains the lookup
to the **dir chain from the pack's home dir up to the store root**:
a REF_DELTA's base MUST live in the same branch dir or in an
ancestor.  Writers that would otherwise delta against an object in
a sibling or descendant dir must materialize the base (emit it raw
into the target dir's pack) instead.

The invariant turns the branch tree into a dependency DAG: leaf
dirs can be dropped wholesale, interior dirs only after descendants
have materialized every cross-dir reference into the ancestor
chain.  See "Drop-a-dir" below.

## Pack bookmarks

Every pack appended to the log gets one wh128 index entry:

    key = file_id20 | offset40 | type4(PACK)
    val = hashlet60 | flags4

`hashlet60` is derived from the pack's trailing SHA-1 in git's
convention: the hash covers the full framed pack (12-byte PACK
header + object bytes + no trailer yet).  For imported git packs,
we copy the trailer as received.  For locally-built packs, we
compute the same hash over the reconstructed framed form.  This
keeps hashlets stable across git↔dog boundaries even though only
dog↔dog transfers ship stripped bytes.

`offset40` is the byte position where the pack's first object
starts within the log file; bookmarks sort by (file_id, offset)
naturally.

Pack byte length is derived from the next bookmark's offset
within the same log file.  For the last pack in a file, the
length comes from the log file's current byte length (from the
FILEBook header or the filesystem).  No sentinel is stored in the
index itself — sentinels are a sync-protocol artefact emitted at
enumeration time (see `SYNC.md`).

Pack bookmarks share the LSM with object entries; the type tag in
the key's low 4 bits keeps them from colliding (objects use
1..4 for commit/tree/blob/tag; pack bookmarks use a reserved
value outside that range — TBD, propose 15).

## Drop-a-dir (replaces GC / repack)

Keeper never rewrites packs.  Consolidation and cleanup happen at
the granularity of a **branch dir**:

  - **Squash / rebase a leaf branch**: delete the dir (pack logs,
    indexes, REFS, WT).  The DAG invariant guarantees no surviving
    REF_DELTA pointed into it from a sibling/descendant; ancestor
    resolution is unaffected because writers never delta upward-
    against-downward.
  - **Drop an interior dir**: precondition — every descendant's
    cross-dir REF_DELTAs into this dir must be materialized into
    the descendant (or into an ancestor that survives).  If not,
    the operation is refused; there is no automatic rewrite.
  - **Uncommitted staging or a live WT**: refused.  `be stash` or
    close the worktree first.

Readers holding mmaps of dropped files continue to work until they
close; new lookups route around the missing dir.

Peer watermarks (`SYNC.md`) pointing at dropped `file_id`s become
stale — the affected peers fall back to full sync on next contact.
This is acceptable: drops are user-initiated, watermarks are cheap
to rebuild.

## Implications for sync

See `keeper/SYNC.md`.  Sync scopes to a branch dir: Q records carry
one `<branch>/NNNNN.keeper` file each, watermarks are per-branch.
The pack bookmark carries exactly what plain dog sync needs: a
globally meaningful hashlet for dedup negotiation, a
(file_id, offset) locator for streaming, and derived length via
the next bookmark / tail sentinel.

## Current code vs. this spec

As of 2026-04-22, Phases 1a + 1b + 1c have landed: `keeper_shard`
owns the per-branch pack/index stacks, the on-disk layout is flat at
`.dogs/NNNNN.keeper` + `.dogs/NNNNN.idx` for trunk (5-hex-char
`file_id` matching the wh64 val's 20-bit field; no `keeper/`, `log/`,
or `idx/` subdirs), the REF_DELTA DAG-invariant guard is wired into
`KEEPPackFeed` (trivial in Phase 1c's single-shard mode), and
`KEEPBranchDrop` refuses trunk (`KEEPTRUNK`) or unknown branches
(`KEEPNOBR`).  Feature branches under `.dogs/<branch>/` and the
`KEEPBRANCHDIRTY` precondition wire up in Phase 2.  `KEEPPackOpen/Close` in `keeper/KEEP.c` still creates one
file per pack (`p->file_id = k->shards[0].npacks + 1`), writes full
PACK headers and trailers into each file, and does **not** emit
pack bookmarks into the index.  Multi-pack-per-file and pack
bookmark emission remain the next refactor — prerequisite for
`SYNC.md` (Phase 0) and for sniff's branch-dir staging (see
`sniff/STAGE.md`).
