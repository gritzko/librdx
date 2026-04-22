#  git/ — Git compatibility layer

Parsers for git wire protocol (pkt-line, packfile) and git objects
(blob, tree, commit).  Includes .gitignore matching.  Uses zlib for
pack decompression and OpenSSL for SHA-1 object IDs.

##  Headers

### GIT.h — git object parsers

Types: none (output via slices).

  - `GITu8sDrainTree`    drain one tree entry (mode+name, 20-byte SHA1)
  - `GITu8sDrainCommit`  drain one commit header; empty field = body
  - `GITu8sCommitTree`   extract the tree SHA-1 from a commit body

### PKT.h — pkt-line framing

  - `PKTu8sDrain`      drain one pkt-line; returns PKTFLUSH/PKTDELIM for specials
  - `PKTu8sFeed`       feed one pkt-line (4-hex prefix + payload)
  - `PKTu8sFeedFlush`  feed a flush packet (0000)

### PACK.h — packfile parser

Types: `pack_hdr` (version, count), `pack_obj` (type, size, delta ref).

Object types: COMMIT=1, TREE=2, BLOB=3, TAG=4, OFS_DELTA=6, REF_DELTA=7.

  - `PACKDrainHdr`     parse PACK magic + version + count
  - `PACKDrainObjHdr`  parse object type/size varint + delta base
  - `PACKInflate`      zlib-inflate compressed object data

### IGNO.h — .gitignore parser/matcher

Types: `igno_pat` (pattern + flags), `igno` (up to 256 patterns).

  - `IGNOLoad`   load .gitignore from directory
  - `IGNOFree`   free resources
  - `IGNOMatch`  check if relative path should be ignored

### SHA1.h — SHA-1 hash (OpenSSL wrapper)

  - `SHA1Sum`  compute 20-byte SHA-1 (isolated from ABC types)

### ZINF.h — zlib inflate/deflate wrapper

  - `ZINFInflate(u8s into, u8cs zipped)`  decompress zlib data
  - `ZINFDeflate(u8s into, u8cs plain)`  compress data


##  Implementation files

  - `GIT.c`     tree/commit drain parsers (~75 lines)
  - `PKT.c`     pkt-line framing (~77 lines)
  - `PACK.c`    packfile header/object/inflate (~101 lines)
  - `IGNO.c`    gitignore glob matching (~233 lines)
  - `SHA1.c`    SHA-1 via OpenSSL EVP (~24 lines)
  - `ZINF.c`    zlib inflate/deflate (~63 lines)
  - `WALK.c`    KEEP-backed tree walker (eager + lazy)

### WALK.h — git object graph traversal

Types: `walk` (walker state), `walk_fn` (visitor callback).

  - `WALKOpen`         open walker on a belt directory (mmaps log+index)
  - `WALKClose`        close walker, unmap everything
  - `WALKGet`          get object by hashlet
  - `WALKGetSha`       get object by raw 20-byte SHA-1
  - `WALKTree`         DFS tree walk over KEEP — eager (blobs resolved), path-aware visitor
  - `WALKTreeLazy`     DFS tree walk over KEEP — lazy (blobs empty, pulled on demand)
  - `WALKu8sModeKind`  classify git tree-entry mode → `WALK_KIND_*`
  - Commit-graph traversal lives in `graf/`, not here.

### DELT.h — git delta instruction applier + encoder

  - `DELTApply`   apply delta instructions (copy/insert) to base object
  - `DELTEncode`  produce a git delta instruction stream for
                  (base, target).  4-byte hash index over `base` with
                  forward + bounded-backward extension.  Returns
                  DELTFAIL when the delta is no smaller than the raw
                  target (caller should emit raw instead).
                  Exercised end-to-end via `test/DELTA_ROUND.c`:
                  feeds a chain of blob versions with a hashlet60
                  hint to `KEEPPackFeed`, splices the log into a git
                  packfile, reads each version back via `git cat-file`.
                  `KEEPPackFeed` emits OFS_DELTA when the base is a
                  raw object in the same in-progress pack, else
                  REF_DELTA against whatever `KEEPGet` resolves from
                  committed runs (delta chains chased transparently).

##  CLI

  - `git-dl.cli.c`  download repo via git-upload-pack, save packfile
                     + kv64 hash index (truncated SHA-1 → offset).
                     Resolves OFS_DELTA and REF_DELTA chains.

##  Build

Library `gitcompat` (static): GIT.c PKT.c PACK.c DELT.c ZINF.c SHA1.c IGNO.c.
Library `keeplib` (static): KEEP.c KEEP.exe.c REFS.c WALK.c.
Links: abc, ZLIB, OpenSSL::Crypto.

##  Tests

  - `test/GIT.c`    tree/commit parser tests (6 cases)
  - `test/PKT.c`    pkt-line drain/feed tests (8 cases)
  - `test/PACK.c`   packfile header/varint/inflate tests (7 cases)
  - `test/DELT.c`   DELTEncode + DELTApply round-trip
  - `test/DELTA_ROUND.c`  KEEPPackFeed with delta hints → valid git
                           packfile → `git cat-file` per version
  - `test/IGNO.c`   gitignore pattern matching tests (3 cases)
  - `test/ZINF.c`   deflate/inflate round-trip chain (20 versions)
  - `test/FETCH.c`  treadmill: clone repo via ssh git-upload-pack,
                     unpack packfile, write loose objects, verify with git
  - `test/WALK.c`   WALKu8sModeKind table + WALKTree/WALKTreeLazy on synthetic KEEP
  - `test/ROUND.c`  full round-trip: create bare repo, clone via ssh,
                     edit+commit, push back, verify files match
  - `test/POST.c`   `keeper post ssh://…` — synthesize a commit and
                     push it via git-receive-pack; verify remote HEAD
