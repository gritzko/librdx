#include "REFS.h"

#include "abc/PRO.h"
#include <stdlib.h>
#include <time.h>

static ron60 refs_now(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    ron60 r = 0;
    RONOfTime(&r, tm);
    return r;
}

// --- helpers ---

static ok64 refs_format_line(u8bp buf, ron60 stamp, u8csc from, u8csc to) {
    u8bReset(buf);
    size_t before = u8bIdleLen(buf);
    ok64 o = RONutf8sFeed(u8bIdle(buf), stamp);
    if (o != OK) return o;

    u8p wp = u8bIdleHead(buf);
    *wp++ = '\t';
    memcpy(wp, from[0], $len(from));
    wp += $len(from);
    *wp++ = '\t';
    memcpy(wp, to[0], $len(to));
    wp += $len(to);
    *wp++ = '\n';
    // feed everything we wrote (RON already advanced idle, now add the rest)
    size_t ron_wrote = before - u8bIdleLen(buf);
    u8bFed(buf, (size_t)(wp - u8bIdleHead(buf)));
    (void)ron_wrote;
    return OK;
}

// --- Append ---

ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri) {
    sane($ok(dir) && $ok(from_uri) && $ok(to_uri));

    a_cstr(fname, REFS_FILE);
    a_path(path, dir, fname);
    int fd = -1;
    fd = open((char *)u8bDataHead(path), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) fail(REFSFAIL);

    a_pad(u8, line, 2048);
    ok64 o = refs_format_line(line, refs_now(), from_uri, to_uri);
    if (o != OK) { close(fd); return o; }

    a_dup(u8c, data, u8bData(line));
    o = FILEFeed(fd, data);
    close(fd);
    if (o != OK) fail(REFSFAIL);

    done;
}

// --- Sync record: bulk append ref→SHA pairs ---

ok64 REFSSyncRecord(u8csc dir, u8css refs, u8css shas, u32 nrefs) {
    sane($ok(dir) && nrefs > 0);

    a_cstr(fname, REFS_FILE);
    a_path(path, dir, fname);
    int fd = -1;
    fd = open((char *)u8bDataHead(path), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) fail(REFSFAIL);

    ron60 now = refs_now();
    a_pad(u8, line, 2048);

    for (u32 i = 0; i < nrefs; i++) {
        // build "?refname" and "?sha" on stack
        a_pad(u8, fbuf, 256);
        u8p fp = u8bIdleHead(fbuf);
        *fp++ = '?';
        memcpy(fp, refs[i][0], $len(refs[i]));
        fp += $len(refs[i]);
        u8bFed(fbuf, (size_t)(fp - u8bIdleHead(fbuf)));

        a_pad(u8, tbuf, 256);
        u8p tp = u8bIdleHead(tbuf);
        *tp++ = '?';
        memcpy(tp, shas[i][0], $len(shas[i]));
        tp += $len(shas[i]);
        u8bFed(tbuf, (size_t)(tp - u8bIdleHead(tbuf)));

        a_dup(u8c, fslice, u8bData(fbuf));
        a_dup(u8c, tslice, u8bData(tbuf));
        ok64 o = refs_format_line(line, now, fslice, tslice);
        if (o != OK) { close(fd); return o; }
        a_dup(u8c, ldata, u8bData(line));
        o = FILEFeed(fd, ldata);
        if (o != OK) { close(fd); fail(REFSFAIL); }
    }

    close(fd);
    done;
}

// --- Parse one line: extract timestamp, from, to slices ---

// All slices point into the scanned buffer (mmap).
typedef struct {
    u8cs ts_str;
    u8cs from;
    u8cs to;
} refs_line;

static b8 refs_parse_line(refs_line *out, u8cp lstart, u8cp lend) {
    if (lstart >= lend) return NO;

    u8cp tab1 = lstart;
    while (tab1 < lend && *tab1 != '\t') tab1++;
    if (tab1 >= lend) return NO;

    u8cp fstart = tab1 + 1;
    u8cp tab2 = fstart;
    while (tab2 < lend && *tab2 != '\t') tab2++;
    if (tab2 >= lend) return NO;

    u8cp tstart = tab2 + 1;
    if (tstart >= lend) return NO;

    out->ts_str[0] = lstart; out->ts_str[1] = tab1;
    out->from[0] = fstart;   out->from[1] = tab2;
    out->to[0] = tstart;     out->to[1] = lend;
    return YES;
}

