//
// RAP tests - path-dependent rapid hashing
//
// Three modes:
//   Mode 0: FIRST-only (only FIRST elements, no PLEX)
//   Mode 1: with PLEX (PLEX hash before contents)
//   Mode 2: with PLEX brackets (PLEX hash before AND after contents)
//
#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

#define RAP_MAX_HASHES 32

typedef struct {
    char const* input;
    u8 mode;
    u64 count;
    u64 hashes[RAP_MAX_HASHES];
    char const* desc;
} RAPcase;

con RAPcase RAP_CASES[] = {
    // ==========================================================
    // PRIMITIVES - all modes produce same single hash (00 marker)
    // Hash layout: [marker:2][depth:6][hash:56]
    // ==========================================================
    {"42", 0, 1, {0x1eb0f9513e86be}, "int/first-only"},
    {"42", 1, 1, {0x1eb0f9513e86be}, "int/with-plex"},
    {"42", 2, 1, {0x1eb0f9513e86be}, "int/plex-brackets"},

    {"\"hello\"", 0, 1, {0x50226c00743a3a}, "string/first-only"},
    {"\"hello\"", 1, 1, {0x50226c00743a3a}, "string/with-plex"},
    {"\"hello\"", 2, 1, {0x50226c00743a3a}, "string/plex-brackets"},

    {"foo", 0, 1, {0xb3b7796508c4e9}, "term/first-only"},
    {"foo", 1, 1, {0xb3b7796508c4e9}, "term/with-plex"},
    {"foo", 2, 1, {0xb3b7796508c4e9}, "term/plex-brackets"},

    {"0-0", 0, 1, {0x7704ed211fecc9}, "ref/first-only"},
    {"0-0", 1, 1, {0x7704ed211fecc9}, "ref/with-plex"},
    {"0-0", 2, 1, {0x7704ed211fecc9}, "ref/plex-brackets"},

    // ==========================================================
    // EMPTY TOP-LEVEL CONTAINERS
    // Mode 0: no FIRST elements = no output
    // Mode 1: PLEX open (01 marker + depth 0)
    // Mode 2: PLEX open (01) + close (10)
    // ==========================================================
    {"[]", 0, 0, {0}, "empty-linear/first-only"},
    {"[]", 1, 1, {0x4059e9b808a2fa48}, "empty-linear/with-plex"},
    {"[]", 2, 2, {0x4059e9b808a2fa48, 0x8059e9b808a2fa48}, "empty-linear/plex-brackets"},

    {"{}", 0, 0, {0}, "empty-euler/first-only"},
    {"{}", 1, 1, {0x40d6b0b302c70fc9}, "empty-euler/with-plex"},
    {"{}", 2, 2, {0x40d6b0b302c70fc9, 0x80d6b0b302c70fc9}, "empty-euler/plex-brackets"},

    {"()", 0, 0, {0}, "empty-tuple/first-only"},
    {"()", 1, 1, {0x4075c27632897f17}, "empty-tuple/with-plex"},
    {"()", 2, 2, {0x4075c27632897f17, 0x8075c27632897f17}, "empty-tuple/plex-brackets"},

    // ==========================================================
    // LINEAR WITH ELEMENTS [1], [1,2,3]
    // ==========================================================
    {"[1]", 0, 1, {0x1b2d5f30eefd873}, "linear-one/first-only"},
    {"[1]", 1, 2, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873}, "linear-one/with-plex"},
    {"[1]", 2, 3, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873, 0x8059e9b808a2fa48}, "linear-one/plex-brackets"},

    {"[1,2,3]", 0, 3, {0x1b2d5f30eefd873, 0x1a246311d6e1615, 0x10723a02b2aa4f0}, "linear-three/first-only"},
    {"[1,2,3]", 1, 4, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873, 0x1a246311d6e1615, 0x10723a02b2aa4f0}, "linear-three/with-plex"},
    {"[1,2,3]", 2, 5, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873, 0x1a246311d6e1615, 0x10723a02b2aa4f0, 0x8059e9b808a2fa48}, "linear-three/plex-brackets"},

    // ==========================================================
    // NESTED EMPTY [[]]
    // Mode 0: empty inner outputs open marker (01 + depth 1)
    // ==========================================================
    {"[[]]", 0, 1, {0x41f597be2da22dc2}, "nested-empty/first-only"},
    {"[[]]", 1, 2, {0x4059e9b808a2fa48, 0x41f597be2da22dc2}, "nested-empty/with-plex"},
    {"[[]]", 2, 4, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x81f597be2da22dc2, 0x8059e9b808a2fa48}, "nested-empty/plex-brackets"},

    // ==========================================================
    // NESTED WITH VALUE [[1]]
    // ==========================================================
    {"[[1]]", 0, 1, {0x23230957dd30268}, "nested-one/first-only"},
    {"[[1]]", 1, 3, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x23230957dd30268}, "nested-one/with-plex"},
    {"[[1]]", 2, 5, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x23230957dd30268, 0x81f597be2da22dc2, 0x8059e9b808a2fa48}, "nested-one/plex-brackets"},

    // ==========================================================
    // TRIPLE NESTED [[[]]]
    // ==========================================================
    {"[[[]]]", 0, 1, {0x422bc661f8e5b001}, "triple-nested/first-only"},
    {"[[[]]]", 1, 3, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x422bc661f8e5b001}, "triple-nested/with-plex"},
    {"[[[]]]", 2, 6, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x422bc661f8e5b001, 0x822bc661f8e5b001, 0x81f597be2da22dc2, 0x8059e9b808a2fa48}, "triple-nested/plex-brackets"},

    // ==========================================================
    // TUPLE (a,1), a:1
    // ==========================================================
    {"(a,1)", 0, 2, {0x12690538b7def54, 0x1de135083a85a86}, "tuple-kv/first-only"},
    {"(a,1)", 1, 3, {0x4043f02d0d91c765, 0x12690538b7def54, 0x1de135083a85a86}, "tuple-kv/with-plex"},
    {"(a,1)", 2, 4, {0x4043f02d0d91c765, 0x12690538b7def54, 0x1de135083a85a86, 0x8043f02d0d91c765}, "tuple-kv/plex-brackets"},

    {"a:1", 0, 2, {0x12690538b7def54, 0x1de135083a85a86}, "inline-tuple/first-only"},
    {"a:1", 1, 3, {0x4043f02d0d91c765, 0x12690538b7def54, 0x1de135083a85a86}, "inline-tuple/with-plex"},
    {"a:1", 2, 4, {0x4043f02d0d91c765, 0x12690538b7def54, 0x1de135083a85a86, 0x8043f02d0d91c765}, "inline-tuple/plex-brackets"},

    // ==========================================================
    // TUPLE IN LINEAR [a:1] - tuple is expanded (depth 1 for tuple)
    // ==========================================================
    {"[a:1]", 0, 2, {0x21b639f7c16b4dc, 0x231b3afd1e38830}, "tuple-in-linear/first-only"},
    {"[a:1]", 1, 4, {0x4059e9b808a2fa48, 0x41eddb756db202e5, 0x21b639f7c16b4dc, 0x231b3afd1e38830}, "tuple-in-linear/with-plex"},
    {"[a:1]", 2, 6, {0x4059e9b808a2fa48, 0x41eddb756db202e5, 0x21b639f7c16b4dc, 0x231b3afd1e38830, 0x81eddb756db202e5, 0x8059e9b808a2fa48}, "tuple-in-linear/plex-brackets"},

    {"[a:1,b:2]", 0, 4, {0x21b639f7c16b4dc, 0x231b3afd1e38830, 0x2143831eb5fc8c0, 0x287eba0e7a179e9}, "two-tuples/first-only"},
    {"[a:1,b:2]", 1, 7, {0x4059e9b808a2fa48, 0x41eddb756db202e5, 0x21b639f7c16b4dc, 0x231b3afd1e38830, 0x41c7fc32eb66798e, 0x2143831eb5fc8c0, 0x287eba0e7a179e9}, "two-tuples/with-plex"},
    {"[a:1,b:2]", 2, 10, {0x4059e9b808a2fa48, 0x41eddb756db202e5, 0x21b639f7c16b4dc, 0x231b3afd1e38830, 0x81eddb756db202e5, 0x41c7fc32eb66798e, 0x2143831eb5fc8c0, 0x287eba0e7a179e9, 0x81c7fc32eb66798e, 0x8059e9b808a2fa48}, "two-tuples/plex-brackets"},

    // ==========================================================
    // EULER SET {1,2}
    // ==========================================================
    {"{1,2}", 0, 2, {0x16584de2e80895e, 0x13726cfffa42ea9}, "euler-two/first-only"},
    {"{1,2}", 1, 3, {0x40d6b0b302c70fc9, 0x16584de2e80895e, 0x13726cfffa42ea9}, "euler-two/with-plex"},
    {"{1,2}", 2, 4, {0x40d6b0b302c70fc9, 0x16584de2e80895e, 0x13726cfffa42ea9, 0x80d6b0b302c70fc9}, "euler-two/plex-brackets"},

    // ==========================================================
    // MIXED NESTED [1,[2],3]
    // ==========================================================
    {"[1,[2],3]", 0, 3, {0x1b2d5f30eefd873, 0x2e6eb9dc37c290a, 0x10723a02b2aa4f0}, "mixed-nested/first-only"},
    {"[1,[2],3]", 1, 5, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873, 0x41f597be2da22dc2, 0x2e6eb9dc37c290a, 0x10723a02b2aa4f0}, "mixed-nested/with-plex"},
    {"[1,[2],3]", 2, 7, {0x4059e9b808a2fa48, 0x1b2d5f30eefd873, 0x41f597be2da22dc2, 0x2e6eb9dc37c290a, 0x81f597be2da22dc2, 0x10723a02b2aa4f0, 0x8059e9b808a2fa48}, "mixed-nested/plex-brackets"},

    // ==========================================================
    // DEEP NESTING [[[[1]]]] - depths 0,1,2,3,4
    // ==========================================================
    {"[[[[1]]]]", 0, 1, {0x4ec4ef71e1eed41}, "deep-nested/first-only"},
    {"[[[[1]]]]", 1, 5, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x422bc661f8e5b001, 0x43b6f1481018d01b, 0x4ec4ef71e1eed41}, "deep-nested/with-plex"},
    {"[[[[1]]]]", 2, 9, {0x4059e9b808a2fa48, 0x41f597be2da22dc2, 0x422bc661f8e5b001, 0x43b6f1481018d01b, 0x4ec4ef71e1eed41, 0x83b6f1481018d01b, 0x822bc661f8e5b001, 0x81f597be2da22dc2, 0x8059e9b808a2fa48}, "deep-nested/plex-brackets"},
};

