#   Replicated Data eXchange (RDX CRDT)

RDX is a format for data versioning and replication using state-of-the-art Replicated Data Types.
RDX can be used for data storage, distributed and asynchronous data exchange and in other applications.
RDX supports versioning, diff/patch and merges (commutative, associative, idempotent).
No central server is required, as RDX fully supports local-first, offline-first and peer-to-peer replication.
Yes, any two *replicas* can merge their changes, like `git` does.
RDX data types can serve as merge operators in an LSM database (leveldb, RocksDB, pebble, Cassandras).
That way, one can effectively have a CRDT database for free (which [Chotki][c] basically is).

RDX has a binary/[TLV][t] and a text/[JSON-done-right][j] variants.

The difference from [Automerge][a] JSON CRDT is the change of direction:

  * Automerge adapted CRDTs to implement "mergeable JSON", while
  * RDX retrofitted JSON and LSM semantics to make them fit CRDT.

RDX employs *unified* CRDTs able to synchronize using

 1. operations and op logs,
 2. full states or
 3. deltas (patches).

RDX types may imply [causal consistency][x] of updates in matters of performance.
Still, their correctness does not depend on that.
RDX data types are fully commutative, associative and idempotent.
Hence, RDX is fully immune to reordering or duplication of updates.


##  Data types

RDX has five "primitive" `FIRST` types:

 1. Float: 64 bit IEEE float known as `double` in most languages,
 2. Integer: `int64_t` little-endian, two's complement,
 3. Reference: 128-bit identifier, a Lamport time stamp,
 4. String: UTF-8,
 5. Term: `true`, `null` and suchlike.

On top of that, there are four `PLEX` "bracket" types which allow for
arbitrary nesting:

 1. x-Ples (tuples: doubles, triples, quadruples, etc),
 2. Linear collections (arrays, editable texts),
 3. Eulerian collections (ordered sets, maps), and
 4. multipleXed collections (counters, version vectors).

The primary RDX format is the binary type-length-value (TLV) one explained here.
The "JSON done right" [RDX-JDR][j] text format is specified elsewhere.
We will use it too, for explanatory purposes.

To specify data types fully, we describe each type's

 1. bitwise format,
 2. ordering rules,
 3. edit operations supported, and
 4. merge rules.


### `FIRST` Float, Integer, Reference, String, Term

A last-write-wins register is the simplest CRDT data type to implement.
It is also the most popular type in practical use.
For each LWW element, we only pick its latest "winner" value.
An RDX element of any type has a logical timestamp attached.
So, picking the latest revision is straightforward.

A logical timestamp, also known as Lamport timestamp, is a pair `rev, src`, where
  * `rev` is the revision number (in our case, 64 bits, unsigned),
  * `src` is the id of the author (64 bits, unsigned).

The least significant bit of the revision number is used as a *tombstone* flag.
As in many MVCC and distributed systems, deleted elements can stay around.
To signal the element is no longer effective, its revision number becomes odd.
Any normal revision numbers are even, 0 being the default value.
Naturally, revision numbers must increase monotonically.
Other than that, RDX does not prescribe any particular logical clock algorithm.

Now, let's see how an  `I` register would look like in TLV.
RDX uses the [ToyTLV][t] format to serialize all data.
Every element is a ToyTLV key-value record, revision id being the "key".
Suppose, the `int64` value is -11 and it is revision 4 authored by replica #5.

 1. `I` values are [zig-zag][g] coded, so -11 becomes 21 or `15` in hex,
 2. the timestamp is a pair 4,5, so `04 05`,
 3. as a ToyTLV key-value record, that becomes `69 04 02 04 05 15`,
    where `69` or "i" is the record type, `04` is the record length
    and `02` is the key/timestamp length, value length thus being 4-1-2=1.

That is 6 bytes for a fully boxed and revision controlled integer.
TLV records for all five `FIRST` types follow this same format.
What differs is the record type and value serialization.

  * `F` float records [zip][z] the value bits, so round floats like `0.0` or `0.25` take one byte;
  * `I` integer payloads are zig-zaged and zipped, as shown above;
  * `R` reference record payloads are timestamps pair-zipped as shown above;
  * `S` string records payloads are simply raw UTF-8 strings;
  * `T` payloads are Base64 strings.

