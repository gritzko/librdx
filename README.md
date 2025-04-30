#   Syncing in RDX

The building blocks define the properties of the resulting system, to a large degree.
Think LEGO vs Meccano, or IP packets vs Ethernet frames, or logs vs bricks.
The [Replicated Data eXchange][R] format is made for easy and consistent syncing.

**Technically**, RDX is [CRDT][C] based (Conflict-free Replicated Data Types).
RDX defines all the commodity data structures: maps/sets, vectors/lists, registers/counters etc, all to be naturally syncable and mergeable.

**Conceptually**, RDX cancels the assumption of a "single source of truth" (like in SQL dbs) as well as the assumption of "requesting the transient data" (like in HTTP REST). There is an event's origin, but all the event's data can be saved anywhere. In an RDX network, every node is both a "cache" (in the HTTP sense) and a "branch" (in the git sense).

**Historically**, RDX is a successor of many past projects, including [Swarm.js][s] and [RON][n]. There are experimental projects using RDX, e.g. the [Chotki][o] database which recently ran out of 32-bit counters in production use.

**Pragmatically**, RDX replicas sync precisely to the last bit and they do it very fast!
You can build anything with RDX. 

**Structurally**, one can see RDX as a superset of JSON. That is a good mental model to start experimenting.

This repo contains a low-level RDX implementation as well as different systems built with RDX as a proof-of-concept.

  - the [ABC C dialect][A], aka Algebraic Bricklaying C,
  - the [RDX format][R] per se, binary and text varieties,
  - a generic [RDX syncing store][B],
  - an RDX based [revision control system][F] (not here yet),
  - a [handbook][B] of basic use cases for the new users (NHY), and
  - the project's [blog][l].

[A]: ./abc/README.md
[C]: http://crdt.tech
[R]: ./rdx/README.md
[B]: ./brix/README.md
[F]: ./fork/README.md
[B]: ./book/README.md
[l]: ./blog/README.md
[s]: http://github.com/gritzko/swarm
[n]: http://github.com/gritzko/ron
[o]: http://github.com/drpcorg/chotki
