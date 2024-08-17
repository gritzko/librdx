#include "LEX.rl.h"


%%{

machine LEX;

alphtype unsigned char;

action LEXSpace0 { state->mark0[LEXSpace] = p - state->doc[0]; }
action LEXSpace1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXSpace], p};
    call(LEXonSpace, tok, state); 
}
action LEXName0 { state->mark0[LEXName] = p - state->doc[0]; }
action LEXName1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXName], p};
    call(LEXonName, tok, state); 
}
action LEXRep0 { state->mark0[LEXRep] = p - state->doc[0]; }
action LEXRep1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXRep], p};
    call(LEXonRep, tok, state); 
}
action LEXOp0 { state->mark0[LEXOp] = p - state->doc[0]; }
action LEXOp1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXOp], p};
    call(LEXonOp, tok, state); 
}
action LEXClass0 { state->mark0[LEXClass] = p - state->doc[0]; }
action LEXClass1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXClass], p};
    call(LEXonClass, tok, state); 
}
action LEXString0 { state->mark0[LEXString] = p - state->doc[0]; }
action LEXString1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXString], p};
    call(LEXonString, tok, state); 
}
action LEXEntity0 { state->mark0[LEXEntity] = p - state->doc[0]; }
action LEXEntity1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXEntity], p};
    call(LEXonEntity, tok, state); 
}
action LEXExpr0 { state->mark0[LEXExpr] = p - state->doc[0]; }
action LEXExpr1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXExpr], p};
    call(LEXonExpr, tok, state); 
}
action LEXRuleName0 { state->mark0[LEXRuleName] = p - state->doc[0]; }
action LEXRuleName1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXRuleName], p};
    call(LEXonRuleName, tok, state); 
}
action LEXEq0 { state->mark0[LEXEq] = p - state->doc[0]; }
action LEXEq1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXEq], p};
    call(LEXonEq, tok, state); 
}
action LEXLine0 { state->mark0[LEXLine] = p - state->doc[0]; }
action LEXLine1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXLine], p};
    call(LEXonLine, tok, state); 
}
action LEXRoot0 { state->mark0[LEXRoot] = p - state->doc[0]; }
action LEXRoot1 {
    $cu8c tok = {state->doc[0]+state->mark0[LEXRoot], p};
    call(LEXonRoot, tok, state); 
}

LEXSpace  = (   [ \t\r\n]   )  >LEXSpace0 %LEXSpace1;

LEXName  = (   [A-Za-z_]  [A-Z0-9a-z_]**   )  >LEXName0 %LEXName1;

LEXRep  = (   "{"  [0-9]*  (","  [0-9]*)?  "}"  -  "{}" )  >LEXRep0 %LEXRep1;

LEXOp  = (   LEXSpace  |  [()+*\-?><:|]  |  LEXRep )  >LEXOp0 %LEXOp1;

LEXClass  = (   "["  ([^\]]|"\\]")*  "]"   )  >LEXClass0 %LEXClass1;

LEXString  = (   "\""  ([^"]|"\\\"")*  "\""   )  >LEXString0 %LEXString1;

LEXEntity  = (   LEXClass  |  LEXName  |  LEXString   )  >LEXEntity0 %LEXEntity1;

LEXExpr  = (   (LEXOp+  LEXEntity)*  LEXOp*   )  >LEXExpr0 %LEXExpr1;

LEXRuleName  = (   LEXName   )  >LEXRuleName0 %LEXRuleName1;

LEXEq  = (   LEXSpace*  "=" )  >LEXEq0 %LEXEq1;

LEXLine  = (   LEXSpace*  LEXRuleName  LEXEq  LEXExpr  ";"   )  >LEXLine0 %LEXLine1;

LEXRoot  = (   LEXLine*  LEXSpace*   )  >LEXRoot0 %LEXRoot1;


main := LEXRoot;

}%%

%%write data;

pro(LEXlexer, LEXstate* state) {
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

    test(p==text[1], LEXfail);

    if (state->tbc) {
        test(cs != LEX_error, LEXfail);
        state->cs = cs;
    } else {
        test(cs >= LEX_first_final, LEXfail);
    }

    nedo(
        state->text[0] = p;
    );
}
