#   Integers

ABC defines `u8..u64, i8..i64`, slices and buffers thereof.
As a bonus, there are `u128` and `i128` defined as `u64`,
`i64` pairs.

Integers are *entry types*, i.e. their bit layout is fixed,
they must serialize and hash the same on every platform.
The bit layout is defined as two's complement little-endian.
