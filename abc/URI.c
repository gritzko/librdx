#include "URI.h"

#include "PRO.h"

ok64 URIonPath(u8cs tok, urip state) {
    u8csMv(state->path, tok);
    return OK;
}
ok64 URIonPathNoscheme(u8cs tok, urip state) {
    u8csMv(state->path, tok);
    return OK;
}
ok64 URIonPathRootless(u8cs tok, urip state) {
    u8csMv(state->path, tok);
    return OK;
}
ok64 URIonScheme(u8cs tok, urip state) {
    $mv(state->scheme, tok);
    --state->scheme[1];
    return OK;
}
ok64 URIonIPv4address(u8cs tok, urip state) { return OK; }
ok64 URIonIPvFuture(u8cs tok, urip state) { return OK; }
ok64 URIonIPv6address(u8cs tok, urip state) { return OK; }
ok64 URIonIP_literal(u8cs tok, urip state) { return OK; }
ok64 URIonUser(u8cs tok, urip state) {
    $mv(state->user, tok);
    return OK;
}
ok64 URIonHost(u8cs tok, urip state) {
    $mv(state->host, tok);
    return OK;
}
ok64 URIonPort(u8cs tok, urip state) {
    $mv(state->port, tok);
    return OK;
}
ok64 URIonFragment(u8cs tok, urip state) {
    $mv(state->fragment, tok);
    return OK;
}
ok64 URIonQuery(u8cs tok, urip state) {
    $mv(state->query, tok);
    return OK;
}
ok64 URIonAuthority(u8cs tok, urip state) {
    u8csMv(state->authority, tok);
    return OK;
}
ok64 URIonURI(u8cs tok, urip state) { return OK; }
ok64 URIonRoot(u8cs tok, urip state) { return OK; }
ok64 URIonSegment(u8cs tok, urip state) {
    if (state->segments) {
        return u8cssFeed1(u8csbIdle(state->segments), tok);
    }
    return OK;
}
ok64 URIonSegment_nz(u8cs tok, urip state) {
    if (state->segments) {
        return u8cssFeed1(u8csbIdle(state->segments), tok);
    }
    return OK;
}

ok64 URIutf8Drain(u8cs from, urip u) {
    u8csbp segs = u->segments;  // preserve optional segments buffer
    zerop(u);
    u->segments = segs;
    $mv(u->data, from);
    return URILexer(u);
}

ok64 URIutf8Feed(u8s into, uricp u) {
    if (!$empty(u->scheme)) {
        ok64 o = u8sFeed(into, u->scheme);
        if (o != OK) return o;
        o = u8sFeed1(into, ':');
        if (o != OK) return o;
    }
    if (!u8csEmpty(u->authority)) {
        ok64 o = u8sFeed(into, u->authority);
        if (o != OK) return o;
    }
    if (!$empty(u->path)) {
        ok64 o = u8sFeed(into, u->path);
        if (o != OK) return o;
    }
    if (!$empty(u->query)) {
        ok64 o = u8sFeed1(into, '?');
        if (o != OK) return o;
        o = u8sFeed(into, u->query);
        if (o != OK) return o;
    }
    if (!$empty(u->fragment)) {
        ok64 o = u8sFeed1(into, '#');
        if (o != OK) return o;
        o = u8sFeed(into, u->fragment);
        if (o != OK) return o;
    }
    return OK;
}

// Static buffer for relative path computation results
static u8 URIRelPathBuf[4096];

// Split path into segments (by '/')
// Skips leading slash, stores each segment in segs buffer
static ok64 URISplitPath(u8csbp segs, u8csc path) {
    sane(segs);
    u8csbReset(segs);
    if (u8csEmpty(path)) done;

    u8cp p = path[0];
    u8cp end = path[1];

    // Skip leading slash
    if (p < end && *p == '/') p++;

    while (p < end) {
        u8cp seg_start = p;
        while (p < end && *p != '/') p++;
        // Add segment (even if empty, for consecutive slashes)
        if (p > seg_start) {
            u8cs seg = {seg_start, p};
            call(u8cssFeed1, u8csbIdle(segs), seg);
        }
        if (p < end) p++;  // skip slash
    }
    done;
}

