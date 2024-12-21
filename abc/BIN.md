#   BIN: logarithmic bins / flat trees

BIN.h implements a logarithmic bin layout to pack your binary tree into a flat buffer.
The layout looks like this:

````
           ---- 7----
          /          \
         /            \
        3              11
      /   \           /  \
    1       5       9     13
   / \     / \     / \    / \
  0   2   4   6   8  10  12 14
````

Cell index calculations are cheap, typically some arithmetics and bitwise ops.
Performance-wise, one can see such a tree as a random access array.

Overall, ABC C advocates the use of [buffer-based][B] data structures to improve code composability.
For example, one can mmap a BIN based tree from a file, checksum it and send it over the network,
all using generic routines. No OOP, no interfaces, just buffers and library procedures.
Other examples of the approach are binary [HEAP][H], [LSM][L] iterator heap and [HASH][D] set.

Originally, the layout was devised in Nov 2008 to host binary Merkle trees for multimedia streams.
`BIN.h` dates back to those times, save for minor convention tweaks.
Later, the layout made its way into some internet drafts and [RFC 7574][r].
Recently, Russ Cox made a [good writeup][x] discussing all ways of packing a tree into a buffer.
BIN specifically implements *in-order* traversal of *binary* trees (there are many other options).
There is a bunch of implementations of the same scheme by M.Buus and [friends][d].

[B]: ./B.md
[D]: ./HASH.md
[H]: ./HEAP.md
[L]: ../rdx/LSM.md
[x]: http://research.swtch.com/tlog
[d]: https://github.com/mafintosh/flat-tree
[r]: https://datatracker.ietf.org/doc/rfc7574
