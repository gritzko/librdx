#   SST files

Originally, Sorted String Tables (SSTs) are building blocks of LSM databases.
ABC implements a simpler deterministic variety of SSTs:

 1. keys are fixed length (SSTu128, SSTsha256, etc),
 2. there is an embedded skiplog (SKIPu8),
 3. an arbitrary amount of metadata precedes the data,
 4. there is an optional page index following the data.

ABC SST file:

````
    +-------+-------+-------+-------+
    |S S T 0|metalen|  data length  |
    +-------+-------+-------+-------+
    |       metadata section        |
    +-------------------------------+
    |                               |
    | Sorted TLV key-value entries  |
    .      interleaved with         .
    | SKIPlog entries (256b blocks) |
    |                               |
    +-------------------------------+
    |      page index section       |
    |...............................|
    +-------------------------------+
````
Key prefix compression works within one skip log block
(the repeated prefix is omitted).
Entry TLV types may vary.

ABC SST files are deterministic, i.e. files containing 
the same set of entries must be bitwise identical.
