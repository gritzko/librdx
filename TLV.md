#   Type-Length-Value binary coding

TLV is the most widely used approach to constructing binary
protocols. Protobuf is TLV under the hood, as well as many
others. This is an implementation of ToyTLV coding. The baseline
ToyTLV is like:

 1. Every record type is a letter `A..Z` (one byte),
 2. the length is a 32-bit little-endian number (4 bytes),
 3. the rest is the value of the specified length.

There are additionally 3 grades of header compaction (small, 
tiny and micro records) to make shorter records efficient.

ToyTLV does not define any semantics (ints are no different
from strings, etc). It is an *enveloping* format only.
