  # LSM: log-structured merge tree

The module implements the most basic log-structured merge tree store primitives.
An LSM-tree is the most common database structure after the B-tree.
One chunk of an LSM tree is often called an SST (sorted strings table).
It is a log of key-value records sorted in the ascending order.
See the [Petrov's Database Internals book][i] for the particulars.

The key LSM algorithm is the chunk merge, working very much like a merge sort.
The `LSMnext/LSMmerge` routines implement exactly that: merge by a heap of iterators.
The contents of the chunks are supposed to be sorted [ToyTLV][T] key-value records.

[T]: ./TLV.md
[i]: https://www.databass.dev
