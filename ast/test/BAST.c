#include "BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/RON.h"
#include "abc/TEST.h"
#include "json/BASON.h"

#define BAST_SETUP(padsize, stksize, idxsize)     \
    a_pad(u8, pad, padsize);                      \
    u64 _stk[stksize];                            \
    u64b stk = {_stk, _stk, _stk, _stk + stksize}; \
    u64 _idx[idxsize];                            \
    u64b idx = {_idx, _idx, _idx, _idx + idxsize};

// Table-driven roundtrip test: parse text, export JSON, concat leaves == original
ok64 BASTtestTextRoundtrip() {
    sane(1);

    static const char *cases[] = {
        "Hello world!",
        "Hello world!\n",
        "line1\nline2\n",
        "  indented text\n",
        "no_spaces_here",
        "foo123 bar456",
        "a + b = c;",
        "multiple   spaces   here",
        "\n\n\n",
        "tabs\there\ttoo",
        "mixed: word 42 + punct!\n",
        "trailing blank   ",
        "",
        "CamelCase under_score 0x1f",
    };
    size_t ncases = sizeof(cases) / sizeof(cases[0]);

    for (size_t t = 0; t < ncases; t++) {
        BAST_SETUP(65536, 64, 256);
        u8csc src = {(u8cp)cases[t], (u8cp)cases[t] + strlen(cases[t])};
        u8cs ext = $u8str(".txt");

        call(BASTParse, pad, idx, src, ext);

        // Read back: open, iterate all string children, concatenate
        u8cs dat = {pad[1], pad[2]};
        call(BASONOpen, stk, dat);

        u8 type;
        u8cs key, val;
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'A');

        call(BASONInto, stk, dat, val);

        a_pad(u8, cat, 65536);
        while (BASONDrain(stk, dat, &type, key, val) == OK) {
            testeq(type, 'S');
            call(u8bFeed, cat, val);
        }

        u8cs result = {cat[1], cat[2]};
        testeq((size_t)$len(result), (size_t)$len(src));
        if ($len(src) > 0)
            testeq(0, memcmp(result[0], src[0], $len(src)));
    }
    done;
}

// Verify token structure for a known input
ok64 BASTtestTextTokens() {
    sane(1);
    BAST_SETUP(65536, 64, 256);

    static const char input[] = "Hello world! foo42\nbar";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};
    u8cs ext = $u8str(".txt");
    call(BASTParse, pad, idx, src, ext);

    // Expected tokens
    static const char *expected[] = {
        "Hello", " world", "!", " foo42", "\n", "bar",
    };
    size_t nexp = sizeof(expected) / sizeof(expected[0]);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type;
    u8cs key, val;
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, dat, val);

    for (size_t i = 0; i < nexp; i++) {
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'S');
        size_t elen = strlen(expected[i]);
        testeq((size_t)$len(val), elen);
        testeq(0, memcmp(val[0], expected[i], elen));
    }

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// Verify 'F' name tags are emitted for function/struct names in C code
ok64 BASTtestNameTag() {
    sane(1);
    BAST_SETUP(65536, 64, 1024);

    static const char input[] =
        "int foo(int x) { return x; }\n"
        "struct MyStruct { int a; };\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};
    u8cs ext = $u8str(".c");
    call(BASTParse, pad, idx, src, ext);

    // Walk entire BASON tree, collect 'F' (name) leaf values
    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    int f_count = 0;
    b8 found_foo = NO;
    b8 found_MyStruct = NO;
    int depth = 0;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, dat, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            BASONOuto(stk);
            depth--;
            continue;
        }
        if (type == BAST_TAG_NAME) {
            f_count++;
            if ($len(val) == 3 && memcmp(val[0], "foo", 3) == 0)
                found_foo = YES;
            if ($len(val) == 8 && memcmp(val[0], "MyStruct", 8) == 0)
                found_MyStruct = YES;
        } else if (BASONPlex(type)) {
            BASONInto(stk, dat, val);
            depth++;
        }
    }

    test(f_count >= 2, FAILsanity);
    test(found_foo == YES, FAILsanity);
    test(found_MyStruct == YES, FAILsanity);

    // Verify roundtrip: concatenating all leaf nodes reproduces source
    u8bReset(pad);
    u64bReset(stk);
    u64bReset(idx);
    call(BASTParse, pad, idx, src, ext);

    u8cs dat2 = {pad[1], pad[2]};
    call(BASONOpen, stk, dat2);

    a_pad(u8, cat, 65536);
    int depth2 = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, dat2, &type, key, val);
        if (o != OK) {
            if (depth2 <= 0) break;
            BASONOuto(stk);
            depth2--;
            continue;
        }
        if (!BASONPlex(type)) {
            call(u8bFeed, cat, val);
        } else {
            BASONInto(stk, dat2, val);
            depth2++;
        }
    }

    u8cs result = {cat[1], cat[2]};
    testeq((size_t)$len(result), (size_t)$len(src));
    testeq(0, memcmp(result[0], src[0], $len(src)));

    done;
}

