#   C Cursors

Cursor is another ABC [star-type][R]. One may understand 
it as a "heavy iterator" or "non-owning buffer". 
To make the distinctions more clear:

 1. ABC [slice][S] is `T *s[2]`, an array of two pointers
    denoting an `[a,b)` memory range to be consumed.
 2. ABC iterator is same as an ABC slice. That is 
    contrary to the C++ tradition that equates an 
    iterator and a pointer. A C++ iterator alone is 
    mostly useless as one needs to check the `end()`
    pointer each time to advance the iterator.
 3. ABC [buffer][B] is `T *b[4]`, a memory range divided
    into three slices (typically, PAST, DATA and IDLE).
    A buffer *owns* the memory, its DATA and IDLE slices
    serve as built-in iterators tracking used/empty
    parts of the buffer.
 4. ABC cursor is `T* c[4]`, an iterator that still
    keeps track of the original memory range. Most
    typically, a cursor is a copy of a buffer, but 
    its DATA slice is positioned differently.

Using a cursor makes a lot of sense when data iteration
is not trivial. For example, one may want to use references
to jump back and forth in the data, or refer to some 
metadata dictionary or simply to rewind an iterator.

Again, these are conventions, as all of the star types are
just arrays of pointers demarcating memory ranges.

[B]: ./B.md
[S]: ./$.md
[R]: ./README.md
