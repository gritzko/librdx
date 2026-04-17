#include "REFS.h"

#include "abc/POL.h"
#include "abc/PRO.h"
#include <stdlib.h>
#include <time.h>

// --- Format one line: timestamp\tfrom\tto\n ---

static ok64 refs_format_line(u8bp buf, refcp r) {
    u8bReset(buf);
    RONutf8sFeed(u8bIdle(buf), r->time);
    u8bFeed1(buf, '\t');
    u8bFeed(buf, r->key);
    u8bFeed1(buf, '\t');
    u8bFeed(buf, r->val);
    u8bFeed1(buf, '\n');
    return OK;
}

// --- Open REFS file for append ---

static int refs_open_append(u8csc dir) {
    a_cstr(fname, REFS_FILE);
    a_path(path, dir, fname);
    return open((char *)u8bDataHead(path), O_WRONLY | O_CREAT | O_APPEND, 0644);
}

// --- Append ---

ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri) {
    sane($ok(dir) && $ok(from_uri) && $ok(to_uri));

    int fd = refs_open_append(dir);
    if (fd < 0) fail(REFSFAIL);

    ref r = {.time = RONNow()};
    r.key[0] = from_uri[0]; r.key[1] = from_uri[1];
    r.val[0] = to_uri[0];  r.val[1] = to_uri[1];

    a_pad(u8, line, 2048);
    ok64 o = refs_format_line(line, &r);
    if (o != OK) { close(fd); return o; }

    a_dup(u8c, data, u8bData(line));
    o = FILEFeed(fd, data);
    close(fd);
    if (o != OK) fail(REFSFAIL);

    done;
}

// --- Sync record: bulk append ref records ---

ok64 REFSSyncRecord(u8csc dir, refcp arr, u32 nrefs) {
    sane($ok(dir) && nrefs > 0);

    int fd = refs_open_append(dir);
    if (fd < 0) fail(REFSFAIL);

    a_pad(u8, line, 2048);

    for (u32 i = 0; i < nrefs; i++) {
        ok64 o = refs_format_line(line, &arr[i]);
        if (o != OK) { close(fd); return o; }
        a_dup(u8c, ldata, u8bData(line));
        o = FILEFeed(fd, ldata);
        if (o != OK) { close(fd); fail(REFSFAIL); }
    }

    close(fd);
    done;
}

// --- Parse one line into ref record ---
// Line: timestamp\tkey\tval (no trailing \n)

static b8 refs_parse_line(refp out, u8csc line) {
    if (u8csEmpty(line)) return NO;

    a_dup(u8c, rest, line);

    // find first \t → end of timestamp
    if (u8csFind(rest, '\t') != OK) return NO;
    u8cs ts_str = {line[0], rest[0]};
    u8csUsed(rest, 1);

    // find second \t → end of key
    u8cp key_head = rest[0];
    if (u8csFind(rest, '\t') != OK) return NO;
    out->key[0] = key_head;
    out->key[1] = rest[0];
    u8csUsed(rest, 1);

    // remainder is val
    if (u8csEmpty(rest)) return NO;
    u8csMv(out->val, rest);

    // decode timestamp
    a_dup(u8c, ts_dup, ts_str);
    out->time = 0;
    RONutf8sDrain(&out->time, ts_dup);

    // classify
    if (u8csLen(out->key) >= 2 &&
        out->key[0][0] == '/' && out->key[0][1] == '/')
        out->type = REF_ALIAS;
    else
        out->type = REF_SHA;

    return YES;
}

// --- Dedup helper: find or insert ---

static void refs_upsert(refp arr, u32 *n, u32 max, refcp entry) {
    for (u32 i = 0; i < *n; i++) {
        if (REFMatch(&arr[i], entry->key)) {
            if (entry->time >= arr[i].time) arr[i] = *entry;
            return;
        }
    }
    if (*n < max) arr[(*n)++] = *entry;
}

// --- Load: scan file, collect latest per key ---

ok64 REFSLoad(refp arr, u32p out_n, u32 max, u8bp *map, u8csc dir) {
    sane(arr && out_n && map);

    a_cstr(fname, REFS_FILE);
    a_path(path, dir, fname);
    ok64 o = FILEMapRO(map, $path(path));
    if (o != OK) { *out_n = 0; return OK; }

    u32 n = 0;
    a_dup(u8c, scan, u8bData(*map));

    while (!u8csEmpty(scan)) {
        a_dup(u8c, probe, scan);
        if (u8csFind(probe, '\n') == OK) {
            u8cs line = {scan[0], probe[0]};
            scan[0] = probe[0] + 1;

            ref entry = {};
            if (refs_parse_line(&entry, line))
                refs_upsert(arr, &n, max, &entry);
        } else {
            // last line, no trailing \n
            ref entry = {};
            if (refs_parse_line(&entry, scan))
                refs_upsert(arr, &n, max, &entry);
            break;
        }
    }

    *out_n = n;
    done;
}

// --- Each ---

ok64 REFSEach(u8csc dir, refs_cb cb, void *ctx) {
    sane($ok(dir) && cb != NULL);

    u8bp map = NULL;
    ref *arr = calloc(REFS_MAX_REFS, sizeof(ref));
    if (!arr) fail(REFSFAIL);

    u32 n = 0;
    ok64 o = REFSLoad(arr, &n, REFS_MAX_REFS, &map, dir);
    if (o != OK) { free(arr); return o; }

    for (u32 i = 0; i < n; i++) {
        o = cb(&arr[i], ctx);
        if (o != OK) break;
    }

    free(arr);
    if (map) u8bUnMap(map);
    done;
}

