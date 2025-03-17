#   `brix` SST CLI tool

`brix` allows for basic manipulations with `.brik` file stacks.
`.brik` files are RDX SSTs, i.e. tables of RDX objects sorted by id.
`brix` compares to an LSM database like a wrench compares to a workshop.

##  Opening a stack

`brix open` opens a BRIX file, including its stack of dependencies (a *version*).
One can refer to a version by its hash, hash prefix or the file path (which
contains the hash).

  - `brix open 8469d149546328e3f8bde5ec179f617d1bfa740f3b44c80f3f6617896cc9b1b6`
  - `brix open 8469d14`
  - `brix open ./.rdx/8469d149546328e3f8bde5ec179f617d1bfa740f3b44c80f3f6617896cc9b1b6.brik`

##  Blending additional `.brik` files

`brix add` adds an SST file to the stack, including its (missing) dependencies.
Any queries would reflect the added data.

  - `brix add c256798798d1a247bebfb833f709f2f1e23c365c88015c421dff9ec426f9517e`
  - `brix add ./.rdx/c256798798d1a247bebfb833f709f2f1e23c365c88015c421dff9ec426f9517e.brik`
  - `brix add c2567987`

##  Reading entries

`brix get` queries the particular record by its ID
  - `brix get b0b-123`

RDX objects may include other objects by reference, e.g.
    `{@Bob-outer key:"value" referTo:[@B0b-inner]}`

`brix reget` queries records recursively, thus building a full document (tree)
  - `brix reget b0b-123`

##  Merging files, compacting stacks

Many database operations boil down to `.brik` file merges.
Those are compactions, branch merges, patching and so on.
`brix merge` merges some topmost files producing one new SST file.
The resulting version hash is printed.

  - `brix merge` merges some reasonable number of files roughly
    to maintain 8:1 size ratio,
  - `brix merge 4` a way to specify the number of files to merge,
  - `brix merge c25679879` merge files on top of this one.

##  Generating standalone patches

`brix patch` converts an RDX document to a temporary SST file and
adds it to the stack. That should be an RDX document with full
metadata as BRIX only stores data and merges the changes.
There is no diffing of the document to the existing state.
Applying standalone patches can be done with `brik add`.

##  Syncing to other repos

`brix pull` downloads the specified `.brik` file from a remote
repo, including its dependencies, e.g.

`brix pull http://repo.dev/brix/c2567987`

`brix push` uploads a file and its dependencies

`brix pull http://repo.dev/brix/9afbc90e`
