#include "GURI.h"

#include <string.h>

#include "abc/PATH.h"
#include "abc/PRO.h"

ok64 GURIu8sDrain(u8cs input, gurip g) {
    sane(g != NULL && $ok(input));
    memset(g, 0, sizeof(guri));
    if ($empty(input)) done;
    $mv(g->data, input);
    call(URILexer, g);
    done;
}

// --- Introspection helpers ---

// Find ".git" marker in a path slice.  Sets `at` to the ".git" range.
static b8 guri_find_git(u8cs at, u8csc path) {
    if ($empty(path) || $len(path) < 4) return NO;
    a_cstr(marker, ".git");
    size_t mlen = $len(marker);
    u8cp found = memmem(path[0], (size_t)$len(path), marker[0], mlen);
    while (found != NULL) {
        u8cp after = found + mlen;
        if (after >= path[1] || *after == '/') {
            at[0] = found;
            at[1] = found + mlen;
            return YES;
        }
        size_t rest = (size_t)(path[1] - (after));
        found = memmem(after, rest, marker[0], mlen);
    }
    return NO;
}

// Get the effective file path (stripped of leading / and .git/ prefix).
static void guri_file_path(u8cs out, u8csc uri_path) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(uri_path)) return;
    a_dup(u8c, p, uri_path);
    // Strip leading /
    if (!$empty(p) && p[0][0] == '/') p[0]++;
    if ($empty(p)) return;
    // Split at .git/ if present
    u8cs git_at = {};
    if (guri_find_git(git_at, p)) {
        u8cp after = git_at[1];
        if (after < p[1] && *after == '/') after++;
        if (after < p[1]) { out[0] = after; out[1] = p[1]; }
    } else {
        $mv(out, p);
    }
}

void GURIRemote(u8csp out, guricp g) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(g->authority)) return;
    a_dup(u8c, a, g->authority);
    // Strip leading "//"
    if ($len(a) >= 2 && a[0][0] == '/' && a[0][1] == '/')
        a[0] += 2;
    $mv(out, a);
}

void GURIRepo(u8csp out, guricp g) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(g->path)) return;
    a_dup(u8c, p, g->path);
    if (!$empty(p) && p[0][0] == '/') p[0]++;
    u8cs git_at = {};
    if (guri_find_git(git_at, p)) {
        out[0] = p[0];
        out[1] = git_at[1];
    }
}

void GURIPath(u8csp out, guricp g) {
    guri_file_path(out, g->path);
}

void GURIExt(u8csp out, guricp g) {
    u8cs fp = {};
    guri_file_path(fp, g->path);
    if ($empty(fp)) { out[0] = NULL; out[1] = NULL; return; }
    // For bare .ext filters, the ext is everything after the dot
    if (fp[0][0] == '.' && $len(fp) > 1 && fp[0][1] != '.') {
        b8 bare = YES;
        $for(u8c, ch, fp) { if (*ch == '/') { bare = NO; break; } }
        if (bare) {
            out[0] = fp[0] + 1;
            out[1] = fp[1];
            return;
        }
    }
    PATHu8sExt(out, fp);
}

b8 GURIHasRepo(guricp g) {
    u8cs git_at = {};
    a_dup(u8c, p, g->path);
    if ($empty(p)) return NO;
    if (p[0][0] == '/') p[0]++;
    return guri_find_git(git_at, p);
}

b8 GURIHasPath(guricp g) {
    u8cs fp = {};
    guri_file_path(fp, g->path);
    return !$empty(fp);
}

b8 GURIIsExtFilter(guricp g) {
    u8cs fp = {};
    guri_file_path(fp, g->path);
    if ($empty(fp) || $len(fp) <= 1) return NO;
    if (fp[0][0] != '.' || fp[0][1] == '.') return NO;
    $for(u8c, ch, fp) { if (*ch == '/') return NO; }
    return YES;
}

b8 GURIIsDir(guricp g) {
    u8cs fp = {};
    guri_file_path(fp, g->path);
    if ($empty(fp)) return NO;
    return *(fp[1] - 1) == '/';
}

b8 GURIIsRange(guricp g) {
    if ($empty(g->query)) return NO;
    a_dup(u8c, scan, g->query);
    while ($len(scan) >= 2) {
        if (scan[0][0] == '.' && scan[0][1] == '.') {
            if ($len(scan) >= 3 && scan[0][2] == '.') {
                u8csFed(scan, 3);
                continue;
            }
            return YES;
        }
        u8csFed(scan, 1);
    }
    return NO;
}

u8 GURISearchType(guricp g) {
    if ($empty(g->fragment)) return GURI_SEARCH_NONE;
    u8 first = g->fragment[0][0];
    if (first == '\'') return GURI_SEARCH_SPOT;
    if (first == '/') return GURI_SEARCH_REGEX;
    if ((first >= '0' && first <= '9') || first == 'L') return GURI_SEARCH_LINE;
    return GURI_SEARCH_GREP;
}
