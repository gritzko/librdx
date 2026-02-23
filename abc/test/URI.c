#include "URI.h"

#include <stdio.h>
#include <stdlib.h>

#include "B.h"
#include "FILE.h"
#include "PRO.h"
#include "TEST.h"

ok64 URItest1() {
    sane(1);
#define LEN1 5
    u8cs inputs[LEN1] = {
        $u8str("http://mit.edu"),
        $u8str("git+ssh://git@github.com/gritzko/librdx"),
        $u8str("ftp://1.2.3.4/some/path"),
        $u8str("//1.2.3.4/some/path"),
        $u8str("http://myserver:123/path?query#fragment"),
    };

    for (int i = 0; i < LEN1; ++i) {
        URIstate state = {};
        $mv(state.data, inputs[i]);
        call(URILexer, &state);
    }
    done;
}

ok64 URItest2() {
    sane(1);
    a$str(uri, "http://myserver:123/path?query#fragment");
    a$str(scheme, "http");
    a$str(host, "myserver");
    a$str(port, "123");
    a$str(path, "/path");
    a$str(query, "query");
    a$str(fragment, "fragment");
    URIstate state = {};
    $mv(state.data, uri);
    call(URILexer, &state);
    $println(state.scheme);
    $testeq(state.scheme, scheme);
    $testeq(state.host, host);
    $testeq(state.port, port);
    $testeq(state.path, path);
    $testeq(state.query, query);
    $testeq(state.fragment, fragment);
    done;
}

ok64 URItest3() {
    sane(1);
    a$str(uri, "http://user@myserver:123/path?query#fragment");
    URIstate state = {};
    call(URIutf8Drain, uri, &state);
    a_pad(u8, out, 256);
    call(URIutf8Feed, out_idle, &state);
    u8cs result = {out[1], *out_idle};
    $testeq(result, uri);
    done;
}

ok64 URItest4() {
    sane(1);
    u8cs inputs[] = {
        $u8str("http://mit.edu"),
        $u8str("git+ssh://git@github.com/gritzko/librdx"),
        $u8str("ftp://1.2.3.4/some/path"),
        $u8str("http://myserver:123/path?query#fragment"),
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        URIstate state = {};
        call(URIutf8Drain, inputs[i], &state);
        a_pad(u8, out, 256);
        call(URIutf8Feed, out_idle, &state);
        u8cs result = {out[1], *out_idle};
        $testeq(result, inputs[i]);
    }
    done;
}

ok64 URItest5() {
    sane(1);
    a$str(fail, " -> FAIL\n");
    a$str(schemelab, " -> scheme=");
    a$str(pathlab, " path=");
    a$str(reconlab, "  reconstructed: ");
    u8cs urns[] = {
        $u8str("urn:isbn:0451450523"),
        $u8str("urn:ietf:rfc:2648"),
        $u8str("urn:uuid:6e8bc430-9c3a-11d9-9669-0800200c9a66"),
        $u8str("mailto:user@example.com"),
        $u8str("tel:+1-816-555-1212"),
        $u8str("file:///etc/passwd"),
        $u8str("data:text/plain;base64,SGVsbG8="),
    };
    for (size_t i = 0; i < sizeof(urns) / sizeof(urns[0]); ++i) {
        URIstate state = {};
        ok64 o = URIutf8Drain(urns[i], &state);
        $print(urns[i]);
        if (o != OK) {
            $print(fail);
            continue;
        }
        $print(schemelab);
        $print(state.scheme);
        $print(pathlab);
        $println(state.path);
        a_pad(u8, out, 256);
        call(URIutf8Feed, out_idle, &state);
        u8cs result = {out[1], *out_idle};
        $print(reconlab);
        $println(result);
    }
    done;
}

