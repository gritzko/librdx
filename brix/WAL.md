#   Write Ahead Log

BRIX WAL is a very basic RDX based Write Ahead Log implementation
that draws some ideas from BitCask. Namely, the log also has an
in-memory index so any entries can be looked up if necessary.
Hence, a WAL is a small database by itself.

Once the log (or the index) overfills, the data has to be flushed
somewhere (SST).
