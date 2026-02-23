#include "PATH.h"

#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "PRO.h"
#include "UTF8.h"

// Validate a segment (not a full path8g, just raw u8cs input)
// Checks: UTF-8, no forbidden chars (\r\t\n\0), no slashes
fun ok64 path8gVerifySegment(u8cs segment) {
    if (!$ok(segment)) return PATHBAD;
    // Check UTF-8 validity
    a_dup(u8c, check, segment);
    ok64 o = utf8sValid(check);
    if (o != OK) return PATHBAD;
    // Check for forbidden characters and slashes
    $for(u8c, c, segment) {
        if (*c == '\r' || *c == '\t' || *c == '\n' || *c == '\0' || *c == '/') {
            return PATHBAD;
        }
    }
    return OK;
}

b8 path8gIsAbsolute(path8cg path) {
    if (!$ok(path) || $empty(path)) return NO;
    return *path[0] == '/';
}

ok64 path8gVerify(path8cg path) {
    sane($ok(path));
    test(*path[1] == 0, PATHBAD);
    // Check UTF-8 validity
    a_dup(u8c, check, path);
    ok64 o = utf8sValid(check);
    test(o == OK, PATHBAD);
    // Check for forbidden characters
    $for(u8c, c, path) {
        test(*c != '\r' && *c != '\t' && *c != '\n' && *c != '\0', PATHBAD);
    }
    done;
}

ok64 path8gNext(path8cg path, u8csp segment) {
    sane($ok(path) && segment != NULL);
    if ($empty(path)) return END;
    // Skip leading slash if present
    if (*path[0] == '/') path[0]++;
    if ($empty(path)) return END;
    // Find next slash or end
    u8cp start = path[0];
    while (!$empty(path) && *path[0] != '/') path[0]++;
    segment[0] = start;
    segment[1] = path[0];
    done;
}

ok64 path8gDup(path8g into, path8cg orig) {
    sane($ok(into) && $ok(orig));
    call(u8sFeed, into + 1, orig);
    call(path8gTerm, into);
    done;
}

ok64 path8gPush(path8g path, u8cs segment) {
    sane($ok(path) && $ok(segment));
    // Validate segment (raw u8cs, not path8g)
    call(path8gVerifySegment, segment);
    // Add separator if path is non-empty and doesn't end with /
    if (!$empty(path)) {
        u8c last = *(path[1] - 1);
        if (last != '/') {
            call(u8sFeed1, path + 1, '/');
        }
    }
    // Add segment
    call(u8sFeed, path + 1, segment);
    call(path8gTerm, path);
    done;
}

ok64 path8gPop(path8g path) {
    sane($ok(path));
    if ($empty(path)) done;  // Nothing to pop
    // Find last / in path (search backwards from path[1])
    u8p p = path[1];
    while (p > path[0] && *(p - 1) != '/') p--;
    if (p > path[0]) p--;  // Skip the slash too (unless at root)
    // Handle root case: keep at least "/"
    if (p == path[0] && *path[0] == '/') p++;
    path[1] = p;
    call(path8gTerm, path);
    done;
}

ok64 path8gAddTmp(path8g path, u8cs tmpl) {
    sane($ok(path) && $ok(tmpl) && !$empty(tmpl));
    // Add separator if path is non-empty and doesn't end with /
    if (!$empty(path)) {
        u8c last = *(path[1] - 1);
        if (last != '/') {
            call(u8sFeed1, path + 1, '/');
        }
    }
    // Remember where we started adding
    u8p start = path[1];
    // Add template
    call(u8sFeed, path + 1, tmpl);
    call(path8gTerm, path);
    // Randomize 'X' characters
    static u32 seed = 0;
    if (seed == 0) seed = (u32)getpid() ^ (u32)time(NULL);
    for (u8p p = start; p < path[1]; p++) {
        if (*p == 'X') {
            seed = seed * 1103515245 + 12345;
            *p = "abcdefghijklmnopqrstuvwxyz0123456789"[(seed >> 16) % 36];
        }
    }
    done;
}

ok64 path8gAdd(path8g into, path8cg rel) {
    sane($ok(into) && $ok(rel));
    // Add relative path segment by segment
    a_dupcg(u8, rem, rel);
    u8cs seg = {};
    while (path8gNext(rem, seg) == OK) {
        call(path8gPush, into, seg);
    }
    call(path8gTerm, into);
    done;
}

