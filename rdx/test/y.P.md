`` `
This file tests tuple merge rules.
Tuples are very important in RDX as they are part primitives
part collections and, essentially, glue everything together.

The merge rules are simple... if you can learn rules of poker,
you can learn these rules too.

First of all, an empty tuple is the RDX value for "nothing".
As such, it can act as a tombstone or as a "null".
```
(() ())
():()
    ~
(() ())

1:2:3
1:():3
    ~
1:():3

```
Any element, including a tuple, can have a revision id attached.
Odd revisions count as tombstones (the element is deleted).
No explicit revision means the revision is 0.

A revision can be a sequence number (one int, preferred) or
a logical stamp consisting of two ints: *src* and *seq*,  e.g.
Alice-1b2. Stamps are rendered in RON Base64 so Alice and
Bob are ints like 1 or 2.

In the colon notation, the revision gets attached to the first
element of a tuple (there is no bracket and the first element
of a tuple can not be updated, hence can not be versioned).
```
1
2
    ~
2

1@2
2
    ~
1@2

1:2:4
1@2:2:3
    ~
1@2:2:3

```
The *id* an RDX element has can be seen as a revision id but
also as an object id. The distinction here is rather fluid,
very much like the distinction between time and space in
relativistic physics. If we want to, we can always merge
two objects exactly the same way we merge two versions of
an object. That is the reason we address those broadly as
"elements" not as "objects" or "versions".

For FIRST elements that distinction is moot as we can not
update them piecemeal. For PLEX collections, that is the
difference between inclusion by value and inclusion by
reference. Once a PLEX collection has a two-component id,
it has an *identity* so it can be included by reference.
When two distinct-identity PLEX elements are merged, it
is one or the other, and the higher id wins.
```
(@Alice-1st "Tweedledum" rattle:spoiled)
(@Alice-2nd "Tweedledee")
        ~
(@Alice-2nd "Tweedledee")

```
A tuple of one element, algebraically speaking, is the element
itself. Within RDX, a tuple of one is called an *envelope*.
Within collections, it orders exactly like its contained element
and its only purpose is to revision-stamp that element. That is
only *necessary* in the case if that element has an identity.
Then, the revision is put on the envelope, not on the element
itself, as changes to the outer collection do not affect such
an element; it "lives" at a different address.

```
1:2:3
(@2 1)
    ~
(@2 1)

1:2:3
(1)
(@1)
    ~
(@1)

{@Alice-seq name: "Alice"}
(@sez {@Alice-seq})
    ~
(@sez {@Alice-seq})

{@Alice-1st name: "Tweedledum"}
(@1st {@Alice-1st})
(@2nd {@Alice-2nd name: "Tweedledee"})
    ~
(@2nd {@Alice-2nd name: "Tweedledee"})

```
Revision 0 (no revision, no envelope or no stamp on an envelope)
is the original "intact" element. Odd revision numbers are put
on "tombstones", i.e. placeholders for deleted elements.
Even revision numbers express undeleted elements.
```
"string to be deleted"
(@1 "string to be deleted")
    ~
(@1 "string to be deleted")

(@1 "string to be undeleted")
(@2 "string to be undeleted")
    ~
(@2 "string to be undeleted")

```
The way tuples themselves are merged depends on their id.
Same id: recursive merge of the contents.
Different id: higher id wins unilaterally.

This rule applies to other PLEX elements as well.

So, for two distinct tuples, higher id wins:
```
(@Alice-1st name:"Tweedledum" rattle:spoiled)
(@Alice-2nd name:"Tweedledee")
    ~
(@Alice-2nd name:"Tweedledee")

```
One very special detail about tuples: their first element can not
be changed, so in case it differs, tuples also count as distinct
no matter what id they have.
```
1:2:3
3:2:1
    ~
3:2:1
```
For two *variants* of a PLEX element (same id), nested elements
will be merged, recursively. When merging specifically a tuple,
elements in the same position get merged.
```
(@1 {@Alice-1 "one":1 } )
(@1 {@Alice-1 "two":2 } )
    ~
(@1 {@Alice-1 "one":1 "two":2 } )

1:1:3
1:2:1
    ~
1:2:3

1:2:3
1:4:5
    ~
1:4:5

```
...now recursively:
```
1:(2,3)
1:(1,4)
1:(2,5)
    ~
1:(2,5)

```
For two different tuples (different id), the contents will not
be merged: newer id just wins. Again, we may understand the id
as an object id or as a version id, that does not matter.
```
( one:1 )
(@1 two:2 )
three:3
(@2 four:4)
    ~
(@2 four:4)

(@1 (@Alice-3 1 2 3))
(@3 (@carol-4 four five))
    ~
(@3 (@carol-4 four five))

(@carol-1 one two)
(@2 (@Alice-3 1 2 3))
    ~
(@2 (@Alice-3 1 2 3))

1@Bob-1:2
1@Bob-1:2:3
1@Bob-1:4
    ~
1@Bob-1:4:3

```
Note that in the colon notation the tuple id
is attached to the first element; in the bracket notation, to the
opening bracket.
```

1@Alice-1:2
1@Alice-2:2:3
1@Alice-1:4
    ~
1@Alice-2:2:3

1 @1
(@2 2, 2)
    ~
(@2 2, 2)

(@1 1, 2)
2@2
    ~
2@2

```
...there are improper cases of nested envelopes. Those have no
special meaning, only the outer envelope counts.
```

(@2 (@34 1 2 3 ))
(@3 1 22 33)
    ~
(@3 1 22 33)
