#ifndef DOG_FRAG_H
#define DOG_FRAG_H

#include "abc/INT.h"

con ok64 FRAGFAIL = 0x3db2903ca495;
con ok64 FRAGBAD  = 0xf6ca40b28d;

// Fragment type after parsing.
#define FRAG_NONE  0
#define FRAG_IDENT 1   // bare identifier: symbol or grep
#define FRAG_LINE  2   // line number/range only
#define FRAG_SPOT  3   // 'structural search'
#define FRAG_PCRE  4   // /regex/

#define FRAG_MAX_EXTS 8

// Parsed URI fragment.
//
// Grammar:
//   fragment = line_or_range ext*
//            | ident (':' line_or_range)? ext*
//            | "'" spot_body "'"? ext*
//            | "/" pcre_body "/" ext*
//
//   line_or_range = NUMBER ('-' NUMBER)?
//   ident         = [A-Za-z_][A-Za-z0-9_]*
//   spot_body     = (any except unescaped ')*
//   pcre_body     = ([^/\\] | '\\' any)*     (must be /-closed)
//   ext           = '.' [A-Za-z0-9]+
//
// Examples:
//   #MSETOpen           ident
//   #MSETOpen:42        ident + line
//   #MSETOpen:10-20     ident + range
//   #FILEFeedAll.c.cpp  ident + ext filters
//   #42                 line
//   #10-20              range
//   #'ok64 o = OK'      spot (closed)
//   #'ok64 o = OK       spot (unclosed, tolerated)
//   #'ok64 o = OK'.c    spot + ext
//   #/u8sFeed/          pcre (must be closed)
//   #/u8s\/Feed/.h      pcre with escaped /
//   #/u8sFeed/.c.h      pcre + ext filters
typedef struct {
    u8cs body;          // the search/symbol text (sans quotes/slashes)
    u32  line;          // line number (0 = not set)
    u32  line_end;      // range end (0 = not a range)
    u8cs exts[FRAG_MAX_EXTS];  // extension filters (sans dots)
    u8   nexts;         // count of ext filters
    u8   type;          // FRAG_NONE/IDENT/LINE/SPOT/PCRE
} frag;

typedef frag *fragp;
typedef frag const *fragcp;

// Parse a URI fragment string (the part after #, without the #).
// All slices point into the original input.
ok64 FRAGu8sDrain(u8cs input, fragp f);

// Percent-encode chars illegal in URI fragment (RFC 3986).
// Encodes: control chars (0x00-0x1F, 0x7F), non-ASCII (0x80+), '#', '%'.
// Everything else in printable ASCII passes through.
ok64 FRAGu8sEsc(u8s into, u8cs raw);

// Percent-decode %XX sequences.
ok64 FRAGu8sUnesc(u8s into, u8cs esc);

#endif
