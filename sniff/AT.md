# sniff checkout: `.dogs` pointer + branch-dir `WT`

A worktree's current branch is encoded by **two cooperating
pointers**: a `.dogs` file inside the wt, and a `WT` file inside
the branch directory on the store side.

    <wt>/.dogs                 → /abs/store?heads/feature
    <store>/heads/feature/WT   → /abs/wt

This replaces the former `.dogs/sniff/at.log` file (single authority
per wt) with a **pair** of authorities — wt-side for "where is my
store and what branch am I on", store-side for "who is the wt on
this branch".  The two sides are kept in sync by every `be get`.

## `<wt>/.dogs` (wt side)

A regular file (not a directory).  One line, one URI:

    /abs/store?heads/feature

Contents:

  * Absolute path to the store root (the directory holding trunk's
    pack logs and branch subdirs).
  * `?heads/<name>` — the branch this wt is checked out on.  The
    `heads/` or `tags/` prefix is optional (bare branch path also
    accepted); the canonical form is what `be get` writes.
  * Detached checkout: `?<hex-sha>` without a branch.  A detached
    wt cannot stage (`be put`/`be delete`/`be post` are refused).

Readers determine the current branch dir by resolving the URI
against the store: `<store>/heads/feature/` (with
`heads/main|master|trunk` aliasing to `<store>/` itself).

A "traditional" co-located layout — store at `<repo>/.dogs/` and
working files next to it in `<repo>/` — is a convention, not a
special case; the same `.dogs` file sits inside the wt (here the
file is the store's `.dogs/` **directory**, and a flat wt is
recognised by finding a `.dogs/` dir instead of a `.dogs` file at
the wt root).

## `<branch-dir>/WT` (store side)

A regular file.  One line, one path:

    /abs/wt

Contents:

  * Absolute path to the wt currently on this branch.
  * Absent file = no wt checked out on this branch.

Rule enforced by `be get`: at most one wt per branch dir.  Trying
to check out a branch whose `WT` names a different live wt is
refused; the user must first close the other wt (which removes
`WT`) or choose another branch.

The back-pointer lets store-side tools find the live checkout
without scanning: pushing a canonical pack from `be post` reads
`WT` to know which wt's staging to drop.

## Detached checkouts

`be get ?<sha>` writes `<wt>/.dogs` with no branch component.  No
`<sha-dir>/WT` file is created (there is no branch dir for a raw
sha).  `be put` / `be delete` / `be post` refuse with "detached
HEAD".  Re-attach with `be get ?heads/<name>`.

## Replaces

  * the old `.dogs/sniff/at.log` append-only file
  * the old `.dogs/sniff/HEAD` single-line file
  * the `file://<reporoot>` entries previously written into
    `keeper/REFS` by `sniff/GET.c`, `sniff/POST.c`, `keeper/KEEP.c`

None of those survives this migration.  Keeper's REFS (now
`<branch-dir>/REFS`, per-branch) carries replicated refs only —
never worktree state (see `keeper/REF.md`).

## API (post-migration target)

    ok64 SNIFFAt     (u8cs *store_out, u8cs *branch_out, u8cs *sha_out);
    ok64 SNIFFAttach (u8cs store, u8cs branch, u8cs sha);
    ok64 SNIFFDetach (u8cs store, u8cs sha);

  * `SNIFFAt` reads the wt's `.dogs` pointer and returns the store
    abspath, branch (empty for detached), and the current tip sha
    (read from the branch's `REFS` tail, or taken directly from
    `.dogs` for detached).
  * `SNIFFAttach` writes `<wt>/.dogs` and `<branch-dir>/WT`
    together (fsync'd in that order to keep the store-side pointer
    accurate even if the wt side crashes first).
  * `SNIFFDetach` writes `<wt>/.dogs` with a bare `?<sha>` and
    removes any stale `WT` file for the prior branch.

`SNIFFAt` callers: `sniff/GET.c` on every checkout, `sniff/POST.c`
for parent-commit lookup, `beagle/BE.cli.c` for dispatch.

## Staging precondition

Staging (`be put` / `be delete` / `be post`) requires a non-empty
branch component in `<wt>/.dogs`.  Detached checkouts cannot
stage — the caller must `be get ?heads/<branch>` first.  See
`sniff/STAGE.md`.

## Format notes

  * `.dogs` is plain text, one line, no trailing newline required.
  * `WT` is plain text, one line, absolute path.
  * Both files are rewritten wholesale on checkout; there is no
    append-only history.  Historical tip progression is recoverable
    from the branch's `REFS` (which is append-only, per-branch —
    see `keeper/REF.md`).
  * No file-level checksum; integrity is one-line parseability.

## Drop-a-dir interaction

Squash/rebase that drops a branch dir removes its `WT` along with
everything else.  Precondition check: the operation refuses if
`WT` names a live wt — close the wt first (which clears `WT`).
This prevents orphaning a wt whose `.dogs` file still points at a
vanished branch.
