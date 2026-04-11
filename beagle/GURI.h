#ifndef BEAGLE_GURI_H
#define BEAGLE_GURI_H

#include "abc/URI.h"

con ok64 GURIFAIL = 0x1e3d23ca495;
con ok64 GURIBAD  = 0x79348b28d;

// A git URI is a standard URI with git-specific conventions:
//
//   [//authority] [/path] [?ref] [#search]
//
// The uri struct from abc/URI.h IS the representation.  Git-specific
// meaning is extracted by introspection functions below.  No extra
// fields, no stored flags — just the parsed URI.

typedef uri guri;
typedef uri *gurip;
typedef uri const *guricp;

// Parse a git URI from a string slice.
// Accepts paths, //remote/path?ref#search, full GitHub URLs, etc.
ok64 GURIu8sDrain(u8cs input, gurip g);

// --- Introspection: extract git-specific parts from the parsed URI ---

// The remote authority (stripped of leading "//").
// Empty if local.
void GURIRemote(u8csp out, guricp g);

// The repo path — portion of the URI path before ".git/" boundary.
// Empty if no .git/ found.
void GURIRepo(u8csp out, guricp g);

// The file path within the repo — portion after ".git/" or the
// entire path if no .git/ boundary.  Leading "/" stripped.
void GURIPath(u8csp out, guricp g);

// The git ref (branch, tag, SHA, range) — the ?query part.
fun void GURIRef(u8csp out, guricp g) { $mv(out, g->query); }

// The search spec (line, grep, spot, regex) — the #fragment part.
fun void GURISearch(u8csp out, guricp g) { $mv(out, g->fragment); }

// File extension from the path (without dot), via abc/PATH.
void GURIExt(u8csp out, guricp g);

// --- Boolean introspection ---

fun b8 GURIHasRemote(guricp g) { return !$empty(g->authority); }
fun b8 GURIHasRef(guricp g)    { return !$empty(g->query); }
fun b8 GURIHasSearch(guricp g) { return !$empty(g->fragment); }

// .git/ boundary found in path?
b8 GURIHasRepo(guricp g);

// Path present and non-empty (after stripping leading / and .git/ prefix)?
b8 GURIHasPath(guricp g);

// Path is a bare .ext filter like ".c"?
b8 GURIIsExtFilter(guricp g);

// Path ends with /?
b8 GURIIsDir(guricp g);

// Ref contains ".." (but not "...")?
b8 GURIIsRange(guricp g);

// Search type classification.
#define GURI_SEARCH_NONE  0
#define GURI_SEARCH_LINE  1   // numeric or L-range
#define GURI_SEARCH_GREP  2   // bare text
#define GURI_SEARCH_SPOT  3   // 'quoted'
#define GURI_SEARCH_REGEX 4   // /slashed/

u8 GURISearchType(guricp g);

#endif
