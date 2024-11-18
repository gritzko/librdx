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
    call(FILEmap, (void **)text, fd, PROT_READ, 0);

    Bu8 fmtbuf = {};
    call(MMAPu8open, fmtbuf, Blen(text));
    Bu8cp linebuf = {};
    call(MMAPu8cpopen, linebuf, Blen(text));
    Bu64 divbuf = {};
    call(MMAPu64open, divbuf, Blen(text));
    Bu64 pbuf = {};
    call(MMAPu64open, pbuf, Blen(text));
    Bu8 intobuf = {};
    call(MMAPu8open, intobuf, roundup(Blen(text) * 8, PAGESIZE));

    MARKstate state = {};
    state.divB = (u64B)divbuf;
    state.lineB = (u8cpB)linebuf;
    state.pB = (u64B)pbuf;
    $mv(state.text, Bu8cdata(text));
    $mv(state.fmt, Bu8idle(fmtbuf));

    call(MARKlexer, &state);
    debug(&state);
    call(MARKMARQ, &state);
    call(MARKANSI, Bu8idle(intobuf), 64, &state);

    call(FILEfeedall, STDOUT_FILENO, Bu8cdata(intobuf));

    nedo(FILEclose(fd); MMAPu8close(fmtbuf), MMAPu8close(intobuf),
                        MMAPu8cpclose(linebuf), MMAPu64close(divbuf),
                        MMAPu64close(pbuf));
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
