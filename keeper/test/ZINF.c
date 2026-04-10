//  ZINF test: deflate then inflate a chain of versions
//
//  Simulates packing ~20 small object versions back-to-back:
//  deflate each into a contiguous buffer, then inflate them
//  one by one, verifying round-trip fidelity.

#include "keeper/ZINF.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define NVER 20
#define VERSZ 256

// Generate a version: "version NN: " followed by fill bytes
static void make_version(uint8_t *buf, uint64_t *len, int idx) {
    int n = snprintf((char *)buf, VERSZ, "version %02d: ", idx);
    // fill the rest with a repeating pattern that varies per version
    for (int i = n; i < VERSZ - 1; i++)
        buf[i] = (uint8_t)('A' + ((idx + i) % 26));
    buf[VERSZ - 1] = '\n';
    *len = VERSZ;
}

ok64 ZINFtest1() {
    sane(1);

    // Pack buffer: holds all deflated versions back-to-back
    uint8_t pack[NVER * VERSZ * 2];
    uint64_t pack_used = 0;

    // Record where each compressed version starts and its compressed size
    uint64_t offsets[NVER];
    uint64_t csizes[NVER];

    // --- deflate chain ---
    for (int i = 0; i < NVER; i++) {
        uint8_t ver[VERSZ];
        uint64_t vlen = 0;
        make_version(ver, &vlen, i);

        uint64_t produced = 0;
        offsets[i] = pack_used;
        int r = ZINFDeflate(ver, vlen,
                            pack + pack_used,
                            sizeof(pack) - pack_used,
                            &produced);
        want(r == 0);
        want(produced > 0);
        csizes[i] = produced;
        pack_used += produced;
    }

    // --- inflate chain and verify ---
    for (int i = 0; i < NVER; i++) {
        uint8_t orig[VERSZ];
        uint64_t olen = 0;
        make_version(orig, &olen, i);

        uint8_t got[VERSZ];
        memset(got, 0, sizeof(got));
        uint64_t consumed = 0, produced = 0;

        int r = ZINFInflate(pack + offsets[i], csizes[i],
                            got, sizeof(got),
                            &consumed, &produced);
        want(r == 0);
        want(produced == olen);
        want(consumed == csizes[i]);
        want(memcmp(orig, got, olen) == 0);
    }

    done;
}

ok64 maintest() {
    sane(1);
    call(ZINFtest1);
    done;
}

TEST(maintest)
