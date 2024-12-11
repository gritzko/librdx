#include "LEX.h"

#include <fcntl.h>
#include <stdio.h>

#include "CT.h"
#include "FILE.h"

a$strc(ext, ".lex");

con char *ragel_template;
con char *header_template;

ABC_INIT;

ok64 lex2rl($u8c mod);

int main(int argn, char **args) {
    if (argn != 2) {
        fprintf(stderr, "Usage: lex2rl MOD\n");
        return -1;
    }
    a$strc(name, args[1]);
    ok64 o = lex2rl(name);
    if (o != OK)
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);

    return o;
}
