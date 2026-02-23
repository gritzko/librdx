#   ABC buffers

>   "As simple as possible, but not simpler" -- A.Einstein

An ABC buffer is an array of four pointers dividing a memory
range into three [slices][S]: PAST, DATA and IDLE.
Consumption of a slice (DATA or IDLE) enlarges the adjoined slice (PAST or DATA resp).

```
       PAST         DATA             IDLE
  |----------->------------->-------------------|
buf[0]      buf[1]        buf[2]              buf[3]
```

Buffers imply ownership; if you hold the buffer you own the memory.
Pass buffers down the call stack by pointer (`u8bp`), never duplicate.
Buffers can be stack-allocated, malloc-ed, or memory-mapped.

##  Typed functions (preferred)

| Operation       | Typed function       | Notes                    |
|-----------------|---------------------|--------------------------|
| Get DATA slice  | `u8bData(buf)`      | mutable `[1,2)`          |
| Get DATA const  | `u8bDataC(buf)`     | const slice              |
| Get IDLE slice  | `u8bIdle(buf)`      | mutable `[2,3)`          |
| Get IDLE const  | `u8bIdleC(buf)`     | const slice              |
| Total capacity  | `u8bLen(buf)`       | elements `[0,3)`         |
| DATA length     | `u8bDataLen(buf)`   | elements in DATA         |
| IDLE length     | `u8bIdleLen(buf)`   | room available           |
| Validate        | `u8bOK(buf)`        | check pointers valid     |
| Has room?       | `u8bHasRoom(buf)`   | IDLE non-empty           |
| Has data?       | `u8bHasData(buf)`   | DATA non-empty           |
| First element   | `u8bHead(buf)`      | first in DATA            |
| Last element    | `u8bLast(buf)`      | last in DATA             |
| Feed slice      | `u8bFeed(buf, s)`   | append to DATA           |
| Feed one        | `u8bFeed1(buf, v)`  | append single element    |
| Push by ptr     | `u8bPush(buf, p)`   | append element by ptr    |
| Pop             | `u8bPop(buf)`       | remove last element      |
| Shed N          | `u8bShed(buf, n)`   | trim N from DATA end     |
| Reset           | `u8bReset(buf)`     | clear PAST and DATA      |
| Shift           | `u8bShift(buf, n)`  | move DATA, set PAST to n |

##  Creation and memory

```c
a_pad(u8, buf, 1024)        // stack buffer (common case)
u8bAlloc(buf, len)          // heap allocate
u8bFree(buf)                // heap free
u8bMap(buf, len)            // mmap anonymous
u8bUnMap(buf)               // munmap
u8bReserve(buf, len)        // ensure capacity (may realloc)
```

##  Example

```c
a_pad(u8, buf, 1024);
u8bFeed1(buf, 0x42);              // append byte
u8bFeed(buf, some_slice);         // append slice
size_t n = u8bDataLen(buf);       // check length
u8cs data = u8bDataC(buf);        // get data as const slice
```

##  Why buffers?

ABC discourages small heap allocations and pointer-heavy structures.

  - Pointer overhead: if payload is 64 bits and you need two pointers
    per node, overhead exceeds data.
  - `malloc/free` bookkeeping is expensive.
  - Scattered heap data has no locality.
  - Serialization of pointer structures is complex.

Buffer-based structures (see [BIN][B], [HEAP][H], [HASH][D], [LSM][L])
allocate once, keep data contiguous, and serialize trivially.

[S]: ./S.md
[B]: ./BIN.md
[D]: ./HASH.md
[H]: ./HEAP.md
[L]: ./LSM.md
[N]: ./NEST.md
