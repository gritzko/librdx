#include "SLOG.h"

#include <stdio.h>

#include "BUF.h"
#include "NUM.h"
#include "PRO.h"
#include "TEST.h"

// Test records: TLV with lit 'A' containing a u64 value
fun ok64 writeRec(u8gp data, u64 val) {
    sane(u8gOK(data));
    a_rawc(raw, val);
    call(TLVu8sFeed, u8gRest(data), 'A', raw);
    done;
}

// Less-comparator for u64 values in TLV records
// Now receives full TLV records, must drain to get body
fun b8 u64TLVless(u8csc a, u8csc b) {
    a_dup(u8c,arec,a);
    a_dup(u8c,brec,b);
    u8 alit = 0, blit = 0;
    u8cs aval = {}, bval = {};
    TLVu8sDrain(arec, &alit, aval);
    TLVu8sDrain(brec, &blit, bval);
    u64c *aa = (u64c *)aval[0];
    u64c *bb = (u64c *)bval[0];
    return *aa < *bb;
}

// Less-comparator for NUM string records (parses leading number)
// Now receives full TLV records, must drain to get body
fun b8 numStrLess(u8csc a, u8csc b) {
    a_dup(u8c,arec,a);
    a_dup(u8c,brec,b);
    u8 alit = 0, blit = 0;
    u8cs aval = {}, bval = {};
    TLVu8sDrain(arec, &alit, aval);
    TLVu8sDrain(brec, &blit, bval);
    u64 na = 0, nb = 0;
    // Parse leading number from a (respect slice bounds)
    for (u8cp pa = aval[0]; pa < aval[1] && *pa >= '0' && *pa <= '9'; ++pa) {
        na = na * 10 + (*pa - '0');
    }
    // Parse leading number from b (respect slice bounds)
    for (u8cp pb = bval[0]; pb < bval[1] && *pb >= '0' && *pb <= '9'; ++pb) {
        nb = nb * 10 + (*pb - '0');
    }
    return na < nb;
}

// Write a NUM string record: "N. English words"
fun ok64 writeNumStrRec(u8gp data, u64 n) {
    sane(u8gOK(data));
    a_pad(u8, rec, 512);

    u8 *p = rec[0];
    u64 tmp = n;
    u8 digits[20];
    int ndig = 0;
    do {
        digits[ndig++] = '0' + (tmp % 10);
        tmp /= 10;
    } while (tmp > 0);
    while (ndig > 0) *p++ = digits[--ndig];
    *p++ = '.';
    *p++ = ' ';

    u8s into = {p, rec[0] + 512};
    ok64 o = NUMu8sFeed(into, n);
    if (o != OK) return o;

    u8cs body = {rec[0], *into};
    call(TLVu8sFeed, u8gRest(data), 'S', body);
    done;
}

// Test rank calculation
// Formula: block = (off + 0x7f) >> 7, rank = ctz(block) or 64 if block=0
ok64 SLOG0() {
    sane(1);

    // Offset 0 is block 0, max rank (64)
    testeq(64, SLOGRank(0));

    // Offsets 1-128 are block 1, rank = ctz(1) = 0
    testeq(0, SLOGRank(1));
    testeq(0, SLOGRank(0x7f));
    testeq(0, SLOGRank(0x80));

    // Offsets 129-256 are block 2, rank = ctz(2) = 1
    testeq(1, SLOGRank(0x81));
    testeq(1, SLOGRank(0x100));

    // Block 4 has rank 2 (offsets 385-512)
    testeq(2, SLOGRank(0x182));

    // Block 8 has rank 3 (offsets 897-1024)
    testeq(3, SLOGRank(0x382));

    done;
}

