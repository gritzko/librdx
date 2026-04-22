# keeper refs format

Keeper's ref state is split between a **per-branch reflog** in each
branch dir and a **store-wide alias file** at the root:

    <store>/
        ALIAS                      host-level URI aliases
        REFS                       trunk reflog
        feature/
            REFS                   feature branch reflog
        tags/v1.0/
            REFS                   tag history

One mapping per line:

    <ron60-time>\t<from-uri>\t<to-uri>\n

e.g.

    26416FJreE\t?tags/v2.9.1\t?5c9159de87e41cf14ec5f2132afb5a06f35c26b3

## Per-branch `REFS`

`<branch-dir>/REFS` records the tip history of **this one branch**
plus peer views of the same branch.  Two forms appear:

  - **Local tip** — the branch's own tip over time.  The branch
    name is implicit from the dir, so the `from-uri` elides to `?`:

        <time>\t?\t?<hex-sha>

  - **Remote-tracking of the same branch** — fully-qualified
    origin URI keyed with `heads/` or `tags/`:

        <time>\t<origin-uri>?heads/<name>\t?<hex-sha>
        <time>\t<origin-uri>?tags/<name>\t?<hex-sha>

For SSH and explicit-host transports the origin URI is what the
user typed (`localhost:src/git`, `//localhost/path`, …).  For local
path access, canonicalise to `file:///<absolute-path>`.

Cross-branch entries do **not** belong in a branch's REFS.  If a
wt on `feature` learns origin's `main` tip, that record lives in
the trunk's `REFS` (the dir that owns `main`), not in `feature/`.
Placement rule: the entry's branch-part must match the owning dir.

Resolution is scoped to the dir: looking up `?heads/feature` reads
`feature/REFS` directly and does NOT walk up the dir chain.  Only
alias resolution walks up.

## Store-wide `ALIAS`

`<store>/ALIAS` is an append-only alias file, resolved from any
branch dir by walking up to the root.  Same line format; typical
entries:

    <time>\t//github\thttps://github.com/torvalds/linux.git
    <time>\t//linux\tssh://git@kernel.org/pub/scm/linux/linux.git

Aliases are host-level, not branch-scoped; keeping them in one
root file avoids duplicating them under every branch and prevents
silent loss when a branch dir is dropped.

## What's not in keeper's ref state

**Worktree state** is not in keeper.  The per-wt branch pointer
lives in the wt's `.dogs` file; the reverse pointer lives in the
branch dir's `WT` file (see `sniff/AT.md`).  Keeper's REFS carry
only replicated refs (local + remote-attributed for the same
branch) — never "which wt is where".

## Drop-a-dir and reflogs

Squash/rebase/drop of a branch dir removes its `REFS` together
with its packs.  Peer views of that branch held in that dir's
`REFS` vanish too, which is exactly right: the dropped commits
are gone, so the memory of where peers had them no longer
matters.  Aliases in `<store>/ALIAS` are unaffected.
