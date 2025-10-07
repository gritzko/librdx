#include "MARK.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/MMAP.h"

a$strc(ext, "abc/.lex");

#define X(M, name) M##u8##name
#include "abc/MMAPx.h"
#undef X
#define X(M, name) M##u64##name
#include "abc/MMAPx.h"
#undef X
#define X(M, name) M##u8cp##name
#include "abc/MMAPx.h"
#undef X

ABC_INIT;

void debug(MARKstate *state) {
    printf("========\n");
    aBpad(u8, t, 9);
    size_t len = Bdatalen(state->lineB);
    u64 *p = state->pB[0];
    for (size_t l = 0; l + 1 < len; ++l) {
        u64 div = Bat(state->divB, l);
        for (u8 d = 0; d < 8; ++d) {
            Bat(t, d + 1) = MARKdivascii[u64byte(div, d)];
        }
        Bat(t, 0) = '|';
        if (*p == l) {
            Bat(t, 0) = '+';
            ++p;
        }
        $print(Bu8cidle(t));
        $print(state->lineB[0] + l);
    }
    printf("========\n");
}

#define a$strf(name, len, tmpl, ...) \
    aBpad(u8, name, len);            \
    $u8c __##name = $u8str(tmpl);    \
    $feedf(Bu8idle(name), __##name, __VA_ARGS__);

pro(mark, $u8c mod) {
    sane($ok(mod) && !$empty(mod) && $len(mod) <= 1000);
    int fd = 0;
    call(FILEopen, &fd, mod, O_RDONLY);
    Bu8 text = {};
    call(FILEmap, text, &fd, PROT_READ);

    Bu8 fmtbuf = {};
    u8cpB linebuf = {};
    Bu64 divbuf = {};
    Bu64 pbuf = {};
    Bu8 intobuf = {};
    then try(MMAPu8open, fmtbuf, Blen(text));
    then try(MMAPu8cpopen, linebuf, Blen(text));
    then try(MMAPu64open, divbuf, Blen(text));
    then try(MMAPu64open, pbuf, Blen(text));
    then try(MMAPu8open, intobuf, roundup(Blen(text) * 8, PAGESIZE));

    then {
        MARKstate state = {};
        state.divB = (u64Bp)divbuf;
        state.lineB = (u8cpBp)linebuf;
        state.pB = (u64Bp)pbuf;
        $mv(state.text, Bu8cdata(text));
        $mv(state.fmt, Bu8idle(fmtbuf));

        try(MARKlexer, &state);
        then try(MARKMARQ, &state);
        then try(MARKANSI, Bu8idle(intobuf), 64, &state);
        then try(FILEfeedall, STDOUT_FILENO, Bu8cdata(intobuf));
    }

    FILEclose(&fd);
    MMAPu8close(fmtbuf);
    MMAPu8close(intobuf);
    MMAPu8cpclose(linebuf);
    MMAPu64close(divbuf);
    MMAPu64close(pbuf);

    return OK;
}

int main(int argn, char **args) {
    if (argn != 2) {
        fprintf(stderr, "Usage: mark [file.md]\n");
        return -1;
    }
    a$strc(name, args[1]);
    ok64 o = mark(name);
    if (o != OK)
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);

    return o;
}