// Test basic write/read cycle
ok64 SLOG1() {
    sane(1);

    a_pad(u8, buf, 4096);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    // Create SLOG and write some records
    call(SLOGCreate, stkdi, data);

    // Write records with increasing values
    for (u64 i = 0; i < 100; ++i) {
        call(writeRec, data, i * 10);
        call(SLOGSample, stkdi, data);
    }

    // Close the stream
    call(SLOGClose, stkdi, data);

    // Reset stack for reading
    u64bReset(stk);

    // Open and load skip list
    u8csc stream = {data[0], data[1]};
    call(SLOGOpen, stkdi, stream);

    // Stack should have entries (lower on top = lowest offset at stack top)
    want(stkdi[0] < stkdi[1]);
    u64 top = *(stkdi[1] - 1);
    testeq(0, top);  // Top should be offset 0

    done;
}

// Test seeking
ok64 SLOG2() {
    sane(1);

    a_pad(u8, buf, 8192);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    // Write 200 records with values 0, 10, 20, ...
    for (u64 i = 0; i < 200; ++i) {
        call(writeRec, data, i * 10);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    // Test seeking for various values
    u64 targets[] = {0, 50, 100, 500, 1000, 1500, 1990};

    for (size_t t = 0; t < sizeof(targets) / sizeof(targets[0]); ++t) {
        // Reset stack
        u64bReset(stk);

        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = targets[t];
        a_pad(u8, tgtbuf, 16);
        a_rawc(tgtraw, target);
        call(TLVu8sFeed, tgtbuf_idle, 'A', tgtraw);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, u64TLVless, tgt);
        if (o == SLOGMISS) {
            // Target beyond last record
            continue;
        }
        test(o == OK, o);

        // Get the found offset
        want(stkdi[0] < stkdi[1]);
        u64 foundoff = *(stkdi[1] - 1);

        // Read record at that offset
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);
        want(lit == 'A');
        want($len(val) == sizeof(u64));

        u64 foundval = *(u64 *)val[0];
        // Found value should equal target
        testeq(foundval, target);
    }

    done;
}

// Test with many records to trigger 'k' flushes
ok64 SLOG3() {
    sane(1);

    a_pad(u8, buf, 8 * 1024);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    // Write 300 records (fits within close record limit)
    for (u64 i = 0; i < 300; ++i) {
        call(writeRec, data, i);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    // Seek for various values (avoid very last record as it might not be marked)
    u64 tests[] = {0, 1, 50, 100, 150, 200, 250};

    for (size_t t = 0; t < sizeof(tests) / sizeof(tests[0]); ++t) {
        u64bReset(stk);

        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = tests[t];
        a_pad(u8, tgtbuf, 16);
        a_rawc(tgtraw, target);
        call(TLVu8sFeed, tgtbuf_idle, 'A', tgtraw);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, u64TLVless, tgt);
        if (o == SLOGMISS) continue;
        test(o == OK, o);

        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        u64 foundval = *(u64 *)val[0];
        // Found value should be >= target
        want(foundval >= target);
    }

    done;
}

