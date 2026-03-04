#   Beagle SCM

Beagle is a source code management system that stores AST trees, not binary blobs.
The general idea is *a database for the code* as a hub of all code-related activities
and storage for all the related data (tickets, CI results, whatsnot).
The backing store is virtually any key-value database (rocksdb as of now).
The data format [AST BASON][j] is CRDT-ish.

Beagle is early stage and experimental.
It host itself, the rest is not guaranteed.
Use at your own risk.

Further reading: 

 1. [design rationale][2]
 2. [branching and storage model][y]
 3. [BASON, cheap mergeable binary JSON][j]

##  Examples of use

````
    $ be post //replicated.live/@gritzko/librdx
    $ be
    $ cd /some/other/dir
    $ be get //replicated.live/@gritzko/librdx
    $ be
````


[2]: https://replicated.wiki/blog/partII
[y]: https://replicated.wiki/be/STORE
[j]: https://github.com/gritzko/librdx/blob/master/json/README.md
