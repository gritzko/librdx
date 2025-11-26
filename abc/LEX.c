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
        $u8str("action $mod${act}0 { mark0[$mod$act] = p - data[0]; }\n"
               "action $mod${act}1 {\n"
               "    tok[0] = data[0] + mark0[$mod$act];\n"
               "    tok[1] = p;\n"
               "    o = ${mod}on$act(tok, state); \n"
               "    if (o!=OK) {\n"
               "        fbreak;\n"
               "    }\n"
               "}\n"),
        $u8str("\t$mod$act = ${mod}enum+$actno,\n"),
        $u8str("ok64 ${mod}on$act (utf8cs tok, ${mod}state* state);\n"),
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
               "ok64 ${mod}lexer(${mod}state* state) {\n"
               "\n"
               "    a_dup(u8c, data, state->data);\n"
               "    sane($$ok(data));\n"
               "\n"
               "    int cs = 0;\n"
               "    u8c *p = (u8c*) data[0];\n"
               "    u8c *pe = (u8c*) data[1];\n"
               "    u8c *eof = pe;\n"
               "    u64 mark0[64] = {};\n"
               "    ok64 o = OK;\n"
               "\n"
               "    utf8cs tok = {p, p};\n"
               "\n"
               "    %% write init;\n"
               "    %% write exec;\n"
               "\n"
               "    state->data[0] = p;\n"
               "    if (o==OK && cs < ${mod}_first_final) \n"
               "        o = ${mod}bad;\n"
               "    \n"
               "    return o;\n"
               "}\n"),
        $u8str("c"),
    },
    {
        $u8str("action $mod${act}0 { mark0[$mod$act] = p; }\n"
               "action $mod${act}1 {\n"
               "    err = ${mod}on$act(data[mark0[$mod$act] : p], state); \n"
               "    if err!=NULL {\n"
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
               "    data := state.data\n"
               "    var mark0 [64]int\n"
               "    cs, p, pe, eof := 0, 0, len(data), len(data)\n"
               "\n"
               "    %% write init;\n"
               "    %% write exec;\n"
               "\n"
               "    if (p<len(data) || cs < ${mod}_first_final) {\n"
               "        state.data = state.data[p:];\n"
               "        return errors.New(\"${mod} bad syntax\")\n"
               "    }\n"
               "    return NULL;\n"
               "}\n"),
        $u8str("go"),
    },
};

con ok64 LEX$ACTIONS = 0xa31d4985dc;
con ok64 LEX$ENUM = 0x397796;
con ok64 LEX$FN = 0x3d7;
con ok64 LEX$RULES = 0x1b79539c;
con ok64 LEX$mod = 0x31ce8;
con ok64 LEX$act = 0x259f8;
con ok64 LEX$actno = 0x259f8cb3;

ok64 LEXonName($cu8c tok, LEXstate *state) {
    ok64 o = u8sFeed(NESTidle(state->ct), state->mod);
    if (o == OK) o = u8sFeed(NESTidle(state->ct), tok);
    return o;
}
ok64 LEXonOp($cu8c tok, LEXstate *state) {
    return u8sFeed(NESTidle(state->ct), tok);
}
ok64 LEXonClass($cu8c tok, LEXstate *state) {
    return u8sFeed(NESTidle(state->ct), tok);
}
ok64 LEXonRange($cu8c tok, LEXstate *state) {
    return u8sFeed(NESTidle(state->ct), tok);
}
ok64 LEXonString($cu8c tok, LEXstate *state) {
    return u8sFeed(NESTidle(state->ct), tok);
}
ok64 LEXonQString($cu8c tok, LEXstate *state) {
    return u8sFeed(NESTidle(state->ct), tok);
}
ok64 LEXonSpace($cu8c tok, LEXstate *state) {
    // return u8sFeed(NESTidle(state->ct), tok);
    return u8sFeed1(NESTidle(state->ct), ' ');
}

ok64 LEXonEntity($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonExpr($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonRep($cu8c tok, LEXstate *state) { return OK; }

ok64 LEXonEq($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != NULL);
    u8c$ tmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACT];
    try(NESTSplice, state->ct, LEX$RULES);
    then try(NESTFeed, state->ct, tmpl);
    done;
}

pro(LEXonRuleName, $cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != NULL);
    $set(state->cur, tok);
    state->ruleno++;

    if (**tok < 'A' || **tok > 'Z') done;

    u8bp ct = (u8bp)state->ct;

    u8c$ tmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACTION];
    call(NESTSplice, ct, LEX$ACTIONS);
    call(NESTFeed, ct, tmpl);

    u8c$ enmtmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ENUM];
    call(NESTSplice, ct, LEX$ENUM);
    call(NESTFeed, ct, enmtmpl);
    call(NESTSplice, ct, LEX$actno);
    call(u64decfeed, NESTidle(ct), state->ruleno);

    u8c$ fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_FN];
    call(NESTSplice, ct, LEX$FN);
    call(NESTFeed, ct, fntmpl);

    done;
}

ok64 LEXonLine($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != NULL);
    u8bp ct = (u8bp)state->ct;
    u8c$ cur = state->cur;

    u8c$ fntmpl;
    if (**cur >= 'A' && **cur <= 'Z') {
        fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_PUBACTNL];
    } else {
        fntmpl = LEX_TEMPL[state->lang][LEX_TEMPL_ACTNL];
    }

    try(NESTFeed, ct, fntmpl);
    then try(NESTSpliceAny, ct, LEX$act);
    then try(u8sFeed, NESTidle(ct), cur);

    done;
}

ok64 LEXonRoot($cu8c tok, LEXstate *state) {
    sane($ok(tok) && state != NULL);
    u8bp ct = (u8bp)state->ct;
    try(NESTSpliceAll, ct, LEX$mod);
    then try(u8sFeed, NESTidle(ct), state->mod);
    done;
}

pro(lex2rl, u8cs mod, $u8c lang) {
    sane($ok(mod));

    a_pad(u8, name, KB);
    a_pad(u8, lex, KB << 8);
    u8cs $namet = $u8str("$s.lex");
    $feedf(name_idle, $namet, mod);
    int fd;
    call(FILEOpen, &fd, name, O_RDONLY);
    call(FILEdrainall, lex_idle, fd);
    call(FILEClose, &fd);

    a_pad(u8, ct, MB);
    NESTreset(ct);
    int nlang = 0;
    if (!$empty(lang)) {
        while (nlang < LEX_TEMPL_LANG_LEN) {
            if ($eq(lang, LEX_TEMPL[nlang][LEX_TEMPL_L])) break;
            ++nlang;
        }
        if (nlang == LEX_TEMPL_LANG_LEN) fail(BADARG);
    }

    LEXstate state = {
        .lang = nlang,
        .ct = (u8bp)ct,
        .mod = mod,
    };
    $mv(state.data, lex_data);

    call(NESTFeed, ct, LEX_TEMPL[nlang][LEX_TEMPL_FILE]);

    a_pad(u8, rl, MB);
    call(LEXlexer, &state);
    call(NESTRender, rl_idle, ct);

    a_pad(u8, rlname, KB);
    u8cs $rnamet = $u8str("$s.$s.rl");
    $feedf(rlname_idle, $rnamet, mod, LEX_TEMPL[nlang][LEX_TEMPL_L]);
    int rfd;
    call(FILECreate, &rfd, rlname);
    call(FILEFeedall, rfd, rl_datac);
    call(FILEClose, &rfd);

    done;
}
