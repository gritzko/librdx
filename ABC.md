#   Algebraic Bricklaying C

The archetypical approach to handling complexity is *layering of abstractions*.
That is good, to some degree, but, as the saying goes, 

 >  You can solve any problem by introducing an extra level of indirection
 >  (except for too many levels of indirection - [FToSE][f])

As a stack of abstractions grows, it inevitably becomes a shaky pile.
Eventually, we are left wondering whether all that makes any sense.

ABC is an experimental C dialect focused on handling complexity _in different ways_.
ABC goal is to make things:
 1. simple,
 2. durable,
 3. fit for the purpose.

Code constructs should be like bricks: small, predictable and arbitrarily composable.
In a way that you can put them in place correctly, then leave them alone for the next 100 years.

ABC strives for
  - determinism,
  - orthogonality and
  - minimalism.

ABC is greatly influenced by go, kernel C and [Jet Propulsion Lab C][j].
ABC sees C++ and Rust as ways to evolve C that earned us a lot of experience, hard.

##  Why C 

One of ABC objectives is durability. 
Instead of making an experimental language that will rot in 5 years, we focus narrowly on C.
C is our Latin, it will not go away anytime soon (if ever). 

The Linux kernel is a civilizational megaproject and is uses C. 
Same with Windows and MacOS kernels and much of the library tier (SSL, JPEG, what's not). 

Finally, ABC must be fit for the purpose of creating low-level system utilities. 
Hence, no GC, no runtime and no feature creep. C. 

##  The ABC method

Instead of encapsulation and versioning, well-specified formats and behavior.
Once the code is complete, it is complete, over.
Instead of layering, compose! Don't pile'em, line'em up!
Instead of "more features", 80/20. How many `ls` flags do you remember by heart?
Those are the real ones!
ABC constructs must be *orthogonal and composable*.
When you bring a banana, you don't drag in a monkey and half the jungle.

At the bottom, it is all about pointers, file descriptors and syscalls.
That is a great "hourglass waist" of systems programming, same as the IP protocol for the networks.
It separates the underworld of devices, firmwares and drivers from the upper world of applications.
One world above, another below, a narrow API between them.

Still, ABC discourages the manual use of pointers and any pointer arithmetics. 
ABC encourages the use of *slices* and other *star types*: buffers, iterators and so on.
Those are all pointers under the hood, but the usage patterns are predefined.
There are standard accessors which do bounds-checking if built with the right flags.

ABC avoids stacking of constructs, encapsulation and suchlike.
Once you deal with a file or a network socket, you always have that `int fd` in your hands.
You may use one or the other IO buffering or serialization system on top of syscalls.
Still, there is no encapsulation as it prevents composition. 
In C++, you can't `printf` to `std::ostream`, right?
In C++, `std::vector<char>` is all different from `std::string`,which in turn is different from `char*`. 
All that despite the obvious fact it is exactly the same thing under the hood.
We can't discourage that enough!

Constructs must be simple, practical, and most importantly: composable.
They must not build their own entirely separate universe.
One fitness metric for an ABC module is how many other modules it can be seamlessly used with.
*Seamless* means the absence of any specific adaptors; 
ideally, modules *don't know a thing* about each other
That is like UNIX toolbox taken to the extreme: minimalist composable single-purpose tools.
Those tools must have well-specified and *unchanging* behavior.
Do one thing, do it well, and once you did it, it is done, that's it.

##  ABC type system

ABC sees three main categories of data types:

 1. *Record types* with a fixed bit layout, e.g. `u64` or `uuid128`.
    These are our most basic "bricks".
 2. *Star types*, which are all pointers, e.g. a byte slice `u8**` aka `$u8`.
    Star types are like Go slices but taken to the extreme".
    Buffers are the most important ones, they *own* the memory.
    Slices and others *reference* memory. 
    Note that we only call it a "star type" if it contains _record_ types,
    so the bit layout is specified.
 3. *Other types* that may contain pointers or have unspecified layouts.
    These are your dirty underwear. 
    They should not appear in APIs.

Overall, ABC type system recommends well-specified bit layouts and solid containers (buffers).
No pointer-chasing, no tree nodes sprayed over the heap!
Transparent serialization comes as a free bonus.
Messy structs are OK as long as you keep them private.

### ABC memory management

Overall, ABC discourages the use of `malloc`/`free`.
One reason is their complex bookkeeping the other one is the unlimited potential for [very subtle failures][m].
Similarly, it highly discourages rug-pulling deep-in-the-call-stack reallocations STL is (in)famous for.
ABC prefers ring buffers, arena allocation and pre-calculated memory limits.
More on that in the [B][B] and [MMAP][M] module docs.

### Error handling

ABC uses the `Status` pattern seen in many C and C++ codebases.
All routines that can fail (_procedures_) should return an `ok64` error code (0 for OK).
`ok64` has a human-readable Base64 representation, e.g. `MMAPfail`.
The [PRO][P] module defines macros and routines for `ok64`-based wary calls, stack traces and suchlike.
Routines that can not fail (_functions_) return whatever value they return.
````
    pro(MODoutput, int fd) {
        sane(fd >= 0);
        aBpad(u8, outbuf, 1024);
        // ...write things to the buf
        call(FILEfeed, fd, Bu8data(outbuf));
        done;
    }
````

### ABC (de)serialization

Record and star types are trivially serializable.
Those can be file-mmapped and used that way (highly encouraged, see the [FILE][F] module).
The main shortcoming is their fixed bit-length.
For variable-length objects, there is a pretty standard-looking type-length-value (TLV) serialization, see the [TLV][T] module.
For holding variable-length object in the RAM, the arena pattern is preferred, see [AREN][A].
For integer compression (aka varints), see the [ZINT][Z] module.

### Containers

ABC containers are buffers, i.e. the API user has access to the raw bits.
That is made for extreme composability.
For example, there is no problem sending your hashmap over the network or file-mmapping it.
This is exactly what _orthogonality_ and _seamless composition_ mean in ABC!
See [HEAP][H] for a non-trivial but simple container example.

Note that ABC containers never do down-the-call-stack reallocations.
That is considered rug-pulling behavior as the caller may still hold pointers to the old range.
Instead, they may return `XYZnospace` errors.
Only the immediate _owner_ of the buffer can memory-manage it.
Typically, the owner is the procedure or a structure at the root of the call tree.
See the [B][B] module doc for the discussion on that.

[S]: ./$.md 
[A]: ./AREN.md
[B]: ./B.md
[F]: ./FILE.md
[I]: ./INT.md
[H]: ./HEAP.md
[M]: ./MMAP.md
[P]: ./PRO.md
[T]: ./TLV.md
[Z]: ./ZINT.md

[f]: https://en.wikipedia.org/wiki/Fundamental_theorem_of_software_engineering
[j]: https://yurichev.com/mirrors/C/JPL_Coding_Standard_C.pdf
[m]: https://www.qualys.com/2024/07/01/cve-2024-6387/regresshion.txt 
