#   ABC buffers

An ABC buffer is an array of four pointers dividing a memory
range into three [slices][S]: PAST, DATA and IDLE.
Buffers imply ownership; if you hold the buffer you own the memory.
Buffers can be stack-allocated, malloc-ed, memory-mapped and file-mapped.
Normally, only the owner can reallocate or free a buffer.
Most downstream routines consume specific slices, although some take buffers too.
Note that consumption of a slice (DATA or IDLE) enlarges the adjoined slice (PAST or DATA resp).

````
    aB(u32, numbers);
    call(Bu32mmap, numbers, 1024);
    // consume IDLE, enlarge DATA by one int
    call($u32feed1, Bu32idle(numbers), 12345);
    call(Bu32unmap, numbers);
````

Many ABC data structures are buffer-based, for a good reason.

>   "As simple as possible, but not simpler" -- A.Einstein

Overall, ABC C discourages heap allocations, especially small ones.
For that reason, pointer-heavy data structures are not used.
For example, C++ STL red-black trees are seen as a horror story.
The reasons are many.

  - First, such data structures carry a lot of pointer overhead,
    as typically every node has to point at other node(s).
    If your payload is 64 bits and a pointer is 64 bits and you 
    need two of those... you get the idea.
  - Second, `malloc/free` requires pretty complicated bookkeeping, which
    a regular developer rarely sees, except in benchmarks.
  - Third, once your data is sprayed all over the heap, you have no locality.
  - Finally, suppose you want to save or checksum your data. 
    Picking your data structure out of a (manure) heap would be 
    a complicated process. It is not generic. 

Meanwhile, all the solid-buffer-based data structures have no 
such limitations. Memory allocation happens once (optimistically)
or a logarithmic number of times (worst case). There are no pointers.
All the data stays in one block, very cache-friendly.
Finally, a buffer is always a buffer, so one can checksum or save it 
to disk by a generic routine.

For concrete examples of the approach see flat [BIN][B] tree, binary [HEAP][H], 
[LSM][L] iterator heap or [HASH][D] set.

[S]: ./$.md
[B]: ./BIN.md
[D]: ./HASH.md
[H]: ./HEAP.md
[L]: ./LSM.md
