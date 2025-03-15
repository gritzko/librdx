#   ABC slices

Originally, C lacked the concept of slices, many reinventing
their own. An ABC slice is a pair of pointers: `[head, term)`.
A slice is consumed by moving the `head` towards `term`. A slice
has four shades of `const`:

 1. a fully writable slice often denotes the free space
    available for output, e.g. `$u8`;
 2. a const-value slice is most often the input data to be
    consumed, e.g. `$u8c`;
 3. an immovable slice `$cu8` can be edited, not consumed;
 4. an immutable slice e.g. `$cu8c` can not be either consumed
    or edited.

A slice argument is always passed as a pointer, so these shades
are important. Also, `$dup` is often used to create a duplicate
slice for consumption by the callee.

An ABC slice also serves as an iterator, e.g.
````
    $u32 numbers;
    ...
    $eat(numbers) {
        printf("%u\n", **numbers);
    }
````

Most often, a slice is a segment of an [ABC buffer][B].

[B]: ./B.md
