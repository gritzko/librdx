#include "MARK2.rl.h"


%%{

machine MARK2;

alphtype unsigned char;

action MARK2Ref00 { state->mark0[MARK2Ref0] = p - state->doc[0]; }
action MARK2Ref01 {
    tok[0] = state->doc[0]+state->mark0[MARK2Ref0];
    tok[1] = p;
    call(MARK2onRef0, tok, state); 
}
action MARK2Ref10 { state->mark0[MARK2Ref1] = p - state->doc[0]; }
action MARK2Ref11 {
    tok[0] = state->doc[0]+state->mark0[MARK2Ref1];
    tok[1] = p;
    call(MARK2onRef1, tok, state); 
}
action MARK2Em0 { state->mark0[MARK2Em] = p - state->doc[0]; }
action MARK2Em1 {
    tok[0] = state->doc[0]+state->mark0[MARK2Em];
    tok[1] = p;
    call(MARK2onEm, tok, state); 
}
action MARK2StA00 { state->mark0[MARK2StA0] = p - state->doc[0]; }
action MARK2StA01 {
    tok[0] = state->doc[0]+state->mark0[MARK2StA0];
    tok[1] = p;
    call(MARK2onStA0, tok, state); 
}
action MARK2StA10 { state->mark0[MARK2StA1] = p - state->doc[0]; }
action MARK2StA11 {
    tok[0] = state->doc[0]+state->mark0[MARK2StA1];
    tok[1] = p;
    call(MARK2onStA1, tok, state); 
}
action MARK2Root0 { state->mark0[MARK2Root] = p - state->doc[0]; }
action MARK2Root1 {
    tok[0] = state->doc[0]+state->mark0[MARK2Root];
    tok[1] = p;
    call(MARK2onRoot, tok, state); 
}

MARK2alpha  = (   [0-9A-Za-z] );

MARK2ws  = (   [ \t\n\r] );

MARK2any  = (   [0-0xff] );

MARK2nonws  = (   [^ \t\n\r] );

MARK2word  = (   MARK2nonws  + );

MARK2words  = (   MARK2word  (  MARK2ws+  MARK2word  )* );


MARK2Ref0  = (   MARK2ws  "["  MARK2nonws )  >MARK2Ref00 %MARK2Ref01;

MARK2Ref1  = (   MARK2nonws  "]["  MARK2alpha  "]" )  >MARK2Ref10 %MARK2Ref11;


MARK2emword  = (   ([^ \t\r\n_]|"\\_")+ );

MARK2Em  = (   "_"  MARK2emword  (MARK2ws+  MARK2emword)*  "_" )  >MARK2Em0 %MARK2Em1;


MARK2StA0  = (   MARK2ws  "*"  MARK2nonws )  >MARK2StA00 %MARK2StA01;

MARK2StA1  = (   [^\t\r\n *]  "*" )  >MARK2StA10 %MARK2StA11;


MARK2inline  = (   MARK2words  |  MARK2Em  |  MARK2StA0  |  MARK2StA1  |  MARK2Ref0  |  MARK2Ref1 );

MARK2Root  = (   (MARK2ws*  MARK2inline)*  MARK2ws* )  >MARK2Root0 %MARK2Root1;

main := MARK2Root;

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