// --- Markdown tests ---

// Roundtrip: concatenating all leaves reproduces original .md text
ok64 BASTtestMDRoundtrip() {
    sane(1);

    static const char *cases[] = {
        "# Hello\n",
        "plain text\n",
        "# Heading\n\nA paragraph.\n",
        "**bold** and *italic*\n",
        "Some `inline code` here\n",
        "- item 1\n- item 2\n",
        "> a quote\n",
        "```\ncode block\n```\n",
        "[link text](http://example.com)\n",
        "~~struck~~\n",
        "",
    };
    size_t ncases = sizeof(cases) / sizeof(cases[0]);

    for (size_t t = 0; t < ncases; t++) {
        BAST_SETUP(65536, 64, 1024);
        u8csc src = {(u8cp)cases[t], (u8cp)cases[t] + strlen(cases[t])};
        u8cs ext = $u8str(".md");

        call(BASTParse, pad, idx, src, ext);

        u8cs dat = {pad[1], pad[2]};
        call(BASONOpen, stk, dat);

        a_pad(u8, cat, 65536);
        int depth = 0;
        for (;;) {
            u8 type = 0;
            u8cs key = {};
            u8cs val = {};
            ok64 o = BASONDrain(stk, dat, &type, key, val);
            if (o != OK) {
                if (depth <= 0) break;
                BASONOuto(stk);
                depth--;
                continue;
            }
            if (!BASONPlex(type)) {
                call(u8bFeed, cat, val);
            } else {
                BASONInto(stk, dat, val);
                depth++;
            }
        }

        u8cs result = {cat[1], cat[2]};
        testeq((size_t)$len(result), (size_t)$len(src));
        if ($len(src) > 0)
            testeq(0, memcmp(result[0], src[0], $len(src)));
    }
    done;
}

// Structure: verify heading→E, emphasis→W, bold→V, code span→G, link text→T
ok64 BASTtestMDStructure() {
    sane(1);
    BAST_SETUP(65536, 64, 1024);

    static const char input[] =
        "# Title\n\n**bold** *italic* `code` [link](url) ~~struck~~\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};
    u8cs ext = $u8str(".md");
    call(BASTParse, pad, idx, src, ext);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    b8 found_Y = NO;   // heading (bold)
    b8 found_W = NO;   // italic text
    b8 found_V = NO;   // bold text
    b8 found_G = NO;   // code span text
    b8 found_T = NO;   // link text
    b8 found_J = NO;   // strikethrough text
    b8 found_P = NO;   // punctuation/delimiter
    int depth = 0;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, dat, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            BASONOuto(stk);
            depth--;
            continue;
        }
        if (type == 'Y') found_Y = YES;
        if (type == 'W') {
            if ($len(val) == 6 && memcmp(val[0], "italic", 6) == 0)
                found_W = YES;
        }
        if (type == 'V') {
            if ($len(val) == 4 && memcmp(val[0], "bold", 4) == 0)
                found_V = YES;
        }
        if (type == 'G') {
            if ($len(val) == 4 && memcmp(val[0], "code", 4) == 0)
                found_G = YES;
        }
        if (type == 'T') {
            if ($len(val) == 4 && memcmp(val[0], "link", 4) == 0)
                found_T = YES;
        }
        if (type == 'J') {
            if ($len(val) == 6 && memcmp(val[0], "struck", 6) == 0)
                found_J = YES;
        }
        if (type == 'P') found_P = YES;

        if (BASONPlex(type)) {
            BASONInto(stk, dat, val);
            depth++;
        }
    }

    test(found_Y == YES, FAILsanity);  // heading (bold)
    test(found_W == YES, FAILsanity);  // italic
    test(found_V == YES, FAILsanity);  // bold
    test(found_G == YES, FAILsanity);  // code
    test(found_T == YES, FAILsanity);  // link
    test(found_J == YES, FAILsanity);  // strikethrough
    test(found_P == YES, FAILsanity);  // delimiters

    done;
}

ok64 BASTtest() {
    sane(1);
    call(BASTtestTextRoundtrip);
    call(BASTtestTextTokens);
    call(BASTtestNameTag);
    call(BASTtestMDRoundtrip);
    call(BASTtestMDStructure);
    done;
}

TEST(BASTtest);