con u64 RAP_CASES_LEN = sizeof(RAP_CASES) / sizeof(RAP_CASES[0]);

ok64 RAPtestCase(RAPcase const* tc) {
    sane(tc);
    a_pad(rdx, inputs, 64);
    rdxp inp = 0;
    call(rdxbFedP, inputs, &inp);
    inp->format = RDX_FMT_JDR;
    u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};
    inp->next = (u8p)input[0]; inp->opt = (u8p)input[1];
    a_pad(u64, hashes, 256);
    call(rdxRapidHashesF, hashes_idle, inp, tc->mode);
    u64sJoin(hashes_idle, hashes);
    u64 n = $len(hashes_datac);
    if (n != tc->count) {
        fprintf(stderr, "    count: got %lu, expected %lu\n", n, tc->count);
        fprintf(stderr, "    actual: {");
        for (u64 i = 0; i < n; i++) {
            fprintf(stderr, "0x%lx", u64csAt(hashes_datac, i));
            if (i < n - 1) fprintf(stderr, ", ");
        }
        fprintf(stderr, "}\n");
        fail(FAIL);
    }
    for (u64 i = 0; i < n && i < RAP_MAX_HASHES; i++) {
        u64 got = u64csAt(hashes_datac, i);
        u64 exp = tc->hashes[i];
        if (got != exp) {
            fprintf(stderr, "    hash[%lu]: got 0x%lx, expected 0x%lx\n", i, got, exp);
            fail(FAIL);
        }
    }
    done;
}