// Test segment collection
ok64 URITestSegments() {
    sane(1);

    // Parse URI with segments buffer
    a$str(uri_str, "http://example.com/a/b/c");
    uri u = {};
    a_pad(u8cs, segs, 16);
    u.segments = segs;
    call(URIutf8Drain, uri_str, &u);

    // Should have 3 segments: "a", "b", "c"
    u8cscs seg_data;
    u8cscsDup(seg_data, u8csbDataC(segs));
    test(u8cscsLen(seg_data) == 3, URIFAIL);

    a$str(expect_a, "a");
    a$str(expect_b, "b");
    a$str(expect_c, "c");
    $testeq(*u8cscsAtP(seg_data, 0), expect_a);
    $testeq(*u8cscsAtP(seg_data, 1), expect_b);
    $testeq(*u8cscsAtP(seg_data, 2), expect_c);

    // Test without segments buffer (should still work)
    uri u2 = {};
    call(URIutf8Drain, uri_str, &u2);
    $testeq(u.path, u2.path);

    done;
}

// Table-driven test for URIRelative and URIAbsolute
// Each triplet: {base, specific, expected_relative}
// Verifies: URIRelative(base, specific) == relative
//           URIAbsolute(base, relative) == specific (round-trip)
ok64 URITestTable() {
    sane(1);

    // Test triplets: base, specific, expected relative
    // NULL relative means "same as specific" (for scheme-differs case)
    static char const* triplets[][3] = {
        // Identical URIs -> empty relative
        {"http://example.com/path", "http://example.com/path", ""},

        // Different scheme -> full URI (scheme included)
        {"http://example.com/path", "https://example.com/path", "https://example.com/path"},
        {"ftp://server/file", "http://server/file", "http://server/file"},

        // Same scheme, different host -> authority + path
        {"http://example.com/path", "http://other.com/newpath", "//other.com/newpath"},
        {"http://a.com/x", "http://b.com/y", "//b.com/y"},

        // Same scheme+host, different port -> authority + path
        {"http://example.com/path", "http://example.com:8080/path", "//example.com:8080/path"},

        // Same scheme+host, different user -> authority + path
        {"http://example.com/path", "http://user@example.com/path", "//user@example.com/path"},

        // Same authority, different path - sibling files (same directory)
        {"http://example.com/path1", "http://example.com/path2", "path2"},
        {"http://example.com/dir/file1", "http://example.com/dir/file2", "file2"},

        // Same authority, different path - go up directories
        {"http://example.com/a/b", "http://example.com/c/d", "../c/d"},
        {"http://example.com/a/b/c", "http://example.com/x/y", "../../x/y"},
        {"http://example.com/a/b/c", "http://example.com/a/x", "../x"},

        // Same authority, go down directories
        {"http://example.com/a", "http://example.com/a/b/c", "a/b/c"},
        {"http://example.com/dir/file", "http://example.com/dir/sub/file", "sub/file"},

        // Same path, different query -> query only
        {"http://example.com/path?q1", "http://example.com/path?q2", "?q2"},
        {"http://example.com/path", "http://example.com/path?newq", "?newq"},

        // Same path, base has query, specific has none -> empty query marker
        {"http://example.com/path?oldq", "http://example.com/path", ""},

        // Same query, different fragment -> fragment only
        {"http://example.com/path#f1", "http://example.com/path#f2", "#f2"},
        {"http://example.com/path", "http://example.com/path#frag", "#frag"},

        // Same fragment in base, none in specific
        {"http://example.com/path#old", "http://example.com/path", ""},

        // Complex cases with query and fragment
        {"http://example.com/path?q#f", "http://example.com/path?q#f2", "#f2"},
        {"http://example.com/path?q1#f", "http://example.com/path?q2#f", "?q2#f"},
        {"http://example.com/a", "http://example.com/b?q#f", "b?q#f"},
        {"http://a.com/x", "http://b.com/y?q#f", "//b.com/y?q#f"},

        // User, port combinations
        {"http://user@example.com:80/path", "http://user@example.com:80/other", "other"},
        {"http://example.com/path", "http://user@other.com:8080/new?q#f",
         "//user@other.com:8080/new?q#f"},

        // Deep relative paths
        {"http://example.com/a/b/c/d", "http://example.com/a/b/x/y", "../x/y"},
        {"http://example.com/a/b/c/d", "http://example.com/a/e/f/g", "../../e/f/g"},
        {"http://example.com/1/2/3/4/5", "http://example.com/1/2/x", "../../x"},
    };

    size_t n = sizeof(triplets) / sizeof(triplets[0]);

    for (size_t i = 0; i < n; i++) {
        char const* base_str = triplets[i][0];
        char const* specific_str = triplets[i][1];
        char const* expected_rel_str = triplets[i][2];

        // Parse base and specific
        u8cs base_slice = {(u8cp)base_str, (u8cp)(base_str + strlen(base_str))};
        u8cs specific_slice = {(u8cp)specific_str, (u8cp)(specific_str + strlen(specific_str))};

        uri base = {}, specific = {}, rel = {};
        call(URIutf8Drain, base_slice, &base);
        call(URIutf8Drain, specific_slice, &specific);

        // Compute relative
        call(URIRelative, &rel, &base, &specific);

        // Serialize relative
        a_pad(u8, relbuf, 512);
        call(URIutf8Feed, relbuf_idle, &rel);
        u8cs rel_result;
        u8csDup(rel_result, u8bDataC(relbuf));

        // Compare with expected
        u8cs expected_rel = {(u8cp)expected_rel_str, (u8cp)(expected_rel_str + strlen(expected_rel_str))};
        if (!$eq(rel_result, expected_rel)) {
            fprintf(stderr, "FAIL test %zu: URIRelative\n", i);
            fprintf(stderr, "  base:     %s\n", base_str);
            fprintf(stderr, "  specific: %s\n", specific_str);
            fprintf(stderr, "  expected: '%s'\n", expected_rel_str);
            fprintf(stderr, "  got:      '%.*s'\n", (int)$len(rel_result), *rel_result);
            return URIFAIL;
        }

        // Round-trip: resolve relative back to absolute
        uri resolved = {};
        call(URIAbsolute, &resolved, &base, &rel);

        // Compare all components
        if (!$eq(resolved.scheme, specific.scheme) ||
            !$eq(resolved.host, specific.host) ||
            !$eq(resolved.port, specific.port) ||
            !$eq(resolved.user, specific.user) ||
            !$eq(resolved.path, specific.path) ||
            !$eq(resolved.query, specific.query) ||
            !$eq(resolved.fragment, specific.fragment)) {
            fprintf(stderr, "FAIL test %zu: round-trip\n", i);
            fprintf(stderr, "  base:     %s\n", base_str);
            fprintf(stderr, "  specific: %s\n", specific_str);
            fprintf(stderr, "  relative: '%s'\n", expected_rel_str);
            a_pad(u8, resbuf, 512);
            call(URIutf8Feed, resbuf_idle, &resolved);
            fprintf(stderr, "  resolved: '%.*s'\n", (int)$len(u8bDataC(resbuf)), *u8bDataC(resbuf));
            return URIFAIL;
        }
    }

    done;
}

