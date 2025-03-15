#   ARENa allocation

Arena is a stack-like allocator.
In the ABC system, the use of `malloc/free` is discouraged.
For larger chunks of RAM, one can use [MMAP][M] (almost)directly.
For smaller pieces, there is stack.
There is one popular case inbetween: tons of small pieces that do not fit on the stack.
That is exactly what *arenas* are for.

 1. you reserve a piece of RAM (mmap/malloc/whatever),
 2. you allocate pieces of it for misc uses,
 3. you free it all at once.

ABC arena is a really thin sugar coating on top of ABC [buffers][B],
specifically `Bu8`.

[B]: ./B.md
[M]: ./MMAP.md
