#include "GURI.h"

#include <string.h>

#include "abc/PRO.h"

// Find ".git/" or ".git" at end of path — splits repo address from file path.
// Returns pointer to the '.' of ".git" or NULL if not found.
static u8cp guri_find_git(u8cs path) {
    if ($empty(path)) return NULL;
    u8cp p = path[0];
    u8cp e = path[1];
    while (p + 3 < e) {
        if (p[0] == '.' && p[1] == 'g' && p[2] == 'i' && p[3] == 't') {
            u8cp after = p + 4;
            if (after == e || *after == '/') return p;
        }
        p++;
    }
    // Check if path ends with ".git" exactly
    if (e - path[0] >= 4) {
        u8cp tail = e - 4;
        if (tail[0] == '.' && tail[1] == 'g' && tail[2] == 'i' && tail[3] == 't')
            return tail;
    }
    return NULL;
}

ok64 GURIu8sDrain(u8cs input, gurip g) {
    sane(g != NULL && $ok(input));
    memset(g, 0, sizeof(guri));

    if ($empty(input)) done;

    // Parse as standard URI first.
    $mv(g->base.data, input);
    ok64 o = URILexer(&g->base);
    if (o != OK) return o;

    // --- Remote (authority) ---
    if (!$empty(g->base.authority)) {
        g->has_remote = YES;
        $mv(g->remote, g->base.authority);
        // Strip leading "//" from authority if present
        if ($len(g->remote) >= 2 &&
            g->remote[0][0] == '/' && g->remote[0][1] == '/')
            g->remote[0] += 2;
    }

    // --- Ref (query) ---
    if (!$empty(g->base.query)) {
        g->has_ref = YES;
        $mv(g->ref, g->base.query);
        // Detect range: contains ".." but not "..."
        u8cp p = g->ref[0];
        u8cp e = g->ref[1];
        while (p + 1 < e) {
            if (p[0] == '.' && p[1] == '.') {
                if (p + 2 < e && p[2] == '.') { p += 3; continue; }
                g->is_range = YES;
                break;
            }
            p++;
        }
    }

    // --- Search (fragment) ---
    if (!$empty(g->base.fragment)) {
        g->has_search = YES;
        $mv(g->search, g->base.fragment);
    }

    // --- Path: split at .git/ boundary ---
    u8cs full_path = {};
    $mv(full_path, g->base.path);

    // Strip leading / from path for repo-relative interpretation
    if (!$empty(full_path) && full_path[0][0] == '/')
        full_path[0]++;

    if (!$empty(full_path)) {
        u8cp git_marker = guri_find_git(full_path);
        if (git_marker != NULL) {
            // Everything up to and including .git is the repo address
            g->has_repo = YES;
            g->repo[0] = full_path[0];
            g->repo[1] = git_marker + 4;  // past "git"
            // File path is after ".git/"
            u8cp after = git_marker + 4;
            if (after < full_path[1] && *after == '/') after++;
            if (after < full_path[1]) {
                g->has_path = YES;
                g->path[0] = after;
                g->path[1] = full_path[1];
            }
        } else {
            // No .git/ — entire path is a file path.
            // But if we have authority, the path might include
            // owner/repo before the file.  Without .git/ we can't
            // distinguish, so treat entire path as repo-relative file.
            if (!$empty(full_path)) {
                g->has_path = YES;
                $mv(g->path, full_path);
            }
        }
    }

    // --- Detect extension filter: bare ".ext" path like ".c" ---
    if (g->has_path && !$empty(g->path)) {
        u8cp p0 = g->path[0];
        if (*p0 == '.' && $len(g->path) > 1) {
            // Check it's a bare ext: no slashes, starts with dot, not ".."
            b8 bare = YES;
            if (p0[1] == '.') bare = NO;  // ".." is not an ext
            $for(u8c, ch, g->path) {
                if (*ch == '/') { bare = NO; break; }
            }
            if (bare) {
                g->is_ext_filter = YES;
                g->ext[0] = p0 + 1;  // skip the dot
                g->ext[1] = g->path[1];
            }
        }
    }

    // --- Detect trailing slash (directory) ---
    if (g->has_path && !$empty(g->path)) {
        if (*(g->path[1] - 1) == '/') g->is_dir = YES;
    }

    // --- Extract file extension if not an ext filter ---
    if (g->has_path && !g->is_ext_filter && !$empty(g->path)) {
        // Walk back from end to find last dot (skip slashes)
        u8cp p = g->path[1];
        while (p > g->path[0]) {
            p--;
            if (*p == '/') break;
            if (*p == '.') {
                g->ext[0] = p + 1;
                g->ext[1] = g->path[1];
                break;
            }
        }
    }

    done;
}

ok64 GURIu8sFeed(u8s into, guricp g) {
    sane(g != NULL && u8sOK(into));

    if (g->has_remote) {
        u8sFeed1(into, '/');
        u8sFeed1(into, '/');
        u8sFeed(into, g->remote);
    }

    if (g->has_repo) {
        u8sFeed1(into, '/');
        u8sFeed(into, g->repo);
        if (g->has_path) u8sFeed1(into, '/');
    }

    if (g->has_path) {
        if (!g->has_repo && !g->has_remote)
            ;  // bare path, no leading /
        u8sFeed(into, g->path);
    }

    if (g->has_ref) {
        u8sFeed1(into, '?');
        u8sFeed(into, g->ref);
    }

    if (g->has_search) {
        u8sFeed1(into, '#');
        u8sFeed(into, g->search);
    }

    done;
}

u8 GURISearchType(guricp g) {
    if (!g->has_search || $empty(g->search)) return GURI_SEARCH_NONE;
    u8 first = g->search[0][0];
    // 'quoted' = structural search
    if (first == '\'') return GURI_SEARCH_SPOT;
    // /slashed/ = regex
    if (first == '/') return GURI_SEARCH_REGEX;
    // Numeric = line number or L-range
    if (first >= '0' && first <= '9') return GURI_SEARCH_LINE;
    if (first == 'L') return GURI_SEARCH_LINE;
    // Everything else = grep
    return GURI_SEARCH_GREP;
}
