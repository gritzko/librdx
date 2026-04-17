#ifndef DOG_DOG_H
#define DOG_DOG_H

#include "abc/URI.h"

// Git object types (from packfile format)
#define DOG_OBJ_COMMIT 1
#define DOG_OBJ_TREE   2
#define DOG_OBJ_BLOB   3
#define DOG_OBJ_TAG    4

// Parse a URI string with dog-specific normalization:
//   1. Invoke abc/URILexer for strict RFC 3986 parsing.
//   2. If the parsed URI has a scheme but no authority and its
//      path has no leading slash (i.e. the text is `word:path...`
//      rather than `proto://host/path`), treat the scheme as a
//      remote alias: move scheme → authority.
//
// Rationale: users routinely type `localhost:src/git/protocol.h`
// or `origin:docs/README.md`.  Per RFC 3986 this parses as
// scheme="localhost", path="src/git/protocol.h".  For the dogs,
// the first token before a bare `:` almost always names a remote,
// not a protocol.
//
// True URIs with protocol schemes (`https://`, `file:///`) are
// unaffected — they have leading-`/` paths or populated authority.
ok64 DOGParseURI(urip uri, u8csc text);

#endif
