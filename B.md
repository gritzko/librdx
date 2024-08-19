#   ABC buffers

shiftable
buffers imply ownership of memory

1.  stack
2.  alloc (extendable)
3.  mmap
4.  file mmap

only the 1 

the only downside is that you have to negotiate with the OS.
That takes longer, so you only want that for bigger chunks of memory.

Resizing file mmaps is a separate story
