# ABC Header Index

| Header | Description |
|--------|-------------|
| 01.h | Basic types and macros foundation (u8, u16, u32, u64, fun, con, slice types) |
| ABC.h | Main umbrella header that includes all major ABC modules |
| ANSI.h | ANSI terminal color and text formatting codes |
| AREA.h | Memory arena allocation with typed buffer carving |
| AREN.h | Arena alignment macros for 32-bit and 64-bit allocation |
| B.h | Buffer type system with mmap support and error codes |
| BIN.h | Logarithmical bins for aligned intervals (tail111 encoding) |
| BSD.h | In-memory bsdiff/bspatch implementation (no compression, no malloc) |
| BUF.h | Buffer manipulation functions and typed buffer instantiations |
| Bx.h | Buffer template for generating typed buffer operations |
| COMB.h | Combined buffer format with magic header and offset tracking |
| CURL.h | libcurl wrapper with POL integration for async HTTP requests |
| DIFF.h | Linear-space Myers diff algorithm for slices |
| DIFFx.h | Myers diff template for typed slice diffing |
| FILE.h | POSIX file operations wrapper with error handling |
| HASH.h | Hash table error codes and constants |
| HASHx.h | Hash table template with linear probing and convergence |
| HEAPx.h | Binary heap template for priority queues |
| HEX.h | Hexadecimal encoding/decoding functions |
| HEXx.h | Hex encoding template |
| HTTP.h | HTTP request/response parser with SAX-style callbacks |
| HTTP.rl.h | Ragel-generated HTTP lexer enums and callbacks |
| INT.h | Integer types, comparison functions, and slice helpers |
| INTx.h | Integer template stub |
| JSON.h | JSON SAX parser with event handlers |
| JSON.rl.h | Ragel-generated JSON lexer compatibility stub |
| KV.h | Key-value pair types (kv32, kv64) with hash and compare functions |
| LEX.h | Lexer framework with ragel support and code generation templates |
| LIST.h | Doubly-linked list with prev/next indices |
| LISTx.h | List template for typed list operations |
| LSM.h | Log-structured merge (LSM) iterator with multi-way merge |
| MMAP.h | Memory-mapped buffer operations (open, close, resize) |
| MMAPx.h | Mmap template for typed memory-mapped buffers |
| NACL.h | Libsodium wrapper for Ed25519 and BLAKE2b cryptography |
| NET.h | Network address utilities and socket helpers |
| NEST.h | Nested context tracking with insert/splice operations |
| NUM.h | Number-to-English-words conversion (64-bit ready) |
| OK.h | Error code system with RON60 encoding and string conversion |
| PACK.h | LZ4 compression wrapper for page-based storage |
| PAGE.h | Paged buffer with lazy load/flush callbacks |
| PATH.h | UTF-8 path manipulation with segment handling |
| POL.h | Event poller with file descriptor and timer tracking |
| POLL.h | Polling framework with read/write buffer management |
| PRO.h | Procedural macros (call, done, sane, try, fail) for error handling |
| RAP.h | rapidhash wrapper for fast non-cryptographic hashing |
| ROCK.h | RocksDB C API wrapper with error handling |
| RON.h | RON Base64 encoding and ron60 type definitions |
| S.h | Slice types and macros ($head, $term, $len, etc.) |
| SAN.h | Namespace sanitizer that undefines ABC macros |
| SHA.h | SHA-256 hashing via libsodium |
| SKIP.h | Skiplist error codes and TLV type constant |
| SKIPx.h | Skiplist template with block-based indexing |
| SLOG.h | Structured log with skip records for seeking |
| SORT.h | Sorting utilities (u64 sort) |
| SST.h | Sorted string table (SST) file format with header |
| SSTx.h | SST template for typed sorted string tables |
| Sx.h | Slice template for generating typed slice operations |
| TCP.h | TCP socket operations (listen, connect, accept, close) |
| TEST.h | Testing macros (want, same, TEST) for table-driven tests |
| TLV.h | Type-Length-Value encoding with variable-length support |
| TTY.h | Terminal styled output with RGB colors and padding |
| UDP.h | UDP socket operations (bind, connect, send, receive) |
| URI.h | URI parser with scheme, authority, path, query, fragment extraction |
| URI.rl.h | Ragel-generated URI lexer enums and callbacks |
| UTF8.h | UTF-8 encoding/decoding and validation |
| Y.h | Y-merge framework error codes and heap instantiation |
| Yx.h | Y-merge template for multi-way merge operations |
| ZINT.h | Variable-length integer encoding (1, 2, 4, or 8 bytes) |
| rapidhash.h | Fast platform-independent hash algorithm (based on wyhash) |
