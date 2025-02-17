#   BRIX: an RDX LSM syncable store

BRIX employs the [Replicated Data eXchange][R] format to build an embeddable
syncable document store with branching and other interesting features.
The internal format is [JSON-like][J], mapped to key-value store.

Usage examples: [brix CLI tool][C].

Further reading:

 1. [The project's mission][M]
 2. [The project's method or "why CRDT works (this time)?"][E]

[M]: ./MISSION.md
[E]: ./METHOD.md
[C]: ./README.cli.md
[J]: ../rdx/JDR.md
[R]: ../rdx/README.md
