#   Lines

*Lines* are TLV records each one having a 128 bit id.
There are no assumptions about the structure of records.
Similarly, ids can be Lamport timestamps or UUIDs or 
anything that fits into 128 bits (hence no hashes).
Ids are [ZINT](ZINT.md) compressed.
