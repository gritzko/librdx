#   Beagle CLI: plumbing and porcelain

##  The `.be` file

The `.be` file in the worktree root links it to a repo/project.
It contains a URI with per-branch watermarks recording the last
seen waypoint timestamp:

    be://repo.fqdn/project?26219b4L5j+main&26219b4L5k+feature

Here `time-branch` (with `-`) is used in DB keys for waypoint
commits, while `time+branch` (with `+`) in `.be` records which
state the worktree is based on. A branch with no watermark yet
(freshly added) is just `+newbranch` or `newbranch`.

POST uses watermarks to detect stale worktrees: if there are
waypoints newer than the watermark on any visible branch, POST
fails — the user must `be get` first to rebase. GET and POST
update watermarks on success.

##  Plumbing

Atomic, one thing each, no side effects:

1. **GET** (repo → worktree)
   1.a. `GET` — export all project files (base + visible waypoints merged)
   1.b. `GET file.c ...` — export specific files
   1.c. `GET ?branch` — set active branch, then 1.a

2. **POST** (worktree → repo)
   2.a. `POST` — diff all worktree files, write waypoint deltas on active branch
   2.b. `POST file.c ...` — diff+waypoint specific files
   2.c. `POST //repo/project` — init: create depot + `.be`, import worktree

3. **PUT** (data → repo, lateral)
   3.a. `PUT file.sst` — ingest SST file into depot
   3.b. `PUT ?branch` — re-key branch waypoints onto active branch
   3.c. `PUT //repo` — ingest SSTs from another local depot
   3.d. `PUT http://remote` — pull SSTs from remote into depot

4. **DELETE**
   4.a. `DELETE file.c` — write tombstone waypoint
   4.b. `DELETE ?branch` — delete all branch waypoints

5. **GREP** (repo search, read-only)
   5.a. `GREP pattern` — trigram-accelerated substring search across project files

##  Porcelain

Composes plumbing, leaves worktree consistent:

5. **get** (end result: worktree refreshed)
   5.a. `be get` — 1.a
   5.b. `be get file.c ...` — 1.b
   5.c. `be get ?branch` — 1.c
   5.d. `be get ?brA&brB` — 1.c (blend multiple)
   5.e. `be get //repo/project` — write `.be` + 1.a
   5.f. `be get http://remote` — 3.d + 5.e

6. **post** (end result: repo updated)
   6.a. `be post` — 2.a
   6.b. `be post file.c ...` — 2.b
   6.c. `be post dir/` — 2.b (files in dir)
   6.d. `be post //repo/project` — 2.c (init, no `.be` found)
   6.e. `be post http://remote` — 2.a + remote 3.d
   6.f. `be post //newrepo` — checkpoint depot (fork)

7. **put** (end result: repo updated + worktree refreshed)
   7.a. `be put ?branch` — 3.b + 4.b + 1.a
   7.b. `be put ?brA&brB` — (3.b + 4.b) × N + 1.a
   7.c. `be put http://remote` — 3.d + 1.a
   7.d. `be put //repo` — 3.c + 1.a
   7.e. `be put file.sst` — 3.a (no GET, raw ingest)

8. **delete**
   8.a. `be delete file.c` — 4.a + rm worktree file
   8.b. `be delete ?branch` — 4.b + 1.a

9. **grep** (search repo without checkout)
   9.a. `be grep pattern` — 5.a, prints matching file paths
   9.b. `be grep "multi word"` — trigram-filtered search