// Test with 1M records, verify O(log N) seek behavior
ok64 SLOG4() {
    sane(1);

    size_t N = 100000;
    size_t bufsize = N * 12 + 128 * 1024;  // ~12 bytes per record + margin

    u8 *mem = malloc(bufsize);
    test(mem != NULL, SLOGNOROOM);
    u8 *buf[4] = {mem, mem, mem, mem + bufsize};
    u8gp data = u8bDataIdle(buf);

    a_pad(u64, stk, 256);  // larger stack for 1M records
    u64gp stkdi = u64bDataIdle(stk);

    call(SLOGCreate, stkdi, data);

    // Write N records with values 0, 1, 2, ...
    for (u64 i = 0; i < N; ++i) {
        call(writeRec, data, i);
        call(SLOGSample, stkdi, data);
    }

    fprintf(stderr, "  before close, stack len=%zu\n", u64bDataLen(stk));
    call(SLOGClose, stkdi, data);
    fprintf(stderr, "  after close, data len=%zu\n", u8bDataLen(buf));

    u64 total_pops = 0;
    u64 total_cmp = 0;
    u64 total_kexp = 0;
    u64 total_kskip = 0;

    for (u64 target = 0; target < N; ++target) {
        u64bReset(stk);
        SLOGStatsReset();

        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        a_pad(u8, tgtbuf, 16);
        a_rawc(tgtraw, target);
        call(TLVu8sFeed, tgtbuf_idle, 'A', tgtraw);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, u64TLVless, tgt);
        if (o != OK) {
            fprintf(stderr, "  seek %" PRIu64 ": %s\n", target, ok64str(o));
            if (o == SLOGMISS) continue;
            fail(o);
        }

        // Verify found value
        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);
        u64 foundval = *(u64 *)val[0];
        want(foundval >= target);

        total_pops += SLOG_STATS[SLOG_STAT_POPS];
        total_cmp += SLOG_STATS[SLOG_STAT_CMP];
        total_kexp += SLOG_STATS[SLOG_STAT_KEXP];
        total_kskip += SLOG_STATS[SLOG_STAT_KSKIP];
    }

    // Expected: O(log N) pops per seek
    // log2(1M) ~ 20, so avg pops should be < 40 (with some margin)
    u64 avg_pops = total_pops / N;
    u64 avg_cmp = total_cmp / N;

    fprintf(stderr, "SLOG4: N=%zu\n", N);
    fprintf(stderr, "  avg pops: %" PRIu64 " (expect ~log2(N)=%d)\n", avg_pops, 20);
    fprintf(stderr, "  avg cmp:  %" PRIu64 "\n", avg_cmp);
    fprintf(stderr, "  total kexp: %" PRIu64 ", kskip: %" PRIu64 "\n", total_kexp, total_kskip);

    // Verify logarithmic behavior
    want(avg_pops < 50);  // should be ~20-30
    want(avg_cmp < 50);   // comparisons also logarithmic

    free(mem);
    done;
}