// Compute relative path from base to specific using segments
// Writes result to URIRelPathBuf, returns slice of written data
static ok64 URIRelativePath(u8csp result, u8cscs base_segs, u8cscs spec_segs) {
    sane(result);
    size_t base_n = u8cscsLen(base_segs);
    size_t spec_n = u8cscsLen(spec_segs);

    // Base "directory" is all but last segment (last is the "file")
    size_t base_dir_n = base_n > 0 ? base_n - 1 : 0;

    // Find common prefix length
    size_t common = 0;
    while (common < base_dir_n && common < spec_n) {
        u8cs const *bs = u8cscsAtP(base_segs, common);
        u8cs const *ss = u8cscsAtP(spec_segs, common);
        size_t blen = u8csLen(*bs);
        size_t slen = u8csLen(*ss);
        if (blen != slen || 0 != memcmp((*bs)[0], (*ss)[0], blen)) break;
        common++;
    }

    // Build result in static buffer
    u8p out = URIRelPathBuf;
    u8cp endp = URIRelPathBuf + sizeof(URIRelPathBuf);

    // Generate ".." for each segment to climb from base dir
    size_t up = base_dir_n - common;
    for (size_t i = 0; i < up; i++) {
        if (i > 0) {
            test(out < endp, URIFAIL);
            *out++ = '/';
        }
        test(out + 2 <= endp, URIFAIL);
        *out++ = '.';
        *out++ = '.';
    }

    // Append remaining specific segments
    for (size_t i = common; i < spec_n; i++) {
        if (up > 0 || i > common) {
            test(out < endp, URIFAIL);
            *out++ = '/';
        }
        u8cs const *seg = u8cscsAtP(spec_segs, i);
        size_t len = u8csLen(*seg);
        test(out + len <= endp, URIFAIL);
        memcpy(out, (*seg)[0], len);
        out += len;
    }

    result[0] = URIRelPathBuf;
    result[1] = out;
    done;
}

// Produce relative URI: parts of `specific` that differ from `base`
ok64 URIRelative(urip rel, uricp base, uricp specific) {
    sane(rel && base && specific);
    zerop(rel);

    // If schemes differ, return full specific URI
    if (!$eq(base->scheme, specific->scheme)) {
        *rel = *specific;
        done;
    }

    // Same scheme - check authority (user, host, port)
    if (!$eq(base->host, specific->host) || !$eq(base->port, specific->port) ||
        !$eq(base->user, specific->user)) {
        // Different authority - include authority and path
        u8csDup(rel->authority, specific->authority);
        u8csDup(rel->host, specific->host);
        u8csDup(rel->port, specific->port);
        u8csDup(rel->user, specific->user);
        u8csDup(rel->path, specific->path);
        u8csDup(rel->query, specific->query);
        u8csDup(rel->fragment, specific->fragment);
        done;
    }

    // Same authority - check path
    if (!$eq(base->path, specific->path)) {
        // Split paths into segments
        a_pad(u8cs, base_segs_arr, 64);
        a_pad(u8cs, spec_segs_arr, 64);
        call(URISplitPath, base_segs_arr, base->path);
        call(URISplitPath, spec_segs_arr, specific->path);

        u8cscs base_segs, spec_segs;
        u8cscsDup(base_segs, u8csbDataC(base_segs_arr));
        u8cscsDup(spec_segs, u8csbDataC(spec_segs_arr));

        // Compute relative path
        call(URIRelativePath, rel->path, base_segs, spec_segs);
        // Copy query/fragment (or mark explicitly empty)
        if (*specific->query) {
            u8csDup(rel->query, specific->query);
        } else if (*base->query) {
            rel->query[0] = rel->query[1] = *specific->data;
        }
        if (*specific->fragment) {
            u8csDup(rel->fragment, specific->fragment);
        } else if (*base->fragment) {
            rel->fragment[0] = rel->fragment[1] = *specific->data;
        }
        done;
    }

    // Same path - check query
    if (!$eq(base->query, specific->query)) {
        // Use non-NULL empty slice to indicate "explicitly no query" vs NULL "not set"
        if (*specific->query) {
            u8csDup(rel->query, specific->query);
        } else {
            // Empty but non-NULL: marks "explicitly no query"
            rel->query[0] = rel->query[1] = *specific->data;
        }
        // Copy fragment (or mark explicitly empty)
        if (*specific->fragment) {
            u8csDup(rel->fragment, specific->fragment);
        } else {
            rel->fragment[0] = rel->fragment[1] = *specific->data;
        }
        done;
    }

    // Same query - check fragment
    if (!$eq(base->fragment, specific->fragment)) {
        if (*specific->fragment) {
            u8csDup(rel->fragment, specific->fragment);
        } else {
            // Empty but non-NULL: marks "explicitly no fragment"
            rel->fragment[0] = rel->fragment[1] = *specific->data;
        }
    }

    done;
}

