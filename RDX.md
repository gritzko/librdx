#   Replicated Data eXchange (RDX CRDT) library

Our goal here is to create a format and a library for data
replication using state-of-the-art Replicated Data Types.
Replicated Data eXchange format RDX is like protobuf,
but CRDT. Apart from [RPC][p] applications, one can use it for
data storage, distributed and asynchronous data exchange and in
other similar applications. RDX fully supports local-first,
offline-first and peer-to-peer replication, with no central
server required, as any two *replicas* can merge their data. By
installing RDX data types as merge operators in an LSM database
(leveldb, RocksDB, pebble, Cassandra, etc) one can effectively
have a CRDT database (which [Chotki][c] basically is).

RDX employs *unified* CRDTs able to synchronize using
operations, full states or deltas. Types may imply [causal
consistency][x] of updates in matters of performance, but their
correctness does not depend on that. RDX data types are fully
commutative, associative and idempotent. Hence, immune to
reordering or duplication of updates.

##  Data types

RDX has five primitive types (aka `FIRST`):

 1. Float: 64 bit IEEE known as `double` in most languages,
 2. Integer: `int64_t` little-endian, two's complement,
 3. Reference: 128-bit identifier, a Lamport time stamp,
 4. String: UTF-8,
 5. Term: `true`, `null` or suchlike.

On top of that, there are four `PLEX` types which allow for 
arbitrary nesting:

 1. x-Ples (tuples, triples, quadruples, etc),
 2. Linear collections (i.e. arrays),
 3. Eulerian collections (sets, maps), and 
 4. multipleXed collections (counters, version vectors).

The primary RDX form is the binary type-length-value (TLV)
format explained here. For explanatory purposes we will also 
use the "JSON-ish" [JRDX][j] text format specified elsewhere.

To specify data types fully, we describe each type's bitwise 
format, its ordering and merge rules and all the edit operations 
supported by it.

### `FIRST` Float, Integer, Reference, String, Term

The last-write-wins register is the simplest CRDT data type to
implement. For each LWW field, we only need the latest "winner"
value containing the logical timestamp and the value per se.

A logical timestamp is a pair `{rev, src}` where `rev` is the
revision number and `src` is the id of the author. Each part is 
limited to 64 bits (uint64_t, little-endian). Another name 
for this construct is "Lamport timestamp". 

Now, let's see how a bare (no TLV envelope) `I` int64 -11
would look like, assuming it is the 4th revision of the register
authored by replica #5. The integer value would look like 
`15` (hex). Here, the number -11 is [zig-zag][g] encoded and 
reduced (zipped) to its only one meaningful byte, 21 or 0x15.
The TLV of the timestamp would look like `02 08 05` which
is a length-prefixed record containing a zipped pair
of ints, 4 (signed, zig-zagged, so `08`) and 5 (unsigned, so `05`). 

The resulting TLV would look like `69 04 02 08 05 15`, where

 1. `69` is `i`, the record type Integer (T of TLV),
 2. `04` is the length (L of TLV),
 3. `02 08 05` is the timestamp, as explained above, and
 4. `15` is the value itself.

TLV records for all five `FIRST` types follow this format.
What differs is the type byte and value serialization.

String type is `S`, while values are simply UTF-8 strings. 
Term `T` is also a string except it is restricted to Base64.
'F' float records compact the value by byte-flipping and 
zipping it, so round floats like `0.0` or `0.25` take one 
byte (see `zip_int`/`ZINT` routines).
`R` reference values are timestamps, so packed as a zip-pair 
of a signed and unsigned integers.

Overlong encodings are forbidden both for strings and
for zip-ints! 

The text representation for `FIRST` types is as follows:

 1. `F` the standard e-notation, e.g. `12.34E+5`,
 2. `I` signed integer notation, e.g. `-123`,
 3. `R` 5-8-3 hex notation, e.g. `c187-3a62-12`,
 4. `S` double-quoted JSON-like, e.g. `"Sarah O'Connor"`,
 5. `T` unquoted Base64 string `[0-9a-zA-Z_~]`.

