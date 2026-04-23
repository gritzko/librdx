//  ULOG — append-only URI event log: round-trip + lookup coverage.
//
//  Cases:
//    1. Open-empty + append three rows + Count/Head/Tail.
//    2. Persistence: close, reopen, verify all rows survive.
//    3. Random access: Row(i) recovers per-row ts + verb + URI.
//    4. Seek/Find/Has: lower_bound, exact, membership.
//    5. Monotonicity: AppendAt with stale ts returns ULOGCLOCK.
//    6. FindVerb: reverse scan picks the latest row for that verb.
//    7. Truncate: keep_n rewinds both book and idx.
//    8. Streaming Feed / Drain round-trip.
//    9. Whitespace tolerance: multi-space / multi-tab separators.

#include "dog/ULOG.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

static ok64 rm_tmp(char const *p) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", p);
    int _ = system(cmd);
    (void)_;
    return OK;
}

#define LOGPATH(name, text)                                             \
    a_pad(u8, name##_buf, FILE_PATH_MAX_LEN);                           \
    a_cstr(name##_cstr, text);                                          \
    u8bFeed(name##_buf, name##_cstr);                                   \
    u8bFeed1(name##_buf, 0);                                            \
    a_dup(u8c, name, u8bData(name##_buf))

//  Parse + keep a raw-text copy so test predicates can compare on the
//  original URI bytes independent of URILexer's consume semantics.
typedef struct { uri u; char raw[512]; } saved_uri;

static ok64 parse_uri_lit(saved_uri *s, char const *text) {
    size_t n = strlen(text);
    if (n >= sizeof(s->raw)) return TESTFAIL;
    memcpy(s->raw, text, n);
    s->raw[n] = 0;
    memset(&s->u, 0, sizeof(s->u));
    s->u.data[0] = (u8cp)s->raw;
    s->u.data[1] = (u8cp)s->raw + n;
    return URILexer(&s->u);
}

//  After URILexer, data is consumed; re-seed from the preserved raw
//  text before handing to ULOGAppend (which re-serialises components).
static uricp saved_uri_for_append(saved_uri *s) {
    size_t n = strlen(s->raw);
    s->u.data[0] = (u8cp)s->raw;
    s->u.data[1] = (u8cp)s->raw + n;
    return &s->u;
}

//  Re-serialise a parsed uri into a local buffer and compare bytes
//  against a literal.  Captures the round-trip through URIutf8Feed,
//  which is what ULOG emits.
static b8 uri_serializes_to(uricp u, char const *expect) {
    a_pad(u8, buf, 512);
    if (URIutf8Feed(u8bIdle(buf), u) != OK) return NO;
    a_dup(u8c, got, u8bData(buf));
    size_t el = strlen(expect);
    return (size_t)$len(got) == el && memcmp(got[0], expect, el) == 0;
}

static ron60 verb_of(char const *s) {
    ron60 v = 0;
    u8cs slice = {(u8cp)s, (u8cp)s + strlen(s)};
    a_dup(u8c, dup, slice);
    RONutf8sDrain(&v, dup);
    return v;
}

static ok64 T_roundtrip(void) {
    sane(1);
    call(FILEInit);
    rm_tmp("/tmp/ulog-rt.log");
    LOGPATH(path, "/tmp/ulog-rt.log");

    ulog l = {};
    call(ULOGOpen, &l, path);
    want(ULOGCount(&l) == 0);

    saved_uri s1 = {}, s2 = {}, s3 = {};
    call(parse_uri_lit, &s1, "//localhost/repo?heads/master");
    call(parse_uri_lit, &s2, "//localhost/repo?staging/abcd1234");
    call(parse_uri_lit, &s3, "?heads/main");

    call(ULOGAppendAt, &l, 1000, verb_of("get"),
         saved_uri_for_append(&s1));
    call(ULOGAppendAt, &l, 1001, verb_of("put"),
         saved_uri_for_append(&s2));
    call(ULOGAppendAt, &l, 1002, verb_of("post"),
         saved_uri_for_append(&s3));
    want(ULOGCount(&l) == 3);

    ron60 ts = 0, verb = 0;
    uri got = {};
    call(ULOGHead, &l, &ts, &verb, &got);
    want(verb == verb_of("get"));
    want(uri_serializes_to(&got, "//localhost/repo?heads/master"));
    call(ULOGTail, &l, &ts, &verb, &got);
    want(verb == verb_of("post"));
    want(uri_serializes_to(&got, "?heads/main"));

    call(ULOGClose, &l);
    done;
}

static ok64 T_persist(void) {
    sane(1);
    LOGPATH(path, "/tmp/ulog-rt.log");

    ulog l = {};
    call(ULOGOpen, &l, path);
    want(ULOGCount(&l) == 3);

    ron60 ts = 0, verb = 0;
    uri got = {};
    call(ULOGRow, &l, 1, &ts, &verb, &got);
    want(verb == verb_of("put"));
    want(uri_serializes_to(&got, "//localhost/repo?staging/abcd1234"));

    call(ULOGClose, &l);
    rm_tmp("/tmp/ulog-rt.log");
    done;
}

static ok64 T_seek(void) {
    sane(1);
    rm_tmp("/tmp/ulog-sk.log");
    LOGPATH(path, "/tmp/ulog-sk.log");

    ulog l = {};
    call(ULOGOpen, &l, path);

    ron60 stamps[] = {100, 200, 300, 400};
    saved_uri s = {};
    call(parse_uri_lit, &s, "//h/p");
    for (u32 i = 0; i < 4; i++)
        call(ULOGAppendAt, &l, stamps[i], verb_of("get"),
             saved_uri_for_append(&s));
    want(ULOGCount(&l) == 4);

    u32 i = 99;
    call(ULOGSeek, &l, 250, &i);    want(i == 2);
    call(ULOGSeek, &l, 200, &i);    want(i == 1);
    call(ULOGSeek, &l, 500, &i);    want(i == 4);
    call(ULOGSeek, &l, 50,  &i);    want(i == 0);

    call(ULOGFind, &l, 300, &i);    want(i == 2);
    want(ULOGHas(&l, 100) == YES);
    want(ULOGHas(&l, 250) == NO);
    want(ULOGHas(&l, 400) == YES);
    want(ULOGFind(&l, 250, &i) == ULOGNONE);

    call(ULOGClose, &l);
    rm_tmp("/tmp/ulog-sk.log");
    done;
}

static ok64 T_clock(void) {
    sane(1);
    rm_tmp("/tmp/ulog-ck.log");
    LOGPATH(path, "/tmp/ulog-ck.log");

    ulog l = {};
    call(ULOGOpen, &l, path);

    saved_uri s = {};
    call(parse_uri_lit, &s, "//h/p");
    call(ULOGAppendAt, &l, 1000, verb_of("get"),
         saved_uri_for_append(&s));

    want(ULOGAppendAt(&l, 1000, verb_of("get"),
                      saved_uri_for_append(&s)) == ULOGCLOCK);
    want(ULOGAppendAt(&l,  999, verb_of("get"),
                      saved_uri_for_append(&s)) == ULOGCLOCK);
    call(ULOGAppendAt, &l, 1001, verb_of("get"),
         saved_uri_for_append(&s));
    want(ULOGCount(&l) == 2);

    call(ULOGClose, &l);
    rm_tmp("/tmp/ulog-ck.log");
    done;
}

static ok64 T_findverb(void) {
    sane(1);
    rm_tmp("/tmp/ulog-fv.log");
    LOGPATH(path, "/tmp/ulog-fv.log");

    ulog l = {};
    call(ULOGOpen, &l, path);

    saved_uri s1 = {}, s2 = {}, s3 = {}, s4 = {};
    call(parse_uri_lit, &s1, "?heads/master");
    call(parse_uri_lit, &s2, "?staging/deadbeef");
    call(parse_uri_lit, &s3, "?heads/main");
    call(parse_uri_lit, &s4, "?staging/cafef00d");

    call(ULOGAppendAt, &l, 10, verb_of("get"),  saved_uri_for_append(&s1));
    call(ULOGAppendAt, &l, 20, verb_of("put"),  saved_uri_for_append(&s2));
    call(ULOGAppendAt, &l, 30, verb_of("post"), saved_uri_for_append(&s3));
    call(ULOGAppendAt, &l, 40, verb_of("put"),  saved_uri_for_append(&s4));

    ron60 ts = 0;
    uri got = {};
    call(ULOGFindVerb, &l, verb_of("put"), &ts, &got);
    want(ts == 40);
    want(uri_serializes_to(&got, "?staging/cafef00d"));

    call(ULOGFindVerb, &l, verb_of("post"), &ts, &got);
    want(ts == 30);
    want(uri_serializes_to(&got, "?heads/main"));

    want(ULOGFindVerb(&l, verb_of("patch"), &ts, &got) == ULOGNONE);

    call(ULOGClose, &l);
    rm_tmp("/tmp/ulog-fv.log");
    done;
}

static ok64 T_truncate(void) {
    sane(1);
    rm_tmp("/tmp/ulog-tr.log");
    LOGPATH(path, "/tmp/ulog-tr.log");

    ulog l = {};
    call(ULOGOpen, &l, path);

    saved_uri s = {};
    call(parse_uri_lit, &s, "//h/p");
    for (u32 i = 0; i < 5; i++)
        call(ULOGAppendAt, &l, (ron60)(100 + i), verb_of("get"),
             saved_uri_for_append(&s));
    want(ULOGCount(&l) == 5);

    call(ULOGTruncate, &l, 3);
    want(ULOGCount(&l) == 3);

    ron60 ts = 0, verb = 0;
    uri got = {};
    call(ULOGTail, &l, &ts, &verb, &got);
    want(ts == 102);

    call(ULOGAppendAt, &l, 200, verb_of("get"),
         saved_uri_for_append(&s));
    want(ULOGCount(&l) == 4);

    call(ULOGClose, &l);

    ulog l2 = {};
    call(ULOGOpen, &l2, path);
    want(ULOGCount(&l2) == 4);
    call(ULOGTail, &l2, &ts, &verb, &got);
    want(ts == 200);
    call(ULOGClose, &l2);
    rm_tmp("/tmp/ulog-tr.log");
    done;
}

//  Feed N rows into a scratch buffer, Drain them back; verb, ts and
//  the re-serialized URI must match.
static ok64 T_stream(void) {
    sane(1);
    a_pad(u8, buf, 1024);

    saved_uri s1 = {}, s2 = {};
    call(parse_uri_lit, &s1, "//host/path?heads/main");
    call(parse_uri_lit, &s2, "?staging/0123");

    call(ULOGu8sFeed, u8bIdle(buf), 1000, verb_of("get"),
         saved_uri_for_append(&s1));
    call(ULOGu8sFeed, u8bIdle(buf), 1001, verb_of("put"),
         saved_uri_for_append(&s2));

    a_dup(u8c, data, u8bData(buf));
    u8cs scan = {data[0], data[1]};

    ron60 ts = 0, verb = 0;
    uri got = {};
    call(ULOGu8sDrain, scan, &ts, &verb, &got);
    want(ts == 1000 && verb == verb_of("get"));
    want(uri_serializes_to(&got, "//host/path?heads/main"));

    call(ULOGu8sDrain, scan, &ts, &verb, &got);
    want(ts == 1001 && verb == verb_of("put"));
    want(uri_serializes_to(&got, "?staging/0123"));

    want(u8csEmpty(scan) == YES);
    want(ULOGu8sDrain(scan, &ts, &verb, &got) == NODATA);

    //  Partial row (no '\n') → NODATA, scan unchanged.
    u8c partial[] = "1000\tget\thttp://x";
    u8cs pscan = {partial, partial + sizeof(partial) - 1};
    u8cp pstart = pscan[0];
    want(ULOGu8sDrain(pscan, &ts, &verb, &got) == NODATA);
    want(pscan[0] == pstart);
    done;
}

//  Multi-space / multi-tab separators must parse the same as a single
//  tab.  Trailing whitespace after the URI is not part of the spec
//  (URIs terminate at '\n') so we don't test that case.
static ok64 T_whitespace(void) {
    sane(1);
    //  Build a canonical single-tab row via Feed, then synthesize a
    //  sloppy variant with wide whitespace and confirm Drain parses
    //  both the same way.
    a_pad(u8, canon, 256);
    saved_uri su = {};
    call(parse_uri_lit, &su, "//host/path");
    call(ULOGu8sFeed, u8bIdle(canon), 1000, verb_of("post"),
         saved_uri_for_append(&su));

    u8c wide_row[] = "Fd  \t post \t\t //host/path\n";
    u8cs wide_scan = {wide_row, wide_row + sizeof(wide_row) - 1};

    ron60 ts = 0, verb = 0;
    uri got = {};
    call(ULOGu8sDrain, wide_scan, &ts, &verb, &got);
    want(ts == 1000);
    want(verb == verb_of("post"));
    want(uri_serializes_to(&got, "//host/path"));
    done;
}

ok64 ULOGtest(void) {
    sane(1);
    call(T_roundtrip);
    call(T_persist);
    call(T_seek);
    call(T_clock);
    call(T_findverb);
    call(T_truncate);
    call(T_stream);
    call(T_whitespace);
    done;
}

TEST(ULOGtest);
