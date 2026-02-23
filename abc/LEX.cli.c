#include "LEX.h"

#include <fcntl.h>
#include <stdio.h>

#include "BUF.h"
#include "PRO.h"

a_cstr(ext, ".lex");

con char *ragel_template;
con char *header_template;

ok64 lex2rl(u8cs mod, u8cs lang);

ok64 lexcli() {
    sane($arglen == 3);
    if ($arglen != 3) {
        fprintf(stderr, "Usage: lex MOD [c|go]\n");
        fail(BADARG);
    }
    a$rg(name, 1);
    a$rg(lang, 2);
    call(lex2rl, name, lang);
    done;
}

MAIN(lexcli);
