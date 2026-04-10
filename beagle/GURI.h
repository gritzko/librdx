#ifndef BEAGLE_GURI_H
#define BEAGLE_GURI_H

#include "abc/URI.h"

con ok64 GURIFAIL = 0x1e3d23ca495;
con ok64 GURIBAD  = 0x79348b28d;

// A parsed git URI.  Built on top of abc/URI, decomposing the
// standard URI components into git-meaningful parts:
//
//   [//authority] [/path] [?ref] [#search]
//
// Where:
//   authority = host/owner/repo.git  OR  remote alias
//   path      = repo-relative file or directory
//   ref       = branch, tag, SHA, HEAD~N, range (main..feat)
//   search    = line number, line range, grep/spot/regex pattern
//
// The .git/ boundary in the path splits the repo address from the
// file path within the repo.  Without .git/, the entire path is
// a repo-relative file path (local repo assumed).
//
// Examples:
//   abc/MSET.h                          local file
//   abc/MSET.h?main                     file at ref
//   abc/MSET.h?main#42                  line 42 at ref
//   //origin/abc/MSET.h?feat            file on remote at ref
//   //github.com/gritzko/dogs.git/abc/MSET.h?main
//   .c#TODO                             grep .c files for TODO
//   .c#'f(x,y)'                         structural search
//   .c#/u\d+sFeed/                      regex search
//   ?main..feat                         commit range (no path)
//   ?HEAD~3                             relative ref
//   //origin/                           remote root

typedef struct {
    uri base;           // parsed standard URI

    u8cs remote;        // //authority part (host/owner/repo or alias)
    u8cs repo;          // repo path portion (up to .git/ if present)
    u8cs path;          // file path within repo
    u8cs ref;           // ?query = git ref (branch, tag, SHA, range)
    u8cs search;        // #fragment = search/line spec

    u8cs ext;           // file extension from path (e.g. "c" from ".c")

    // Introspection flags
    b8 has_remote;      // //authority present
    b8 has_repo;        // .git/ boundary found in path
    b8 has_path;        // file path present
    b8 has_ref;         // ?ref present
    b8 has_search;      // #search present
    b8 is_ext_filter;   // path is a bare .ext (like ".c")
    b8 is_dir;          // path ends with /
    b8 is_range;        // ref contains ".." (commit range)
} guri;

typedef guri *gurip;
typedef guri const *guricp;

// Parse a git URI string into a guri struct.
// The input can be: a bare path, a full URI with //, or any mix.
// All slices in the result point into the original `input` data.
ok64 GURIu8sDrain(u8cs input, gurip g);

// Serialize a guri back to a URI string.
ok64 GURIu8sFeed(u8s into, guricp g);

// Search type classification for the #fragment part.
#define GURI_SEARCH_NONE  0   // no search
#define GURI_SEARCH_LINE  1   // numeric: line number or L-range
#define GURI_SEARCH_GREP  2   // bare text: substring grep
#define GURI_SEARCH_SPOT  3   // 'quoted': structural search
#define GURI_SEARCH_REGEX 4   // /slashed/: regex search

// Classify the search fragment.
u8 GURISearchType(guricp g);

#endif
