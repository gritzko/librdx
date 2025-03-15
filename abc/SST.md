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
Entry TLV types may vary. The 'K' type is reserved for
skiplog entries. The digit in the magic bytes denotes
the key size, e.g. `SST4` is 16 byte keys.

The implied structure of the key is u64, id128, sha256 or
something like that. For that reason, prefix compression
is not implemented: the resulting complexity is not worth
it. That kind of gains are best harvested by page-level
DEFLATE or something like that.

ABC SST files are deterministic, i.e. files containing
the same set of entries must be bitwise identical.
