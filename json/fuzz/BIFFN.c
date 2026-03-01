//
// Fuzz test: BASONMergeN with 2 inputs == BASONMerge
// Input: raw bytes split into two JSON docs, converted to BASON.
//
#include "json/BIFF.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

#define FUZZ_BUF (64 * 1024)

// Parse raw bytes as JSON into BASON; skip non-plex or dup-key inputs.
static ok64 BIFFNFuzzParse(u8bp buf, u8csc raw) {
    sane(buf != NULL);
    u8cp p = raw[0];
    while (p < raw[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= raw[1]) return BADARG;
    if (*p != '{' && *p != '[') return BADARG;
    u8cs json = {raw[0], raw[1]};
    call(BASONParseJSON, buf, NULL, json);
    u8cp d0 = buf[1], d1 = buf[2];
    u8cs data = {d0, d1};
    if ($len(data) == 0) return BADARG;
    done;
}

FUZZ(u8, BIFFNfuzz) {
    sane(1);
    if ($len(input) < 4 || $len(input) > 4096) done;

    size_t mid = $len(input) / 2;
    u8csc left_raw = {input[0], input[0] + mid};
    u8csc right_raw = {input[0] + mid, input[1]};

    // Parse both halves
    a_pad(u8, lbuf, FUZZ_BUF);
    if (BIFFNFuzzParse(lbuf, left_raw) != OK) done;

    a_pad(u8, rbuf, FUZZ_BUF);
    if (BIFFNFuzzParse(rbuf, right_raw) != OK) done;

    u8cp ld0 = lbuf[1], ld1 = lbuf[2];
    u8cs ldata = {ld0, ld1};
    u8cp rd0 = rbuf[1], rd1 = rbuf[2];
    u8cs rdata = {rd0, rd1};

    // Must be same root type
    if ((ldata[0][0] & ~TLVaA) != (rdata[0][0] & ~TLVaA)) done;

    // 2-way merge: BASONMerge
    a_pad(u8, out2, FUZZ_BUF);
    u64 _lstk[256];
    u64b lstk = {_lstk, _lstk, _lstk, _lstk + 256};
    u64 _rstk[256];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 256};

    ok64 o2 = BASONMerge(out2, NULL, lstk, ldata, rstk, rdata);
    must(o2 == OK, "BASONMerge failed");

    // N-way merge with 2 inputs: BASONMergeN
    a_pad(u8, outn, FUZZ_BUF);
    u8cs recs[2] = {{ldata[0], ldata[1]}, {rdata[0], rdata[1]}};
    u8css inputs = {recs, recs + 2};

    ok64 on = BASONMergeN(outn, NULL, inputs);
    must(on == OK, "BASONMergeN failed");

    // Compare results
    u8cp r2_0 = out2[1], r2_1 = out2[2];
    u8cs result2 = {r2_0, r2_1};
    u8cp rn_0 = outn[1], rn_1 = outn[2];
    u8cs resultn = {rn_0, rn_1};

    must($len(result2) == $len(resultn), "MergeN size mismatch");
    must(memcmp(result2[0], resultn[0], $len(result2)) == 0,
         "MergeN data mismatch");

    done;
}
