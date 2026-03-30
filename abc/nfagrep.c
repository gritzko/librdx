//
// nfagrep — grep-like tool using Thompson NFA
// Usage: nfagrep PATTERN [FILE ...]
// Reads stdin if no files given.
//

#include "NFA.h"

#include "FILE.h"
#include "PRO.h"

#define NFA_CAP 4096

ok64 grep_stream(nfau8cs nfa, u32 *ws[2], FILE *fp, u8c *fname) {
    sane(1);
    char line[8192];
    u64 lineno = 0;
    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        u64 len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') len--;
        u8cs text = {(u8 *)line, (u8 *)line + len};
        if (NFAu8Search(nfa, text, ws)) {
            if (fname)
                printf("%s:%" PRIu64 ":%.*s\n", fname, lineno, (int)len,
                       line);
            else
                printf("%.*s\n", (int)len, line);
        }
    }
    done;
}

ok64 nfagrep() {
    sane($arglen >= 2);
    if ($arglen < 2) {
        fprintf(stderr, "usage: nfagrep PATTERN [FILE ...]\n");
        fail(NFABADSYN);
    }

    a$rg(pat, 1);

    nfau8 buf[NFA_CAP];
    nfau8g prog = {buf, buf + NFA_CAP, buf};
    u32 pbuf[NFA_CAP * 2];
    u32 *pws[2] = {pbuf, pbuf + NFA_CAP * 2};

    ok64 o = NFAu8Compile(prog, pat, pws);
    if (o != OK) {
        fprintf(stderr, "nfagrep: bad pattern: %s\n", ok64str(o));
        return o;
    }

    nfau8cs nfa = {prog[2], prog[0]};
    u32 wbuf[NFA_CAP * 3];
    u32 *ws[2] = {wbuf, wbuf + NFA_CAP * 3};

    if ($arglen <= 2) {
        call(grep_stream, nfa, ws, stdin, NULL);
    } else {
        for (int i = 2; i < $arglen; i++) {
            a$rg(fn, i);
            char fname[256];
            snprintf(fname, sizeof(fname), "%.*s", (int)$len(fn), fn[0]);
            FILE *fp = fopen(fname, "r");
            if (!fp) {
                fprintf(stderr, "nfagrep: %s: %s\n", fname, strerror(errno));
                continue;
            }
            call(grep_stream, nfa, ws, fp, (u8c *)fname);
            fclose(fp);
        }
    }

    done;
}

MAIN(nfagrep);
