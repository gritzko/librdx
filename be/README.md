#   Beagle SCM

Beagle is a source code management system that stores AST trees, not binary blobs.
The general idea is *a database for the code* as a hub of all code-related activities
and storage for all the related data (tickets, CI results, whatsnot).
The backing store is virtually any key-value database (rocksdb as of now).
The data format is [basically JSON][j] (but binary and mergeable).

Beagle is early stage and experimental.
It hosts itself, the rest is not guaranteed.
Use at your own risk.

Further reading: 

 1. [design rationale][2]
 2. [branching and storage model][y]
 3. [BASON, cheap mergeable binary JSON][j]

##  Examples of use

````
    # Initial POST of a project. We specify repo, project path.
    # Absence of http: or ssh: makes it local to the store in $HOME/.be
    $ be post //replicated.live/@gritzko/librdx
    ...
    OK   h    ast/tree-sitter/point.h
    OK   h    ast/tree-sitter/parser.h
    OK   c    ast/tree-sitter/query.c
    OK   text ast/verilog/LICENSE
    ...million more lines...

    # Inspect RocksDB inners just out of curiosity.
    $ ls $HOME/.be/replicated.live/
    # Inspect worktree/repo state
    $ cat .be
    //replicated.live/@gritzko/librdx
    $ be
    repo: replicated.live
    project: /@gritzko/librdx
    branches: *main
    base files: 574, waypoints: 0

    $ cd /some/other/dir
    $ be get //replicated.live/@gritzko/librdx
    ...a million lines...
    OK   h    rdx/test/YX.h
    OK   h    rdx/test/ZE.h
    OK   sh   rdx/test/http-test.sh
    OK   sh   rdx/test/query-test.sh
    OK   sh   scripts/ci-fast.sh
    $ cd librdx
    $ be
    repo: replicated.live
    project: /@gritzko/librdx
    branches: *main
    base files: 574, waypoints: 0
````

##  CSS-like AST selectors

`be cat` supports CSS-like selectors via URL fragments (`file#selector`)
to filter code by structure. Since Beagle stores ASTs, not blobs, you
can query by node type, definition name, line range, or text content.

````
    # Show all function definitions in a file
    $ be cat main.c#fn
    int main(int argc, char **argv) {
        ...
    }

    # Show a specific function by name
    $ be cat main.c#fn.main
    int main(int argc, char **argv) {
        ...
    }

    # Show only comments
    $ be cat main.c#cmt
    // Main entry point
    --
    /* Calculates the factorial of n */
````

Available type selectors: `fn` (functions), `class`, `cmt` (comments),
`str` (string literals), `kw` (keywords), `num` (number literals),
`type` (type names), `def` (definition names), `block`, `obj`, `args`.
Line ranges use GitHub convention: `#L10-20`.
Plain words are text search: `#malloc` finds all lines mentioning malloc.

[2]: https://replicated.wiki/blog/partII
[y]: https://replicated.wiki/be/STORE
[j]: https://github.com/gritzko/librdx/blob/master/json/README.md
