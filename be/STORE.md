#   Repos, branches, waypoints and milestones

The document details the forking/branching/merging model and
its internal representation in the store.

##  The process

Repos have FQDN names, like project.team.org. A repo contains
a full file tree that may host any number of projects. The
division into projects and subprojects is rather arbitrary,
for the repo it is all one big file tree.

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
at some intervals.

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
picked.

##  The inner db representation

A repo or a snapshot is a directory, one instance of the
database. The current implementation employs rocksdb.
All values are BASON. A single merge operator (`BASONMergeY`,
BASON set-union) is registered for the entire database.

Keys use URI scheme prefixes to separate data planes:

  - `stat:/@project/dir/file.c -> BASON metadata (mtime, mode, ftype)`
    base file metadata as per repo/milestone
  - `stat:/@project/dir/file.c?timestamp-branch -> BASON metadata`
    waypoint file metadata
  - `be:/@project/dir/file.c -> BASON AST (BAST)`
    base version as per repo/milestone
  - `be:/@project/dir/file.c?timestamp-branch -> BASON patch`
    waypoint commit changes
  - `be:/@project/?timestamp-branch#commit -> BASON string`
    commit message
  - `tri:/@project?XYZ -> BASON object { hashlet: "", ... }`
    trigram posting list (see below)

The `stat:` prefix scan gives fast file listing and metadata
without touching bulk BASON content. The `be:` keys store
pure BASON trees/patches. The `tri:` keys store trigram
posting lists for substring search.

Waypoint and branch changes are kept separately from the
base version till the next milestone. Normally, branches
are edited in conjunction with the current `main`.

##  Trigram index

The trigram index accelerates substring search (`be grep`)
without scanning all file content. It works as follows:

  - On `be post`, each file's BASON string leaves are
    scanned for 3-character alphanumeric trigrams (ASCII
    letters, digits, and RON Base64 characters).
  - Each file path is hashed to a 2-character RON64
    *hashlet* (12 bits = 4096 buckets).
  - For each unique trigram, a BASON object `{ hashlet: "" }`
    is merged into the posting list at `tri:/@project?XYZ`.
    RocksDB compaction merges these via set-union,
    accumulating hashlets from all files containing that
    trigram.
  - On `be grep`, the query is decomposed into trigrams,
    their posting lists are intersected into a 4096-bit
    bitset, and only files whose hashlet passes the filter
    are actually read and verified.

The index is append-only: hashlets are never removed. Stale
entries (from deleted or modified files) are harmless false
positives that get filtered out during verification. This
makes writes cheap and compaction-friendly.

##  The worktree representation

The worktree contains project files as plain text.
The `.be` file in the root of the project identifies the
repo/milestone and the branches/overlays currently turned
on, as one URI.

##  Remotes and network operations

 1. Syncing a branch
 2. Syncing all branches
 3. Syncing a milestone
