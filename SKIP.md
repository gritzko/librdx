  # SKIP log

This module turns any binary stream into a skiplog. 
A skip log is an append-only log where payload entries are interleaved with backward-pointing skiplist entries.
That way, one can skip to any record in the log in a logarithmic number of hops.
It is assumed that there is a monotonous metric defined on the records.
For example, records can be timestamped (syslog), sorted (LSM) or numbered (WAL).
There could be multiple such metrics at the same time, no biggie.
Or there can be none.
Then, we can search for valid record start positions without rescanning the entire log.

 ## The skiplist

SKIP tries to insert a skiplist entry into every `2**g` byte segment.
With such "road sign" records at regular intervals, we can do a binary-search,
then maybe scan to the exact spot.
An average SKIP entry is 5 bytes for `g=>8` and u16 offsets or 3 bytes for `g<8` and u8 offsets.
Hence a `g=9` skiplog creates about 1% space overhead while a more fine-grained `g=6` adds <5% to the stream.
`g=9` is nice because a typical disk sector these days is more than `2**9=512` bytes.
That way, the scanning stage will not trigger any additional disk reads, normally.

One may wonder, how a skipllog entry can be 5 bytes "on average".
First, SKIP is a logarithmic skiplist, so 1/2 entries contain 1 offset, 1/4 have 2 and so on.
Also, it does not store the log position or the offset to the entry (that would probably be 8 bytes).
It only stores the offset within the byte segment, so 1 or 2 bytes is enough.

 ## The ABC way

Note that SKIP implies nothing specific about entry format or semantics.
It can be mixed with any record formats and any monotonous metrics.
SKIP entries per se use [ToyTLV][T], although that is easy to tweak too.
Extreme composability without abstraction layering!

[T]: ./TLV.md