static ron60 refs_decode_stamp(u8cs ts_str) {
    ron60 stamp = 0;
    a_dup(u8c, ts_dup, ts_str);
    RONutf8sDrain(&stamp, ts_dup);
    return stamp;
}

// --- Each: latest value per from-key ---

typedef struct {
    u8cp  from0[REFS_MAX_REFS];
    u8cp  from1[REFS_MAX_REFS];
    u8cp  to0[REFS_MAX_REFS];
    u8cp  to1[REFS_MAX_REFS];
    ron60 stamps[REFS_MAX_REFS];
    u32   n;
} refs_latest;

static void refs_collect(refs_latest *r, u8cs from, u8cs to, ron60 stamp) {
    for (u32 i = 0; i < r->n; i++) {
        size_t flen = (size_t)(r->from1[i] - r->from0[i]);
        if (flen == $len(from) &&
            memcmp(r->from0[i], from[0], flen) == 0) {
            if (stamp >= r->stamps[i]) {
                r->to0[i] = to[0];
                r->to1[i] = to[1];
                r->stamps[i] = stamp;
            }
            return;
        }
    }

    if (r->n >= REFS_MAX_REFS) return;
    u32 j = r->n;
    r->from0[j] = from[0];
    r->from1[j] = from[1];
    r->to0[j] = to[0];
    r->to1[j] = to[1];
    r->stamps[j] = stamp;
    r->n++;
}

// Scan mapped file, collect latest entry per from-key
static void refs_scan_collect(u8bp map, refs_latest *r) {
    a_dup(u8c, scan, u8bData(map));

    while (!$empty(scan)) {
        u8cp lstart = scan[0];
        while (scan[0] < scan[1] && *scan[0] != '\n') scan[0]++;
        u8cp lend = scan[0];
        if (scan[0] < scan[1]) scan[0]++;

        refs_line ln = {};
        if (!refs_parse_line(&ln, lstart, lend)) continue;
        ron60 stamp = refs_decode_stamp(ln.ts_str);
        refs_collect(r, ln.from, ln.to, stamp);
    }
}

ok64 REFSEach(u8csc dir, refs_cb cb, void *ctx) {
    sane($ok(dir) && cb != NULL);

    a_cstr(fname, REFS_FILE);
    a_path(path, dir, fname);
    u8bp map = NULL;
    ok64 o = FILEMapRO(&map, PATHu8cgIn(path));
    if (o != OK) { done; }

    refs_latest *r = calloc(1, sizeof(refs_latest));
    if (!r) { u8bUnMap(map); fail(REFSFAIL); }

    refs_scan_collect(map, r);

    for (u32 i = 0; i < r->n; i++) {
        u8cs from = {r->from0[i], r->from1[i]};
        u8cs to = {r->to0[i], r->to1[i]};
        o = cb(from, to, r->stamps[i], ctx);
        if (o != OK) break;
    }

    free(r);
    u8bUnMap(map);
    done;
}

// --- Resolve: find latest value for a given key ---

static b8 refs_find_latest(u8bp map, u8csc key, u8cs *out, ron60 *out_stamp) {
    a_dup(u8c, scan, u8bData(map));
    b8 found = NO;

    while (!$empty(scan)) {
        u8cp lstart = scan[0];
        while (scan[0] < scan[1] && *scan[0] != '\n') scan[0]++;
        u8cp lend = scan[0];
        if (scan[0] < scan[1]) scan[0]++;

        refs_line ln = {};
        if (!refs_parse_line(&ln, lstart, lend)) continue;

        if ($len(ln.from) == $len(key) &&
            memcmp(ln.from[0], key[0], $len(key)) == 0) {
            ron60 stamp = refs_decode_stamp(ln.ts_str);
            if (!found || stamp >= *out_stamp) {
                (*out)[0] = ln.to[0];
                (*out)[1] = ln.to[1];
                *out_stamp = stamp;
                found = YES;
            }
        }
    }

    return found;
}