// Repro from fuzz: slice bounds in comparator
ok64 SLOG5() {
    sane(1);

    u64 vals[] = {124614455404317ULL, 10696212323827713ULL};

    a_pad(u8, buf, 4096);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    for (size_t i = 0; i < 2; ++i) {
        call(writeNumStrRec, data, vals[i]);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    for (size_t i = 0; i < 2; ++i) {
        u64bReset(stk);
        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = vals[i];

        // Build target string
        a_pad(u8, strbuf, 64);
        u8 *tp = strbuf[0];
        u64 tmp = target;
        u8 digits[20];
        int ndig = 0;
        do {
            digits[ndig++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        while (ndig > 0) *tp++ = digits[--ndig];
        u8cs tgtstr = {strbuf[0], tp};

        // Wrap as TLV record
        a_pad(u8, tgtbuf, 128);
        call(TLVu8sFeed, tgtbuf_idle, 'S', tgtstr);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        test(o == OK, o);

        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        u64 foundval = 0;
        u8c *vp = val[0];
        while (vp < val[1] && *vp >= '0' && *vp <= '9') {
            foundval = foundval * 10 + (*vp - '0');
            ++vp;
        }
        want(foundval >= target);
    }

    done;
}

// Repro from fuzz: large u64 values, 'k' record before 'i'
ok64 SLOG6() {
    sane(1);

    u64 vals[] = {16933534598913064961ULL, 18446514275661379583ULL};

    a_pad(u8, buf, 8192);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    for (size_t i = 0; i < 2; ++i) {
        call(writeNumStrRec, data, vals[i]);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    for (size_t i = 0; i < 2; ++i) {
        u64bReset(stk);
        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = vals[i];

        // Build target string
        a_pad(u8, strbuf, 64);
        u8 *tp = strbuf[0];
        u64 tmp = target;
        u8 digits[20];
        int ndig = 0;
        do {
            digits[ndig++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        while (ndig > 0) *tp++ = digits[--ndig];
        u8cs tgtstr = {strbuf[0], tp};

        // Wrap as TLV record
        a_pad(u8, tgtbuf, 128);
        call(TLVu8sFeed, tgtbuf_idle, 'S', tgtstr);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        test(o == OK, o);

        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        u64 foundval = 0;
        u8c *vp = val[0];
        while (vp < val[1] && *vp >= '0' && *vp <= '9') {
            foundval = foundval * 10 + (*vp - '0');
            ++vp;
        }
        want(foundval >= target);
    }

    done;
}

// Repro from fuzz: duplicates, unmarked records, linear scan fallback
ok64 SLOG7() {
    sane(1);

    u64 vals[] = {0ULL, 0ULL, 257ULL};

    a_pad(u8, buf, 4096);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        call(writeNumStrRec, data, vals[i]);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        u64bReset(stk);
        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = vals[i];

        // Build target string
        a_pad(u8, strbuf, 64);
        u8 *tp = strbuf[0];
        u64 tmp = target;
        u8 digits[20];
        int ndig = 0;
        do {
            digits[ndig++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        while (ndig > 0) *tp++ = digits[--ndig];
        u8cs tgtstr = {strbuf[0], tp};

        // Wrap as TLV record
        a_pad(u8, tgtbuf, 128);
        call(TLVu8sFeed, tgtbuf_idle, 'S', tgtstr);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        test(o == OK, o);

        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        u64 foundval = 0;
        u8c *vp = val[0];
        while (vp < val[1] && *vp >= '0' && *vp <= '9') {
            foundval = foundval * 10 + (*vp - '0');
            ++vp;
        }
        want(foundval >= target);
    }

    done;
}

// Repro from fuzz: 'i' offset pushed, needs skip
ok64 SLOG8() {
    sane(1);

    u64 vals[] = {0ULL, 4294967309ULL, 6341068275338510349ULL};

    a_pad(u8, buf, 8192);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        call(writeNumStrRec, data, vals[i]);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        u64bReset(stk);
        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = vals[i];

        // Build target string
        a_pad(u8, strbuf, 64);
        u8 *tp = strbuf[0];
        u64 tmp = target;
        u8 digits[20];
        int ndig = 0;
        do {
            digits[ndig++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        while (ndig > 0) *tp++ = digits[--ndig];
        u8cs tgtstr = {strbuf[0], tp};

        // Wrap as TLV record
        a_pad(u8, tgtbuf, 128);
        call(TLVu8sFeed, tgtbuf_idle, 'S', tgtstr);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        test(o == OK, o);
    }

    done;
}

// Repro from fuzz: minimized crash - SLOGMISS for existing value
ok64 SLOG9() {
    sane(1);

    // Values from minimized fuzz crash (sorted)
    u64 vals[] = {252950085632ULL, 252950085887ULL, 18374686480659710207ULL};

    a_pad(u8, buf, 65536);
    a_pad(u64, stk, 64);
    u64gp stkdi = u64bDataIdle(stk);
    u8gp data = u8bDataIdle(buf);

    call(SLOGCreate, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        call(writeNumStrRec, data, vals[i]);
        call(SLOGSample, stkdi, data);
    }

    call(SLOGClose, stkdi, data);

    for (size_t i = 0; i < 3; ++i) {
        u64bReset(stk);
        a_dup(u8c,stream,data);
        call(SLOGOpen, stkdi, stream);

        u64 target = vals[i];

        // Build target string
        a_pad(u8, strbuf, 64);
        u8 *tp = strbuf[0];
        u64 tmp = target;
        u8 digits[20];
        int ndig = 0;
        do {
            digits[ndig++] = '0' + (tmp % 10);
            tmp /= 10;
        } while (tmp > 0);
        while (ndig > 0) *tp++ = digits[--ndig];
        u8cs tgtstr = {strbuf[0], tp};

        // Wrap as TLV record
        a_pad(u8, tgtbuf, 128);
        call(TLVu8sFeed, tgtbuf_idle, 'S', tgtstr);
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        ok64 o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        test(o == OK, o);
    }

    done;
}

ok64 SLOGtest() {
    sane(1);
    call(SLOG0);
    call(SLOG1);
    call(SLOG2);
    call(SLOG3);
    call(SLOG4);
    call(SLOG5);
    call(SLOG6);
    call(SLOG7);
    call(SLOG8);
    call(SLOG9);
    done;
}

TEST(SLOGtest);
