# sniff — worktree management

Checkout, status, stage, commit.  State is a single append-only
ULOG file at `<wt>/.sniff`; no pack files, no caches, no path
registry (keeper owns paths).  The worktree's files-on-disk plus the
ULOG plus keeper's object store are the ground truth.

Worktrees may share a store: each wt has its own `.sniff` but the
`repo` row (row 0) points them all at the same `.dogs/`.  Colocated
wts set it to their own sibling `.dogs/`; secondary wts set it to
the primary's store.

## The one-paragraph model

Every op that touches the wt — `get`, `post`, `patch` (attribution-bearing)
or `put`, `delete` (intent-only) — appends one row to
`.sniff`.  Each attribution row's timestamp is the mtime that
sniff sets on every file it wrote via `utimensat`.  A file is "clean"
iff its mtime is in the ULOG's stamp-set; anything else is a user
edit.  The baseline tree at any moment is the URI of the most recent
`get`/`post`/`patch` row: one hash in the fragment → keeper commit
tree; two or more (comma-separated) → graf-reconstructed merge tree.
POST walks that baseline plus the wt, applies the change-set rules
below, emits one keeper pack `commit → trees → blobs`, stamps every
surviving wt file, and appends a `post` row.

## Change-set at commit time

For each candidate path:

1. **Explicit delete** (`delete <path>` row after last post): drop
   from the new tree, `unlink` from disk.
2. **Explicit put** (`put <path>` row after last post): include,
   content from wt.
3. **Selective mode** (any put/delete rows after last post, no
   explicit match for this path): carry the baseline entry through
   unchanged (or skip the path if it was never in baseline).
4. **Implicit mode** (no put/delete rows since the last post):
   `mtime ∉ stamp-set` ⇒ rewrite from wt; `mtime ∈ stamp-set` ⇒
   carry baseline; file missing from wt ⇒ drop.

## Headers

| Header | Role |
|--------|------|
| SNIFF.h | Singleton state (open/close, ULOG handle, per-process sorted path index), path-registry wrappers over keeper (`SNIFFIntern` / `SNIFFPath` / `SNIFFCount` / `SNIFFRootIdx` / `SNIFFInternDir` / `SNIFFIsDir` / `SNIFFFullpath` / `SNIFFSort`). |
| AT.h | ULOG façade: verb constants (`SNIFFAtVerbGet/Post/Patch/Put/Delete`), append (`SNIFFAtAppend`, `SNIFFAtAppendAt`), baseline/post-ts/scan lookups (`SNIFFAtBaseline`, `SNIFFAtLastPostTs`, `SNIFFAtScanPutDelete`), stamp I/O (`SNIFFAtNow`, `SNIFFAtStampPath`, `SNIFFAtOfTimespec`, `SNIFFAtKnown`). |
| GET.h | Checkout: walk target tree via keeper → materialise files, dirty-protect via stamp-set, futimens every write to a shared ts, append one `get` row, prune any stamped wt file not in the new tree. |
| PUT.h | `put <path>` — one row per URI, no pack I/O, no tree work. |
| DEL.h | `delete <path>` — mirror of PUT. |
| POST.h | Commit: resolve baseline URI, fetch baseline tree, scan wt (`FILEScan`), compute change-set per the rules above, pre-hash blobs, build dirty-spine trees, emit one pack `commit → trees → blobs`, advance keeper REFS, unlink explicit-deletes, append `post` row, stamp surviving files. |
| PATCH.h | 3-way wt merge via graf; `refuse_if_dirty` is a wt-scan against the stamp-set; on success appends a `patch` row whose fragment extends the prior baseline fragment with the `theirs` sha (comma-separated multi-hash URI). |

## CLI (`sniff`)

| Command | Effect |
|---------|--------|
| `sniff get <hex>` | Checkout commit (alias `checkout`) |
| `sniff put [files]` | Append `put <path>` rows (no-op without args) |
| `sniff delete [files]` | Append `delete <path>` rows (no-op without args) |
| `sniff post -m <msg>` | Commit (alias `commit`).  Auto-selects change-set per the rules above. |
| `sniff patch ?<ref>` | 3-way merge into wt |
| `sniff status` | List mtime-dirty files (M) |
| `sniff list` | List keeper-interned paths |
| `sniff watch` | Start inotify drain loop (fork, pidfile) |
| `sniff stop` | Stop the watch daemon |

Flags: `-m <msg>` commit message, `--author <who>` author string.

## On-disk layout

Per worktree (at the wt root):

| Path | Format |
|------|--------|
| `.sniff` | `<ron60-ts>\t<verb>\t<uri>\n` — see `dog/ULOG.md`.  Row 0 is a `repo` anchor whose `file://` URI names the store.  Subsequent rows are `get`/`post`/`patch`/`put`/`delete`. |
| `.sniff.pid` | Watch daemon PID (if `sniff watch` is running; dead weight in the ULOG-only model and may be retired). |

Nothing else.  The store (`.dogs/`) is the keeper's; sniff never
writes there directly — it hands objects to keeper via `KEEPPackFeed`.

## Tests

C (`test/SNIFF.c`):

| Test | What |
|------|------|
| `SNIFFInternPath` | Intern + path round-trip, dedup (root `/` reserved at idx 0). |
| `SNIFFAtHelpers` | Verb constants are distinct; empty-log invariants; seeded-log baseline pick (most recent get/post/patch with multi-hash fragment recognised); stamp-set membership; last-post-ts lookup; `SNIFFAtScanPutDelete` forward-scan across different floors. |
| `SNIFFCheckoutCommit` | Hand-built initial commit → `GETCheckout` → modify file → `POSTCommit` produces a new commit object keeper can retrieve (verifies the full GET + POST integration on the ULOG-only path). |

Scripts:

| Script | Scope |
|--------|-------|
| `test/workflow.sh` (`SNIFFworkflow`) | The 6 canonical scenarios against the `sniff` CLI: initial post, get, accumulating put+post, implicit all-dirty via bare post, explicit delete, implicit-delete via vanished file. |
| `../beagle/test/workflow.sh` (`BEworkflow`) | Same scenarios through the `be` dispatcher — covers keeper + spot + graf pipeline wiring. |
| `../beagle/test/history.sh` (`BEhistory`) | Three tagged commits with a delete and an add, then round-trip every tag through `be get` and verify files appear and disappear.  This is the regression that motivated the rewrite — the old per-path cache incorrectly re-added deleted files across processes; mtime attribution fixes it architecturally. |

## Dependencies

Links `keeplib`, `gitcompat`, `dog`, `abc-core`.  Uses `abc/FSW` for
inotify in the `watch` daemon.
