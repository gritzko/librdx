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
//   3. Non-numeric "ports" (`ssh://host:src/...` — `src` isn't a
//      port) get glued back onto the front of the path.
//   4. If the path's head-segment contains `@` (`user@host/rest`),
//      promote that prefix to authority/host/user.
//
// Rationale: users routinely type `localhost:src/git/protocol.h`
// or `origin:docs/README.md`.  Per RFC 3986 this parses as
// scheme="localhost", path="src/git/protocol.h".  For the dogs,
// the first token before a bare `:` almost always names a remote,
// not a protocol.
//
// True URIs with protocol schemes (`https://`, `file:///`) are
// unaffected — they have leading-`/` paths or populated authority.
//
// Path convention for remote transports (ssh/https/etc.): the
// path is always treated as **relative to the user's home on the
// remote**.  `ssh://host:src/repo` means `~/src/repo` on `host`.
// No `~` expansion in the grammar; no `/absolute/path` escape.
// Absolute local paths must use `file:///abs/path`.
ok64 DOGParseURI(urip uri, u8csc text);

// Canonicalise a parsed URI for ref-key comparison: feed
// `//<authority><path>[?<query>]` into `out`.  Transport scheme
// (ssh:, https:, git:) is dropped — `ssh://host/x` and
// `https://host/x` produce the same key.  Bare `host:x` (DOGParseURI
// already moved scheme→authority) gets the leading `//` added.
// When `with_query` is YES the `?<query>` tail is included; NO
// strips it (useful to build an origin-only key).  Inputs without
// host/authority pass through via `path` only (no leading `//`).
ok64 DOGCanonURIKey(u8bp out, urip u, b8 with_query);

#endif