// Static buffer for merged path computation
static u8 URIMergePathBuf[4096];

// Merge relative path with base path per RFC 3986 Section 5.2.3
// Then resolve . and .. segments per Section 5.2.4
static ok64 URIMergePath(u8csp result, u8csc base_path, u8csc rel_path) {
    sane(result);

    // If relative path starts with '/', it's absolute - use as-is
    if (!u8csEmpty(rel_path) && rel_path[0][0] == '/') {
        size_t len = u8csLen(rel_path);
        test(len <= sizeof(URIMergePathBuf), URIFAIL);
        memcpy(URIMergePathBuf, rel_path[0], len);
        result[0] = URIMergePathBuf;
        result[1] = URIMergePathBuf + len;
        done;
    }

    // Find last '/' in base path (directory portion)
    u8cp base_dir_end = base_path[0];
    for (u8cp p = base_path[0]; p < base_path[1]; p++) {
        if (*p == '/') base_dir_end = p + 1;
    }

    // Build merged path: base_dir + rel_path
    size_t dir_len = base_dir_end - base_path[0];
    size_t rel_len = u8csLen(rel_path);
    test(dir_len + rel_len <= sizeof(URIMergePathBuf), URIFAIL);
    memcpy(URIMergePathBuf, base_path[0], dir_len);
    memcpy(URIMergePathBuf + dir_len, rel_path[0], rel_len);

    // Now resolve . and .. segments (RFC 5.2.4 remove_dot_segments)
    u8p in = URIMergePathBuf;
    u8p in_end = URIMergePathBuf + dir_len + rel_len;
    u8p out = URIMergePathBuf;

    while (in < in_end) {
        // A: If input starts with "../" or "./" remove that prefix
        if (in + 3 <= in_end && in[0] == '.' && in[1] == '.' && in[2] == '/') {
            in += 3;
            continue;
        }
        if (in + 2 <= in_end && in[0] == '.' && in[1] == '/') {
            in += 2;
            continue;
        }

        // B: If input starts with "/./" or "/." at end, replace with "/"
        if (in + 3 <= in_end && in[0] == '/' && in[1] == '.' && in[2] == '/') {
            in += 2;
            continue;
        }
        if (in + 2 == in_end && in[0] == '/' && in[1] == '.') {
            *out++ = '/';
            break;
        }

        // C: If input starts with "/../" or "/.." at end, replace with "/" and remove last output segment
        if (in + 4 <= in_end && in[0] == '/' && in[1] == '.' && in[2] == '.' && in[3] == '/') {
            in += 3;  // skip "/.." leaving "/" for next iteration
            // Remove last segment and its preceding "/" from output
            while (out > URIMergePathBuf && out[-1] != '/') out--;
            if (out > URIMergePathBuf) out--;
            continue;
        }
        if (in + 3 == in_end && in[0] == '/' && in[1] == '.' && in[2] == '.') {
            // At end - remove last segment
            while (out > URIMergePathBuf && out[-1] != '/') out--;
            if (out > URIMergePathBuf) out--;
            *out++ = '/';  // add final slash per RFC
            break;
        }

        // D: If input is "." or "..", remove it
        if ((in + 1 == in_end && in[0] == '.') ||
            (in + 2 == in_end && in[0] == '.' && in[1] == '.')) {
            break;
        }

        // E: Copy first path segment (including initial "/" if any) to output
        if (in[0] == '/') {
            *out++ = *in++;
        }
        while (in < in_end && in[0] != '/') {
            *out++ = *in++;
        }
    }

    result[0] = URIMergePathBuf;
    result[1] = out;
    done;
}

