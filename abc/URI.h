#ifndef ABC_URI_H
#define ABC_URI_H

#include "BUF.h"

con ok64 URIFAIL = 0x1e6d23ca495;
con ok64 URIBAD	= 0x79b48b28d;


#define URIenum 0

typedef struct {
    u8cs data;

    u8cs scheme;
    u8cs authority;
    u8cs user;
    u8cs host;
    u8cs port;
    u8cs path;
    u8cs query;
    u8cs fragment;

    u8csbp segments;  // optional: if non-NULL, lexer populates with path segments
} uri;

typedef uri* urip;
typedef uri const* uricp;

// Backwards compatibility
typedef uri URIstate;

// Parse URI from string
ok64 URILexer(urip state);

// Parse URI from string slice
ok64 URIutf8Drain(u8cs from, urip u);

// Serialize URI to string
ok64 URIutf8Feed(u8s into, uricp u);

// Produce relative URI: parts of `specific` that differ from `base`
// Result contains only the changed components
ok64 URIRelative(urip relative, uricp base, uricp specific);

// Resolve relative URI against base to produce absolute URI
ok64 URIAbsolute(urip absolute, uricp base, uricp relative);

// Percent-encode: all non-unreserved chars → %XX
// Unreserved: A-Za-z0-9 -._~
ok64 URIu8sEsc(u8s into, u8cs raw);

// Percent-decode: %XX → raw bytes
ok64 URIu8sUnesc(u8s into, u8cs esc);

// URI component presence flags (bitmask)
#define URI_SCHEME    0x01
#define URI_AUTHORITY 0x02
#define URI_USER      0x04
#define URI_HOST      0x08
#define URI_PORT      0x10
#define URI_PATH      0x20
#define URI_QUERY     0x40
#define URI_FRAGMENT  0x80

// Return bitmask of which URI components are defined (non-empty)
u8 URIPattern(uricp u);

// Build URI string from component slices (pass 0 to omit a component)
ok64 URIMake(u8s into, u8cs scheme, u8cs auth, u8cs path, u8cs query, u8cs fragm);

// Make URI on-stack: a_uri(name, scheme, auth, path, query, fragm)
// Result: u8cs `name` with the serialized URI
#define a_uri(n, sc, au, pa, qu, fr)                                      \
    u8 _##n##_pad[1024];                                                   \
    u8s _##n##_g = {_##n##_pad, _##n##_pad + sizeof(_##n##_pad)};          \
    URIMake(_##n##_g, (sc), (au), (pa), (qu), (fr));                       \
    u8cs n = {_##n##_pad, _##n##_g[0]}

#endif
