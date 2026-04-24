//  REFS — ULOG-backed ref/tip reflog for keeper.
//  See REFS.h for the API and REF.md for the on-disk format.

#include "REFS.h"

#include <string.h>
#include <unistd.h>

#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RON.h"
#include "abc/URI.h"
#include "dog/ULOG.h"

// --- verb (cached ron60 of "set") ---

static ron60 refs_verb_set(void) {
    static ron60 cached = 0;
    if (cached) return cached;
    u8c raw[] = {'s', 'e', 't'};
    u8cs s = {raw, raw + sizeof(raw)};
    a_dup(u8c, dup, s);
    RONutf8sDrain(&cached, dup);
    return cached;
}

// --- path builder ---
//
//  Expands to: path8b `name##_pbuf` holding `<dir>/refs`, plus a
//  const slice view `name` (NUL-terminated) suitable for ULOGOpen.

#define REFS_LOG_PATH(name, dir)                                \
    a_cstr(name##_fname, REFS_FILE);                            \
    a_path(name##_pbuf, dir, name##_fname);                     \
    a_dup(u8c, name, u8bDataC(name##_pbuf))

// --- URI helpers ---

//  Parse `input` with URILexer into `u_out`, running against the bytes
//  of `dup` (a writable copy).  Leaves `u_out->data` pointing at the
//  pre-lex bytes so URIutf8Feed can re-serialise later if needed.
static ok64 refs_lex_uri(urip u_out, u8csc input) {
    sane(u_out);
    memset(u_out, 0, sizeof(*u_out));
    u_out->data[0] = input[0];
    u_out->data[1] = input[1];
    call(URILexer, u_out);
    //  URILexer consumes data; re-seed so URIutf8Feed can reproduce.
    u_out->data[0] = input[0];
    u_out->data[1] = input[1];
    done;
}

//  Build the URI bytes `<from>#?<sha>` into `out` (out must be empty).
//  `to` is accepted with or without a leading `?`; the leading `?` is
//  then re-emitted as the first byte of the fragment payload.
static ok64 refs_build_row_uri(u8b out, u8csc from, u8csc to) {
    sane(u8bOK(out));
    u8cs sha = {to[0], to[1]};
    if (!u8csEmpty(sha) && sha[0][0] == '?') u8csUsed(sha, 1);
    u8bFeed(out, from);
    u8bFeed1(out, '#');
    u8bFeed1(out, '?');
    u8bFeed(out, sha);
    done;
}

// --- append helpers (shared by REFSAppend / REFSSyncRecord / REFSCompact) ---

//  Read the log's current tail timestamp (0 if empty).  Caller has l open.
static ron60 refs_tail_ts(ulog const *l) {
    u32 n = ULOGCount(l);
    if (n == 0) return 0;
    ron60 ts = 0, verb = 0;
    uri u = {};
    if (ULOGTail(l, &ts, &verb, &u) != OK) return 0;
    return ts;
}

//  Append a single (from, to) row at timestamp `ts` (clamped above
//  the log's tail for monotonicity).  Returns the effective ts via
//  `*ts_inout`.
static ok64 refs_push_row(ulogp l, ron60 *ts_inout, u8csc from, u8csc to) {
    sane(l && ts_inout);
    a_pad(u8, urib, 2048);
    call(refs_build_row_uri, urib, from, to);
    a_dup(u8c, uri_bytes, u8bData(urib));

    uri u = {};
    call(refs_lex_uri, &u, uri_bytes);

    ron60 ts = *ts_inout;
    ron60 last = refs_tail_ts(l);
    if (ts <= last) ts = last + 1;

    call(ULOGAppendAt, l, ts, refs_verb_set(), &u);
    *ts_inout = ts;
    done;
}

// --- public: append ---

ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri) {
    sane($ok(dir) && $ok(from_uri) && $ok(to_uri));
    if (u8csEmpty(from_uri) || u8csEmpty(to_uri)) fail(REFSBAD);

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    call(ULOGOpen, &l, log_path);

    ron60 ts = RONNow();
    ok64 o = refs_push_row(&l, &ts, from_uri, to_uri);
    ULOGClose(&l);
    if (o != OK) return o;
    done;
}

ok64 REFSSyncRecord(u8csc dir, refcp arr, u32 nrefs) {
    sane($ok(dir) && nrefs > 0);
    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    call(ULOGOpen, &l, log_path);

    ron60 ts = RONNow();
    for (u32 i = 0; i < nrefs; i++) {
        u8csc from = {arr[i].key[0], arr[i].key[1]};
        u8csc to   = {arr[i].val[0], arr[i].val[1]};
        ok64 o = refs_push_row(&l, &ts, from, to);
        if (o != OK) { ULOGClose(&l); return o; }
        ts++;  // next row must exceed this one (refs_push_row clamps, too)
    }
    ULOGClose(&l);
    done;
}

// --- public: load / each ---

//  Split a re-serialised URI on its `#` separator.  Returns NO if no
//  fragment is present.
static b8 refs_split_hash(u8cs uri_bytes, u8csp key_out, u8csp val_out) {
    u8cp p = uri_bytes[0];
    u8cp e = uri_bytes[1];
    while (p < e && *p != '#') p++;
    if (p == e) return NO;
    key_out[0] = uri_bytes[0]; key_out[1] = p;
    val_out[0] = p + 1;        val_out[1] = e;
    return YES;
}

ok64 REFSLoad(refp arr, u32p out_n, u32 max, u8b arena, u8csc dir) {
    sane(arr && out_n && u8bOK(arena));
    *out_n = 0;

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    ok64 oo = ULOGOpen(&l, log_path);
    if (oo != OK) done;  // missing / unreadable file ⇒ 0 refs, no error

    u32 nrows = ULOGCount(&l);
    u32 cnt = 0;

    for (u32 i = 0; i < nrows; i++) {
        ron60 ts = 0, verb = 0;
        uri u = {};
        if (ULOGRow(&l, i, &ts, &verb, &u) != OK) continue;
        if (verb != refs_verb_set()) continue;

        //  Re-serialise into arena, then split on '#'.
        u8 *uri_head = u8bIdleHead(arena);
        if (URIutf8Feed(u8bIdle(arena), &u) != OK) continue;
        u8 *uri_term = u8bIdleHead(arena);
        u8cs uri_bytes = {uri_head, uri_term};

        u8cs key_s = {}, val_s = {};
        if (!refs_split_hash(uri_bytes, key_s, val_s)) continue;

        //  Upsert: replace existing entry with same key; later row wins.
        b8 replaced = NO;
        for (u32 j = 0; j < cnt; j++) {
            if (REFMatch(&arr[j], key_s)) {
                arr[j].time = ts;
                arr[j].key[0] = key_s[0]; arr[j].key[1] = key_s[1];
                arr[j].val[0] = val_s[0]; arr[j].val[1] = val_s[1];
                arr[j].type = REF_SHA;
                replaced = YES;
                break;
            }
        }
        if (!replaced && cnt < max) {
            ref *e = &arr[cnt++];
            e->time = ts;
            e->key[0] = key_s[0]; e->key[1] = key_s[1];
            e->val[0] = val_s[0]; e->val[1] = val_s[1];
            e->type = REF_SHA;
        }
    }

    ULOGClose(&l);
    *out_n = cnt;
    done;
}

ok64 REFSEach(u8csc dir, refs_cb cb, void *ctx) {
    sane($ok(dir) && cb != NULL);

    Bu8 arena = {};
    call(u8bMap, arena, (size_t)REFS_MAX_REFS * 320);

    ref *arr = calloc(REFS_MAX_REFS, sizeof(ref));
    if (!arr) { u8bUnMap(arena); fail(REFSFAIL); }

    u32 n = 0;
    ok64 o = REFSLoad(arr, &n, REFS_MAX_REFS, arena, dir);
    if (o != OK) { free(arr); u8bUnMap(arena); return o; }

    for (u32 i = 0; i < n; i++) {
        o = cb(&arr[i], ctx);
        if (o != OK) break;
    }

    free(arr);
    u8bUnMap(arena);
    done;
}

// --- public: resolve ---

//  Lower-level literal-prefix check.
static b8 refs_starts_with(u8csc s, char const *pfx) {
    size_t pl = strlen(pfx);
    if ((size_t)$len(s) < pl) return NO;
    return memcmp(s[0], pfx, pl) == 0;
}

//  Substring match: does haystack contain needle?  Empty needle matches.
static b8 refs_host_match(u8csc host, u8csc needle) {
    if (u8csEmpty(needle)) return YES;
    size_t nl = u8csLen(needle);
    size_t hl = u8csLen(host);
    if (hl < nl) return NO;
    for (size_t off = 0; off + nl <= hl; off++) {
        if (memcmp(host[0] + off, needle[0], nl) == 0) return YES;
    }
    return NO;
}

//  Refname equality / heads|tags-prefix variant match.
//    in_query="heads/master" matches r_query="heads/master".
//    in_query="master"       matches r_query in {"master","heads/master","tags/master"}.
//    in_query=""             matches any row (authority-only lookup).
static b8 refs_query_match(u8csc in_query, u8csc r_query) {
    if (u8csEmpty(in_query)) return YES;
    if (u8csLen(in_query) == u8csLen(r_query) &&
        memcmp(in_query[0], r_query[0], u8csLen(in_query)) == 0)
        return YES;
    //  Try heads/<in_query> and tags/<in_query>.
    a_pad(u8, hbuf, 128);
    a_cstr(heads_pfx, "heads/");
    u8bFeed(hbuf, heads_pfx);
    u8bFeed(hbuf, in_query);
    a_dup(u8c, h, u8bData(hbuf));
    if (u8csLen(h) == u8csLen(r_query) &&
        memcmp(h[0], r_query[0], u8csLen(h)) == 0)
        return YES;
    a_pad(u8, tbuf, 128);
    a_cstr(tags_pfx, "tags/");
    u8bFeed(tbuf, tags_pfx);
    u8bFeed(tbuf, in_query);
    a_dup(u8c, t, u8bData(tbuf));
    if (u8csLen(t) == u8csLen(r_query) &&
        memcmp(t[0], r_query[0], u8csLen(t)) == 0)
        return YES;
    return NO;
}

//  Feed `src` into `arena` and capture the written slice into `out`.
static ok64 refs_capture(u8bp arena, u8csc src, u8csp out) {
    sane(arena && out);
    u8 *head = u8bIdleHead(arena);
    call(u8bFeed, arena, src);
    out[0] = head;
    out[1] = u8bIdleHead(arena);
    done;
}

ok64 REFSResolve(urip resolved, u8bp arena, u8csc dir, u8csc input) {
    sane($ok(dir) && $ok(input) && resolved != NULL && arena != NULL);
    memset(resolved, 0, sizeof(*resolved));

    uri in = {};
    call(refs_lex_uri, &in, input);

    //  Normalise input query: strip leading `refs/`.
    u8cs in_query = {in.query[0], in.query[1]};
    if ($len(in_query) > 5 && memcmp(in_query[0], "refs/", 5) == 0)
        u8csUsed(in_query, 5);

    //  Host needle: prefer `in.host`, fall back to `in.authority` minus
    //  leading `//`.  `.` is treated as "any host" (local refs).
    u8cs host_needle = {};
    b8   auth_is_dot = NO;
    if (!u8csEmpty(in.host)) {
        host_needle[0] = in.host[0];
        host_needle[1] = in.host[1];
        if (u8csLen(host_needle) == 1 && host_needle[0][0] == '.')
            auth_is_dot = YES;
    } else if (!u8csEmpty(in.authority)) {
        u8cs a = {in.authority[0], in.authority[1]};
        if ($len(a) >= 2 && a[0][0] == '/' && a[0][1] == '/')
            u8csUsed(a, 2);
        host_needle[0] = a[0];
        host_needle[1] = a[1];
        if (u8csLen(host_needle) == 1 && host_needle[0][0] == '.')
            auth_is_dot = YES;
    } else if (!u8csEmpty(in.path)) {
        //  `.?ref` parses with path=`.` and empty authority.
        if ($len(in.path) == 1 && in.path[0][0] == '.') auth_is_dot = YES;
    }
    if (auth_is_dot) {
        host_needle[0] = NULL;
        host_needle[1] = NULL;
    }

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    ok64 oo = ULOGOpen(&l, log_path);
    if (oo != OK) fail(REFSNONE);

    u32 nrows = ULOGCount(&l);
    b8 found = NO;
    for (u32 i = nrows; i > 0 && !found; ) {
        i--;
        ron60 ts = 0, verb = 0;
        uri u = {};
        if (ULOGRow(&l, i, &ts, &verb, &u) != OK) continue;
        if (verb != refs_verb_set()) continue;

        u8cs r_host  = {u.host[0],  u.host[1]};
        u8cs r_query = {u.query[0], u.query[1]};
        if (!refs_query_match(in_query, r_query)) continue;
        if (!u8csEmpty(host_needle) && !refs_host_match(r_host, host_needle))
            continue;

        //  Fill resolved.query = terminal sha (strip leading `?`).
        u8cs frag = {u.fragment[0], u.fragment[1]};
        if (!u8csEmpty(frag) && frag[0][0] == '?') u8csUsed(frag, 1);
        if (!u8csEmpty(frag))
            call(refs_capture, arena, frag, resolved->query);

        //  Fill scheme/host/path from the matched row (for transport URI
        //  builders that previously leaned on `keeper alias`).
        if (!u8csEmpty(u.scheme))
            call(refs_capture, arena, u.scheme, resolved->scheme);
        if (!u8csEmpty(r_host))
            call(refs_capture, arena, r_host,  resolved->host);
        if (!u8csEmpty(u.path))
            call(refs_capture, arena, u.path,  resolved->path);

        found = YES;
    }
    ULOGClose(&l);
    if (!found) fail(REFSNONE);
    done;
}

// --- public: compact ---

ok64 REFSCompact(u8csc dir) {
    sane($ok(dir));

    Bu8 arena = {};
    call(u8bMap, arena, (size_t)REFS_MAX_REFS * 320);

    ref *arr = calloc(REFS_MAX_REFS, sizeof(ref));
    if (!arr) { u8bUnMap(arena); fail(REFSFAIL); }

    u32 n = 0;
    ok64 lo = REFSLoad(arr, &n, REFS_MAX_REFS, arena, dir);
    if (lo != OK) { free(arr); u8bUnMap(arena); return lo; }
    if (n == 0) { free(arr); u8bUnMap(arena); done; }

    //  Sort kept entries by time so monotonicity holds on rewrite.
    //  Insertion sort — N ≤ REFS_MAX_REFS and REFSCompact is rare.
    for (u32 i = 1; i < n; i++) {
        ref tmp = arr[i];
        u32 j = i;
        while (j > 0 && arr[j - 1].time > tmp.time) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = tmp;
    }

    //  Build tmp path `<dir>/refs.tmp`, unlink any stale file, rewrite.
    a_cstr(tmp_fname, "refs.tmp");
    a_path(tmp_pbuf, dir, tmp_fname);
    a_dup(u8c, tmp_path, u8bDataC(tmp_pbuf));
    (void)unlink((char const *)tmp_path[0]);

    ulog nl = {};
    ok64 to = ULOGOpen(&nl, tmp_path);
    if (to != OK) { free(arr); u8bUnMap(arena); return to; }

    for (u32 i = 0; i < n; i++) {
        u8csc from = {arr[i].key[0], arr[i].key[1]};
        u8csc to_v = {arr[i].val[0], arr[i].val[1]};
        a_pad(u8, urib, 2048);
        ok64 bo = refs_build_row_uri(urib, from, to_v);
        if (bo != OK) { ULOGClose(&nl); free(arr); u8bUnMap(arena); return bo; }
        a_dup(u8c, uri_bytes, u8bData(urib));

        uri u = {};
        if (refs_lex_uri(&u, uri_bytes) != OK) {
            ULOGClose(&nl); free(arr); u8bUnMap(arena);
            fail(REFSBAD);
        }
        ok64 ao = ULOGAppendAt(&nl, arr[i].time, refs_verb_set(), &u);
        if (ao != OK) {
            ULOGClose(&nl); free(arr); u8bUnMap(arena);
            fail(REFSFAIL);
        }
    }
    ULOGClose(&nl);

    free(arr);
    u8bUnMap(arena);

    REFS_LOG_PATH(log_path, dir);
    call(FILERename, tmp_path, log_path);
    done;
}
