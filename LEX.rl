#include "LEX.rl.h"


%%{

machine LEX;

alphtype unsigned char;

action LEXSpace0 { mark0[LEXSpace] = p - text[0]; }
action LEXSpace1 {
    tok[0] = text[0] + mark0[LEXSpace];
    tok[1] = p;
    call(LEXonSpace, tok, state); 
}
action LEXName0 { mark0[LEXName] = p - text[0]; }
action LEXName1 {
    tok[0] = text[0] + mark0[LEXName];
    tok[1] = p;
    call(LEXonName, tok, state); 
}
action LEXRep0 { mark0[LEXRep] = p - text[0]; }
action LEXRep1 {
    tok[0] = text[0] + mark0[LEXRep];
    tok[1] = p;
    call(LEXonRep, tok, state); 
}
action LEXOp0 { mark0[LEXOp] = p - text[0]; }
action LEXOp1 {
    tok[0] = text[0] + mark0[LEXOp];
    tok[1] = p;
    call(LEXonOp, tok, state); 
}
action LEXClass0 { mark0[LEXClass] = p - text[0]; }
action LEXClass1 {
    tok[0] = text[0] + mark0[LEXClass];
    tok[1] = p;
    call(LEXonClass, tok, state); 
}
action LEXString0 { mark0[LEXString] = p - text[0]; }
action LEXString1 {
    tok[0] = text[0] + mark0[LEXString];
    tok[1] = p;
    call(LEXonString, tok, state); 
}
action LEXEntity0 { mark0[LEXEntity] = p - text[0]; }
action LEXEntity1 {
    tok[0] = text[0] + mark0[LEXEntity];
    tok[1] = p;
    call(LEXonEntity, tok, state); 
}
action LEXExpr0 { mark0[LEXExpr] = p - text[0]; }
action LEXExpr1 {
    tok[0] = text[0] + mark0[LEXExpr];
    tok[1] = p;
    call(LEXonExpr, tok, state); 
}
action LEXRuleName0 { mark0[LEXRuleName] = p - text[0]; }
action LEXRuleName1 {
    tok[0] = text[0] + mark0[LEXRuleName];
    tok[1] = p;
    call(LEXonRuleName, tok, state); 
}
action LEXEq0 { mark0[LEXEq] = p - text[0]; }
action LEXEq1 {
    tok[0] = text[0] + mark0[LEXEq];
    tok[1] = p;
    call(LEXonEq, tok, state); 
}
action LEXLine0 { mark0[LEXLine] = p - text[0]; }
action LEXLine1 {
    tok[0] = text[0] + mark0[LEXLine];
    tok[1] = p;
    call(LEXonLine, tok, state); 
}
action LEXRoot0 { mark0[LEXRoot] = p - text[0]; }
action LEXRoot1 {
    tok[0] = text[0] + mark0[LEXRoot];
    tok[1] = p;
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

    int cs = 0;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u8c *pb = p;
    u64 mark0[64] = {};

    u32 sp = 2;
    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    test(p==text[1], LEXfail);

    test(cs >= LEX_first_final, LEXfail);

    nedo(
        text[0] = p;
    );
}