ok64 URITestEscape() {
    sane(1);

    // Test escape
    static struct { char const* raw; char const* escaped; } esc_cases[] = {
        {"hello", "hello"},
        {"hello world", "hello%20world"},
        {"a/b/c", "a%2Fb%2Fc"},
        {"foo@bar.com", "foo%40bar.com"},
        {"100%", "100%25"},
        {"a-b_c.d~e", "a-b_c.d~e"},  // unreserved pass through
        {"файл", "%D1%84%D0%B0%D0%B9%D0%BB"},  // UTF-8
        {"", ""},
    };

    for (size_t i = 0; i < sizeof(esc_cases) / sizeof(esc_cases[0]); i++) {
        u8cs raw = {(u8cp)esc_cases[i].raw, (u8cp)(esc_cases[i].raw + strlen(esc_cases[i].raw))};
        u8cs expected = {(u8cp)esc_cases[i].escaped, (u8cp)(esc_cases[i].escaped + strlen(esc_cases[i].escaped))};

        a_pad(u8, buf, 256);
        call(URIu8sEsc, buf_idle, raw);
        u8cs result = {buf[1], *buf_idle};

        if (!$eq(result, expected)) {
            fprintf(stderr, "FAIL escape[%zu]: '%s' expected '%s' got '%.*s'\n",
                    i, esc_cases[i].raw, esc_cases[i].escaped, (int)$len(result), *result);
            return URIFAIL;
        }
    }

    // Test unescape
    static struct { char const* escaped; char const* raw; } unesc_cases[] = {
        {"hello", "hello"},
        {"hello%20world", "hello world"},
        {"a%2Fb%2Fc", "a/b/c"},
        {"foo%40bar.com", "foo@bar.com"},
        {"100%25", "100%"},
        {"%D1%84%D0%B0%D0%B9%D0%BB", "файл"},
        {"bad%zz", "bad%zz"},  // invalid hex passes through
        {"trailing%", "trailing%"},  // incomplete passes through
        {"", ""},
    };

    for (size_t i = 0; i < sizeof(unesc_cases) / sizeof(unesc_cases[0]); i++) {
        u8cs escaped = {(u8cp)unesc_cases[i].escaped, (u8cp)(unesc_cases[i].escaped + strlen(unesc_cases[i].escaped))};
        u8cs expected = {(u8cp)unesc_cases[i].raw, (u8cp)(unesc_cases[i].raw + strlen(unesc_cases[i].raw))};

        a_pad(u8, buf, 256);
        call(URIu8sUnesc, buf_idle, escaped);
        u8cs result = {buf[1], *buf_idle};

        if (!$eq(result, expected)) {
            fprintf(stderr, "FAIL unescape[%zu]: '%s' expected '%s' got '%.*s'\n",
                    i, unesc_cases[i].escaped, unesc_cases[i].raw, (int)$len(result), *result);
            return URIFAIL;
        }
    }

    // Round-trip test
    a$str(original, "hello world/path?query=value&foo=bar");
    a_pad(u8, escbuf, 256);
    call(URIu8sEsc, escbuf_idle, original);
    u8cs escaped = {escbuf[1], *escbuf_idle};

    a_pad(u8, unescbuf, 256);
    call(URIu8sUnesc, unescbuf_idle, escaped);
    u8cs roundtrip = {unescbuf[1], *unescbuf_idle};

    if (!$eq(roundtrip, original)) {
        fprintf(stderr, "FAIL escape round-trip\n");
        return URIFAIL;
    }

    done;
}

