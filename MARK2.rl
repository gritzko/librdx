#include "PRO.h"
#include "MARK2.h"

enum {
	MARK2 = 0,
	MARK2plain = MARK2+5,
	MARK2ref = MARK2+6,
	MARK2tostress = MARK2+7,
	MARK2stress = MARK2+8,
	MARK2toemph = MARK2+9,
	MARK2emph = MARK2+10,
	MARK2inline = MARK2+11,
	MARK2root = MARK2+12,
};

#define MARK2maxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : MARK2fail;
}

#define lexpush(t) { \
    if (sp>=MARK2maxnest) fail(MARK2fail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _MARK2plain ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2ref ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2tostress ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2stress ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2toemph ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2emph ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2inline ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2root ($cu8c text, $cu8c tok, MARK2state* state);


%%{

machine MARK2;

alphtype unsigned char;

action MARK2plain0 { lexpush(MARK2plain); }
action MARK2plain1 { lexpop(MARK2plain); call(_MARK2plain, text, tok, state); }
action MARK2ref0 { lexpush(MARK2ref); }
action MARK2ref1 { lexpop(MARK2ref); call(_MARK2ref, text, tok, state); }
action MARK2tostress0 { lexpush(MARK2tostress); }
action MARK2tostress1 { lexpop(MARK2tostress); call(_MARK2tostress, text, tok, state); }
action MARK2stress0 { lexpush(MARK2stress); }
action MARK2stress1 { lexpop(MARK2stress); call(_MARK2stress, text, tok, state); }
action MARK2toemph0 { lexpush(MARK2toemph); }
action MARK2toemph1 { lexpop(MARK2toemph); call(_MARK2toemph, text, tok, state); }
action MARK2emph0 { lexpush(MARK2emph); }
action MARK2emph1 { lexpop(MARK2emph); call(_MARK2emph, text, tok, state); }
action MARK2inline0 { lexpush(MARK2inline); }
action MARK2inline1 { lexpop(MARK2inline); call(_MARK2inline, text, tok, state); }
action MARK2root0 { lexpush(MARK2root); }
action MARK2root1 { lexpop(MARK2root); call(_MARK2root, text, tok, state); }

MARK2_a  = (   [0-9A-Za-z]
 );
MARK2_sp  = (   [ \t]
 );
MARK2_nl  = (   "\n"
 );
MARK2_nonl  = (   [^\n]
 );
MARK2plain  = (   [^\n\r \t]+
 ) >MARK2plain0 %MARK2plain1;
MARK2ref  = (   "["  MARK2plain  "]["  MARK2_a  "]"
 ) >MARK2ref0 %MARK2ref1;
MARK2tostress  = (   MARK2plain  :>>  MARK2ref
 ) >MARK2tostress0 %MARK2tostress1;
MARK2stress  = (   "*"  MARK2tostress  (MARK2_sp+  MARK2tostress)*  "*"
 ) >MARK2stress0 %MARK2stress1;
MARK2toemph  = (   MARK2plain  :>>  (MARK2stress  |  MARK2ref)
 ) >MARK2toemph0 %MARK2toemph1;
MARK2emph  = (   "_"  MARK2toemph  (MARK2_sp+  MARK2toemph*)  "_"
 ) >MARK2emph0 %MARK2emph1;
MARK2inline  = (   MARK2plain  :>>  (MARK2ref)

 ) >MARK2inline0 %MARK2inline1;
MARK2root  = (   MARK2inline*
 ) >MARK2root0 %MARK2root1;
main := MARK2root;

}%%

%%write data;

pro(MARK2lexer, MARK2state* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[MARK2maxnest] = {0, MARK2};
    u32 sp = 2;
    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    test(p==text[1], MARK2fail);

    if (state->tbc) {
        test(cs != MARK2_error, MARK2fail);
        state->cs = cs;
    } else {
        test(cs >= MARK2_first_final, MARK2fail);
    }

    nedo(
        state->text[0] = p;
    );
}


