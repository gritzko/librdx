  # J-RDX text format (RDX-jsonish)

J-RDX is a human-readable variant of RDX.
The [binary RDX][r] variant is [TLV-based][t].
J-RDX is a superset of [JSON][j] that anyone familiar with JSON can easily read.
But, J-RDX has somewhat deeper foundations than JSON.
Naturally, there is 1:1 mapping between TLV-RDX and J-RDX.

RDX types come in two groups: `FIRST` and `PLEX`.

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

*x-Ples* are short fixed-order collections: tuples, triples, quadruples, and so on.
These are `1:2` or `"Alice":"Bob":"Carol"`.
The only way to edit a tuple is to replace an element.

*Linear* collections are essentially arrays. 
As with x-ples, the relative order of elements gets preserved on copy, conversion or merge.
But differently from x-ples, arrays can have elements inserted or removed.
We can edit `[1, 3, 4, 5]` to become `[1, 2, 3, 4]`.
So the order is preserved, but not fixed.

*Eulerian* collections are sets, maps and suchlike.
The order of their elements is not preserved.
Simply put, they always go sorted: `{"A", "B", "C"}`.
Set elements can be inserted, removed or replaced.

*Multiplexed* collections are version vectors or counters.
In such a collection, each author's contribution is kept separately.
There is at most one contribution from each author: `(40@a1ec-3, 20@b0b-1)`.
Those can be added or updated.

 ## Nesting

The power of `PLEX` elements is the ability to nest them arbitrarily.
For example, a map is an Eulerian collection of tuples:
`{color:"orange", is_fruit:true, name:"orange"}`
Yep, a map is not a primitive!

Sets can host tuples or arrays and vice-versa:
`{absent:["Bob","Carol"], present:["Alex"]}`, 
`[{name:"Alex",age:32}, {name:"Bob",age:40}]` and so on.

As you may see, any JSON document is a valid J-RDX.
Except maybe for revision-related `R` and `M`, J-RDX primitives are same as JSON has or even simpler.
The trick is, they combine better.

 ## Revisioning

Note that each `FIRST` or `PLEX` element *can* be `@` stamped with a 128-bit revision id.
RDX is a versioned data format, so every element is versioned too, e.g. `(40@a1ec-3, 20@b0b-1)`.
In the example above, `a1ec` made three edits to the counter contributing 40. 
`b0b` made one contributing 20. 
That resulted in the counter value of 60.

We can merge versions of an RDX object or produce a patch for any two of them.
Revision control is the main feature of RDX.

[j]: http://json.org
[r]: ./RDX.md
[t]: ./TLV.md
