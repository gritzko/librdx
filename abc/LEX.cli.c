#include "LEX.h"

#include <fcntl.h>
#include <stdio.h>

#include "PRO.h"

a$strc(ext, ".lex");

static const char *ragel_template;
static const char *header_template;

ABC_INIT;

ok64 lex2rl($u8c mod, $u8c lang);

int main(int argn, char **args) {
    if (argn != 3) {
        fprintf(stderr, "Usage: lex2rl MOD [c|go]\n");
        return -1;
    }
    a$strc(name, args[1]);
    a$strc(lang, args[2]);
    ok64 o = lex2rl(name, lang);
    if (o != OK)
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);

    return o;
}
