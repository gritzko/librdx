#   be — AST-aware revision control

`be` is a revision control system that works with abstract syntax trees, not
lines of text. It parses source files into ASTs using tree-sitter, stores them
in a key-value database (RocksDB), and diffs/merges at the level of functions,
classes, statements, and other structural units. Everything is URI-addressed.
The storage layer is pluggable, the architecture is open and extensible.
The core command language is HTTP-alike (get, post, etc).

    be post                     # commit worktree to repo
    be get                      # checkout repo to worktree
    be diff                     # AST-level diff of local changes
    be cat file.c#fn.Scan       # CSS selector: function(s) named Scan
    be grep needle              # trigram-indexed search
    be come feature             # switch branch
    be fit feature              # merge branch

## Building

Dependencies: C compiler, cmake, librocksdb, libsodium, libcurl, liblz4.

    # Ubuntu/Debian
    sudo apt install clang ninja-build librocksdb-dev libsodium-dev libcurl4-gnutls-dev liblz4-dev

    # macOS
    brew install rocksdb libsodium curl lz4 ninja

    # Build
    cmake -B build -DCMAKE_BUILD_TYPE=Release -GNinja
    ninja -C build be/be
    cp build/be/be ~/.local/bin/

## Contents

| Directory | What it does |
|-----------|-------------|
| `be/`     | CLI and core: post/get/diff/grep/cat, branching, milestones, sync |
| `ast/`    | Tree-sitter AST parsing (BAST) and CSS-like structural selectors |
| `json/`   | BASON binary JSON coding; BIFF N-way tree diff and merge |
| `rdx/`    | Replicated Data eXchange format — mergeable CRDTs, version vectors |
| `abc/`    | ABC C dialect — slices, buffers, typed records, no pointer arithmetic |
| `mark/`   | Deterministic Markdown dialect with a fixed Ragel grammar |
| `js/`     | JavaScriptCore bindings for scripting |
| `scripts/`| CI and build automation |
