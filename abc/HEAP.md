#   Binary heaps

A binary heap is a surprisingly simple and useful data
structure. It perfectly fits the way ABC [slices][S] and [buffers ][B]
work. See `HEAPx.h` for the implementation.

It is also a good example on how to create data structure
templates in ABC. The technique is borrowed from the Linux
kernel. For the API and usage examples please see
`test/HEAP.c` and `fuzz/HEAP.c`. That is general rule with ABC:
tests are the API docs. The Markdown documentation only
provides the motivation and the explanation. Any API
documentation not covered by the tests either becomes stale
soon or demands too much attention.

[S]: ./$.md
[B]: ./B.md
