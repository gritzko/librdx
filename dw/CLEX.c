#include "CLEX.h"

#include <unistd.h>

#include "abc/BUF.h"
#include "abc/FILE.h"
#include "rdx/RDX.h"

ok64 CLEXonToken($cu8c tok, CLEXstate *state) {
    sane(state != nil);
    id128 id0 = {};
    call(RDXfeed, state->rdx, RDX_STRING, id0, tok);
    done;
}

ok64 CLEXonRoot($cu8c tok, CLEXstate *state) { return OK; }
