#ifndef DOG_CLI_H
#define DOG_CLI_H

#include "abc/FILE.h"
#include "abc/URI.h"
#include "dog/HOME.h"

#define CLI_MAX_URIS  16
#define CLI_MAX_FLAGS 32  // pairs: 16 flags max

// Parsed CLI state.
//
//   dog [verb] [--flags] URI*
//
// All slices borrow from argv (no allocation).
// flags[] is interleaved: [flag0, val0, flag1, val1, ...].
// Boolean flags have an empty val. nuris/nflags count entries,
// not pairs — nflags is always even (flag + val = 2 entries).
typedef struct {
    u8cs   verb;                     // first non-flag non-URI arg, or empty
    u8cs   flags[CLI_MAX_FLAGS * 2]; // interleaved [flag, val] pairs
    u32    nflags;                   // count of entries (= 2 * npairs)
    uri    uris[CLI_MAX_URIS];       // parsed URI targets
    u32    nuris;
    u8cs   repo;                     // repo root from HOMEFind
    b8     tty_out;                  // isatty(STDOUT)
} cli;

// Parse $args into cli struct. verb_names is a NULL-terminated
// array of known verb strings (or NULL to disable verb detection).
// Flags start with '-'; everything else is a URI.
// Flags that take values: if the flag appears in val_flags
// (e.g. "-g\0-C\0-r\0"), the next arg is consumed as value.
// Otherwise value is set to an empty (non-NULL) sentinel.
ok64 CLIParse(cli *c, char const *const *verb_names,
              char const *val_flags);

// Check if a flag is set. Returns the value slice (non-empty
// for value flags, empty-but-non-NULL for boolean flags).
// Returns a NULL slice if not found.
fun void CLIFlag(u8csp out, cli const *c, char const *flag) {
    out[0] = NULL; out[1] = NULL;
    a_cstr(fs, flag);
    for (u32 i = 0; i + 1 < c->nflags; i += 2) {
        if ($eq(c->flags[i], fs)) {
            $mv(out, c->flags[i + 1]);
            return;
        }
    }
}

// Check if a flag is present (boolean test).
fun b8 CLIHas(cli const *c, char const *flag) {
    u8cs v = {};
    CLIFlag(v, c, flag);
    return v[0] != NULL;
}

#endif
