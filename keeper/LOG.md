# keeper pack log

`.dogs/keeper/log/` is a sequence of numbered append-only log
files holding concatenated *packs*. It is the sole storage for
object bytes; nothing is ever mutated in place. The pack format
is based on git packfiles.

    .dogs/keeper/log/
        0000000001.pack
        0000000002.pack
        ...

## Append-only pack logs

One log file holds **many packs** appended back-to-back.  Small
packs (e.g. a single local commit) MUST be appended to the
current tail log file — never open a new file just to store one
pack.  A new log file is only started when the current one
crosses a size threshold, or when GC consolidates.
A file has git packfile header, but no trailing checksums.

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

**Current status (2026-04-20):** enforcement in `KEEPPackFeed` is
**live** for canonical (main-log) packs.  `keep_pack.strict_order`
opts in; `KEEPPackOpen` sets it automatically.  Sniff staging packs
(opened via `sniff/STAGE.c`) leave it off — they accumulate
tree/blob in DFS order and are repacked canonically on `be post`
(commit first, then trees via staging walk, then blobs).  A handful
of legacy tests that hand-roll non-canonical packs clear the flag
explicitly.

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
but can cheaply be made one. Dog-to-dog sync (see SYNC.md)
ships stripped packs directly — both sides store the same way.

Object count for reconstruction is recovered by walking the
stripped byte range on demand (varint scan of object headers).
Git-compat reconstruction is rare (only `KEEPPush` today), so the
cost is paid per push, not per read.  No count is stored in the
index. File/pack checksums are only kept as 60-bit *hashlets*.

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
index itself — sentinels are a sync-protocol artifact emitted at
enumeration time (see `SYNC.md`).

Pack bookmarks share the LSM with object entries; the type tag in
the key's low 4 bits keeps them from colliding (objects use
1..4 for commit/tree/blob/tag; pack bookmarks use a reserved
value outside that range — TBD, propose 15).

## GC / repack

To consolidate: pick seq N greater than any existing log, write a
fresh log file `NNNNNNNNNN.pack` containing all reachable objects
(optionally re-delta'd), emit a new `.idx` for it, then discard
older log+idx pairs atomically.

No in-place rewriting.  Readers holding mmaps of old files
continue to work until they close; new lookups resolve via the
new index.

Peer watermarks (`SYNC.md`) pointing at discarded file_ids become
stale — the affected peers fall back to full sync on next
contact.  This is acceptable: GC is rare, watermarks are cheap to
rebuild.

## Implications for sync

See `keeper/SYNC.md`.  The pack bookmark carries exactly what
plain dog sync needs: a globally meaningful hashlet for dedup
negotiation, a (file_id, offset) locator for streaming, and
derived length via the next bookmark / tail sentinel.

## Current code vs. this spec

As of this writing, `KEEPPackOpen/Close` in `keeper/KEEP.c` still
creates one file per pack (`p->file_id = k->npacks + 1`), writes
full PACK headers and trailers into each file, and does **not**
emit pack bookmarks into the index.  Aligning the implementation
with this spec is a prerequisite for `SYNC.md` (Phase 0).
