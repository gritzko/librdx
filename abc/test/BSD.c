#include "BSD.h"

#include "PRO.h"
#include "TEST.h"

// Helper: roundtrip old->neu via diff+patch, verify result
ok64 BSDroundtrip(u8csc old, u8csc neu) {
    sane(old != NULL && neu != NULL);

    i64 oldlen = (i64)$len(old);
    i64 neulen = (i64)$len(neu);

    // Allocate workspace for suffix sort
    u64 wlen = BSDWorkLen(oldlen);
    i64 wbuf[wlen > 0 ? wlen : 1];
    i64s work = {wbuf, wbuf + wlen};

    // Patch buffer: generous upper bound
    u64 psize = 24 + (neulen + oldlen + 1) * 25;
    u8 pbuf[psize > 0 ? psize : 1];
    u8s patch = {pbuf, pbuf + psize};

    call(BSDDiff, patch, old, neu, work);

    // patch[0] now points past written data
    u64 patch_written = patch[0] - pbuf;
    u8csc cpatch = {pbuf, pbuf + patch_written};

    // Verify header
    u64 nsz = BSDPatchNewSize(cpatch);
    testeq(nsz, (u64)neulen);

    // Apply patch
    u8 result[neulen > 0 ? neulen : 1];
    u8s res = {result, result + neulen};

    call(BSDPatch, res, old, cpatch);

    // Verify result matches neu
    u8csc got = {result, result + neulen};
    $testeq(got, neu);

    done;
}

ok64 BSDtestIdentical() {
    sane(1);
    a$str(data, "hello world");
    call(BSDroundtrip, data, data);
    done;
}

ok64 BSDtestEmptyToData() {
    sane(1);
    u8csc empty = {NULL, NULL};
    a$str(data, "new content here");
    // empty old -> new data (all extra bytes)
    u8 zbuf = 0;
    u8csc zero_old = {&zbuf, &zbuf};  // zero-length
    call(BSDroundtrip, zero_old, data);
    done;
}

ok64 BSDtestDataToEmpty() {
    sane(1);
    a$str(data, "old content here");
    u8 zbuf = 0;
    u8csc zero_new = {&zbuf, &zbuf};
    call(BSDroundtrip, data, zero_new);
    done;
}

ok64 BSDtestSmallEdit() {
    sane(1);
    a$str(old, "the quick brown fox jumps over the lazy dog");
    a$str(neu, "the quick brown cat jumps over the lazy dog");
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestLargeInsertion() {
    sane(1);
    a$str(old, "AB");
    a$str(neu, "A_inserted_text_here_B");
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestCompleteReplacement() {
    sane(1);
    a$str(old, "aaaaaaaaaa");
    a$str(neu, "bbbbbbbbbb");
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestBinaryData() {
    sane(1);
    u8c oldb[] = {0, 1, 2, 3, 0, 0, 0, 4, 5, 6, 7, 8, 9, 10};
    u8c neub[] = {0, 1, 2, 3, 0xFF, 0xFE, 0, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    u8csc old = {oldb, oldb + sizeof(oldb)};
    u8csc neu = {neub, neub + sizeof(neub)};
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestSingleByte() {
    sane(1);
    u8c a = 'A';
    u8c b = 'B';
    u8csc old = {&a, &a + 1};
    u8csc neu = {&b, &b + 1};
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestRepetitive() {
    sane(1);
    // Pattern with lots of repeats to stress suffix sort
    u8 oldb[256];
    u8 neub[256];
    for (int i = 0; i < 256; i++) {
        oldb[i] = i & 0x0F;
        neub[i] = (i + 3) & 0x0F;
    }
    u8csc old = {oldb, oldb + 256};
    u8csc neu = {neub, neub + 256};
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestGrow() {
    sane(1);
    a$str(old, "short");
    a$str(neu, "a much longer string that has grown significantly from the original");
    call(BSDroundtrip, old, neu);
    done;
}

ok64 BSDtestShrink() {
    sane(1);
    a$str(old, "a very long string that should be shrunk down to something small");
    a$str(neu, "small");
    call(BSDroundtrip, old, neu);
    done;
}

typedef struct {
    char const *name;
    ok64 (*fn)();
} BSDTestCase;

ok64 BSDtest() {
    sane(1);

    BSDTestCase cases[] = {
        {"identical", BSDtestIdentical},
        {"empty_to_data", BSDtestEmptyToData},
        {"data_to_empty", BSDtestDataToEmpty},
        {"small_edit", BSDtestSmallEdit},
        {"large_insertion", BSDtestLargeInsertion},
        {"complete_replacement", BSDtestCompleteReplacement},
        {"binary_data", BSDtestBinaryData},
        {"single_byte", BSDtestSingleByte},
        {"repetitive", BSDtestRepetitive},
        {"grow", BSDtestGrow},
        {"shrink", BSDtestShrink},
    };

    i64 count = sizeof(cases) / sizeof(cases[0]);
    for (i64 i = 0; i < count; i++) {
        fprintf(stderr, "  %s...", cases[i].name);
        call(cases[i].fn);
        fprintf(stderr, " ok\n");
    }

    done;
}

TEST(BSDtest);
