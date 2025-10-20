#   "Zipped" integers

A performant TLV-centric variation of variable length integers, aka. "varints".
Varints are necessary as real-world integers tend to be small.
Spending the full 8 bytes per integer might be too generous.
The fact that [protobuf][g] has about ten integer types hints at certain complexity here.
While protobuf per se is a TLV format, ints use a separate bit-level [LEB128][b] coding.
[LEB128][b] uses a flag bit in each byte.
If the flag is 1, there is one more byte; flag 0 means it is the last byte.

ABC is TLV-centric, so there is no need for LEB128.
Normally, the length is already mentioned in the TLV header.

  - `ZINTu64feed/drain` packs an integer skipping the upper zero bytes,
  - `ZINTu128feed/drain` packs a pair of ints, each one taking 1,2,4 or 8 bytes,
  - `ZINTu8sFeedInt/drain` packs a signed integer using the zig-zag coding,
  - `ZINTu8sFeedFloat/drain` packs a float (integers and binary fractions pack well).

All zipints are little-endian.

[b]: https://en.wikipedia.org/wiki/LEB128
[g]: https://protobuf.dev/programming-guides/encoding/
