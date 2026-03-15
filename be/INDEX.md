#  be/ — Beagle SCM

A source-code management system that stores AST trees (not blobs)
in RocksDB.  Keys are URIs, values are BASON.  Branches are
formula-filtered overlays; merges use CRDT set-union.

##  Headers

### BE.h — core SCM handle and CRUD

Types: `BE` (handle), `BEmeta` (mtime/mode/ftype), `BEScanCBf`,
`BEFileCBf`, `BEGrepCBf`, `BASTNodeCBf`, `BETriCBf`.

Key schemes: `stat:` metadata, `be:` content, `tri:` trigram index.

Scratch buffers (1 GB lazy-mmap each):
`BE_READ`, `BE_RENDER`, `BE_PARSE`, `BE_PATCH`, `BE_WRAP`,
`BE_PATHS`, `BE_SCRATCH`.

Lifecycle:
  - `BEInit`       create repo + .be file + open DB
  - `BEOpen`       find .be, parse URI, open DB
  - `BEClose`      close DB
  - `BEAddBranch` / `BERemoveBranch` / `BESetActive`

CRUD:
  - `BEPost`       worktree → repo (waypoint)
  - `BEGet`        repo → worktree (merge base+waypoints)
  - `BEPut`        merge source branch into active
  - `BEDelete`     tombstone file or branch
  - `BEGetDeps`    process .beget dependency file
  - `BECheckpoint` fork repo via RocksDB checkpoint
  - `BEMilestone`  fold waypoints into base

Scan / query:
  - `BEScan`       iterate all files under project
  - `BEScanChanged` iterate files with matching waypoints
  - `BEScanFile`   iterate base+waypoints for one file
  - `BEGetFileMerged` read+merge single file

Status / diff:
  - `BEStatusFiles` print mtime-changed files
  - `BEDiffFiles`   colored line-level diff vs repo

Export:
  - `BASTExport`   flatten BASON tree → source text

Grep / trigrams:
  - `BEGrep`        search with trigram acceleration
  - `BEHashlet`     2-char hashlet from path (12-bit bucket)
  - `BETriExtract`  extract trigrams from BASON leaves
  - `BASTGrepNodes` walk tree, filter nodes, output lines

Diff building:
  - `BASTDiffBuild`  unified diff BASON from old+patch
  - `BASTDiffRender` render diff BASON with ANSI colors
  - `BASTTextDiff`   plain line-level text diff

Key helpers:
  - `BEKeyBuild`    scheme:path?query#fragment
  - `BEQueryBuild` — stamp-branch format
  - `BEKeyBranch` / `BEKeyStamp` (via `VERParse`)
  - `BEMetaFeedBason` / `BEMetaDrainBason` / `BEMetaFromStat`

### VER.h — version formulas

Type: `ron120` (u128: time in MS word, origin in LS word, op in bits 60-63).
Operators: `VER_ANY`(0), `VER_LE`(1), `VER_GT`(2), `VER_EQ`(3).

  - `VERMake` / `VERPoint` / `VERTime` / `VEROrigin` / `VEROp` — inline accessors
  - `VERParse`            parse single "time-origin" entry → ron120
  - `VERFormParse`        parse "brA&stamp-brB&stamp+brC"
  - `VERFormFromBranches` build all-ANY formula from branch list
  - `VERFormMatch`        check waypoint against formula
  - `VERutf8Feed`         encode ron120 → text

### IGNO.h — .gitignore parser/matcher

Types: `igno_pat` (pattern + flags), `igno` (up to 256 patterns).

  - `IGNOLoad`   load .gitignore from directory
  - `IGNOFree`   free resources
  - `IGNOMatch`  check if relative path should be ignored

### BESYNC.h — remote sync (HTTP)

  - `BESyncIsRemote`  check for http(s):// prefix
  - `BESyncClone`     clone remote into $HOME/.be/<repo>/

### BESRV.h — HTTP streaming server

Types: `BEClient` (per-client iterator + write buffer), `BESRVctx`.
Modes: `BESRV_MODE_BASON`, `_STATE`, `_RAW`, `_DIR`.

  - `BESRVInit`  bind port, set up POL
  - `BESRVRun`   event loop
  - `BESRVStop`  signal stop
  - `BESRVFree`  cleanup

##  Implementation files

  - `BE.c`       core CRUD, scan, diff, merge, milestone (~2200 lines)
  - `GREP.c`     trigram search, AST grep, diff build/render (~800 lines)
  - `VER.c`      formula parse/match (~90 lines)
  - `IGNO.c`     gitignore glob matching (~230 lines)
  - `BESYNC.c`   HTTP clone via libcurl (~230 lines)
  - `BESRV.c`    streaming server via POL (~680 lines)
  - `BE.cli.c`   `be` CLI tool (~490 lines)
  - `BE.http.c`  `be-srv` entry point (~60 lines)

##  Build

Library `beagle` (static): BE.c VER.c BESYNC.c IGNO.c GREP.c.
Links: bast, abc.  Requires `WITH_ROCKS`.

Executables:
  - `be`      (BE.cli.c) — CLI
  - `be-srv`  (BE.http.c + BESRV.c) — requires `WITH_INET`

##  Tests

  - `test/BE.c`     core operations (~1300 lines)
  - `test/VER.c`    formula tests (~230 lines)
  - `test/BESRV.c`  server tests (~460 lines, WITH_INET)
  - `test/BESYNC.c` sync tests (~180 lines, WITH_INET)
  - `test/cli-test.sh` end-to-end CLI test