// --- Resolve ---

ok64 REFSResolve(urip resolved, u8bp arena, u8csc dir, u8csc input) {
    sane($ok(dir) && $ok(input) && resolved != NULL && arena != NULL);

    uri u = {};
    u.data[0] = input[0]; u.data[1] = input[1];
    call(URILexer, &u);

    u8bp map = NULL;
    ref *arr = calloc(REFS_MAX_REFS, sizeof(ref));
    if (!arr) fail(REFSFAIL);

    u32 n = 0;
    ok64 o = REFSLoad(arr, &n, REFS_MAX_REFS, &map, dir);
    if (o != OK) { free(arr); return o; }

    // resolve authority (alias): //name → full URL
    if (!u8csEmpty(u.authority)) {
        a_pad(u8, keybuf, 256);
        a_cstr(slashes, "//");
        u8bFeed(keybuf, slashes);
        u8bFeed(keybuf, u.authority);

        for (int chain = 0; chain < REFS_MAX_CHAIN; chain++) {
            a_dup(u8c, key, u8bData(keybuf));

            refp found = NULL;
            for (u32 i = 0; i < n; i++) {
                if (REFMatch(&arr[i], key)) { found = &arr[i]; break; }
            }
            if (!found) break;

            u8bFeed(arena, found->val);
            size_t vlen = u8csLen(found->val);
            u8cs aval = {u8bIdleHead(arena) - vlen, u8bIdleHead(arena)};

            uri next = {};
            u8csMv(next.data, aval);
            ok64 oo = URILexer(&next);
            if (oo != OK) break;

            if (!u8csEmpty(next.scheme)) {
                *resolved = next;
                break;
            }
            u8bReset(keybuf);
            u8bFeed(keybuf, found->val);
        }
    }

    // resolve query (ref): ?refname → ?sha. Chain through aliases —
    // e.g. `?HEAD → ?master`, `?master → ?<sha>` should resolve to
    // the SHA, not stop at the alias. Stop when we hit a value that
    // looks like a 40-char hex SHA (the terminal node) or a key with
    // no further binding.
    if (!u8csEmpty(u.query)) {
        a_pad(u8, qbuf, 1024);
        u8bFeed1(qbuf, '?');
        u8bFeed(qbuf, u.query);

        u8cs final_val = {};
        for (int chain = 0; chain < REFS_MAX_CHAIN; chain++) {
            a_dup(u8c, qkey, u8bData(qbuf));
            refp found = NULL;
            for (u32 i = 0; i < n; i++) {
                if (REFMatch(&arr[i], qkey)) { found = &arr[i]; break; }
            }
            if (!found) break;
            // Check if val[1..] is a 40-hex SHA — if so, we're done.
            u8cs vfull = {found->val[0], found->val[1]};
            if ($len(vfull) == 41 && vfull[0][0] == '?') {
                b8 is_sha = YES;
                for (int j = 1; j < 41; j++) {
                    u8 c = vfull[0][j];
                    if (!((c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'f') ||
                          (c >= 'A' && c <= 'F'))) {
                        is_sha = NO;
                        break;
                    }
                }
                if (is_sha) {
                    u8csMv(final_val, vfull);
                    break;
                }
            }
            // Not a SHA — treat val as the next key and chain.
            u8bReset(qbuf);
            u8bFeed(qbuf, vfull);
        }

        if (!u8csEmpty(final_val)) {
            // Strip leading '?' for the resolved->query slot.
            u8cs out = {};
            u8csMv(out, final_val);
            if (out[0][0] == '?') u8csUsed(out, 1);
            u8bFeed(arena, out);
            size_t vlen = u8csLen(out);
            resolved->query[0] = u8bIdleHead(arena) - vlen;
            resolved->query[1] = u8bIdleHead(arena);
        }
    }

    free(arr);
    if (map) u8bUnMap(map);
    done;
}

// --- Compact ---

ok64 REFSCompact(u8csc dir) {
    sane($ok(dir));

    u8bp map = NULL;
    ref *arr = calloc(REFS_MAX_REFS, sizeof(ref));
    if (!arr) fail(REFSFAIL);

    u32 n = 0;
    ok64 o = REFSLoad(arr, &n, REFS_MAX_REFS, &map, dir);
    if (o != OK) { free(arr); return o; }

    if (n == 0) {
        free(arr);
        if (map) u8bUnMap(map);
        done;
    }

    a_cstr(tmpname, "refs.tmp");
    a_cstr(fname, REFS_FILE);
    a_path(tmppath, dir, tmpname);
    int fd = -1;
    o = FILECreate(&fd, $path(tmppath));
    if (o != OK) { free(arr); if (map) u8bUnMap(map); fail(REFSFAIL); }

    a_pad(u8, line, 2048);
    for (u32 i = 0; i < n; i++) {
        ok64 oo = refs_format_line(line, &arr[i]);
        if (oo != OK) { close(fd); free(arr); if (map) u8bUnMap(map); return oo; }
        a_dup(u8c, ldata, u8bData(line));
        oo = FILEFeed(fd, ldata);
        if (oo != OK) { close(fd); free(arr); if (map) u8bUnMap(map); return oo; }
    }
    close(fd);
    free(arr);
    if (map) u8bUnMap(map);

    a_path(fpath, dir, fname);
    o = FILERename($path(tmppath), $path(fpath));
    if (o != OK) fail(REFSFAIL);

    done;
}
