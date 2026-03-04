//  IGNO: .gitignore parser and matcher using cc/PATH
//
#include "IGNO.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

static a_cstr(GITIGNORE_NAME, ".gitignore");

// Gitignore-style glob: * matches non-/, ** matches everything, ? matches one
static b8 IGNOGlob(u8cs pat, u8cs str) {
    u8cp pp = pat[0], pe = pat[1];
    u8cp sp = str[0], se = str[1];
    while (pp < pe && sp < se) {
        if (*pp == '*') {
            if (pp + 1 < pe && pp[1] == '*') {
                pp += 2;
                if (pp < pe && *pp == '/') pp++;
                // ** matches everything including /
                for (u8cp try = sp; try <= se; try++) {
                    u8cs rp = {pp, pe}, rs = {try, se};
                    if (IGNOGlob(rp, rs)) return YES;
                }
                return NO;
            }
            pp++;
            // * matches anything except /
            for (u8cp try = sp; try <= se; try++) {
                u8cs rp = {pp, pe}, rs = {try, se};
                if (IGNOGlob(rp, rs)) return YES;
                if (try < se && *try == '/') break;
            }
            return NO;
        }
        if (*pp == '?') {
            if (*sp == '/') return NO;
            pp++;
            sp++;
            continue;
        }
        if (*pp != *sp) return NO;
        pp++;
        sp++;
    }
    while (pp < pe && *pp == '*') pp++;
    return pp == pe && sp == se;
}

// Try to match pattern against path
static b8 TryMatch(igno_pat const *pat, u8cs path, b8 is_dir) {
    // Directory-only patterns don't match files
    if (pat->dir_only && !is_dir) return NO;

    u8cs pattern;
    u8csDup(pattern, pat->pattern);
    if ($empty(pattern)) return NO;

    // Strip leading / from pattern for matching (we handle anchoring separately)
    u8cs match_pattern;
    u8csDup(match_pattern, pattern);
    if (!$empty(match_pattern) && *match_pattern[0] == '/') {
        match_pattern[0]++;
    }

    // If pattern has no slash and is not anchored, match against basename only
    // (gitignore rule: patterns without / match anywhere in the tree)
    if (!pat->has_slash && !pat->anchored) {
        u8cs basename = {};
        path8gBase(basename, (path8cg){path[0], path[1], path[1]});
        return IGNOGlob(match_pattern, basename);
    }

    // Pattern has slash or is anchored - match against full path
    // For anchored patterns, path must match from root
    // For unanchored patterns with slash, pattern can match at any level

    u8cs match_path;
    u8csDup(match_path, path);

    // Skip leading / in path for comparison
    if (!$empty(match_path) && *match_path[0] == '/') {
        match_path[0]++;
    }

    if (pat->anchored) {
        // Anchored: must match from root
        return IGNOGlob(match_pattern, match_path);
    }

    // Unanchored with slash: try matching at each directory level
    // e.g., "foo/bar" should match "x/foo/bar" or "y/z/foo/bar"
    u8cs try_path;
    u8csDup(try_path, match_path);

    while (!$empty(try_path)) {
        if (IGNOGlob(match_pattern, try_path)) {
            return YES;
        }
        // Move to next component
        while (!$empty(try_path) && *try_path[0] != '/') {
            try_path[0]++;
        }
        if (!$empty(try_path) && *try_path[0] == '/') {
            try_path[0]++;
        }
    }

    return NO;
}

ok64 IGNOLoad(ignop out, u8cs dir_path) {
    sane(out && u8csOK(dir_path));
    memset(out, 0, sizeof(*out));

    // Build path to .gitignore
    a_path(gi_path, "");
    call(u8sFeed, u8bIdle(gi_path), dir_path);
    call(path8gTerm, path8gIn(gi_path));
    call(path8gPush, path8gIn(gi_path), GITIGNORE_NAME);

    // Try to load file
    ok64 o = FILEMapRO(&out->buf, path8cgIn(gi_path));
    if (o != OK) {
        // No .gitignore - not an error
        return NONE;
    }

    // Save root directory
    u8csDup(out->root, dir_path);

    // Parse patterns
    u8cs data;
    u8csDup(data, u8bDataC(out->buf));

    while (!$empty(data) && out->count < IGNO_MAX_PATTERNS) {
        // Skip leading whitespace (but not newlines)
        while (!$empty(data) && (**data == ' ' || **data == '\t')) {
            data[0]++;
        }

        // Find end of line
        u8cp line_start = *data;
        while (!$empty(data) && **data != '\n' && **data != '\r') {
            data[0]++;
        }
        u8cp line_end = *data;

        // Skip newline
        while (!$empty(data) && (**data == '\n' || **data == '\r')) {
            data[0]++;
        }

        // Skip blank lines
        if (line_start == line_end) continue;

        // Trim trailing whitespace
        while (line_end > line_start && (line_end[-1] == ' ' || line_end[-1] == '\t')) {
            line_end--;
        }

        // Skip comment lines
        if (*line_start == '#') continue;

        // Parse pattern
        igno_pat *pat = &out->patterns[out->count];
        memset(pat, 0, sizeof(*pat));

        u8cp p = line_start;

        // Check for negation
        if (*p == '!') {
            pat->negated = YES;
            p++;
        }

        // Check for anchor
        if (*p == '/') {
            pat->anchored = YES;
        }

        // Check for trailing / (directory only)
        if (line_end > p && line_end[-1] == '/') {
            pat->dir_only = YES;
            line_end--;  // Trim from pattern
        }

        // Check if pattern contains / (besides leading/trailing)
        for (u8cp c = p; c < line_end; c++) {
            if (*c == '/' && c != p) {
                pat->has_slash = YES;
                break;
            }
        }

        // Store pattern
        pat->pattern[0] = p;
        pat->pattern[1] = line_end;

        out->count++;
    }

    done;
}

void IGNOFree(ignop ig) {
    if (!ig) return;
    if (ig->buf && ig->buf[0]) {
        u8bUnMap(ig->buf);
    }
    memset(ig, 0, sizeof(*ig));
}

b8 IGNOMatch(ignocp ig, u8cs rel_path, b8 is_dir) {
    if (!ig || ig->count == 0) return NO;
    if ($empty(rel_path)) return NO;

    // Start with not ignored
    b8 ignored = NO;

    // Apply patterns in order (last matching pattern wins)
    for (u64 i = 0; i < ig->count; i++) {
        igno_pat const *pat = &ig->patterns[i];

        if (TryMatch(pat, rel_path, is_dir)) {
            ignored = !pat->negated;
        }
    }

    return ignored;
}
