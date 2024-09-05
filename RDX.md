#   Replicated Data eXchange (RDX CRDT) library

Our goal here is to create a format and a library for data
replication using state-of-the-art Replicated Data Types.
Replicated Data eXchange format ([RDX][j]) is like protobuf,
but CRDT. Apart from [RPC][p] applications, one can use it for
data storage, distributed and asynchronous data exchange and in
other similar applications. RDX fully supports local-first,
offline-first and peer-to-peer replication, with no central
server required, as any two *replicas* can merge their data. By
installing RDX data types as merge operators in an LSM database
(leveldb, RocksDB, pebble, Cassandra, etc) one can effectively
have a CRDT database (which [Chotki](./ARCHITECTURE.md) basically is).

We will implement *unified* CRDTs able to synchronize using
operations, full states or deltas. Types may imply [causal
consistency][x] of updates in matters of performance, but their
correctness does not depend on that. RDX data types are fully
commutative, associative and idempotent. Hence, immune to
reordering or duplication of updates.

The default syncing protocol (not described here) generally
relies on [version vectors][v]. Do not confuse that with [vector
clocks][r] used by Amazon Dynamo and similar systems. While
there are strong parallels, inner workings of VV and VC are not
identical.

##  Data types

Our objects can have fields of the following CRDT types. Each
type is named by a letter. 

 1. last-write-wins variables (`I` for int64, `S` for string, `F`
    is float64, and `R` is [id64](./id.go#12))
 2. counters, `N` increment-only uint64 and `Z` two-way int64
 3. maps (M), like key-value maps, where keys and values are `FIRST`
 4. sets (E), contain arbitrary `FIRST` elements
 5. arrays (L) of arbitrary `FIRST` elements
 6. version vectors (V)
 7. codegen

The format and the merge rules are as follows.

### `FIRST` Float, Integer, Reference, String, Term

The last-write-wins register is the simplest data type to
implement. For each LWW field, we only need the latest "winner"
op containing the logical timestamp and the value per se. A
logical timestamp is a pair `{rev, src}` where `rev` is the
revision number and `src` is the id of the author. For example,
let's see how a bare (no TLV envelope) `I` int64 `-11` would
look like, assuming it is the 4th revision of the register
autored by replica #5. The TLV would look like: `32 08 05 15`
(hex) where `0x15` is a [zig-zag][g] encoded and zipped `-11`,
while `32 08 05` is a tiny [ToyTLV](../protocol/tlv.go) record for a zipped pair
of ints, 4 (signed, zig-zagged, so `08`) and 5 (unsigned, so
`05`). If we add a ToyTLV envelope, that becomes `69 04 32 08 05
15` (type of record `I`, length 4, then the bare part).

String `S` values are simply UTF-8 strings. Int64 `I`, float64
`F` and id64 `R` values get compressed using [`zip_int`](./zipint.go)
routines. Overlong encodings are forbidden both for strings and
for zip-ints! 

`T` ops have a timestamp, but no value. That is the equivalent
of a `nil` or `void` value. Those are used as placeholders in
various cases.

The text representation for `FIRST` types is as follows:

 1. `F` the e-notation, JSON-like
 2. `I` signed integer notation,
 3. `R` 5-8-3 hex notation (e.g. `c187-3a62-12`)
 4. `S` double-quoted JSON-like, e.g. "Sarah O\'Connor"
 5. `T` null

Merge rules for LWW are straighforward:

 1. higher revision wins
 2. in case of a tie, higher value wins (like bytes.Compare())
 3. in case of a tie, who cares, but higher replica id wins

### `NZ` Counters

`N` are increment-only counters. Their TLV state is a sequence
of `T` records containing zipped uint64 pairs {val,src}, the
counter value and source replica id. As the counter is inc-only,
we may use the value itself as a revision number. The merge
operator is per-replica `max`, as later versions are greater.
The native value is the sum of all replica values (sum of
contributions).

`Z` are two-way counters (inc/dec). Their TLV format is a
sequence of `I` records each having `{rev,src}` metadata as
described in the `FIRST` section. One record corresponds to one
source, per-source merge rules are same as LWW. The native value
is the sum of all `I` values.

### `E` Eulerian

Generic sets containing any `FIRST` elements. The TLV format is
a sequence of enveloped FIRST records. It can contain records
with negative revision numbers. Those are tombstones (deleted
entries). For example, `I{4,5}-11` from the `FIRST` example
would go as `69 04 32 08 05 15`. Then, if replica #3 would want
to remove that entry, it will issue a tombstone op `I{-5,3}-11`
or `69 04 32 09 03 15`. Here, the version number changes from
`08` to `09` or 4 to -5, the author changes to 3.

Within a set, the ops are sorted in the *value order*. Namely,
if the type differs, they go in the alphabetical order (`F`,
`I`, `R`, `S`, `T`). If the type is the same, they go in the
ascending order, as per `strcmp` or `bytes.Compare`. That way,
merging multiple versions of a set only requires one parallel
pass of those, no additional allocation or sorting, very much
like [mergesort][m] works.

The text representation of a set is like `{1,2,3}` where 
`1,2,3` are `FIRST` elements of the set.

### `M` Mapping

Generic maps, mapping any `FIRST` value to any other `FIRST`
value. The TLV format is a sequence of enveloped key-value op
pairs. Any update should also contain the affected key-value
pairs. Deleted entries might have `T` values (the key is
present, the value is null) or the key might have a negative
revision (no such key present).

Pairs are sorted in the value-order of their keys. When merging
two pairs having an identical value of their keys, both the key
and the value ops are merged according to the LWW rules. As with
`E` sets, this only requires one parallel pass of the versions.

The text representation for a map is like `{4:null, "key":"value"}`

### `L` Linear

Generic arrays store any `FIRST` elements. Internally, `L` are
Causal Trees (also known as Replicated Growable Arrays, RGAs).
The TLV format is a sequence of enveloped FIRST ops. The
order of the sequence is a *weave*, i.e. ops go in the same
order as they appear(ed) in the resulting array. Deleted ops 
change to tombstones, same as E.

The merging procedure follows the tree-traversal logic. Any
change to an array must have a form of *subtrees*, each one
arranged in the same weave order, each one prepended with a `T`
op specifying its attachment point in the edited tree.

Deletions look like `T` ops with negative revision numbers. As
an example, suppose we have an array authored by #3 `I{1,3}1
I{2,3}2 I{3,3}3` or `[1,2,3]` and replica #4 wants to delete the
first entry. Then, it issues a patch `T{1,3}T{-4,4}` that merges
to produce `I{1,3}1 T{-4,4} I{2,3}2 I{3,3}3` or `[2,3]`.

The text representation for an array is like `[1,2,3]`

### `V` Version vector

[Version vector][v] is a way to track dataset versions in a
causally ordered system. It is a vector of `seq` numbers, where
each `seq` is the version of the state as seen by each
respective replica. Alternatively, that is a map `{src: seq}`,
where `src` is the replica `id`. It is assumed, that we received
updates from replica `src` all the way up to `seq`.

Bare TLV for a version vector is a sequence of `V` records (yes,
`V` nested in `V`) each containing one id64 as a zipped seq-src
pair (see ZipUint64Pair). The sequence is sorted in the
ascenting order of record bytes, like `bytes.Compare()`.

The merge algorithm for version vectors is simple: take the
maximum `seq` for each `src`. Note that `seq=0` is distinct from
having no record.

##  Data type implementation

To fully implement an RDT one has to implement these 10
functions. The function name starts with the type name letter,
here we imply `I` last-write-wins int64.

````go
    // Xvalid verifies validity of a bare TLV record.
    // Any other function may assume the input is valid.
    func Ivalid(tlv []byte) bool 


    // Xstring converts a TLV representation into a string.
    func Istring(tlv []byte) (txt string) 

    // Xparse converts a string back into bare TLV.
    // Must round-trip with Xstring.
    func Iparse(txt string) (tlv []byte) 


    // Xtlv converts the native type into a TLV, zero metadata.
    func Itlv(i int64) (tlv []byte)

    // Xnative converts TLV into the native value.
    // Must round-trip with Xtlv.
    func Inative(tlv []byte) int64


    // Xdelta produces a TLV value that, once merged with
    // the old TLV value using Xmerge, will produce the new
    // native value using Xnative. Returns nil if none needed.
    // This function we need to *save changes* from a native
    // object/struct into RDX.
    func Idelta(tlv []byte, new_val int64) (tlv_delta []byte) 

    // Xmerge CRDT-merges several bare TLV values into the
    // resulting one. For example, given two I records
    // {3,8}15 and {4,1}44 will return {4,1}44 as version 4 is
    // newer than version 3.
    func Imerge(tlvs [][]byte) (tlv []byte) 

    // Xdiff produces a TLV delta given a TLV value and a
    // version vector of suspected changes (may skip this).
    func Idiff(tlv []byte, vvdiff VV) (tlv []byte)
````

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
[j]: https://en.wikipedia.org/wiki/RDX
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[g]: https://protobuf.dev/programming-guides/encoding/
[b]: https://en.wikipedia.org/wiki/LEB128
[m]: https://en.wikipedia.org/wiki/Merge_sort