ok64 path8gRelative(path8g rel, path8cg absbase, path8cg abs) {
    sane($ok(rel) && $ok(absbase) && $ok(abs));
    test(path8gIsAbsolute(absbase) && path8gIsAbsolute(abs), PATHBAD);

    // Special case: identical paths return "."
    if ($len(absbase) == $len(abs) && 0 == $cmp(absbase, abs)) {
        rel[1] = rel[0];  // Reset output
        u8c dot[1] = {'.'};
        u8cs dot_seg = {dot, dot + 1};
        call(path8gPush, rel, dot_seg);
        call(path8gTerm, rel);
        done;
    }

    // Check if abs starts with base as directory prefix
    // If so, just return the remaining part of abs
    size_t base_len = $len(absbase);
    u8cs abs_prefix = {abs[0], abs[0] + base_len};
    if ($len(abs) > base_len && $eq(absbase, abs_prefix) &&
        (absbase[1][-1] == '/' || abs[0][base_len] == '/')) {
        // abs starts with base as directory
        rel[1] = rel[0];
        u8cp start = abs[0] + base_len;
        if (*start == '/') start++;  // skip separator
        u8cs remainder = {start, abs[1]};
        call(u8sFeed, rel + 1, remainder);
        call(path8gTerm, rel);
        done;
    }

    // Treat all paths as directories (Python os.path.relpath semantics)
    // Find common prefix by comparing segments
    a_dupcg(u8, base_rem, absbase);
    a_dupcg(u8, abs_rem, abs);
    u8cs base_seg = {}, abs_seg = {};

    // Skip common prefix segments
    ok64 bo = OK, ao = OK;
    while (1) {
        bo = path8gNext(base_rem, base_seg);
        ao = path8gNext(abs_rem, abs_seg);
        if (bo != OK || ao != OK) break;
        if ($len(base_seg) != $len(abs_seg)) break;
        if (0 != $cmp(base_seg, abs_seg)) break;
    }

    // Count remaining segments in base (need that many ..)
    // If base exhausted (bo != OK), we have 0 segments left to go up
    // If base has more (bo == OK), count base_seg plus remaining
    u8cs skip = {};
    u64 up_count = (bo == OK) ? 1 : 0;
    while (path8gNext(base_rem, skip) == OK) up_count++;

    // If abs exhausted, clear abs_seg so we don't add stale segment
    if (ao != OK) {
        abs_seg[0] = abs_seg[1] = NULL;
    }

    // Build relative path: up_count times ".." + remaining abs segments
    rel[1] = rel[0];  // Reset output

    u8c dotdot[2] = {'.', '.'};
    u8cs dotdot_seg = {dotdot, dotdot + 2};
    for (u64 i = 0; i < up_count; i++) {
        call(path8gPush, rel, dotdot_seg);
    }

    // Add remaining absolute path segments
    if (!$empty(abs_seg)) {
        call(path8gPush, rel, abs_seg);
    }
    while (path8gNext(abs_rem, abs_seg) == OK) {
        call(path8gPush, rel, abs_seg);
    }

    // Handle case where paths are equal
    if ($empty(rel) || rel[0] == rel[1]) {
        u8c dot[1] = {'.'};
        u8cs dot_seg = {dot, dot + 1};
        call(path8gPush, rel, dot_seg);
    }

    call(path8gTerm, rel);
    done;
}

ok64 path8gAbsolute(path8g abs, path8cg base, path8cg rel) {
    sane($ok(abs) && $ok(base) && $ok(rel));

    // If rel is absolute, just copy it
    if (path8gIsAbsolute(rel)) {
        // Copy rel to temp, then normalize into abs
        a_pad(u8, tmp, PATH_MAX);
        call(u8sFeed, tmp_idle, rel);
        u8cg tmp_gc = {tmp[1], *tmp_idle, tmp[3]};
        abs[1] = abs[0];  // reset abs
        call(path8gNorm, abs, tmp_gc);
        done;
    }

    // Start with base (treat as directory, don't strip)
    call(path8gDup, abs, base);

    // Add relative path
    call(path8gAdd, abs, rel);

    // Normalize to resolve . and .. (copy to temp first)
    a_pad(u8, tmp2, PATH_MAX);
    u8cs abs_data = {abs[0], abs[1]};
    call(u8sFeed, tmp2_idle, abs_data);
    u8cg tmp2_gc = {tmp2[1], *tmp2_idle, tmp2[3]};
    abs[1] = abs[0];  // reset abs
    call(path8gNorm, abs, tmp2_gc);
    done;
}

