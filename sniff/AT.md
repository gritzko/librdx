# sniff at.log — per-worktree branch + commit pointer

`.dogs/sniff/at.log` is a per-worktree, append-only record of "what
this worktree is currently checked out at".  Each line is:

    <ron60-time>\t?<branch>\t?<sha>

e.g.

    26416FJreE\t?heads/main\t?68aba62e560c0ebc3396e8ae9335232cd93a3f60

The file's **tail** is authoritative — earlier lines are history
(useful for debugging and reflog-style browsing, no semantic role).
`<branch>` is the ref path minus leading `refs/`
(e.g. `heads/main`).  For a detached checkout (`be get ?<sha>`) the
branch field is empty: `?\t?<sha>`.

## Why sniff owns it

A worktree under dogs is a `.dogs/` where `keeper/`, `graf/`, `spot/`
are symlinks to the primary repo and **only `sniff/` is real, local
state** (see `beagle/test/worktree.sh:40-57`).  The currently-checked-
out commit is therefore local to each worktree — it MUST NOT live in
the shared, replicated keeper/REFS.  Placing it in sniff's private dir
makes per-worktree independence trivial: two worktrees of the same
primary have distinct `at.log` files and cannot clobber each other.

## Replaces

  * the old `.dogs/sniff/HEAD` file (single-line, rewritten)
  * the `file://<reporoot>` entries in keeper/REFS written by
    `sniff/GET.c:274`, `sniff/POST.c:225`, `keeper/KEEP.c:2242`

Neither source of truth survives this patch.  keeper/REFS becomes
purely "replicated refs".

## API

    ok64 SNIFFAt (u8cs *branch_out, u8cs *sha_out);
    ok64 SNIFFAtAppend (u8cs branch, u8cs sha);

`SNIFFAt` reads the tail of `at.log` and returns slices into a
sniff-owned buffer (stable until the next `at.log` write or
`SNIFFClose`).  `SNIFFAtAppend` writes a new line with a fresh
ron60 timestamp; used by:

  * `sniff/GET.c` on every checkout
  * `sniff/POST.c` after every successful commit

Callers that previously resolved a worktree-keyed `file://` entry in
keeper/REFS (`sniff/POST.c` for parent commit, `keeper/KEEP.exe.c`
for push, `beagle/BE.cli.c` for dispatch) switch to `SNIFFAt`.

## Staging precondition

Staging (`be put` / `be delete` / `be post`) requires a non-empty
branch field in the `at.log` tail.  Detached checkouts cannot stage
— the caller must `be get ?<branch>` first.  See `sniff/STAGE.md`.

## Format notes

  * Append-only; torn appends detected by incomplete trailing line
    and truncated on next open.
  * No file-level checksum; integrity is covered by each line being
    self-validating (time parses, URIs parse).
  * Compaction is not required; the file is small (one line per
    checkout/commit) and never consulted hot-path beyond the tail.
