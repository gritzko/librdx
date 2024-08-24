#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "FILE.h"
#include "INT.h"
#include "LEX.h"

a$strc(ext, ".lex");

con char *ragel_template;
con char *header_template;

ABC_INIT;

pro(lex2rl, $u8c mod) {
    aBpad(u8, synpad, KB * 4);
    aBpad(u8, actpad, KB * 4);
    aBpad(u8, enmpad, KB);
    aBpad(u8, fnspad, KB * 4);
    LEXstate state = {
        .lex = {},
        .mod = mod,
        .syn = Bu8idle(synpad),
        .act = Bu8idle(actpad),
        .enm = Bu8idle(enmpad),
        .fns = Bu8idle(fnspad),
    };

    aBpad(u8, name, KB);
    $u8c $namet = $u8str("$s.lex");
    $feedf(Bu8idle(name), $namet, mod);

    int fd;
    call(FILEopen, &fd, Bu8cdata(name), O_RDONLY);

    aBpad(u8, hname, KB);
    $u8c $hnamet = $u8str("$s.rl.h");
    $feedf(Bu8idle(hname), $hnamet, mod);

    int hfd;
    call(FILEcreate, &hfd, Bu8cdata(hname));

    aBpad(u8, rname, KB);
    $u8c $rnamet = $u8str("$s.rl");
    $feedf(Bu8idle(rname), $rnamet, mod);

    int rfd;
    call(FILEcreate, &rfd, Bu8cdata(rname));

    aBpad(u8, syn, MB);
    call(FILEdrainall, Bu8idle(syn), fd);

    $mv(state.lex.text, Bu8cdata(syn));
    call(LEXlexer, &state);

    call(FILEclose, fd);

    aBpad(u8, rpad, MB);
    a$strc(rtmpl, ragel_template);
    $feedf(Bu8idle(rpad), rtmpl, mod, mod, Bu8cdata(actpad), Bu8cdata(synpad),
           mod, mod, mod, mod, mod, mod, mod, mod, mod, mod, mod);
    call(FILEfeed, rfd, Bu8cdata(rpad));
    call(FILEclose, rfd);

    aBpad(u8, hpad, MB);
    a$strc(htmpl, header_template);
    $feedf(Bu8idle(hpad), htmpl, mod, Bu8cdata(enmpad), Bu8cdata(fnspad));
    call(FILEfeed, hfd, Bu8cdata(hpad));
    call(FILEclose, hfd);

    nedo($println(state.lex.text));
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

con char *header_template =
    "#include \"PRO.h\"\n"
    "#include \"INT.h\"\n"
    "#include \"$s.h\"\n"
    "\n"
    "enum {\n"
    "$s"
    "};\n"
    "$s\n"  // FUNCTIONS
    "\n";

con char *ragel_template =
    "#include \"$s.rl.h\"\n"
    "\n"
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
    "main := $sRoot;\n"
    "\n"
    "}%%\n"
    "\n"
    "%%write data;\n"
    "\n"
    "pro($slexer, $sstate* state) {\n"
    "    LEXbase* lex = &(state->lex);\n"
    "\n"
    "    a$dup(u8c, text, lex->text);\n"
    "    sane($ok(text));\n"
    "\n"
    "    int cs = lex->cs;\n"
    "    int res = 0;\n"
    "    u8c *p = (u8c*) text[0];\n"
    "    u8c *pe = (u8c*) text[1];\n"
    "    u8c *eof = pe;\n"
    "    u8c *pb = p;\n"
    "\n"
    "    u32 sp = 2;\n"
    "    $$u8c tok = {p, p};\n"
    "\n"
    "    %% write init;\n"
    "    %% write exec;\n"
    "\n"
    "    test(p==text[1], $sfail);\n"
    "\n"
    "    test(cs >= $s_first_final, $sfail);\n"
    "\n"
    "    nedo(\n"
    "        lex->text[0] = p;\n"
    "    );\n"
    "}\n";