ok64 REFSResolve(urip resolved, u8bp arena, u8csc dir, u8csc input) {
    sane($ok(dir) && $ok(input) && resolved != NULL && arena != NULL);

    // parse input URI
    uri u = {};
    u.data[0] = input[0];
    u.data[1] = input[1];
    call(URILexer, &u);

    a_cstr(fname, REFS_FILE);
    a_path(fpath, dir, fname);
    u8bp map = NULL;
    ok64 o = FILEMapRO(&map, PATHu8cgIn(fpath));
    if (o != OK) { done; }  // no refs file

    // resolve authority (alias): //name → full URL
    if (!$empty(u.authority)) {
        a_pad(u8, keybuf, 256);
        u8p kp = u8bIdleHead(keybuf);
        *kp++ = '/'; *kp++ = '/';
        memcpy(kp, u.authority[0], $len(u.authority));
        kp += $len(u.authority);
        u8bFed(keybuf, (size_t)(kp - u8bIdleHead(keybuf)));

        for (int chain = 0; chain < REFS_MAX_CHAIN; chain++) {
            a_dup(u8c, key, u8bData(keybuf));

            u8cs val = {};
            ron60 stamp = 0;
            if (!refs_find_latest(map, key, &val, &stamp)) break;

            // copy to arena, parse as URI
            u8p ap = u8bIdleHead(arena);
            memcpy(ap, val[0], $len(val));
            u8bFed(arena, $len(val));
            u8cs aval = {ap, ap + $len(val)};

            uri next = {};
            u8csMv(next.data, aval);
            ok64 oo = URILexer(&next);
            if (oo != OK) break;

            if (!$empty(next.scheme)) {
                *resolved = next;
                break;
            }
            // chase: rewrite keybuf
            u8bReset(keybuf);
            u8p kp2 = u8bIdleHead(keybuf);
            memcpy(kp2, val[0], $len(val));
            u8bFed(keybuf, $len(val));
        }
    }

    // resolve query (ref): ?refname → ?sha
    if (!$empty(u.query)) {
        a_pad(u8, qbuf, 256);
        u8p qp = u8bIdleHead(qbuf);
        *qp++ = '?';
        memcpy(qp, u.query[0], $len(u.query));
        qp += $len(u.query);
        u8bFed(qbuf, (size_t)(qp - u8bIdleHead(qbuf)));

        a_dup(u8c, qkey, u8bData(qbuf));

        u8cs val = {};
        ron60 stamp = 0;
        if (refs_find_latest(map, qkey, &val, &stamp)) {
            // strip leading ?, copy to arena
            u8cp vp = val[0];
            if ($len(val) > 0 && *vp == '?') vp++;
            size_t vlen = (size_t)(val[1] - vp);
            u8p ap = u8bIdleHead(arena);
            memcpy(ap, vp, vlen);
            u8bFed(arena, vlen);
            resolved->query[0] = ap;
            resolved->query[1] = ap + vlen;
        }
    }

    u8bUnMap(map);
    done;
}

// --- Compact ---

ok64 REFSCompact(u8csc dir) {
    sane($ok(dir));

    a_cstr(fname, REFS_FILE);
    a_path(fpath, dir, fname);
    u8bp map = NULL;
    ok64 o = FILEMapRO(&map, PATHu8cgIn(fpath));
    if (o != OK) { done; }

    refs_latest *r = calloc(1, sizeof(refs_latest));
    if (!r) { u8bUnMap(map); fail(REFSFAIL); }

    refs_scan_collect(map, r);

    if (r->n == 0) {
        free(r);
        u8bUnMap(map);
        done;
    }

    // write compacted to REFS.tmp
    a_cstr(tmpname, "REFS.tmp");
    a_path(tmppath, dir, tmpname);
    int fd = -1;
    o = FILECreate(&fd, PATHu8cgIn(tmppath));
    if (o != OK) { free(r); u8bUnMap(map); fail(REFSFAIL); }

    a_pad(u8, line, 2048);
    for (u32 i = 0; i < r->n; i++) {
        u8cs from = {r->from0[i], r->from1[i]};
        u8cs to = {r->to0[i], r->to1[i]};
        ok64 oo = refs_format_line(line, r->stamps[i], from, to);
        if (oo != OK) { close(fd); free(r); u8bUnMap(map); return oo; }
        a_dup(u8c, ldata, u8bData(line));
        oo = FILEFeed(fd, ldata);
        if (oo != OK) { close(fd); free(r); u8bUnMap(map); return oo; }
    }
    close(fd);
    free(r);
    u8bUnMap(map);

    o = FILERename(PATHu8cgIn(tmppath), PATHu8cgIn(fpath));
    if (o != OK) fail(REFSFAIL);

    done;
}
