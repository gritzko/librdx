#ifndef DOG_QURY_H
#define DOG_QURY_H

#include "abc/INT.h"

con ok64 QURYFAIL = 0x69e6e23ca495;
con ok64 QURYBAD  = 0x1a79b88b28d;

// Ref type after parsing.
#define QURY_NONE 0
#define QURY_REF  1   // branch/tag/ref name
#define QURY_SHA  2   // hex SHA prefix (>=6 hex chars, all hex)

#define QURY_MIN_SHA 6

// One parsed ref spec from a URI query.
//
// Grammar (single spec, between '&' separators):
//   spec     = path ancestry?
//   path     = seg ('/' seg)*
//   seg      = atom+ ('.' atom+)*
//   atom     = alnum | [_\-]
//   ancestry = ('~' | '^') digit*
//
// SHA vs REF decided at parse time: all hex and len >= 6 → SHA.
//
// Examples:
//   main                   REF
//   refs/tags/v2.8.6       REF
//   tags/gitgui-0.16.0     REF
//   HEAD~3                 REF, anc_type='~', ancestry=3
//   main^                  REF, anc_type='^', ancestry=0
//   a1b2c3d4               SHA
typedef struct {
    u8cs body;       // ref path or SHA hex (points into input)
    u8   type;       // QURY_NONE/REF/SHA
    u8   anc_type;   // '~' or '^' or 0
    u32  ancestry;   // N value (0 if bare ~/^)
} qref;

typedef qref *qrefp;
typedef qref const *qrefcp;

// Slice types for iteration.
typedef qrefp  qrefs[2];
typedef qrefcp qrefcs[2];

// Drain one ref spec from input.  Advances input past the
// parsed spec and any trailing '&' separator.
// Returns OK on success, QURYFAIL on bad syntax.
// On empty input, returns OK with out->type = QURY_NONE.
ok64 QURYu8sDrain(u8cs input, qrefp out);

#endif
