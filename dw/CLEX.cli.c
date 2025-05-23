#include "CLEX.h"

#include <fcntl.h>
#include <stdio.h>

#include "abc/FILE.h"

a$strc(ext, ".lex");

static const char *ragel_template;
static const char *header_template;

ABC_INIT;

ok64 CLEX($u8c path) {
    sane($ok(path));
    Bu8 c = {};
    call(FILEmapro, c, path);
    Bu8 rdx = {};
    call(Bu8alloc, rdx, roundup(Bdatalen(c) * 4, PAGESIZE));
    CLEXstate state = {};
    $mv(state.text, Bu8data(c));
    state.rdx = Bu8idle(rdx);
    ok64 o = CLEXlexer(&state);
    if (o != OK) {
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);
        $println(state.text);
        fail(CLEXfail);
    } else {
        call(FILEfeedall, STDOUT_FILENO, Bu8cdata(rdx));
    }
    done;
}

int main(int argn, char **args) {
    if (argn != 2) {
        fprintf(stderr, "Usage: clex file.c\n");
        return -1;
    }
    a$strc(path, args[1]);
    return CLEX(path);
}
