#   FILE operations

The module contains various file-related routines.
Creating, opening, removing, memory-mapping files, etc.
Those are mostly ABC wrappers for POSIX routines.

For read-only files, `FILEMapFD` is highly recommended.
For append-only files, `FILEfeed` and a `Bu8` buffer might work.
Andy Pavlo insists that `FILEMapFD` is not good for random-access writes.
You may use the `pwrite` wrapper instead.
Also, see the relevant [discussion][h] on HN.

[h]: https://news.ycombinator.com/item?id=36563187
