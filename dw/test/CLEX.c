#include "CLEX.h"

#include <stdlib.h>

#include "abc/FILE.h"
#include "abc/TEST.h"

a$strc(C1,
       "#include \"stdio.h\"\n\n"
       "int main(int argn, char **args) {\n"
       "  int abc = 0;\n"
       "  for (int i = 0; i < argn; ++i) {\n"
       "    printf(\"%s\", args[i]);\n"
       "  }\n"
       "  return 0;\n"
       "}\n");

ok64 CLEXtest1() {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    CLEXstate state = {};
    $mv(state.text, C1);
    state.rdx = padidle;
    call(CLEXlexer, &state);
    test(!$empty(paddata), FAILsanity);
    done;
}

ok64 CLEXtest() {
    sane(1);
    call(CLEXtest1);
    done;
}

TEST(CLEXtest);
