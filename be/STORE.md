#   Repos, branches, waypoints and milestones

The document details the forking/branching/merging model and
its internal representation in the store.

The store must be able to carry CRDT-ish representation of 
the source code, fork and merge things at will. At the same 
time, it must be simple and standard enough so virtually 
anything can be built on top. Our three building blocks are
 
 1. key-value databases (almost any would work, rocksdb is
    used for now due to its flexibility),
 2. JSON in binary form (BASON, is a JSON superset, in fact,
    but cruder and cheaper than RDX),
 3. URIs to address *everything*.

All three things are universally understood and implemented
in all imaginable forms and shapes.

The store must support any forms of use and abuse, including
real-time collaboration and Ctrl+S commits with Ctrl+Z
rollbacks. Is must be infinitely extensible; we implement
[trigram-based][t] search index as a PoC and a universally
useful feature for humans and AIs alike.

[t]: https://en.wikipedia.org/wiki/Trigram_search

##  The process

Repos have FQDN names, like project.team.org. A repo contains
a full file tree that may host any number of projects. The
division into projects and subprojects is rather arbitrary,
as the repo only sees one big file tree, which a worktree
is a subset of.

Milestones are notable states of a repo that have public FQDN
names, like `v1_2_3.project.team.org` or anything `v[\dA-Z].*`
e.g. `vChristmas.project.company.com`. Development may be 
based on the titular repo or on a milestone. Physically, they
are indistinguishable from repos.

Branches are lines of development with limited duration. The
`main` branch is the special default one. A working copy is
typically a milestone plus some number of branches and other
changes merged in. The default process is to merge branches
into `main` eventually, snapshotting its state as a milestone
at some intervals. The base (titular repo or a milestone) is
basically a coprolite. Nothing there can be rolled back or
cherry-picked. The editing only happens in branches.

Overlays are branches that live indefinitely long. Your .md
files for LLMs may live in an overlay, for example, to be
versioned in the same flow, but still be separable from code.
Overlay names start uppercase, branches lowercase, up to
10 characters in RON Base 64. 

The basic unit of change is a *waypoint* commit. A waypoint
is nameless, identified by a timestamp. These are essentially
Ctrl+S events that might be triggered by succesfull builds or
test suite runs. A waypoint belongs to a branch/overlay.
A branch may be merged wholy, till some waypoint, or cherry
picked. A waypoint may have a commit message, although the
main way of describing changes between versions is CHANGELOG
(there might be many). As changes can be grouped in arbitrary
ways and reedited (100% normal), sticking detailed messages
to commits is not really convenient. The diff of a CHANGELOG
is more flexible in this regard (can be broken down by subsystem
as well). If you collaborate with some number of LLMs, linear
history becomes a stumble point which Beagle intends to fix.

##  The inner db representation

A repo or a snapshot is a directory, one instance of the
database. The current implementation employs rocksdb, where
snapshots may hard-link share most of their data for space
efficiency.  

All keys are URIs. All values are BASON (binary JSON). Beagle's
internals are designed as an internet protocol, intentionally.
A single merge operator (`BASONMergeY`, BASON set-union) is
registered for the entire database.

Keys use URI scheme prefixes to separate data planes:

  - `stat:/project/dir/file.c -> BASON metadata (mtime, mode, ftype)`
    base file metadata as per repo/milestone
  - `stat:/project/dir/file.c?timestamp-branch -> BASON metadata`
    waypoint file metadata
  - `be:/project/dir/file.c -> BASON AST (BAST)`
    base version as per repo/milestone
  - `be:/project/dir/file.c?timestamp-branch -> BASON patch`
    waypoint commit changes
  - `be:/project/?timestamp-branch#commit -> BASON string`
    commit message
  - `tri:/project?XYZ -> BASON object { hashlet: "", ... }`
    trigram posting list (see below)

The `stat:` prefix scan gives fast file listing and metadata
without touching bulk BASON content. The `be:` keys store
pure BASON trees/patches. The `tri:` keys store trigram
posting lists for substring search. Any new database use
likely requires a new scheme.

Waypoint and branch changes are kept separately from the
base version till the next milestone. Normally, branches
are edited in conjunction with the current `main`.

##  Trigram index

The trigram index accelerates substring search (`be grep`)
without scanning all file content. It maps symbol trigrams to
file/path hashlets.  The index is append-only: hashlets are
never removed. Stale entries (from deleted or modified files)
are harmless false positives that get filtered out during
verification. This makes writes cheap and compaction-friendly.

Other layers of functionality can be hosted in the same key-
value store and be updated/replicated by the same machinery.

##  The worktree representation

The worktree contains project files as plain text.
The `.be` file in the root of the project identifies the
repo/milestone and the branches/overlays currently turned
on, as one URI. There is no separate index/staging as the
original state can be cheaply recovered from the repo and
commits can be very incremental, aggregated post-factum.
Beagle change model is much closer to an oplog/wal of a
database than git-like chain-of-blocks.

##  Remotes and network operations

LSM databases consist of immutable files, hence extremely
sync-friendly, so no special magic here.

 1. Clone is trivial: all SST files get copied to the new
    host. Compression happens at the database level.
 2. Syncing a branch: as a branch is a sequence of deltas,
    syncing is basically exchanging deltas. Some indexing
    should also be done, depending on the replica.
 3. Syncing a milestone: like clone, but some SST files
    (larger ones) are likely to be already present on the
    other side, so those can be skipped.

There is hardly anything else to talk about.
