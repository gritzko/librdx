#   RDX-JDR format or "JSON done right"

RDX-JDR is a human-readable variant of RDX, the Replicated Data eXchange format.
The [binary RDX][r] variant is [TLV-based][t].
RDX-JDR is a superset of [JSON][j] that anyone familiar with JSON can easily read.
Naturally, there is 1:1 mapping between RDX-TLV and RDX-JDR.

RDX-JDR has somewhat deeper foundations than JSON.
It has a formal mutation semantics, i.e. edits, merges and deltas.

RDX element types come in two groups: `FIRST` and `PLEX`.
`FIRST` types are basic atomic building blocks:
 1. Float `12.34E+5` (64-bit),
 2. Integer `123` (64-bit signed),
 3. Reference `b0b-37e2` (128-bit ids in hex),
 4. String `"Hello world!"` (UTF-8), and
 5. Term, e.g. `true`.

`PLEX` types allow for composition and nesting:
 1. x-Ples `1:2`,
 2. Linear `["a", "b", "c"]`,
 3. Eulerian `{false, true}` and
 4. multipleXed `(1@b0b-0, 2@a1ec-3)` collections.

*x-Ples* or *tuples* are short fixed-order collections: couples, triples, quadruples, and so on.
These are `1:2` or `"Alice":"Bob":"Carol"`.
The only way to edit a tuple is to replace an element.
A tuple can optionally be enclosed in angled brackets.
That is only necessary if we nest tuples, e.g. `"Corned Beef" : <0.25:kg> : <3.45:EUR>`.

*Linear* collections are essentially arrays.
As with tuples, the relative order of elements gets preserved on copy, conversion or merge.
But differently from tuples, arrays can have elements inserted or removed.
We can edit `[1, 3, 4, 5]` to become `[1, 2, 3, 4]`.
So the order is preserved, but not fixed.

*Eulerian* collections are sets, maps and suchlike.
The order of their elements is not preserved.
Simply put, they always go sorted: `{"A", "B", "C"}`.
Set elements can be inserted, removed or replaced.

*Multiplexed* collections are version vectors, counters and suchlike.
In such a collection, each author's contribution is kept separately.
There is at most one element contributed by each author: `(20@b0b-1, 40@a1ec-3)`.
Elements can be inserted or updated by the respective author.

Note the `@` notation.
Every RDX element, `FIRST` or `PLEX`, has a revision id attached.
A revision id is a 128-bit logical time stamp, same as `R`.
The author identifier takes 64 bits, another 64 is the revision number ("time").

##  Nesting

The power of `PLEX` elements is the ability to nest them arbitrarily.
For example, a map is an Eulerian collection of tuples:
`{color:"orange", is_fruit:true, name:"orange"}`
Yep, a map is not a primitive!

Sets can host tuples or arrays and vice-versa:
`{absent:["Bob","Carol"], present:["Alex"]}`,
`[{name:"Alex",age:32}, {name:"Bob",age:40}]` and so on.

As you may see, any JSON document is a valid RDX-JDR.
Except maybe for revision-related `R` and `X`, RDX-JDR constructs are same as JSON has or even simpler.
The trick is, they combine better!

##  Revisioning

Note that each `FIRST` or `PLEX` element *can* be `@` stamped with a 128-bit revision id.
RDX is a versioned data format, so every element is versioned too, e.g. `(20@b0b-2, 40@a1ec-6)`.
In the example above, `a1ec` made four additions to the counter contributing 40.
`b0b` made two contributing 20.
That resulted in the counter value of 60.
Now suppose we merge that with `(25@b0b-4, 32@a1ec-4)`
The first version of the counter has a newer version from `a1ec`.
The other version has newer data from `b0b`.
The result would be `(25@b0b-4, 40@a1ec-6)` and the counter value is now 65.

We can merge versions of an RDX object or produce a patch for any two of them.
Revision control is the main feature of RDX.

[j]: http://json.org
[r]: ./RDX.md
[t]: ../abc/TLV.md
