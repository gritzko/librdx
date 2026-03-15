#include "sm/SM.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/RON.h"
#include "abc/TEST.h"
#include "json/BASON.h"

#define SM_SETUP(padsize, stksize, idxsize)          \
    a_pad(u8, pad, padsize);                         \
    u64 _stk[stksize];                               \
    u64b stk = {_stk, _stk, _stk, _stk + stksize}; \
    u64 _idx[idxsize];                                \
    u64b idx = {_idx, _idx, _idx, _idx + idxsize};

// Roundtrip: parse .mkd, concat all leaf nodes, compare with original
ok64 SMtestRoundtrip() {
    sane(1);

    static const char *cases[] = {
        "#   Hello world\n",
        "##  Second heading\n",
        "Some paragraph text.\n",
        "-   item one\n-   item two\n",
        "1.  first\n2.  second\n",
        ">   quoted text\n",
        "``` code\nfoo bar\n``` end\n",
        "#   Heading\n\nParagraph.\n",
        "    indented line\n",
        "-   list item\n    continued\n",
        "#   Title\n\n##  Sub\n\nBody text.\n\n-   a\n-   b\n",
    };
    size_t ncases = sizeof(cases) / sizeof(cases[0]);

    for (size_t t = 0; t < ncases; t++) {
        SM_SETUP(65536, 64, 256);
        u8csc src = {(u8cp)cases[t], (u8cp)cases[t] + strlen(cases[t])};

        call(SMParse, pad, idx, src);

        // Read back and concatenate all leaf nodes
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
        if ((size_t)$len(result) != (size_t)$len(src)) {
            fprintf(stderr, "ROUNDTRIP FAIL case %zu: got %zu expected %zu\n",
                    t, (size_t)$len(result), (size_t)$len(src));
            fail(faileq);
        }
        if ($len(src) > 0)
            testeq(0, memcmp(result[0], src[0], $len(src)));
    }
    done;
}

// Structure test: verify type tags for heading
ok64 SMtestHeading() {
    sane(1);
    SM_SETUP(65536, 64, 256);

    static const char input[] = "#   Hello world\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};

    call(SMParse, pad, idx, src);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    // Root should be 'A'
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'A');
    call(BASONInto, stk, dat, val);

    // First child should be 'E' (heading container)
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'E');
    call(BASONInto, stk, dat, val);

    // Inside heading: first token is div markup 'S', then 'F' name tokens
    b8 found_s = NO;
    b8 found_f = NO;
    while (BASONDrain(stk, dat, &type, key, val) == OK) {
        if (type == 'S') found_s = YES;
        if (type == 'F') found_f = YES;
    }
    test(found_s == YES, SMFAIL);
    test(found_f == YES, SMFAIL);

    done;
}

// Structure test: verify list types
ok64 SMtestList() {
    sane(1);
    SM_SETUP(65536, 64, 256);

    static const char input[] = "-   item one\n-   item two\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};

    call(SMParse, pad, idx, src);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'A');
    call(BASONInto, stk, dat, val);

    // First child should be 'U' (unordered list)
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'U');

    done;
}

// Structure test: blockquote
ok64 SMtestQuote() {
    sane(1);
    SM_SETUP(65536, 64, 256);

    static const char input[] = ">   quoted text\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};

    call(SMParse, pad, idx, src);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'A');
    call(BASONInto, stk, dat, val);

    // First child should be 'I' (blockquote)
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'I');

    done;
}

// Structure test: code block
ok64 SMtestCode() {
    sane(1);
    SM_SETUP(65536, 64, 256);

    static const char input[] = "``` code\nint x = 1;\n``` end\n";
    u8csc src = {(u8cp)input, (u8cp)input + strlen(input)};

    call(SMParse, pad, idx, src);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'A');
    call(BASONInto, stk, dat, val);

    // First child should be 'I' (code block)
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'I');

    done;
}

// Empty input
ok64 SMtestEmpty() {
    sane(1);
    SM_SETUP(65536, 64, 256);

    static const char input[] = "";
    u8csc src = {(u8cp)input, (u8cp)input};

    call(SMParse, pad, idx, src);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, (u8)'A');
    call(BASONInto, stk, dat, val);

    // Should be empty inside
    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);

    done;
}

ok64 SMtest() {
    sane(1);
    call(SMtestRoundtrip);
    call(SMtestHeading);
    call(SMtestList);
    call(SMtestQuote);
    call(SMtestCode);
    call(SMtestEmpty);
    done;
}

TEST(SMtest);
