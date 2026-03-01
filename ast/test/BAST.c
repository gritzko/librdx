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

ok64 BASTtest() {
    sane(1);
    call(BASTtestTextRoundtrip);
    call(BASTtestTextTokens);
    done;
}

TEST(BASTtest);
