//
// Fuzz test: merge(old, diff(old, new)) == new
// Input: raw bytes split into two JSON docs, converted to BASON.
//
#include "json/BIFF.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

#define FUZZ_BUF (64 * 1024)

// Recursively check that no level has duplicate keys.
static ok64 BIFFFuzzNoDupKeys(u64bp stk, u8csc data,
                               u8 type, u8cs val) {
    sane(stk != NULL);
    if (!BASONPlex(type)) done;
    call(BASONInto, stk, data, val);
    u8 ct; u8cs ck, cv;
    u8cs prev_key = {NULL, NULL};
    while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
        if (prev_key[0] != NULL && $cmp(prev_key, ck) >= 0)
            return BADARG;
        prev_key[0] = ck[0];
        prev_key[1] = ck[1];
        ok64 o = BIFFFuzzNoDupKeys(stk, data, ct, cv);
        if (o != OK) return o;
    }
    call(BASONOuto, stk);
    done;
}

// Parse raw bytes as JSON into BASON; verify canonical (no dup keys).
static ok64 BIFFFuzzParse(u8bp buf, u64bp idx, u8csc raw) {
    sane(buf != NULL);
    u8cp p = raw[0];
    while (p < raw[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= raw[1]) return BADARG;
    if (*p != '{' && *p != '[') return BADARG;
    u8cs json = {raw[0], raw[1]};
    call(BASONParseJSON, buf, idx, json);

    // Verify: exactly one plex root, no duplicate keys
    u8cp d0 = buf[1], d1 = buf[2];
    u8cs data = {d0, d1};
    if ($len(data) == 0) return BADARG;
    u64 _ts[64];
    u64b ts = {_ts, _ts, _ts, _ts + 64};
    if (BASONOpen(ts, data) != OK) return BADARG;
    u8 t; u8cs k, v;
    if (BASONDrain(ts, data, &t, k, v) != OK) return BADARG;
    if (!BASONPlex(t)) return BADARG;
    ok64 o = BIFFFuzzNoDupKeys(ts, data, t, v);
    if (o != OK) return o;
    // must be the only top-level element
    u8 xt; u8cs xk, xv;
    if (BASONDrain(ts, data, &xt, xk, xv) != BASONEND) return BADARG;
    done;
}

FUZZ(u8, BIFFfuzz) {
    sane(1);
    if ($len(input) < 4 || $len(input) > 4096) done;

    // Split input into two halves at the midpoint
    size_t mid = $len(input) / 2;
    u8csc old_raw = {input[0], input[0] + mid};
    u8csc new_raw = {input[0] + mid, input[1]};

    // Parse old and new (with dup-key check, no index)
    a_pad(u8, obuf, FUZZ_BUF);
    if (BIFFFuzzParse(obuf, NULL, old_raw) != OK) done;

    a_pad(u8, nbuf, FUZZ_BUF);
    if (BIFFFuzzParse(nbuf, NULL, new_raw) != OK) done;

    u8cp od0 = obuf[1], od1 = obuf[2];
    u8cs odata = {od0, od1};
    u8cp nd0 = nbuf[1], nd1 = nbuf[2];
    u8cs ndata = {nd0, nd1};

    // must be same root type (both already validated as single plex root)
    if ((odata[0][0] & ~TLVaA) != (ndata[0][0] & ~TLVaA)) done;

    // Diff: patch = diff(old, new) — no index (NULL idx)
    a_pad(u8, pbuf, FUZZ_BUF);
    u64 _ostk[256];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 256};
    u64 _nstk[256];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 256};

    ok64 o = BASONDiff(pbuf, NULL, ostk, odata, nstk, ndata);
    must(o == OK, "BASONDiff failed");

    // Merge: result = merge(old, patch) — no index (NULL idx)
    a_pad(u8, rbuf, FUZZ_BUF);
    u64 _lstk[256];
    u64b lstk = {_lstk, _lstk, _lstk, _lstk + 256};
    u64 _rstk[256];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 256};

    u8cp pd0 = pbuf[1], pd1 = pbuf[2];
    u8cs pdata = {pd0, pd1};

    if ($len(pdata) == 0) {
        must($len(odata) == $len(ndata), "empty diff but sizes differ");
        must(memcmp(odata[0], ndata[0], $len(odata)) == 0,
             "empty diff but data differs");
        done;
    }

    o = BASONMerge(rbuf, NULL, lstk, odata, rstk, pdata);
    must(o == OK, "BASONMerge failed");

    // Verify: merge(old, patch) == new
    u8cp rd0 = rbuf[1], rd1 = rbuf[2];
    u8cs rdata = {rd0, rd1};
    must($len(rdata) == $len(ndata), "roundtrip size mismatch");
    must(memcmp(rdata[0], ndata[0], $len(ndata)) == 0,
         "roundtrip data mismatch");

    done;
}
