#   SKIP log

A skiplog is an append-only log where payload entries are interleaved with backward-pointing skiplist entries.
That way, one can skip to any record in the log in a logarithmic number of hops.
The overhead of the skiplist is negligible thanks to some smart optimizations.

So, we can write a log normally, then we can search in that log, fast.
For example, records can be timestamped (syslog), sorted (LSM) or numbered (WAL);
a skiplist can use any such monotonous metric.
There could be multiple such metrics at the same time, no biggie.
Or there can be none, in case we only need valid record offsets.
No need to rescan the log, no need for a separate database or index.


##  The skiplist

SKIP tries to insert a skiplist entry into every `2**g` byte segment.
With such "road sign" records at regular intervals, we can do a binary-search,
then maybe scan to the exact spot.
An average SKIP entry is 5 bytes for `g=>8` and u16 offsets or 3 bytes for `g<8` and u8 offsets.
Hence a `g=9` skiplog creates about 1% space overhead while a more fine-grained `g=6` adds <5% to the stream.
`g=9` is nice because a typical disk sector these days is more than `2**9=512` bytes.
That way, the scanning stage will not trigger any additional disk reads, normally.
`g=6` is even nicer as it leads you to the correct CPU cache line.

One may wonder, how a skiplog entry can be 5 bytes "on average".
First, SKIP is a logarithmic skiplist, so 1/2 entries contain 1 offset, 1/4 have 2 and so on.
Also, it does not store the log position or the offset to the entry; 
that would take 4- or 8-byte offsets.
It only stores the offset within the byte segment, so 1 or 2 bytes is enough.

##  The ABC way

Note that SKIP implies nothing specific about entry format or semantics.
It can be mixed with any record formats and any monotonous metrics.
SKIP entries per se use [ToyTLV][T], although that is easy to tweak too.
Extreme composability without abstraction layering!

[T]: ./TLV.md