ok64 RAPtest() {
    sane(1);
    b8 generate = NO;  // Set to YES to print expected values
    fprintf(stderr, "RAP test (%lu cases)\n", RAP_CASES_LEN);
    for (u64 i = 0; i < RAP_CASES_LEN; i++) {
        RAPcase const* tc = &RAP_CASES[i];
        if (generate) {
            // Generate mode: print actual hashes for updating test table
            a_pad(rdx, inputs, 64);
            rdxp inp = 0;
            rdxbFedP(inputs, &inp);
            inp->format = RDX_FMT_JDR;
            u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};
            inp->next = (u8p)input[0]; inp->opt = (u8p)input[1];
            a_pad(u64, hashes, 256);
            rdxRapidHashesF(hashes_idle, inp, tc->mode);
            u64sJoin(hashes_idle, hashes);
            u64 n = $len(hashes_datac);
            fprintf(stderr, "    {\"%s\", %d, %lu, {", tc->input, tc->mode, n);
            for (u64 j = 0; j < n; j++) {
                fprintf(stderr, "0x%lx", u64csAt(hashes_datac, j));
                if (j < n - 1) fprintf(stderr, ", ");
            }
            fprintf(stderr, "}, \"%s\"},\n", tc->desc);
        } else {
            fprintf(stderr, "  %s\n", tc->desc);
            ok64 o = RAPtestCase(tc);
            if (o != OK) {
                fprintf(stderr, "FAIL: %s (input: %s, mode: %d)\n",
                        tc->desc, tc->input, tc->mode);
                fail(o);
            }
        }
    }
    done;
}

TEST(RAPtest);