`PLEX` types use the same [TLKV][t] format, value being all their nested elements.
Overlong encodings are forbidden both for UTF-8 strings and for zip-ints.
There must be only one correct way to serialize a value.

#### Ordering

The ordering rules are necessary for two reasons.
First, to order `FIRST` values within a container type, e.g. keys in a map.
Second, to resolve a tie when two competing versions have the same revision number.
The `FIRST` *value-order* is as follows:

 1. for values of differing types, use the type-alphabetical order
    (`F`, `I`, `R`, `S`, `T`),
 2. for values of the same type:
     1. `F` compare values numerically,
     2. `I` also numerically,
     3. `R` order by the value, then by the author,
     4. `S` alphanumeric, as in `strcmp(3)`,
     5. `T` alphanumeric.

#### Edits

The only operation on a LWW register is to overwrite the value.
The new record should have a higher revision number.

#### Merge

Merge rules for LWW are straightforward:

 1. higher revision wins (*revision-order*),
 2. in case of a tie, use the *value-order* (higher wins),
 3. in case of a tie, use the *author-order* (higher replica id wins),
 4. in case of a tie, we look at the same value on both sides.

 This cascade of tie resolution we call the *LWW-order*.


### `PLEX` x-Ples, Linear, Eulerian and multipleXed collections

Collection types allow for arbitrary bundling and nesting of `FIRST` values and other collections.

#### x-Ples

*Tuples* are short fixed-order collections: couples, triples, quadruples, and so on.
Those can be as simple as `1:2`, a couple of integers.
The corresponding TLV coding is `70 09 00 69 02 00 02 69 02 00 04` assuming zero
timestamps on the tuple and each of the integers.
A triple of stings would look like `"Alice":"Bob":"Carol"` in J-RDX.
In TLV, that would be `70 16 73 06 00 41 6c 69 63 65 73 04 00 42 6f 62 73 06 00 43 61 72 6f 6c`.
Again, if all four timestamps are zero.

Like all other `PLEX` types, the TLV is a key-value record containing TLV key-value records of elements.

#### Ordering

When we value-compare two tuples, the value order is defined by their first elements only.
That way, the first element serves as a *key* for the tuple.
For example, `{fist_name:"Sarah", last_name:"Connor"}` is correct,
as the order of `P` tuples in a `E` set is defined by their first element, a `T` key.

When comparing to `FIRST` elements, the order is again defined by the first element of a tuple.
When comparing to another tuple, the same.

When comparing to `LEX` elements, the type-alphabetical, then the revision id order is used.

#### Edits

The ways to edit a tuple is to replace or to append an element.
The types may not match, e.g. we can replace `I` with `S`.
The new version of an element should have a higher revision number as per LWW rules.

The first element (the key) can not be changed.
The only way to change it is to replace the entire tuple by one with a higher revision id.
The revision id of the key is always the same as the one of the tuple.

#### Merging

The merge rules for a tuple are simple as each element has a fixed spot.
We compare two versions of a tuple element by element.
For each spot, if both versions are `PLEX` elements of a matching type and id, we recurse inside it.
Otherwise, we do an LWW order comparison to determine the winner.
The result is the merged tuple.

As an object of a merge, a tuple behaves like other `PLEX` types.
If the other version is also a tuple with the same revision id, the merge recurses into both.
In other cases, it is treated same as `FIRST` types, based on the LWW order.

### Linear

*Linear* collections are essentially arrays.
As with x-ples, the relative order of elements gets preserved on edit, conversion or merge.
But differently from x-ples, arrays can have elements inserted or removed.
For example, we can edit `[1, 3, 4, 5]` to become `[1, 2, 3, 4]`.
So the order is preserved, but not fixed.

