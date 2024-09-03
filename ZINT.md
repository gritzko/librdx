#   "Zipped" integers

A somewhat more performant TLV-centric variation of variable 
length integers, aka. "varints". No need for LEB128, as the 
length is already mentioned in the TLV header.

Packs `u64` as 0, 1, 2, 4 or 8 bytes. Zig-zags and packs `i64`.
Can pack a pair of `u64` as a single record, 0+0, 1+0, 1+1,
1+2, etc.
