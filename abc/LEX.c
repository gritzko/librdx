#include "LEX.h"

#include <unistd.h>

#include "01.h"
#include "BUF.h"
#include "FILE.h"
#include "NEST.h"
#include "PRO.h"

#define LEX_TEMPL_ACTION 0
#define LEX_TEMPL_ENUM 1
#define LEX_TEMPL_FN 2
#define LEX_TEMPL_ACT 3
#define LEX_TEMPL_PUBACTNL 4
#define LEX_TEMPL_ACTNL 5
#define LEX_TEMPL_FILE 6
#define LEX_TEMPL_L 7

const u8c *LEX_TEMPL[LEX_TEMPL_LANG_LEN][LEX_TEMPL_LEN][2] = {
    {
        $u8str("action $mod${act}0 { mark0[$mod$act] = p - text[0]; }\n"
               "action $mod${act}1 {\n"
               "    tok[0] = text[0] + mark0[$mod$act];\n"
               "    tok[1] = p;\n"
               "    call(${mod}on$act, tok, state); \n"
               "}\n"),
        $u8str("\t$mod$act = ${mod}enum+$actno,\n"),
        $u8str("ok64 ${mod}on$act ($$cu8c tok, ${mod}state* state);\n"),
        $u8str("$mod$act = ( "),
        $u8str(" )  >$mod${act}0 %$mod${act}1;\n"),
        $u8str(" ); # no $act callback\n"),
        $u8str("#include \"abc/INT.h\"\n"
               "#include \"abc/PRO.h\"\n"
               "#include \"$mod.h\"\n"
               "\n"
               "// action indices for the parser\n"
               "#define ${mod}enum 0\n"
               "enum {\n"
               "$ENUM"
               "};\n"
               "\n"
               "// user functions (callbacks) for the parser\n"
               "$FN\n"
               "\n"
               "\n"
               "%%{\n"
               "\n"
               "machine $mod;\n"
               "\n"
               "alphtype unsigned char;\n"
               "\n"
               "# ragel actions\n"
               "$ACTIONS"
               "\n"
               "# ragel grammar rules\n"
               "$RULES"
               "\n"
               "main := ${mod}Root;\n"
               "\n"
               "}%%\n"
               "\n"
               "%%write data;\n"
               "\n"
               "// the public API function\n"
               "pro(${mod}lexer, ${mod}state* state) {\n"
               "\n"
               "    a$$dup(u8c, text, state->text);\n"
               "    sane($$ok(text));\n"
               "\n"
               "    int cs = 0;\n"
               "    u8c *p = (u8c*) text[0];\n"
               "    u8c *pe = (u8c*) text[1];\n"
               "    u8c *eof = pe;\n"
               "    u64 mark0[64] = {};\n"
               "\n"
               "    $$u8c tok = {p, p};\n"
               "\n"
               "    %% write init;\n"
               "    %% write exec;\n"
               "\n"
               "    state->text[0] = p;\n"
               "    if (p!=text[1] || cs < ${mod}_first_final) {\n"
               "        return ${mod}fail;\n"
               "    }\n"
               "    done;\n"
               "}\n"),
        $u8str("c"),
    },
    {
        $u8str("action $mod${act}0 { mark0[$mod$act] = p; }\n"
               "action $mod${act}1 {\n"
               "    err = ${mod}on$act(data[mark0[$mod$act] : p], state); \n"
               "    if err!=nil {\n"
               "        fbreak;\n"
               "    }\n"
               "}\n"),
        $u8str("    $mod$act = ${mod}enum+$actno\n"),
        $u8str("// func ${mod}on$act (tok []byte, state *${mod}state) error\n"),
        $u8str("$mod$act = ( "),
        $u8str(" )  >$mod${act}0 %$mod${act}1;\n"),
        $u8str(" ); # no $act callback\n"),
        $u8str("package main\n"
               "import \"errors\""
               "\n"
               "// action indices for the parser\n"
               "\n"
               "const ("
               "${mod}enum = 0\n"
               "$ENUM"
               ")\n"
               "\n"
               "// user functions (callbacks) for the parser\n"
               "$FN\n"
               "\n"
               "\n"
               "%%{\n"
               "\n"
               "machine $mod;\n"
               "\n"
               "alphtype byte;\n"
               "\n"
               "# ragel actions\n"
               "$ACTIONS"
               "\n"
               "# ragel grammar rules\n"
               "$RULES"
               "\n"
               "main := ${mod}Root;\n"
               "\n"
               "}%%\n"
               "\n"
               "%%write data;\n"
               "\n"
               "// the public API function\n"
               "func ${mod}lexer (state *${mod}state) (err error) {\n"
               "\n"
               "    data := state.text\n"
               "    var mark0 [64]int\n"
               "    cs, p, pe, eof := 0, 0, len(data), len(data)\n"
               "\n"
               "    %% write init;\n"
               "    %% write exec;\n"
               "\n"
               "    if (p!=len(data) || cs < ${mod}_first_final) {\n"
               "        state.text = state.text[p:];\n"
               "        return errors.New(\"${mod} bad syntax\")\n"
               "    }\n"
               "    return nil;\n"
               "}\n"),
        $u8str("go"),
    },
};

