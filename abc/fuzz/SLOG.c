#include "NUM.h"
#include "PRO.h"
#include "SLOG.h"
#include "TEST.h"

// Record format: "1023. One thousand twenty three"
// Less-comparator: receives full TLV records, extracts leading u64
fun b8 numStrLess(u8csc a, u8csc b) {
    // Drain TLV to get body
    a_dup(u8c,arec,a);
    a_dup(u8c,brec,b);
    u8 alit = 0, blit = 0;
    u8cs aval = {}, bval = {};
    TLVu8sDrain(arec, &alit, aval);
    TLVu8sDrain(brec, &blit, bval);
    // Parse leading number from each body
    u64 na = 0, nb = 0;
    for (u8cp pa = aval[0]; pa < aval[1] && *pa >= '0' && *pa <= '9'; ++pa) {
        na = na * 10 + (*pa - '0');
    }
    for (u8cp pb = bval[0]; pb < bval[1] && *pb >= '0' && *pb <= '9'; ++pb) {
        nb = nb * 10 + (*pb - '0');
    }
    return na < nb;
}

// Write a record: "N. English words"
fun ok64 writeNumRec(u8gp data, u64 n) {
    sane(u8gOK(data));
    a_pad(u8, rec, 512);

    // Format: "N. "
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

    // English words
    u8s into = {p, rec[0] + 512};
    ok64 o = NUMu8sFeed(into, n);
    if (o != OK) return o;

    // Write as TLV with lit 'S'
    u8cs body = {rec[0], *into};
    call(TLVu8sFeed, u8gRest(data), 'S', body);
    done;
}

FUZZ(u64, SLOGfuzz) {
    sane(1);

    // Limit input size
    size_t maxn = 256;
    if ($len(input) > maxn) input[1] = input[0] + maxn;
    size_t n = $len(input);
    if (n == 0) done;

    // Copy and sort input
    a_pad(u64, sorted, 256);
    u64cs inp = {input[0], input[1]};
    call(u64gFeed, u64bDataIdle(sorted), inp);
    u64s sortslice = {sorted[1], sorted[2]};
    $sort(sortslice, &u64cmp);

    // Build SLOG stream with sorted records
    size_t bufsize = n * 600 + 4096;  // ~600 bytes per record max
    u8 *mem = malloc(bufsize);
    must(mem != NULL, "malloc failed");
    u8 *buf[4] = {mem, mem, mem, mem + bufsize};
    u8gp data = u8bDataIdle(buf);

    a_pad(u64, stk, 128);
    u64gp stkdi = u64bDataIdle(stk);

    ok64 o = SLOGCreate(stkdi, data);
    must(o == OK, "SLOGCreate failed");

    // Write sorted records
    for (u64 *p = sorted[1]; p < sorted[2]; ++p) {
        o = writeNumRec(data, *p);
        must(o == OK, "writeNumRec failed");
        o = SLOGSample(stkdi, data);
        must(o == OK, "SLOGSample failed");
    }

    o = SLOGClose(stkdi, data);
    must(o == OK, "SLOGClose failed");

    // Now seek each number, opening fresh each time
    a_dup(u8c,stream,data);

    for (u64 *p = sorted[1]; p < sorted[2]; ++p) {
        u64 target = *p;

        // Open fresh
        u64bReset(stk);
        o = SLOGOpen(stkdi, stream);
        must(o == OK, "SLOGOpen failed");

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
        o = TLVu8sFeed(tgtbuf_idle, 'S', tgtstr);
        must(o == OK, "TLVu8sFeed failed");
        u8csc tgt = {tgtbuf[0], tgtbuf_idle[0]};

        // Seek
        o = SLOGSeek(stkdi, stream, numStrLess, tgt);
        if (o == SLOGMISS) {
            // Should find at least the value itself
            must(0, "SLOGSeek MISS for existing value");
        }
        must(o == OK, "SLOGSeek failed");

        // Verify found value >= target
        must(stkdi[0] < stkdi[1], "empty stack after seek");
        u64 foundoff = *(stkdi[1] - 1);
        a$tail(u8c, rec, stream, foundoff);
        u8 lit = 0;
        u8cs val = {};
        o = TLVu8sDrain(rec, &lit, val);
        must(o == OK, "TLVu8sDrain failed");
        must(lit == 'S', "wrong literal");  // 'S' already uppercase

        // Extract found number
        u64 found = 0;
        u8c *vp = val[0];
        while (vp < val[1] && *vp >= '0' && *vp <= '9') {
            found = found * 10 + (*vp - '0');
            ++vp;
        }
        must(found >= target, "found value less than target");
    }

    free(mem);
    done;
}
