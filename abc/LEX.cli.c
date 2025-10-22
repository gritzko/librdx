#include "LEX.h"

#include <fcntl.h>
#include <stdio.h>

#include "PRO.h"

a_cstr(ext, ".lex");

con char *ragel_template;
con char *header_template;

ABC_INIT;

ok64 lex2rl(u8cs mod, u8cs lang);

int main(int argn, char **args) {
    if (argn != 3) {
        fprintf(stderr, "Usage: lex2rl MOD [c|go]\n");
        return -1;
    }
    a_cstr(name, args[1]);
    a_cstr(lang, args[2]);
    ok64 o = lex2rl(name, lang);
    if (o != OK)
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);

    return o;
}
