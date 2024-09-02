#   Algebraic Bricklaying C

ABC is an experimental C dialect focused on handling complexity *in different ways*.
The archetypical approach to handling complexity is *layering of abstractions*.
That is good, to some degree, but, as the saying goes, 
 >  You can solve any problem by adding a layer of abstraction,
 >  except for too many layers of abstraction.
As a stack of abstractions grows, it inevitably becomes a shaky pile.
Eventually, we are left wondering whether all that makes any sense.

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

Instead of encapsulation and versioning, well-specified formats and behavior.
Once the code is complete, it is complete, over.
Instead of layering, composability. Don't pile, line'em up!
Instead of "more features", 80/20. How many `ls` flags do you remember by heart?
Those are the real ones!

ABC is greatly influenced by go, kernel C and JPL C.

##  Abstraction stacking

While such languages as C++ and Rust evolve C by stacking
layers of abstraction, ABC rejects stacking altogether. 

##  The mission

ABC constructs 
do not stack, they line up.

ABC is durable. Instead of making an experimental language that
will rot in 5 years, we focus narrowly on C. C is our Latin, it
will not go away anytime soon. The Linux kernel is a civilizational
megaproject, is uses C. Same with Windows and MacOS kernels. 

Finally, ABC is fit for the purpose: creating low-level system
utilities. No GC. No feature creep. We do work from here to
there, we have no opinion about higher layers of the stack.

##  The ABC method

It is all about pointers and file descriptors.
That is a great "hourglass waist" on par with the IP protocol.
One world above, another below, a narrow API between them.
Still, ABC discourages the manual use of pointers and any pointer 
arithmetics. There are accessors which do bounds-checking with
the right flags. ABC encourages the use of *slices* and other
*star types*: buffers, iterators and so on. Those are all
pointers under the hood.

ABC avoids stacking of constructs, encapsulation and suchlike.
In C++, `std::vector<char>` is all different from `std::string`,
different from `char*`. We can't discourage that enough!
Some higher-level languages make a file descriptor different 
from a socket descriptor. That is abstraction stacking, we 
do not want that.

ABC constructs must be *orthogonal and composable*!!! When you 
bring a banana, you don't drag in a monkey and half the jungle.

MUJI
doing just that, doing it well, not consuming any attention
building its own universe


minimalist one-purpose tools with well-specified unchanging behavior
UNIX toolbox

depending on the order of calls 
don't know about each other

### ABC type system

ABC type system recommends:

 1. Entry types with *fixed bit layout*, e.g. `u64` or
    `uuid128`. These are trivially serializable.
 2. Handler types that may contain pointers, e.g. `line`.
 3. Star types, which are all pointers, e.g. `u8**`.
    Star types are like "Go slices taken to the extreme".
    Buffers are the most important ones, they own the memory.
    Slices and other star-types reference memory. 

ABC prefers ring buffers and arena allocation.
The use of the heap is discouraged. Separate small allocations
are discouraged.
More on that in the [B document][B].

[B]: ./B.md

### The modules

 1. [$](S.md) slices and everything related,
 2. [B](B.md) buffers and everything related,
 3. [PRO](PRO.md) defines macros for ABC *procedures*.
 4. [INT](INT.md) u8..u64, i8..i64,
 5. [HEAP](HEAP.md) is the most basic container, a binary heap.
 6. [TLV](TLV.md) type-length-value serialization,
 7. ...



