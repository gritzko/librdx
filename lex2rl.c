#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "FILE.h"
#include "INT.h"
#include "LEX.h"

a$strc(ext, ".lex");

con char *ragel_template;

ABC_INIT;

pro(lex2rl, $u8c mod) {
    aBpad(u8, synpad, KB * 4);
    aBpad(u8, actpad, KB * 4);
    aBpad(u8, enmpad, KB);
    aBpad(u8, fnspad, KB * 4);
    LEXstate state = {
        .cs = 0,
        .tbc = 0,
        .mod = mod,
        .syn = Bu8idle(synpad),
        .act = Bu8idle(actpad),
        .enm = Bu8idle(enmpad),
        .fns = Bu8idle(fnspad),
    };

    aBpad(u8, name, KB);
    call(Bu8feed$, name, mod);
    call(Bu8feed$, name, ext);

    int fd;
    call(FILEopen, &fd, Bu8cdata(name), O_RDONLY);

    aBpad(u8, syn, MB);
    call(FILEdrainall, Bu8idle(syn), fd);

    $mv(state.text, Bu8cdata(syn));
    call(LEXlexer, &state);

    call(FILEclose, fd);

    aBpad(u8, pad, MB);
    a$strc(tmpl, ragel_template);
    $feedf(Bu8idle(pad), tmpl, mod, mod, Bu8cdata(enmpad), mod, mod, mod, mod,
           Bu8cdata(fnspad), mod, Bu8cdata(actpad), Bu8cdata(synpad), mod, mod,
           mod, mod, mod, mod, mod, mod, mod, mod, mod);
    $println(Bu8cdata(pad));

    nedo($println(state.text));
}

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

con char *ragel_template =
    "#include \"PRO.h\"\n"
    "#include \"$s.h\"\n"
    "\n"
    "enum {\n"
    "\t$s = 0,\n"
    "$s"
    "};\n"
    "\n"
    "#define $smaxnest 1024\n"
    "\n"
    "fun ok64 popfails(u32* stack, u32* sp, u32 type) {\n"
    "    while (*sp && stack[*sp]!=type) *sp -= 2;\n"
    "    return *sp ? OK : $sfail;\n"
    "}\n"
    "\n"
    "#define lexpush(t) { \\\n"
    "    if (sp>=$smaxnest) fail($sfail); \\\n"
    "    stack[++sp] = p - pb; \\\n"
    "    stack[++sp] = t; \\\n"
    "}\n"
    "#define lexpop(t)  \\\n"
    "    if (stack[sp]!=t) call(popfails, stack, &sp, t); \\\n"
    "    tok[0] = *(text)+stack[sp-1]; \\\n"
    "    tok[1] = p; \\\n"
    "    sp -= 2;\n"
    "\n"
    "$s\n"  // FUNCTIONS
    "\n"
    "%%{\n"
    "\n"
    "machine $s;\n"
    "\n"
    "alphtype unsigned char;\n"
    "\n"
    "$s"  // ACTIONS
    "\n"
    "$s"  // SYNTAX
    "main := $sroot;\n"
    "\n"
    "}%%\n"
    "\n"
    "%%write data;\n"
    "\n"
    "pro($slexer, $sstate* state) {\n"
    "    a$dup(u8c, text, state->text);\n"
    "    sane($ok(text));\n"
    "\n"
    "    int cs = state->cs;\n"
    "    int res = 0;\n"
    "    u8c *p = (u8c*) text[0];\n"
    "    u8c *pe = (u8c*) text[1];\n"
    "    u8c *eof = state->tbc ? NULL : pe;\n"
    "    u8c *pb = p;\n"
    "\n"
    "    u32 stack[$smaxnest] = {0, $s};\n"
    "    u32 sp = 2;\n"
    "    $$u8c tok = {p, p};\n"
    "\n"
    "    %% write init;\n"
    "    %% write exec;\n"
    "\n"
    "    test(p==text[1], $sfail);\n"
    "\n"
    "    if (state->tbc) {\n"
    "        test(cs != $s_error, $sfail);\n"
    "        state->cs = cs;\n"
    "    } else {\n"
    "        test(cs >= $s_first_final, $sfail);\n"
    "    }\n"
    "\n"
    "    nedo(\n"
    "        state->text[0] = p;\n"
    "    );\n"
    "}\n";
