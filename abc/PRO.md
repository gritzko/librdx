#   Procedures

ABC treats functions and procedures differently.

ABC *functions* are
 1. inlinable,
 2. perform computations that don't fail,
 3. return the result.

`PRO.h` defines helper macros for *procedures* that:
 1. can be inlined, but not necessarily,
 2. can fail,
 3. return `ok64`.

You can write and invoke ABC procedures without using PRO
macros. You may not use this convention at all. As every
ABC component, this one is orthogonal and optional.

Note that PRO macros also allow for fine-grained tracing of
the call stack.
