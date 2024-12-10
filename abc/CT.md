  # CT (Causal Trees Light)

Virtually every programming environment has some formatted output API.
At the very least, they must allow for messages like "%i bytes sent".
Some environments, like PHP, *are* huge formatted output APIs.
The `printf` family of functions is an example of a small but still
immensely useful implementation. ABC adapts that as `$printf`.
Still, there is a need for a more sophisticated templated output API.
For example, LEX code generation needs one. It should be small, simple,
orthogonal to the rest of ABC and arbitrarily composable.

CT is a simplification of a Causal Tree CRDT serving this exact purpose.
CT allows for complex templated output: ifs, cycles, nested templates,
and so on. That is achieved by *reusing* existing `$u8feedX` routines.

  - `CTfeed(ct, template)` parses the templates fed to the buffer and 
    remembers the insertion points, like the one in `"$BYTES bytes sent"`. 
  - Later, we can invoke `CTsplice(ct, BYTES)` and all the following
    output will be spliced into that insertion point, e.g.
    `$u8feed10u64(CTidle(ct), byte_count)`, including any `CTfeed`
    templates. Note that later mentions of an insertion point
    overwrite earlier ones.
  - `CTinsert()` allows for repeated insertions into the same place. 
  - `CTrender(into, ct)` produces the resulting text. As CT is
    using the usual ABC buffers, they can be saved and cloned at any
    point in time. 

