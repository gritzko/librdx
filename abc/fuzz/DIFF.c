//
// DIFF fuzz test
//

#include "DIFF.h"
#include "INT.h"
#include "S.h"

#define X(M, name) M##u8##name
#include "DIFFx.h"
#undef X

#include "PRO.h"
#include "TEST.h"

#define MAX_HALF 64

static i32 workbuf[2 * (2 * MAX_HALF * 2 + 1)];
static e32 edlbuf[MAX_HALF * 2 + 1];
static u8 outbuf[MAX_HALF + 1];

FUZZ(u8, DIFFfuzz) {
    sane(1);
    if ($empty(input)) done;

    u8cp nl = NULL;
    $for(u8c, p, input) {
        if (*p == '\n') {
            nl = p;
            break;
        }
    }

    u8cs a, b;
    if (nl) {
        a_head(u8c, ah, input, nl - input[0]);
        a_rest(u8c, br, input, nl + 1 - input[0]);
        a[0] = ah[0]; a[1] = ah[1];
        b[0] = br[0]; b[1] = br[1];
    } else {
        a[0] = input[0]; a[1] = input[1];
        b[0] = input[1]; b[1] = input[1];
    }

    if ($len(a) > MAX_HALF || $len(b) > MAX_HALF) done;

    i32s work = {workbuf, workbuf + sizeof(workbuf) / sizeof(workbuf[0])};
    e32g edl = {edlbuf, edlbuf + sizeof(edlbuf) / sizeof(edlbuf[0]), edlbuf};

    call(DIFFu8s, edl, work, a, b);
    e32cs edlc = {edl[2], edl[0]};

    u8s out = {outbuf, outbuf + sizeof(outbuf)};
    u8 *outstart = out[0];

    call(DIFFu8sApply, out, a, b, edlc);

    u64 outlen = out[0] - outstart;
    if (outlen != (u64)$len(b)) return FAILSANITY;
    if (outlen > 0 && memcmp(outstart, b[0], outlen) != 0) return FAILSANITY;

    done;
}
