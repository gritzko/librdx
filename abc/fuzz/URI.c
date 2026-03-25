// URI round-trip fuzz test: parse -> relative -> absolute should preserve URI
#include "URI.h"

#include "PRO.h"
#include "TEST.h"

FUZZ(u8, URIfuzz) {
    sane(1);

    // Skip inputs that are too large or empty
    if ($len(input) > 1024) done;
    if ($len(input) < 5) done;

    // Input must look like a URI (start with scheme-like chars)
    u8c first = *input[0];
    if (first < 'a' || first > 'z') done;

    // Parse specific URI from input
    uri specific = {};
    ok64 o = URIutf8Drain(input, &specific);
    if (o != OK) done;  // Invalid URI is fine, just skip

    // Use http://example.com/path as base
    a_cstr(base_str, "http://example.com/path");
    uri base = {};
    o = URIutf8Drain(base_str, &base);
    if (o != OK) done;

    // Compute relative
    uri rel = {};
    o = URIRelative(&rel, &base, &specific);
    if (o != OK) done;

    // Resolve back to absolute
    uri resolved = {};
    o = URIAbsolute(&resolved, &base, &rel);
    if (o != OK) done;

    // Round-trip must preserve all components
    if (!$eq(resolved.scheme, specific.scheme)) return FAILSANITY;
    if (!$eq(resolved.host, specific.host)) return FAILSANITY;
    if (!$eq(resolved.port, specific.port)) return FAILSANITY;
    if (!$eq(resolved.user, specific.user)) return FAILSANITY;
    if (!$eq(resolved.path, specific.path)) return FAILSANITY;
    if (!$eq(resolved.query, specific.query)) return FAILSANITY;
    if (!$eq(resolved.fragment, specific.fragment)) return FAILSANITY;

    done;
}
