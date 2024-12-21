#   HASH container

The module implements a very basic open-addressed hash table.
The template expects a record type with `Xcmp` and `Xhash` functions defined.
Can use it as a standalone table or as a part of some bigger construct.

Macro parameters:

  - ABC_HASH_LINE the length of a line. Cells can only be displaced
    within their line. If a line is full, `HASHxXput()` returns `HASHnoroom`.
    The line length and the buffer length MUST be powers of 2!
  - ABC_HASH_CONVERGE whether we want the hash table to be convergent.
    A "bit state" of a convergent table does not depend on the order of
    operations, if only the resulting "API state" is the same.
    Operations that commute can arrive in any order!
    For example, "put 1, put 2, remove 1, put 3" and "put 3, put 2" would
    result in the same bit-state of the *buffer*. 