The only operation on a LWW register is to overwrite the value.
The new value should have a higher revision number.

The ordering rules are necessary for two reasons. First, to 
order `FIRST` values within a container type, e.g. keys in a map.
Second, if two competing values have the same revision number,
the tie is resolved using the value-order.
The `FIRST` *value-order* is as follows:

 1. for values of differing types, order alphabetically 
    (`F`, `I`, `R`, `S`, `T`),
 2. for values of the same type:
     1. `F` compare values numerically,
     2. `I` also numerically,
     3. `R` order by the value, then by the author,
     4. `S` alphanumeric, as in `strcmp(3)`,
     5. `T` alphanumeric.

Merge rules for LWW are straightforward:

 1. higher revision wins (*revision-order*),
 2. in case of a tie, use the *value-order* (higher wins),
 3. in case of a tie, use the *author-order* (higher replica id wins),
 4. in case of a tie, we look at the same value on both sides.

 This cascade of tie resolution we call the *LWW-order*.

### `PLEX` x-Ples, Linear, Eulerian and multipleXed collections

Collection types allow for arbitrary nesting and bundling of 
`FIRST` values and other collections.

#### Ples 

*x-Ples* are short fixed-order collections: tuples, triples, quadruples, and so on.
Those can look like `1:2`, a tuple of integers.
The corresponding TLV coding is `70 09 00 69 02 00 02 69 02 00 04` assuming zero 
timestamps on the tuple and each of the integers.
A triple of stings would look like `"Alice":"Bob":"Carol"` in J-RDX.
In TLV, that would be `70 16 73 06 00 41 6c 69 63 65 73 04 00 42 6f 62 ...`.
Again, all timestamps are zero.

The only way to edit a tuple is to replace an element.
The types may not match, e.g. we can replace `I` with `S`.
The new version should have a higher revision number.

Merge rules for a tuple are simple as each element has a fixed spot.
We compare two versions of a tuple element by element.
For each spot, we do an LWW comparison to determine the winner.
The result is the merged tuple.

Note that for `FIRST` types we compare two versions to pick the winner,
while for `PLEX` types we recur into the collection to merge it.

When we value-compare two tuples, the value order is defined by their first elements only.
That way, the first element serves as a *key* for the tuple, e.g. 
`{fist_name:"Sarah", last_name:"Connor"}` is correct.
The order of `P` tuples in a `E` set is defined by their first element, a `T` key.

#### Linear

*Linear* collections are essentially arrays. 
As with x-ples, the relative order of elements gets preserved on copy, conversion or merge.
But differently from x-ples, arrays can have elements inserted or removed.
We can edit `[1, 3, 4, 5]` to become `[1, 2, 3, 4]`.
So the order is preserved, but not fixed.

The TLV format is a `L` record containing a sequence of `FIRST` or `PLEX` records.
The order of the sequence is a *weave*, i.e. ops go in the same
order as they appear(ed) in the resulting array. Deleted ops 
change to tombstones, as their id flips to negative.

The underlying CRDT algorithm is the Causal Tree.
It is also known as Replicated Growable Array and under other names.
That means, each edit mentions explicitly the id of the location it applies to.
The merging procedure follows the tree-traversal logic. Any
change to an array must have a form of *subtrees*, each one
arranged in the same weave order, each one prepended with a `T`
op specifying its attachment point in the edited tree.
The particulars of this algorithm are described separately.

#### Eulerian

*Eulerian* collections are sets, maps and suchlike.
The order of their elements is not preserved.
Simply put, they always go sorted: `{"A", "B", "C"}`.
Set elements can be inserted, removed or replaced.

It can contain records with negative revision numbers. 
Those are tombstones (deleted
entries). For example, `I{4,5}-11` from the `FIRST` example
would go as `69 04 32 08 05 15`. Then, if replica #3 would want
to remove that entry, it will issue a tombstone op `I{-5,3}-11`
or `69 04 32 09 03 15`. Here, the version number changes from
`08` to `09` or 4 to -5, the author changes to 3.

