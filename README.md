  # Syncing in RDX

The [Replicated Data eXchange][R] format for easy consistent syncing.
Here, "consistent" means lots of algebra, starting with Conflict-free Replicated Data Types ([CRDTs][C]).
When RDX replicas sync, they sync precisely to the last bit and they do it very fast!

This repo contains the C RDX universe:

  - the [ABC C dialect][A], aka Algebraic Bricklaying C,
  - the [RDX format][R] per se, binary and text varieties,
  - a generic [RDX syncing store][B],
  - an RDX based [revision control system][F], and
  - a [handbook][B] of basic use cases for the new users.

[A]: ./abc/README.md
[C]: http://crdt.tech
[R]: ./rdx/README.md
[B]: ./brix/README.md
[F]: ./fork/README.md
[B]: ./book/README.md
