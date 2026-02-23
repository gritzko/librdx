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
    $ xx hash '1e1'
    BLAKE256: 16c8bb58dffda8523a97834cfa8fcc2b7cd132edd4bccfde11729802784b7ae9
    $ xx hash '10.0'
    BLAKE256: 16c8bb58dffda8523a97834cfa8fcc2b7cd132edd4bccfde11729802784b7ae9

    # here we play with a set
    $ xx hash '{1 2 1 4 3 five}'
    BLAKE256: 8b534c0e309768fa57ebcc4028ce3c457836b5221da2ced1907c47baeadbe828
    $ xx hash '{five 1 2 3 4}'
    BLAKE256: 8b534c0e309768fa57ebcc4028ce3c457836b5221da2ced1907c47baeadbe828

    # here we play with subsets of that set
    $ xx merge '{1 2 3}' '{4 five}' | xx hash
    BLAKE256: 8b534c0e309768fa57ebcc4028ce3c457836b5221da2ced1907c47baeadbe828
    $ xx merge '{1 2 3}' '{4}' '{1 2 five}' | xx hash
    BLAKE256: 8b534c0e309768fa57ebcc4028ce3c457836b5221da2ced1907c47baeadbe828
````

The RDX data change model is defined through the merge operation:
updates get merged into the preexisting state, producing the resulting state.
The difference between "state", "update", "patch", or "operation" is the one
of quantity, not quality. Those are chunks of RDX, big and small.
To enable deterministic merges, RDX allows for element versioning metadata.

````
    # here we merge versions of a tuple
    $ xx merge '(1 2 4)' '(1 2 3@2 5)' '(1 2 4 5@1)'
    (1, 2, 3@2, 5@1)
    # odd revision numbers are tombstones (deleted elements)
    $ xx merge '(1 2 3@2 5)' '(1 2 4 5@1)' '(1 2 4 5)'
    (1, 2, 3@2, 5@1)

    # here we strip the metadata and the deleted element
    $ xx strip '(1 2 3@2 5@1)'
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

##  Iterator API

RDX provides a unified iterator interface for traversing documents across
all format variants (JDR text, TLV binary, SKIL indexed, etc). The core
abstraction is the `rdx` struct which serves as both parsed element and
iterator state.

Three operations define the traversal:

 1. `rdxNext(rdxp x)` — move horizontally to the next sibling element
 2. `rdxInto(rdxp child, rdxp parent)` — descend into a collection (PLEX)
 3. `rdxOuto(rdxp child, rdxp parent)` — ascend back to parent

Return codes: `OK` on success, `END` when exhausted, error codes otherwise.

````c
    // Iterate elements in a set
    rdx root = {.format = RDX_FMT_JDR};
    u8csFork(root.data, input);

    call(rdxNext, &root);           // position at root element

    rdx child = {};
    call(rdxInto, &child, &root);   // enter collection

    scan(rdxNext, &child) {         // iterate children
        // child.type has RDX_TYPE_*, child.i/f/s/r has value
    }

    call(rdxOuto, &child, &root);   // exit collection
````

For nested structures, use `rdxb` — a stack of iterators:

````c
    a_pad(rdx, stack, RDX_MAX_NESTING);
    rdxbFed1(stack);
    (**stack).format = RDX_FMT_JDR;
    u8csFork((**stack).data, input);

    call(rdxbNext, stack);          // uses top of stack
    call(rdxbInto, stack);          // pushes new iterator
    call(rdxbOuto, stack);          // pops iterator
````

The same API works for writing by setting `RDX_FMT_WRITE`:

````c
    rdx writer = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    u8sFork(writer.into, output_buffer);

    // Write a primitive: set type, value, then call Next
    writer.type = RDX_TYPE_INT;
    writer.i = 42;
    call(rdxNext, &writer);

    // Write a string
    writer.type = RDX_TYPE_STRING;
    writer.cformat = RDX_UTF_ENC_UTF8;
    u8csMv(writer.s, some_string);
    call(rdxNext, &writer);
````

For collections, write the container header, enter it, write children, exit:

````c
    // Write a tuple (key, value)
    writer.type = RDX_TYPE_TUPLE;
    writer.id = (id128){source, seq};  // optional metadata
    call(rdxNext, &writer);            // emit container header

    rdx child = {};
    call(rdxInto, &child, &writer);    // enter container

    child.type = RDX_TYPE_STRING;
    u8csMv(child.s, key);
    call(rdxNext, &child);             // emit key

    child.type = RDX_TYPE_STRING;
    u8csMv(child.s, value);
    call(rdxNext, &child);             // emit value

    call(rdxOuto, &child, &writer);    // close container
````

To seek within a collection, set the child's type and value before calling
`rdxInto`. For keyed containers (EULER, MULTIX), it positions at the first
element >= the search key. For positional containers (TUPLE, LINEAR), an
INT value seeks to that index position.

````c
    rdx child = {};
    child.type = RDX_TYPE_STRING;
    u8csMv(child.s, search_key);
    ok64 o = rdxInto(&child, &parent);  // positions at >= search_key

    // Verify exact match using the parent's comparator
    if (o == NONE || Z(&search, &child)) {
        // not found or passed it
    }
````

For write mode, a similar insert convention (optional to implement, depends
on format): set the child's type and value, call `rdxInto`. If a child with
that key exists, it positions on it; if not, it creates one and positions.

````c
    rdx child = {};
    child.type = RDX_TYPE_STRING;
    u8csMv(child.s, key);
    call(rdxInto, &child, &writer);    // find or create

    // Now positioned on existing or new child - write into it
````

Format conversion happens through `rdxMerge` which reads from input
iterators and writes to an output iterator, normalizing as it goes.
