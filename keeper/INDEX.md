#  git/ — Git compatibility layer

Parsers for git wire protocol (pkt-line, packfile) and git objects
(blob, tree, commit).  Includes .gitignore matching.  Uses zlib for
pack decompression and OpenSSL for SHA-1 object IDs.

##  Headers

### GIT.h — git object parsers

Types: none (output via slices).

  - `GITu8sDrainTree`    drain one tree entry (mode+name, 20-byte SHA1)
  - `GITu8sDrainCommit`  drain one commit header; empty field = body

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

  - `ZINFInflate`  decompress zlib data
  - `ZINFDeflate`  compress data

Note: SHA1.h and ZINF.h are isolated from ABC headers to avoid
typedef clashes (zlib `voidpc` vs ABC `voidpc`).

### BELT.h — ersatz git-compatible repository format

Types: `belt128` (index entry: hashlet+type+offset+gen), `walk` via WALK.h.

belt128.b layout: `(gen:20 << 44) | (offset:40 << 4) | flags:4`.
Generation = 1 + max(parent gens), computed during indexing.

  - `BELTEntry`      construct belt128 entry (hashlet, type, offset, gen)
  - `BELTHashlet`    extract hashlet from entry
  - `BELTType`       extract object type
  - `BELTOffset`     extract log offset (40-bit)
  - `BELTGen`        extract generation number (20-bit)
  - `BELTClone`      clone repo via git-upload-pack into belt
  - `BELTImport`     import packfile into belt
  - `BELTGet`        get object by hex hash
  - `BELTLookup`     lookup by hashlet in sorted run stack
  - `BELTResolve`    resolve object at pack offset (chasing deltas)
  - `BELTCommitGen`  compute generation number from commit content

##  Implementation files

  - `GIT.c`     tree/commit drain parsers (~75 lines)
  - `PKT.c`     pkt-line framing (~77 lines)
  - `PACK.c`    packfile header/object/inflate (~101 lines)
  - `IGNO.c`    gitignore glob matching (~233 lines)
  - `SHA1.c`    SHA-1 via OpenSSL EVP (~24 lines)
  - `ZINF.c`    zlib inflate/deflate (~63 lines)
  - `BELT.c`    belt repository: clone, import, index, get (~775 lines)
  - `WALK.c`    graph traversal: commits, trees, ancestor, missing (~400 lines)

### WALK.h — git object graph traversal

Types: `walk` (walker state), `walk_fn` (visitor callback).

  - `WALKOpen`         open walker on a belt directory (mmaps log+index)
  - `WALKClose`        close walker, unmap everything
  - `WALKGet`          get object by hashlet
  - `WALKGetSha`       get object by raw 20-byte SHA-1
  - `WALKCommitTree`   parse tree SHA-1 from commit content
  - `WALKCommits`      BFS walk of commit history (gen-pruned)
  - `WALKTree`         DFS walk of tree (recursive)
  - `WALKAncestor`     find common ancestor (gen-pruned simultaneous BFS)
  - `WALKMissing`      enumerate objects reachable from head but not base
  - `WALKCheckout`     materialize tree to filesystem

### DELT.h — git delta instruction applier

  - `DELTApply`  apply delta instructions (copy/insert) to base object

##  CLI

  - `git-dl.cli.c`  download repo via git-upload-pack, save packfile
                     + kv64 hash index (truncated SHA-1 → offset).
                     Resolves OFS_DELTA and REF_DELTA chains.

##  Build

Library `gitcompat` (static): GIT.c PKT.c PACK.c DELT.c ZINF.c SHA1.c IGNO.c BELT.c WALK.c.
Links: abc, ZLIB, OpenSSL::Crypto.

##  Tests

  - `test/GIT.c`    tree/commit parser tests (6 cases)
  - `test/PKT.c`    pkt-line drain/feed tests (8 cases)
  - `test/PACK.c`   packfile header/varint/inflate tests (7 cases)
  - `test/IGNO.c`   gitignore pattern matching tests (3 cases)
  - `test/ZINF.c`   deflate/inflate round-trip chain (20 versions)
  - `test/FETCH.c`  treadmill: clone repo via ssh git-upload-pack,
                     unpack packfile, write loose objects, verify with git
  - `test/WALK.c`   belt128 gen encoding, WALKCommitTree, BELTCommitGen (4 cases)
  - `test/ROUND.c`  full round-trip: create bare repo, clone via ssh,
                     edit+commit, push back, verify files match