ok64 path8gReal(path8g real, path8cg path) {
    sane($ok(real) && $ok(path));
    test($len(real) >= PATH_MAX, PATHNOROOM);

    // Need null-terminated string for realpath
    a_dup(u8c, tmp, path);
    test(!$empty(tmp), PATHBAD);

    // Temporarily null-terminate (path should have space based on gauge
    // contract)
    char pathbuf[PATH_MAX];
    size_t len = $len(path);
    if (len >= PATH_MAX) len = PATH_MAX - 1;
    for (size_t i = 0; i < len; i++) pathbuf[i] = (char)path[0][i];
    pathbuf[len] = 0;

    char resolved[PATH_MAX];
    char* result = realpath(pathbuf, resolved);
    test(result != NULL, PATHFAIL);

    // Copy result to output
    real[1] = real[0];
    size_t rlen = 0;
    while (resolved[rlen] != 0 && rlen < PATH_MAX) rlen++;
    u8cs res = {(u8c*)resolved, (u8c*)resolved + rlen};
    call(u8sFeed, real + 1, res);
    call(path8gTerm, real);
    done;
}

void path8gBase(u8csp out, path8cg path) {
    if (!$ok(path) || $empty(path)) {
        out[0] = out[1] = NULL;
        return;
    }
    // Find last slash by scanning backward
    u8cp p = path[1];
    while (p > path[0] && *(p - 1) != '/') p--;
    out[0] = p;
    out[1] = path[1];
}

void path8gDir(u8csp out, path8cg path) {
    if (!$ok(path) || $empty(path)) {
        out[0] = out[1] = NULL;
        return;
    }
    // Find last slash by scanning backward
    u8cp p = path[1];
    while (p > path[0] && *(p - 1) != '/') p--;
    if (p == path[0]) {
        // No slash found - return "." as static
        static u8c dot[1] = {'.'};
        out[0] = dot;
        out[1] = dot + 1;
        return;
    }
    // Skip trailing slash unless at root
    u8cp end = p - 1;
    if (end == path[0] && *path[0] == '/') end++;
    out[0] = path[0];
    out[1] = end;
}

ok64 path8gNorm(path8g norm, path8cg orig) {
    sane($ok(norm) && $ok(orig));

    // Collect segments, resolving . and ..
    u8cp segments[256];  // pairs of [start, end] pointers
    u64 seg_count = 0;
    b8 is_abs = path8gIsAbsolute(orig);

    a_dupcg(u8, rem, orig);
    u8cs seg = {};
    while (path8gNext(rem, seg) == OK) {
        // Skip empty segments and "."
        if ($empty(seg)) continue;
        if ($len(seg) == 1 && *seg[0] == '.') continue;

        // Handle ".."
        if ($len(seg) == 2 && seg[0][0] == '.' && seg[0][1] == '.') {
            if (seg_count > 0) {
                u8cp prev_start = segments[(seg_count - 1) * 2];
                u8cp prev_end = segments[(seg_count - 1) * 2 + 1];
                b8 prev_is_dotdot =
                    (prev_end - prev_start == 2 && prev_start[0] == '.' &&
                     prev_start[1] == '.');
                if (!prev_is_dotdot) {
                    seg_count--;  // Pop last segment
                    continue;
                }
            }
            if (!is_abs) {
                // Keep .. for relative paths that can't go higher
                test(seg_count < 128, PATHFAIL);
                segments[seg_count * 2] = seg[0];
                segments[seg_count * 2 + 1] = seg[1];
                seg_count++;
            }
            // For absolute paths, ignore .. at root
            continue;
        }

        // Regular segment
        test(seg_count < 128, PATHFAIL);
        segments[seg_count * 2] = seg[0];
        segments[seg_count * 2 + 1] = seg[1];
        seg_count++;
    }

    // Build normalized path
    norm[1] = norm[0];  // Reset output

    if (is_abs) {
        call(u8sFeed1, norm + 1, '/');
    }

    for (u64 i = 0; i < seg_count; i++) {
        if (i > 0) {
            call(u8sFeed1, norm + 1, '/');
        }
        u8cs seg_i = {segments[i * 2], segments[i * 2 + 1]};
        call(u8sFeed, norm + 1, seg_i);
    }

    // Handle empty result
    if (norm[0] == norm[1]) {
        u8c dot[1] = {'.'};
        u8cs dot_seg = {dot, dot + 1};
        call(u8sFeed, norm + 1, dot_seg);
    }

    call(path8gTerm, norm);
    done;
}