con ok64 LEX$ACTIONS = 0x1c5d849d30a;
con ok64 LEX$ENUM = 0x59e5ce;
con ok64 LEX$FN = 0x5cf;
con ok64 LEX$RULES = 0x1c39579b;

con ok64 LEX$mod = 0x28cf1;
con ok64 LEX$act = 0x389e5;
con ok64 LEX$actno = 0x33cb89e5;

ok64 LEXonName($cu8c tok, LEXstate *state) {
    ok64 o = $u8feed(NESTidle(state->ct), state->mod);
    if (o == OK) o = $u8feed(NESTidle(state->ct), tok);
    return o;
}
ok64 LEXonOp($cu8c tok, LEXstate *state) {
    return $u8feed(NESTidle(state->ct), tok);
}
ok64 LEXonClass($cu8c tok, LEXstate *state) {
    return $u8feed(NESTidle(state->ct), tok);
}
ok64 LEXonRange($cu8c tok, LEXstate *state) {
    return $u8feed(NESTidle(state->ct), tok);
}
ok64 LEXonString($cu8c tok, LEXstate *state) {
    return $u8feed(NESTidle(state->ct), tok);
}
ok64 LEXonQString($cu8c tok, LEXstate *state) {
    return $u8feed(NESTidle(state->ct), tok);
}
ok64 LEXonSpace($cu8c tok, LEXstate *state) {
    // return $u8feed(NESTidle(state->ct), tok);
    return $u8feed1(NESTidle(state->ct), ' ');
}

ok64 LEXonEntity($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonExpr($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonRep($cu8c tok, LEXstate *state) { return OK; }

ok64 LEXonEq($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != nil);
    u8c$ tmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACT];
    try(NESTsplice, state->ct, LEX$RULES);
    then try(NESTfeed, state->ct, tmpl);
    done;
}

pro(LEXonRuleName, $cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != nil);
    $set(state->cur, tok);
    state->ruleno++;

    if (**tok < 'A' || **tok > 'Z') done;

    u8B ct = (u8B)state->ct;

    u8c$ tmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACTION];
    call(NESTsplice, ct, LEX$ACTIONS);
    call(NESTfeed, ct, tmpl);

    u8c$ enmtmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ENUM];
    call(NESTsplice, ct, LEX$ENUM);
    call(NESTfeed, ct, enmtmpl);
    call(NESTsplice, ct, LEX$actno);
    call(u64decfeed, NESTidle(ct), state->ruleno);

    u8c$ fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_FN];
    call(NESTsplice, ct, LEX$FN);
    call(NESTfeed, ct, fntmpl);

    done;
}

ok64 LEXonLine($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != nil);
    u8B ct = (u8B)state->ct;
    u8c$ cur = state->cur;

    u8c$ fntmpl;
    if (**cur >= 'A' && **cur <= 'Z') {
        fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_PUBACTNL];
    } else {
        fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACTNL];
    }

    try(NESTfeed, ct, fntmpl);
    then try(NESTspliceany, ct, LEX$act);
    then try($u8feed, NESTidle(ct), cur);

    done;
}

ok64 LEXonRoot($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != nil);
    u8B ct = (u8B)state->ct;
    try(NESTspliceall, ct, LEX$mod);
    then try($u8feed, NESTidle(ct), state->mod);
    done;
}

pro(lex2rl, $u8c mod, $u8c lang) {
    sane($ok(mod));

    aBcpad(u8, name, KB);
    aBcpad(u8, lex, KB << 8);
    $u8c $namet = $u8str("$s.lex");
    $feedf(nameidle, $namet, mod);
    int fd;
    call(FILEopen, &fd, namedata, O_RDONLY);
    call(FILEdrainall, lexidle, fd);
    call(FILEclose, &fd);

    aBcpad(u8, ct, MB);
    NESTreset(ctbuf);
    int nlang = 0;
    if (!$empty(lang)) {
        while (nlang < LEX_TEMPL_LANG_LEN) {
            if ($eq(lang, LEX_TEMPL[nlang][LEX_TEMPL_L])) break;
            ++nlang;
        }
        if (nlang == LEX_TEMPL_LANG_LEN) fail(badarg);
    }

    LEXstate state = {
        .lang = nlang,
        .ct = (u8B)ctbuf,
        .mod = mod,
    };
    $mv(state.text, lexdata);

    call(NESTfeed, ctbuf, LEX_TEMPL[nlang][LEX_TEMPL_FILE]);

    aBcpad(u8, rl, MB);
    call(LEXlexer, &state);
    call(NESTrender, rlidle, ctbuf);

    aBcpad(u8, rlname, KB);
    $u8c $rnamet = $u8str("$s.$s.rl");
    $feedf(rlnameidle, $rnamet, mod, LEX_TEMPL[nlang][LEX_TEMPL_L]);
    int rfd;
    call(FILEcreate, &rfd, rlnamedata);
    call(FILEfeedall, rfd, rldata);
    call(FILEclose, &rfd);

    done;
}
