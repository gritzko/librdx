//  REFS — thin ULOG glue for keeper's per-branch reflog.
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
    a_cstr(s, "set");
    a_dup(u8c, dup, s);
    RONutf8sDrain(&cached, dup);
    return cached;
}

// --- path builder ---

#define REFS_LOG_PATH(name, dir)                                \
    a_cstr(name##_fname, REFS_FILE);                            \
    a_path(name##_pbuf, dir, name##_fname);                     \
    a_dup(u8c, name, u8bDataC(name##_pbuf))

// --- append ---

//  Populate a `uri` component-wise so URIutf8Feed re-emits
//  `<from-uri>#?<sha>`.  `from` is lexed to pick up scheme/auth/path/
//  query; `to` is planted directly in the fragment (leading `?` kept).
static ok64 refs_uri_for_row(urip u_out, u8csc from, u8csc to,
                             u8 *frag_bytes, size_t frag_cap,
                             size_t *frag_len_out) {
    sane(u_out && from && to && frag_bytes);
    memset(u_out, 0, sizeof(*u_out));

    //  Lex `from` into components.  URILexer consumes `u.data`; re-seed
    //  afterwards so URIutf8Feed can walk the components.
    u_out->data[0] = from[0];
    u_out->data[1] = from[1];
    call(URILexer, u_out);
    u_out->data[0] = from[0];
    u_out->data[1] = from[1];

    //  Build the fragment payload into caller-owned storage: `?<sha>`.
    //  Accept `to` with or without a leading `?`; always emit exactly
    //  one.
    u8cs sha = {to[0], to[1]};
    if (!u8csEmpty(sha) && sha[0][0] == '?') u8csUsed(sha, 1);
    size_t need = 1 + (size_t)$len(sha);
    if (need > frag_cap) fail(REFSBAD);
    frag_bytes[0] = '?';
    memcpy(frag_bytes + 1, sha[0], (size_t)$len(sha));
    u_out->fragment[0] = frag_bytes;
    u_out->fragment[1] = frag_bytes + need;
    *frag_len_out = need;
    done;
}

//  Return max(RONNow(), tail_ts + 1).  ULOG enforces strict
//  monotonicity; RONNow()'s ms resolution means two rapid appends
//  would collide, so clamp past the tail.
static ron60 refs_next_ts(ulogcp l) {
    ron60 now = RONNow();
    u32 n = ULOGCount(l);
    if (n == 0) return now;
    ron60 ts = 0, verb = 0;
    uri u = {};
    if (ULOGTail(l, &ts, &verb, &u) != OK) return now;
    return now > ts ? now : ts + 1;
}

ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri) {
    sane($ok(dir) && $ok(from_uri) && $ok(to_uri));
    if (u8csEmpty(from_uri) || u8csEmpty(to_uri)) fail(REFSBAD);

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    call(ULOGOpen, &l, log_path);

    a_pad(u8, fragb, 256);
    uri u = {};
    size_t fl = 0;
    ok64 bo = refs_uri_for_row(&u, from_uri, to_uri,
                               u8bIdleHead(fragb), u8bIdleLen(fragb), &fl);
    if (bo != OK) { ULOGClose(&l); return bo; }

    ok64 o = ULOGAppendAt(&l, refs_next_ts(&l), refs_verb_set(), &u);
    ULOGClose(&l);
    return o;
}

ok64 REFSSyncRecord(u8csc dir, refcp arr, u32 nrefs) {
    sane($ok(dir) && nrefs > 0);
    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    call(ULOGOpen, &l, log_path);

    ron60 ts = refs_next_ts(&l);
    for (u32 i = 0; i < nrefs; i++) {
        u8csc from = {arr[i].key[0], arr[i].key[1]};
        u8csc to   = {arr[i].val[0], arr[i].val[1]};
        a_pad(u8, fragb, 256);
        uri u = {};
        size_t fl = 0;
        ok64 bo = refs_uri_for_row(&u, from, to,
                                   u8bIdleHead(fragb), u8bIdleLen(fragb), &fl);
        if (bo != OK) { ULOGClose(&l); return bo; }
        ok64 ao = ULOGAppendAt(&l, ts++, refs_verb_set(), &u);
        if (ao != OK) { ULOGClose(&l); return ao; }
    }
    ULOGClose(&l);
    done;
}

// --- load / each ---

typedef struct {
    refp  arr;
    u32   cnt;
    u32   max;
    u8bp  arena;
} refs_load_ctx;

//  Callback fed by ULOGeachLatest: capture (key = URI minus fragment,
//  val = fragment bytes) into the caller's arena, push a `ref` entry.
static ok64 refs_each_store(ron60 ts, ron60 verb, uricp u, void *ctx) {
    sane(u && ctx);
    (void)verb;
    refs_load_ctx *c = (refs_load_ctx *)ctx;
    if (c->cnt >= c->max) done;

    //  Key: URI with fragment elided, serialised straight into
    //  arena's IDLE (URIutf8Feed advances the idle slice in place).
    uri ku = *u;
    ku.fragment[0] = ku.fragment[1];  // empty the fragment slice
    u8 *kb = u8bIdleHead(c->arena);
    call(URIutf8Feed, u8bIdle(c->arena), &ku);
    u8 *ke = u8bIdleHead(c->arena);
    u8cs key_s = {kb, ke};

    //  Val: `?<sha>` — u.fragment is stored with the `?` already.
    u8 *vb = u8bIdleHead(c->arena);
    u8cs frag = {u->fragment[0], u->fragment[1]};
    call(u8bFeed, c->arena, frag);
    u8 *ve = u8bIdleHead(c->arena);
    u8cs val_s = {vb, ve};

    ref *e = &c->arr[c->cnt++];
    e->time   = ts;
    e->type   = REF_SHA;
    e->key[0] = key_s[0]; e->key[1] = key_s[1];
    e->val[0] = val_s[0]; e->val[1] = val_s[1];
    done;
}

ok64 REFSLoad(refp arr, u32p out_n, u32 max, u8b arena, u8csc dir) {
    sane(arr && out_n && u8bOK(arena));
    *out_n = 0;

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    ok64 oo = ULOGOpen(&l, log_path);
    if (oo != OK) done;  // missing file ⇒ 0 refs, not an error

    refs_load_ctx ctx = {arr, 0, max, arena};
    ok64 eo = ULOGeachLatest(&l, refs_verb_set(), refs_each_store, &ctx);
    ULOGClose(&l);
    if (eo != OK) return eo;
    *out_n = ctx.cnt;
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

// --- resolve ---

//  Matcher state for ULOGFindLatest's predicate.
typedef struct {
    u8cs host_needle;   // substring to match against row's host (or empty)
    u8cs in_query;      // query to match (heads|tags variant-aware)
    b8   auth_is_dot;   // input was `.?…` — accept any host
} match_ctx;

static b8 refs_host_match(u8csc host, u8csc needle) {
    if (u8csEmpty(needle)) return YES;
    size_t nl = u8csLen(needle);
    size_t hl = u8csLen(host);
    if (hl < nl) return NO;
    for (size_t off = 0; off + nl <= hl; off++)
        if (memcmp(host[0] + off, needle[0], nl) == 0) return YES;
    return NO;
}

//  in_query="heads/X" matches r_query="heads/X".
//  in_query="X"       matches r_query in {"X","heads/X","tags/X"}.
//  in_query=""        matches any row.
static b8 refs_query_match(u8csc in_query, u8csc r_query) {
    if (u8csEmpty(in_query)) return YES;
    if (u8csLen(in_query) == u8csLen(r_query) &&
        memcmp(in_query[0], r_query[0], u8csLen(in_query)) == 0)
        return YES;
    a_pad(u8, hbuf, 128);
    a_cstr(heads_pfx, "heads/");
    u8bFeed(hbuf, heads_pfx);
    u8bFeed(hbuf, in_query);
    a_dup(u8c, h, u8bData(hbuf));
    if (u8csLen(h) == u8csLen(r_query) &&
        memcmp(h[0], r_query[0], u8csLen(h)) == 0) return YES;
    a_pad(u8, tbuf, 128);
    a_cstr(tags_pfx, "tags/");
    u8bFeed(tbuf, tags_pfx);
    u8bFeed(tbuf, in_query);
    a_dup(u8c, t, u8bData(tbuf));
    if (u8csLen(t) == u8csLen(r_query) &&
        memcmp(t[0], r_query[0], u8csLen(t)) == 0) return YES;
    return NO;
}

static b8 refs_match_pred(uricp u, void *ctx) {
    match_ctx *m = (match_ctx *)ctx;
    u8cs r_host  = {u->host[0],  u->host[1]};
    u8cs r_query = {u->query[0], u->query[1]};
    if (!refs_query_match(m->in_query, r_query)) return NO;
    if (!m->auth_is_dot &&
        !u8csEmpty(m->host_needle) &&
        !refs_host_match(r_host, m->host_needle)) return NO;
    return YES;
}

static ok64 refs_capture_cs(u8bp arena, u8csc src, u8csp out) {
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

    //  Parse the input URI.
    uri in = {};
    in.data[0] = input[0]; in.data[1] = input[1];
    call(URILexer, &in);

    //  Strip a leading `refs/` from the query (`refs/heads/X` → `heads/X`).
    u8cs in_query = {in.query[0], in.query[1]};
    if ($len(in_query) > 5 && memcmp(in_query[0], "refs/", 5) == 0)
        u8csUsed(in_query, 5);

    //  Derive a host needle from input.host / authority / path.
    match_ctx m = {};
    m.in_query[0] = in_query[0];
    m.in_query[1] = in_query[1];
    if (!u8csEmpty(in.host)) {
        m.host_needle[0] = in.host[0];
        m.host_needle[1] = in.host[1];
        if (u8csLen(m.host_needle) == 1 && m.host_needle[0][0] == '.')
            m.auth_is_dot = YES;
    } else if (!u8csEmpty(in.authority)) {
        u8cs a = {in.authority[0], in.authority[1]};
        if ($len(a) >= 2 && a[0][0] == '/' && a[0][1] == '/') u8csUsed(a, 2);
        m.host_needle[0] = a[0];
        m.host_needle[1] = a[1];
        if (u8csLen(m.host_needle) == 1 && m.host_needle[0][0] == '.')
            m.auth_is_dot = YES;
    } else if (!u8csEmpty(in.path)) {
        if ($len(in.path) == 1 && in.path[0][0] == '.') m.auth_is_dot = YES;
    }
    if (m.auth_is_dot) {
        m.host_needle[0] = NULL;
        m.host_needle[1] = NULL;
    }

    //  No discriminator — bare-text input (no `?`, no `//`, no `.`)
    //  would otherwise match any row via the documented "empty
    //  in_query matches any row" rule.  Refuse instead of returning
    //  an arbitrary HEAD.
    if (u8csEmpty(m.in_query) && u8csEmpty(m.host_needle) && !m.auth_is_dot)
        fail(REFSNONE);

    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    ok64 oo = ULOGOpen(&l, log_path);
    if (oo != OK) fail(REFSNONE);

    ron60 ts = 0;
    uri u = {};
    ok64 fo = ULOGFindLatest(&l, refs_match_pred, &m, &ts, &u);
    if (fo != OK) { ULOGClose(&l); fail(REFSNONE); }

    //  Fill resolved.query = fragment bytes (minus leading `?`).
    u8cs frag = {u.fragment[0], u.fragment[1]};
    if (!u8csEmpty(frag) && frag[0][0] == '?') u8csUsed(frag, 1);
    if (!u8csEmpty(frag))
        call(refs_capture_cs, arena, frag, resolved->query);
    if (!u8csEmpty(u.scheme))
        call(refs_capture_cs, arena, u.scheme, resolved->scheme);
    u8cs r_host = {u.host[0], u.host[1]};
    if (!u8csEmpty(r_host))
        call(refs_capture_cs, arena, r_host, resolved->host);
    if (!u8csEmpty(u.path))
        call(refs_capture_cs, arena, u.path, resolved->path);

    ULOGClose(&l);
    done;
}

// --- compact ---

ok64 REFSCompact(u8csc dir) {
    sane($ok(dir));
    REFS_LOG_PATH(log_path, dir);
    ulog l = {};
    ok64 oo = ULOGOpen(&l, log_path);
    if (oo != OK) done;  // nothing to compact

    ok64 co = ULOGCompactLatest(&l, log_path, refs_verb_set());
    ULOGClose(&l);
    return co;
}
