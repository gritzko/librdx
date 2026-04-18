#include "DOG.h"

#include <string.h>

#include "abc/B.h"
#include "abc/PRO.h"

ok64 DOGParseURI(urip uri, u8csc text) {
    sane(uri != NULL);
    memset(uri, 0, sizeof(*uri));
    uri->data[0] = text[0];
    uri->data[1] = text[1];
    call(URILexer, uri);

    // Dog normalization: bare `host:path` with no `//` ends up
    // as scheme="host", path="path".  Promote scheme to authority
    // when authority is empty and the path has no leading slash
    // (proper RFC schemes have either an authority or a path
    // starting with '/').
    if (!$empty(uri->scheme) && $empty(uri->authority)) {
        b8 rooted = !$empty(uri->path) && $at(uri->path, 0) == '/';
        if (!rooted) {
            u8csMv(uri->authority, uri->scheme);
            u8csMv(uri->host, uri->scheme);
            uri->scheme[0] = NULL;
            uri->scheme[1] = NULL;
        }
    }

    // Dog normalization: `user@host/path` (no `//`) parses as one big
    // path.  If path's head-segment contains `@`, promote it to
    // authority+host and leave `/rest` as path.  Handles the ergonomic
    // `gritzko@pm.me/dogs-sniff?ref` form without requiring `//`.
    if ($empty(uri->authority) && !$empty(uri->path)) {
        u8cp slash = NULL;
        u8cp at    = NULL;
        for (u8cp p = uri->path[0]; p < uri->path[1]; p++) {
            if (*p == '/' && slash == NULL) { slash = p; break; }
            if (*p == '@' && at == NULL)    at = p;
        }
        u8cp head_end = slash ? slash : uri->path[1];
        if (at != NULL && at < head_end) {
            u8cp auth_start = uri->path[0];
            uri->authority[0] = auth_start;
            uri->authority[1] = head_end;
            uri->user[0] = auth_start;
            uri->user[1] = at;
            uri->host[0] = at + 1;
            uri->host[1] = head_end;
            if (slash != NULL) {
                uri->path[0] = slash;      // keep leading '/'
            } else {
                uri->path[0] = NULL;
                uri->path[1] = NULL;
            }
        }
    }

    // Dog normalization: symbolic "ports" are overwhelmingly path
    // segments that got eaten by RFC 3986 (`ssh://host:src/dogs-sniff`
    // — `src` is not a port).  If the port slice has any non-digit
    // byte, glue it onto the front of the path and clear the port.
    if (!$empty(uri->port)) {
        b8 numeric = YES;
        $for(u8c, p, uri->port) {
            if (*p < '0' || *p > '9') { numeric = NO; break; }
        }
        if (!numeric) {
            // port and path slices are contiguous in the original
            // text (port ends exactly where path begins), so the
            // union slice is {port[0], old_path_end_or_port_end}.
            u8cp new_path_end = $empty(uri->path) ? uri->port[1]
                                                  : uri->path[1];
            uri->path[0] = uri->port[0];
            uri->path[1] = new_path_end;
            // Trim the ':' from authority/host by pointing their
            // end at port[0]-1 (one byte back over the colon).
            if (!$empty(uri->host))      uri->host[1]      = uri->port[0] - 1;
            if (!$empty(uri->authority)) uri->authority[1] = uri->port[0] - 1;
            uri->port[0] = NULL;
            uri->port[1] = NULL;
        }
    }

    done;
}

ok64 DOGCanonURIKey(u8bp out, urip u, b8 with_query) {
    sane(out != NULL && u != NULL);
    // Preserve `file:` — absolute local paths are ambiguous without
    // it (`/etc/x` could be a filesystem path or a key-prefix).
    // Transport schemes (ssh, https, git) are fungible and dropped.
    b8 is_file = NO;
    if (!u8csEmpty(u->scheme) && $len(u->scheme) == 4 &&
        memcmp(u->scheme[0], "file", 4) == 0)
        is_file = YES;
    if (is_file) {
        a_cstr(filepfx, "file://");
        u8bFeed(out, filepfx);
        if (!u8csEmpty(u->path)) {
            if ($at(u->path, 0) != '/') u8bFeed1(out, '/');
            u8bFeed(out, u->path);
        }
    } else if (!u8csEmpty(u->authority) || !u8csEmpty(u->host)) {
        a_cstr(slashes, "//");
        u8bFeed(out, slashes);
        u8cs auth = {u->authority[0], u->authority[1]};
        if ($len(auth) >= 2 && auth[0][0] == '/' && auth[0][1] == '/')
            u8csUsed(auth, 2);
        u8bFeed(out, auth);
        // Ensure a single `/` separator: `host` + `/path` stays as-is,
        // `host` + `path` gets a `/` inserted.  `ssh://host:x` and
        // `ssh://host/x` thus produce identical keys.
        if (!u8csEmpty(u->path)) {
            if ($at(u->path, 0) != '/') u8bFeed1(out, '/');
            u8bFeed(out, u->path);
        }
    } else if (!u8csEmpty(u->path)) {
        u8bFeed(out, u->path);
    }
    if (with_query && !u8csEmpty(u->query)) {
        u8bFeed1(out, '?');
        u8bFeed(out, u->query);
    }
    done;
}
