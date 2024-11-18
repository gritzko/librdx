  # Syncing in RDX

The [Replicated Data eXchange][R] format for easy consistent syncing.
Here, "consistent" means plenty of algebra, starting with Conflict-free Replicated Data Types (CRDTs).
When RDX replicas sync, they sync precisely to the last bit and they do it very fast!

This repo contains the C RDX universe:

  - the [ABC C dialect][A], aka Algebraic Bricklaying C,
  - the [RDX format][R] per se, binary and text varieties,
  - an [RDX storage engine][C] (LSM type, like Level or Rocks),
  - a generic [RDX syncing client/server][Z],
  - an RDX based [revision control system][Z], and
  - a [handbook][B] of basic use cases for the new users.

[A]: ./abc/README.md
[R]: ./rdx/README.md
[C]: ./cho/README.md
[Z]: ./zync/README.md
[F]: ./fork/README.md
[B]: ./book/README.md
