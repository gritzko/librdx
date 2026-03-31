//
// NFA fuzz test — stability only
//
// Input: u8cs, prefix up to first \n is regex, rest is content.
// Compile the regex, match/search the content. Must not crash.
//

#include "NFA.h"
#include "PRO.h"
#include "TEST.h"

#define NFA_FUZZ_MAX 256

static nfau8 nfabuf[NFA_FUZZ_MAX];
static u32 pbuf[NFA_FUZZ_MAX * 2];
static u32 wbuf[NFA_FUZZ_MAX * 3];

FUZZ(u8, NFAfuzz) {
    sane(1);
    if ($empty(input)) done;

    // split at first \n
    u8cp nl = NULL;
    $for(u8c, p, input) {
        if (*p == '\n') {
            nl = p;
            break;
        }
    }

    u8cs pat, text;
    if (nl) {
        a_head(u8c, ph, input, nl - input[0]);
        a_rest(u8c, tr, input, nl + 1 - input[0]);
        pat[0] = ph[0];
        pat[1] = ph[1];
        text[0] = tr[0];
        text[1] = tr[1];
    } else {
        pat[0] = input[0];
        pat[1] = input[1];
        text[0] = input[1];
        text[1] = input[1];
    }

    if ($len(pat) == 0 || $len(pat) > NFA_FUZZ_MAX) done;

    nfau8g prog = {nfabuf, nfabuf + NFA_FUZZ_MAX, nfabuf};
    u32 *pws[2] = {pbuf, pbuf + NFA_FUZZ_MAX * 2};

    ok64 o = NFAu8Compile(prog, pat, pws);
    if (o != OK) done;

    nfau8cs nfa = {prog[2], prog[0]};
    u16 n = NFAu8States(nfa);
    if (n == 0 || 3 * (u64)n > sizeof(wbuf) / sizeof(wbuf[0])) done;

    u32 *ws[2] = {wbuf, wbuf + 3 * n};
    NFAu8Match(nfa, text, ws);
    NFAu8Search(nfa, text, ws);

    done;
}