// Test zeroing the path component of a parsed URI
ok64 URITestPathZero() {
    sane(1);

    a$str(uri_str, "xx://branch.name/some/path?v=123");
    uri u = {};
    call(URIutf8Drain, uri_str, &u);

    // Verify initial parse
    a$str(expect_host, "branch.name");
    a$str(expect_path, "/some/path");
    a$str(expect_query, "v=123");
    $testeq(u.host, expect_host);
    $testeq(u.path, expect_path);
    $testeq(u.query, expect_query);

    // Zero the path
    u.path[0] = u.path[1] = NULL;

    // Serialize back
    a_pad(u8, out, 256);
    call(URIutf8Feed, out_idle, &u);
    u8cs result = {out[1], *out_idle};

    // Verify serialization: scheme + authority + query, no path
    a$str(expect_result, "xx://branch.name?v=123");
    $testeq(result, expect_result);

    // Re-parse with a "/" path prefix so the parser is happy
    // (RFC 3986: path-abempty after authority can be empty,
    //  but our ragel grammar requires "/" to start a path)
    a$str(uri_fixed, "xx://branch.name/?v=123");
    uri u2 = {};
    call(URIutf8Drain, uri_fixed, &u2);

    // Host and query intact, path is just "/"
    $testeq(u2.host, expect_host);
    $testeq(u2.query, expect_query);
    a$str(expect_slash, "/");
    $testeq(u2.path, expect_slash);

    done;
}

ok64 URItest() {
    sane(1);
    call(URItest1);
    call(URItest2);
    call(URItest3);
    call(URItest4);
    call(URItest5);
    call(URITestSegments);
    call(URITestTable);
    call(URITestEscape);
    call(URITestPathZero);
    done;
}

TEST(URItest);