Merging two versions of a set only requires one parallel
pass of those. That is very similar to a merge sort algorithm.
At each step, if the value-order of elements is the same (equal),
the merge algorithm recurs into the element. If elements differ,
the lesser one is included into the resulting collection.

#### multipleXed

*Multiplexed* collections are version vectors or counters.
In such a collection, each author's contribution is kept separately.
There is at most one contribution from each author: `(40@a1ec-3, 20@b0b-1)`.
Those can be added or updated.

The TLV representation is an `X` record containing `FIRST` and/or `PLEX`
element records.

### `L` Linear

Generic arrays store any `FIRST` elements. Internally, `L` are
Causal Trees (also known as Replicated Growable Arrays, RGAs).
The TLV format is a sequence of enveloped FIRST ops. 


Deletions look like `T` ops with negative revision numbers. As
an example, suppose we have an array authored by #3 `I{1,3}1
I{2,3}2 I{3,3}3` or `[1,2,3]` and replica #4 wants to delete the
first entry. Then, it issues a patch `T{1,3}T{-4,4}` that merges
to produce `I{1,3}1 T{-4,4} I{2,3}2 I{3,3}3` or `[2,3]`.

The text representation for an array is like `[1,2,3]`

##  Serialization format

We use the [ToyTLV](../protocol/tlv.go) format for enveloping/nesting all data.
That is a bare-bones type-length-value format with zero
semantics. What we put into ToyTLV envelopes is integers,
strings, and floats. Strings are UTF-8, no surprises. Floats are
taken as raw bits and treated same as integers. id64 is stored
as a compressed pair of integers.

A note on integer compression. From the fact that protobuf
has about ten integer types, one can guess that things can
be complicated here. We use [ZipInt](./zipint.go) routines to produce
efficient varints in a TLV format (differently from protobuf
which has a separate bit-level [LEB128][b] coding for ints). 

  - ZipUint64 packs an integer skipping all leading zeroes
  - ZipUint64Pair packs a pair of ints, each one taking 1,2,4 or
    8 bytes
  - ZipZagInt64 packs a signed integer using the zig-zag coding
  - ZipFloat64 packs a float (integers and binary fractions pack
    well)

id64 and logical timestamps get packed as pairs of uint64s. All
zip codings are little-endian.

##  Enveloping

RDX values can be bare, enveloped or double-enveloped. We use
bare values when we already know what field of what object we
are dealing with and what RDT it belongs to. That might be the
case when we read a value from a key-value storage where the key
contains object id, field and RDT. In such a case, a bare
Integer is like `{3,2}1` or `32 03 02 02`.

Within a network packet, that integer may need to be
single-enveloped: `I({3,2}1)` or `69 04 32 03 02 02` assuming
the other metadata is known from the context. 

A bare `ELM` or `NZ` value would only contain a sequence of
single-enveloped `FIRST` values. To make that single-enveloped
we only prepend a TLV header.

In case we also have to convey the rest of the metadata, namely
the object id and the field, we have to use the double-enveloped
form. For a simple `map[string]string{"Key":"Value"}` that
looks like: `M({b0b-af0-3} S({0,0}"Key") S({0,0}"Value"))` or
`6D 15  36 03 00 af 00 0b 0b  73 04 30 4b 65 79  73 06 30 56 61 6c 75 65`.
For `FIRST` values, there is no need to use two nested TLV
records, so a double-enveloped Integer looks like:
`I({b0b-af0-7}{3,2}1)`

Object/fields ids are serialized as tiny `ZipUint64Pair`s.
Revisions are serialized as tiny `ZipIntUint64Pair`s.

[x]: https://en.wikipedia.org/wiki/Causal_consistency
[v]: https://en.wikipedia.org/wiki/Version_vector
[r]: https://www.educative.io/answers/how-are-vector-clocks-used-in-dynamo
[d]: https://en.wikipedia.org/wiki/RDX
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[g]: https://protobuf.dev/programming-guides/encoding/
[b]: https://en.wikipedia.org/wiki/LEB128
[m]: https://en.wikipedia.org/wiki/Merge_sort
[c]: https://github.com/drpcorg/chotki
[j]: ./JRDX.md
