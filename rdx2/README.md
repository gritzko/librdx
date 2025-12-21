#   Replicated Data eXchange format (RDX)

RDX is a syncable data format that is very similar to JSON in look and feel,
but has a formal data model and a formal mutation model, so any implementations
can interpret the data identically and keep replicas in perfect sync with no
global consensus.

RDX is influenced by JSON, relational model, LSM key-value stores and CRDTs,
and it can express these data models at least on the 80/20 basis. The RDX
formal model can guarantee that RDX implementations map 1:1 to one another.
The mutation model can guarantee that the same update merged to the same data
produces the same result, irrespectively of the implementation or the format
variant being used. Compare that to e.g. JSON where parsers may diverge in
interpretation and note that no "binary JSON" format maps to JSON 1:1.

````
    # 10.0 is 10.0 irrespectively of the notation
    $ rdx hash '1e1'
    e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    $ rdx hash '10.0'
    e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

    # here we play with a set
    $ rdx hash '{1 2 1 4 3 five}'
    629243be731e5348220fc07bb6caf48a37996ee8e752074d34320dd70701c850
    $ rdx hash '{five 1 2 3 4}'
    629243be731e5348220fc07bb6caf48a37996ee8e752074d34320dd70701c850

    # here we play with subsets of that set
    $ rdx merge '{1 2 3}' '{4 five}' | rdx hash
    629243be731e5348220fc07bb6caf48a37996ee8e752074d34320dd70701c850
    $ rdx merge '{1 2 3}' '{4}' '{1 2 five}' | rdx hash
    629243be731e5348220fc07bb6caf48a37996ee8e752074d34320dd70701c850
````

The RDX data change model is defined through the merge operation:
updates get merged into the preexisting state, producing the resulting state.
The difference between "state", "update", "patch", or "operation" is the one
of quantity, not quality. Those are chunks of RDX, big and small.
To enable deterministic merges, RDX allows for element versioning metadata.

````
    # here we merge versions of a tuple
    $ rdx merge '(1 2 4)' '(1 2 3@2 5)' '(1 2 4 5@1)'
    (1, 2, 3@2, 5@1)
    # odd revision numbers are tombstones (deleted elements)
    $ rdx merge '(1 2 3@2 5)' '(1 2 4 5@1)' '(1 2 4 5)'
    (1, 2, 3@2, 5@1)

    # here we strip the metadata and the deleted element
    $ rdx strip '(1 2 3@2 5@1)'
    (1, 2, 3)
````

An RDX documents consists of *elements*. Five primitive elements types are:

 1. Float: 64 bit IEEE float known as `double` in most languages,
 2. Integer: `int64_t` little-endian, two's complement,
 3. Reference: 128-bit identifier, a Lamport time stamp,
 4. String: UTF-8,
 5. Term: `true`, `null` and suchlike.

On top of that, there are four collection types which allow for
arbitrary nesting:

 1. x-Ples (tuples: doubles, triples, quadruples, etc),
 2. Linear collections (arrays, editable texts),
 3. Eulerian collections (ordered sets, maps), and
 4. multipleXed collections (counters, version vectors).

RDX makes no attempt to innovate on the syntax; everything is JSON like.
For the FIRST elements, the notation is identical to JSON. Similarly,
Linear and Eulerian collections use the same brackets, `[]` and `{}`.
JSON has rudimentary tuples, like `"key":"value"`; RDX generalizes that
to an arbitrary number of elements, e.g. 
`TEL555230975:"John":"Smith":"johnsmith@gmail.com"`.
RDX tuples not only turn Eulerian sets into maps; they also express 
relational records. Alternatively to the colon notation, RDX tuples 
can go in brackets like the other collection types:
`(TEL555230975, "John", "Smith", "johnsmith@gmail.com")`.
Finally, multiplexed collections can store version vectors, distributed
counters and similar types. They use angled brackets `<>`.

Overall, the syntax is trying to follow the rule of least surprise.

The last, but not the least important aspect of RDX is the *invariants*.
For all the format varieties, there is a guarantee that any valid document
in one version of the format can be converted into another version and back
with no corruption. For example, the JSON-like text format to the in-memory
representation in whatever language and back. That is the *round-trip*
guarantee.
With the merge operation, the invariants are pretty standard: idempotence,
commutativity, associativity. Namely, having RDX documents `A, B, C` and `+` 
being the merge operation, it is guaranteed that:

 1. `A+A = A`
 2. `A+B = B+A`
 3. `A + (B + C) = (A + B) + C`

 These invariants are easy to formalize, analyze, prove, test and fuzz.
