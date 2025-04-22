#   BRIX: an RDX LSM syncable store

BRIX employs the [Replicated Data eXchange][R] format to build
an embeddable syncable document store with branching, versioning,
and other interesting features. Its internal format is RDX,
which is [JSON-like][J], but stored as key-value records.
It is up to the user to gauge the granularity of that mapping.

##  Inner workings

BRIX is a very basic Log-Structured Merge Tree (LSMT, LSM) store.
The idea is to use a *stack* of SST files. Each SST file is a sorted
array of id-value records. Each file references the previous file
on the stack and the only way to change data here is to push a new 
SST file onto that stack. When we read from the database, records
(versions, patches) for that id are found in the files and merged.

Another component of an LSM database is a write-ahead log (WAL). 
WAL accumulates records temporarily before making an SST file.
Typically, LSM databases accumulate writes in a memtable. BRIX
is heavily influenced by BitCask, so it indexes the WAL instead.

When the number of SST files grows inconveniently big, BRIX can
merge some number of files on the top of the stack into one file.
The merge proceeds by a parallel pass, very much like merge sort
works.

BRIX records are all RDX. Every BRIX SST file mentions its previous
file by the hash. A *version hash* is the SHA256 hash of the stack's
top file. Once you open a *version*, you open the entire stack with
a guarantee of data integrity. Merges preserve the RDX state, but
the version hash changes (merged vs original).

All BRIX SST files are stored in the `./.rdx/brix` directory by default.


##  Further reading

 0. `brix` the [CLI tool][C]
 1. [The project's mission][M]
 2. [The project's method or "why CRDT works (this time)?"][E]

[M]: ./MISSION.md
[E]: ./METHOD.md
[C]: ./README.cli.md
[J]: ../rdx/JDR.md
[R]: ../rdx/README.md
