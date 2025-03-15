#   RDX Command Line Utility

`rdx` is a toolbox utility for raw RDX data.
It converts between RDX and JDR, merges, produces patches, and so on.

For example, this pipeline invocation parses JDR into RDX, merges two
versions and converts the result back into JDR.
````
    $ echo "<1,2>,<1,1,3>" > 123.jdr
    $ rdx parse 123.jdr, merge, print
    <1,2,3>
````


##  Print JDR

JDR is the JSON-like text form of RDX.
Differences are many, but JDR is a superset of JSON, so any JSON is
also JDR and if you stay careful about features, your JDR is JSON.

````
$ rdx print doc.rdx
````


##  Parsing

````
$ rdx parse test.jdr, write test.rdx
````

The `tlv` subcommand parses JDR into binary RDX.
Parsing can normalize the RDX, e.g. reorder entries in `PLEX` collections.
One way to "canonize" your JDR is to round-trip it to RDX and back to JDR.

````
$ rdx parse handwritten.jdr, print normalized.jdr
````

##  Merging

RDX is all about merging versions of objects, versions of datasets.
Specifically `rdx` can only understand standalone RDX objects, so
it can only merge versions of one object.

````
    $ cat test.jdr
    {1:2}
    {1@2-2:6}
    {eight}
    $ cat test2.jdr
    {3:4,4:5,"seven"}
    $ rdx merge test.jdr test2.jdr, print
    {<@2-2 1:6>,3:4,4:5,"seven",eight}
````


##  Stripping

As RDX is a versioned data format, an RDX document normally has
some amount of metadata: tombstones, revision numbers, ids.
Stripping produces a clean JDR/JSON document with no metadata:
````
    $ rdx print doc.rdx
    {"key": 123@4, <"dead key">}
    $ rdx strip doc.rdx, print
    {"key": 123}
````
Any changes to that document would have to be imported back into
RDX using the `diff` command.


##  Diffing

This one can produce a standalone patch for two versions of an object.
The first version MUST have full metadata or the patch may turn ineffective.
The second version can be stripped. Formally, for versions `A`, `B`:
`diff(A, B) = P` so `strip(merge(A,P))==strip(B)`.

````
    $ cat A.jdr
    {1:2 eight}
    $ cat B.jdr
    {1:1 3:4 4:5 "seven" eight}
    $ rdx diff A.jdr B.jdr, write P.rdx
    $ rdx print P.rdx
    {1:1@2,3:4,4:5,"seven"}
    $ rdx merge A.jdr P.rdx, write B2.rdx
    $ rdx print B2.rdx
    {1:1@2 3:4 4:5 "seven" eight}
````


##  Deltas


##  Testing

RDX test files are both RDX and valid Markdown at the same time.
They contain extensive comments to explain intricacies of RDX.
That trick keeps the docs in the testing/CI loop.

To process test files nicely, there is `rdx test` command, e.g.

````
    rdx test y.E.md

````
