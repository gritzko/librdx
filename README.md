#   LibRDX: Replicated Data eXchange format

Today's popular data formats are either "one-way" or "RPC".
"One-way" is something like CSV or JSON.
I can dump my data into such format, send it to somebody, never see it again.
If they send me back my JSON with some changes, that might be a problem.
["RPC"][p] formats are likes of Protobuf, Capnproto, etc.
Those are "I tell you, you tell me" formats to exchange pre-agreed messages.

RDX is a versioned format for **data synchronization**.
It is designed to send around data patches.

RDX is the greatest-common-denominator data format.
Systems should be able to import/export data from/to RDX,
be it the entire state or separate patches.

RDX has a formal *mutation semantics*.
In other words, the [spec][X] says exactly what the result should be if we merge A and B.

RDX is a very algebraic data format on the inside.
First, it employs [CRDTs][T], Commutative Replicated Data Types.
That means, any two versions of an object can *merge deterministically*.
Second, its basic constructs are neatly orthogonal and *arbitrarily composable*.
For example, `{"a":"map"}` is not a primitive, it is `{"a","set"}` `"of":"tuples"`.
Tuples `"can":"be":"standalone"`, or they `<"can":"nest">:<"in":"other">:"tuples"` or `[{"any"}, <"other":"containers">]`.

RDX has a text variant [RDX-JDR][J] "JSON Done Right".
There is also a binary variant [RDX-TLV][X] which is the most straightforward [Type-Length-Value][L].
Variants correspond 1:1.

`librdx` is a low-level [RDX][X] system library implemented in [ABC C][A].

[A]: ./ABC.md
[X]: ./RDX.md
[J]: ./RDXJ.md
[c]: http://github.com/drpcorg/chotki/rdx
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[T]: http://en.wikipedia.org/wiki/CRDT
[L]: https://en.wikipedia.org/wiki/Type-length-value