// Resolve relative URI against base to produce absolute URI
ok64 URIAbsolute(urip abs, uricp base, uricp rel) {
    sane(abs && base && rel);
    zerop(abs);

    // If relative has scheme, it's already absolute
    if (!$empty(rel->scheme)) {
        *abs = *rel;
        done;
    }

    // Inherit scheme from base
    u8csDup(abs->scheme, base->scheme);

    // If relative has authority, use it
    if (!$empty(rel->authority) || !$empty(rel->host)) {
        u8csDup(abs->authority, rel->authority);
        u8csDup(abs->host, rel->host);
        u8csDup(abs->port, rel->port);
        u8csDup(abs->user, rel->user);
        u8csDup(abs->path, rel->path);
        u8csDup(abs->query, rel->query);
        u8csDup(abs->fragment, rel->fragment);
        done;
    }

    // Inherit authority from base
    u8csDup(abs->authority, base->authority);
    u8csDup(abs->host, base->host);
    u8csDup(abs->port, base->port);
    u8csDup(abs->user, base->user);

    // If relative has path, merge with base path
    if (!$empty(rel->path)) {
        call(URIMergePath, abs->path, base->path, rel->path);
        u8csDup(abs->query, rel->query);
        u8csDup(abs->fragment, rel->fragment);
        done;
    }

    // Relative has no path - inherit from base
    u8csDup(abs->path, base->path);

    // Check if relative specifies query (non-NULL pointer = explicitly set)
    if (*rel->query) {
        // Non-empty means use that query, empty means explicitly no query
        if (!$empty(rel->query)) {
            u8csDup(abs->query, rel->query);
        }
        // else: abs->query stays empty (explicitly no query)
        // Fragment: use rel's if set, otherwise inherit from base
        if (*rel->fragment) {
            u8csDup(abs->fragment, rel->fragment);
        } else {
            u8csDup(abs->fragment, base->fragment);
        }
        done;
    }

    // No relative query info - inherit from base
    u8csDup(abs->query, base->query);

    // Fragment: use rel's if specified, otherwise inherit from base
    if (*rel->fragment) {
        u8csDup(abs->fragment, rel->fragment);
    } else {
        u8csDup(abs->fragment, base->fragment);
    }

    done;
}

// Check if character is unreserved per RFC 3986
fun b8 URIIsUnreserved(u8c c) {
    if (c >= 'A' && c <= 'Z') return YES;
    if (c >= 'a' && c <= 'z') return YES;
    if (c >= '0' && c <= '9') return YES;
    if (c == '-' || c == '.' || c == '_' || c == '~') return YES;
    return NO;
}

con u8c HEX_DIGITS[16] = "0123456789ABCDEF";

// Percent-encode: all non-unreserved chars → %XX
ok64 URIu8sEsc(u8s into, u8cs raw) {
    sane($ok(into) && $ok(raw));
    $for(u8c, c, raw) {
        if (URIIsUnreserved(*c)) {
            call(u8sFeed1, into, *c);
        } else {
            test($len(into) >= 3, URIFAIL);
            call(u8sFeed1, into, '%');
            call(u8sFeed1, into, HEX_DIGITS[(*c >> 4) & 0xF]);
            call(u8sFeed1, into, HEX_DIGITS[*c & 0xF]);
        }
    }
    done;
}

// Hex digit to value, returns -1 if not hex
fun int URIHexVal(u8c c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Percent-decode: %XX → raw bytes
ok64 URIu8sUnesc(u8s into, u8cs esc) {
    sane($ok(into) && $ok(esc));
    while (!$empty(esc)) {
        u8c c = *esc[0]++;
        if (c == '%' && $len(esc) >= 2) {
            int hi = URIHexVal(esc[0][0]);
            int lo = URIHexVal(esc[0][1]);
            if (hi >= 0 && lo >= 0) {
                call(u8sFeed1, into, (u8c)((hi << 4) | lo));
                esc[0] += 2;
                continue;
            }
        }
        call(u8sFeed1, into, c);
    }
    done;
}

ok64 URIMake(u8s into, u8cs scheme, u8cs auth, u8cs path, u8cs query, u8cs fragm) {
    sane($ok(into));
    uri u = {};
    if (scheme) u8csMv(u.scheme, scheme);
    if (auth) u8csMv(u.authority, auth);
    if (path) u8csMv(u.path, path);
    if (query) u8csMv(u.query, query);
    if (fragm) u8csMv(u.fragment, fragm);
    call(URIutf8Feed, into, &u);
    done;
}

// Return bitmask of which URI components are defined (non-empty)
u8 URIPattern(uricp u) {
    u8 p = 0;
    if (!$empty(u->scheme)) p |= URI_SCHEME;
    if (!$empty(u->authority)) p |= URI_AUTHORITY;
    if (!$empty(u->user)) p |= URI_USER;
    if (!$empty(u->host)) p |= URI_HOST;
    if (!$empty(u->port)) p |= URI_PORT;
    if (!$empty(u->path)) p |= URI_PATH;
    if (!$empty(u->query)) p |= URI_QUERY;
    if (!$empty(u->fragment)) p |= URI_FRAGMENT;
    return p;
}