The TLV format is an `L` record containing a sequence of `FIRST` or `PLEX` child records.
The order of that sequence is a *weave*.
That means, ops go in the same order as they appear(ed) in the resulting array.
Deleted elements are kept for revision control purposes.
Those change to tombstones by flipping their ids to negative.

#### Ordering

When ordering relative to elements of other types, the order is type-alphabetical.
Otherwise, linear collections are ordered according to their id.

#### Edits

Insertions and removals are supported, often generalized as a *splice* operation.

#### Merging

When merging two versions of the same `L` collection, the algorithm is Causal Tree CRDT.
It is also known as Replicated Growable Array and under other names.
That means, each edit mentions explicitly the id of the location it applies to.
The merging procedure follows the tree-traversal logic.
Any change to an array must have a form of *subtrees*.
Each subtree is arranged in the same weave order,
and specifying its attachment point in the edited tree.
The particulars of this algorithm are described separately.

As an object of a merge, `L` behaves as other `PLEX` types.
If merged with a version of itself, the algorithm recurses (CT CRDT).
Otherwise, it is treated according to the LWW merge rules.

### Eulerian

*Eulerian* collections are sets, maps and suchlike.
The order of their elements is not preserved.
Simply put, they always go sorted: `{"A", "B", "C"}`.
A map is a set containing key-value couples.

#### Ordering

As an element, a set obeys the default `PLEX` ordering:

 1. type-alphabetical,
 2. in case of a tie, revision-order.

Elements of a set use the value order.

#### Edits

Set elements can be inserted, removed or replaced.

Removal is done by a tombstone, a record with an odd revision number.
For example, `-11@5-4` from the `FIRST` example would go as `69 04 02 04 05 15`.
Then, if replica #3 would want to remove that entry, it will issue a tombstone.
That would be `-11@3-5` or `69 04 02 05 03 15`.
Here, the version number changes from `04` to `05`, the author changes to 3.
The tombstone will take the place of the original element in the set.

Replacement is done by adding an element that is equal in the value-order,
but is higher in the revision order.

Consider a set element `remarks:"need recheck"`.
It may be replaced by `remarks@b0b-2: none`.
Or it can be removed by `remarks @b0b-1` (a tombstone).

#### Merging

Merging two versions of a set is very similar to the [merge sort][m] algorithm.
It only requires one parallel pass of both collections.
At each step, if the value-order of elements is the same (equal),
the merge algorithm recurses into the elements. If elements differ,
the lesser one is included into the resulting collection and the
respective iterator is advanced.

### multipleXed

*Multiplexed* collections are version vectors, counters, etc.
In such a collection, each author's contribution is kept separately.
There is at most one contribution from each author: `(40@a1ec-3, 20@b0b-1)`.
Those can be added or updated.

The TLV representation is an `X` record containing `FIRST` and/or `PLEX`
element records.

#### Ordering

As an element, the standard `PLEX` ordering, type-alphabetical then revision order.

`X` elements go in the author order (ascending).

#### Edits

The two ways to edit `X` is to create or update *your* element.
The new version should have a higher revision number (for `FIRST`).
For `PLEX`, LWW replace is done by using a higher revision number.
Edits are done according to the type's rules.

#### Merging

`X` merge is done by a parallel pass.
Same as in `E`, but in the author id order.

Elements get merged normally.
For non-matching types, LWW-order determines the winner.
Otherwise, the merge is done according to the type's rules.

[a]: http://google.com/?q=automerge
[b]: https://en.wikipedia.org/wiki/LEB128
[c]: https://github.com/drpcorg/chotki
[d]: https://en.wikipedia.org/wiki/RDX
[g]: https://protobuf.dev/programming-guides/encoding/
[j]: ./JDR.md
[m]: https://en.wikipedia.org/wiki/Merge_sort
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[r]: https://www.educative.io/answers/how-are-vector-clocks-used-in-dynamo
[t]: ../abc/TLV.md
[v]: https://en.wikipedia.org/wiki/Version_vector
[x]: https://en.wikipedia.org/wiki/Causal_consistency
[z]: ../abc/ZINT.md
