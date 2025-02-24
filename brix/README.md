#   BRIX: an RDX LSM syncable store

BRIX employs the [Replicated Data eXchange][R] format to build an embeddable
syncable document store with branching and other interesting features.
The internal format is [JSON-like][J] RDX, mapped to a key-value store.

##  Inner workings

BRIX is a very basic Log-Structured Merge Tree (LSMT, LSM) store.
The idea is to use a stack of SST files. Each SST file is a sorted
array of key-value records. When we read from the database, all
records for that key are found and merged. To add something to the
store, a new SST file has to be created (actual databases use a
temporary in-memory store for small writes, then flush them as SST).
When the number of SST files grows inconveniently big, some are 
merged. The merge proceeds by a parallel pass, very much like merge
sort works.

BRIX records are all RDX. Every BRIX SST file mentions its previous
file by its hash, so once you open a version of the data, you open 
the entire stack of files. Then, you are free to add more files 
on top of that (patches, merges, and so on). Merges affect the top
of the stack, i.e. you merge top `k` files into one.

All BRIX SST files are stored in the `./.rdx` directory by default.

##  `brix` the CLI tool

 1. `brix open` opens an SST file by its hash, including its stack 
     of dependencies.
      - `brix open 8469d149546328e3f8bde5ec179f617d1bfa740f3b44c80f3f6617896cc9b1b6`
      - `brix open 8469d14`
      - `brix open ./.rdx/8469d149546328e3f8bde5ec179f617d1bfa740f3b44c80f3f6617896cc9b1b6.brik`
 2. `brix add` add an SST file by its hash, including its dependencies,
    to the stack (i.e. push); any queries would reflect the added data.
      - `brix add c256798798d1a247bebfb833f709f2f1e23c365c88015c421dff9ec426f9517e`
      - `brix add ./.rdx/c256798798d1a247bebfb833f709f2f1e23c365c88015c421dff9ec426f9517e.brik`
      - `brix add c2567987`
 3. `brix get` queries the particular record by its ID
      - `brix get b0b-123`
 4. `brix get` queries records recursively, building a document
      - `brix reget b0b-123`
 5. `brix merge` merges the added files producing one new SST file
      - `brix merge`
      - `brix merge 4` explicitly specify the number of files to merge
      - `brix merge c25679879` merge files on top of this one
 6. `brix patch` converts an RDX document to a temporary SST file and 
    adds it to the stack. That should be an RDX document with full
    metadata as BRIX only stores data and merges the changes.

##  Further reading

 1. [The project's mission][M]
 2. [The project's method or "why CRDT works (this time)?"][E]

[M]: ./MISSION.md
[E]: ./METHOD.md
[C]: ./README.cli.md
[J]: ../rdx/JDR.md
[R]: ../rdx/README.md
