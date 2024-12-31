#include "JDR.rl.h"


%%{

machine JDR;

alphtype unsigned char;

action JDRInt0 { mark0[JDRInt] = p - text[0]; }
action JDRInt1 {
    tok[0] = text[0] + mark0[JDRInt];
    tok[1] = p;
    call(JDRonInt, tok, state); 
}
action JDRFloat0 { mark0[JDRFloat] = p - text[0]; }
action JDRFloat1 {
    tok[0] = text[0] + mark0[JDRFloat];
    tok[1] = p;
    call(JDRonFloat, tok, state); 
}
action JDRRef0 { mark0[JDRRef] = p - text[0]; }
action JDRRef1 {
    tok[0] = text[0] + mark0[JDRRef];
    tok[1] = p;
    call(JDRonRef, tok, state); 
}
action JDRString0 { mark0[JDRString] = p - text[0]; }
action JDRString1 {
    tok[0] = text[0] + mark0[JDRString];
    tok[1] = p;
    call(JDRonString, tok, state); 
}
action JDRTerm0 { mark0[JDRTerm] = p - text[0]; }
action JDRTerm1 {
    tok[0] = text[0] + mark0[JDRTerm];
    tok[1] = p;
    call(JDRonTerm, tok, state); 
}
action JDRStamp0 { mark0[JDRStamp] = p - text[0]; }
action JDRStamp1 {
    tok[0] = text[0] + mark0[JDRStamp];
    tok[1] = p;
    call(JDRonStamp, tok, state); 
}
action JDROpenP0 { mark0[JDROpenP] = p - text[0]; }
action JDROpenP1 {
    tok[0] = text[0] + mark0[JDROpenP];
    tok[1] = p;
    call(JDRonOpenP, tok, state); 
}
action JDRCloseP0 { mark0[JDRCloseP] = p - text[0]; }
action JDRCloseP1 {
    tok[0] = text[0] + mark0[JDRCloseP];
    tok[1] = p;
    call(JDRonCloseP, tok, state); 
}
action JDROpenL0 { mark0[JDROpenL] = p - text[0]; }
action JDROpenL1 {
    tok[0] = text[0] + mark0[JDROpenL];
    tok[1] = p;
    call(JDRonOpenL, tok, state); 
}
action JDRCloseL0 { mark0[JDRCloseL] = p - text[0]; }
action JDRCloseL1 {
    tok[0] = text[0] + mark0[JDRCloseL];
    tok[1] = p;
    call(JDRonCloseL, tok, state); 
}
action JDROpenE0 { mark0[JDROpenE] = p - text[0]; }
action JDROpenE1 {
    tok[0] = text[0] + mark0[JDROpenE];
    tok[1] = p;
    call(JDRonOpenE, tok, state); 
}
action JDRCloseE0 { mark0[JDRCloseE] = p - text[0]; }
action JDRCloseE1 {
    tok[0] = text[0] + mark0[JDRCloseE];
    tok[1] = p;
    call(JDRonCloseE, tok, state); 
}
action JDROpenX0 { mark0[JDROpenX] = p - text[0]; }
action JDROpenX1 {
    tok[0] = text[0] + mark0[JDROpenX];
    tok[1] = p;
    call(JDRonOpenX, tok, state); 
}
action JDRCloseX0 { mark0[JDRCloseX] = p - text[0]; }
action JDRCloseX1 {
    tok[0] = text[0] + mark0[JDRCloseX];
    tok[1] = p;
    call(JDRonCloseX, tok, state); 
}
action JDRComma0 { mark0[JDRComma] = p - text[0]; }
action JDRComma1 {
    tok[0] = text[0] + mark0[JDRComma];
    tok[1] = p;
    call(JDRonComma, tok, state); 
}
action JDRColon0 { mark0[JDRColon] = p - text[0]; }
action JDRColon1 {
    tok[0] = text[0] + mark0[JDRColon];
    tok[1] = p;
    call(JDRonColon, tok, state); 
}
action JDROpen0 { mark0[JDROpen] = p - text[0]; }
action JDROpen1 {
    tok[0] = text[0] + mark0[JDROpen];
    tok[1] = p;
    call(JDRonOpen, tok, state); 
}
action JDRClose0 { mark0[JDRClose] = p - text[0]; }
action JDRClose1 {
    tok[0] = text[0] + mark0[JDRClose];
    tok[1] = p;
    call(JDRonClose, tok, state); 
}
action JDRInter0 { mark0[JDRInter] = p - text[0]; }
action JDRInter1 {
    tok[0] = text[0] + mark0[JDRInter];
    tok[1] = p;
    call(JDRonInter, tok, state); 
}
action JDRFIRST0 { mark0[JDRFIRST] = p - text[0]; }
action JDRFIRST1 {
    tok[0] = text[0] + mark0[JDRFIRST];
    tok[1] = p;
    call(JDRonFIRST, tok, state); 
}
action JDRRoot0 { mark0[JDRRoot] = p - text[0]; }
action JDRRoot1 {
    tok[0] = text[0] + mark0[JDRRoot];
    tok[1] = p;
    call(JDRonRoot, tok, state); 
}

JDRws  = (   [\r\n\t ] );

JDRhex  = (   [0-9a-fA-F] );


JDRcp  = (   (0x20..0xff)  -  ["\\] );

JDRutf8  = (   JDRcp+ );

JDResc  = (   [\\]  ["\\/bfnrt] );

JDRhexEsc  = (     "\\u"  JDRhex{4} );

JDRutf8esc  = (   JDRutf8  |  JDResc  |  JDRhexEsc );


JDRid128  = (   [0-9a-fA-F]+  ("-"  [0-9a-fA-F]+)? );


JDRInt  = (   [\-]?  (  [0]  |  [1-9]  [0-9]*  ) )  >JDRInt0 %JDRInt1;

JDRFloat  = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  )

                        ("."  [0-9]+)?

                        ([eE]  [\-+]?  [0-9]+  )?  )  -  JDRInt )  >JDRFloat0 %JDRFloat1;

JDRRef  = (   JDRid128  -JDRFloat  -JDRInt )  >JDRRef0 %JDRRef1;

JDRString  = (   ["]  JDRutf8esc*  ["] )  >JDRString0 %JDRString1;

JDRTerm  = (   [a-zA-Z0-9_~]+  -JDRInt  -JDRFloat )  >JDRTerm0 %JDRTerm1;


JDRStamp  = (   "@"  JDRid128 )  >JDRStamp0 %JDRStamp1;


JDROpenP  = (   "<" )  >JDROpenP0 %JDROpenP1;

JDRCloseP  = (   ">" )  >JDRCloseP0 %JDRCloseP1;

JDROpenL  = (   "[" )  >JDROpenL0 %JDROpenL1;

JDRCloseL  = (   "]" )  >JDRCloseL0 %JDRCloseL1;

JDROpenE  = (   "{" )  >JDROpenE0 %JDROpenE1;

JDRCloseE  = (   "}" )  >JDRCloseE0 %JDRCloseE1;

JDROpenX  = (   "(" )  >JDROpenX0 %JDROpenX1;

JDRCloseX  = (   ")" )  >JDRCloseX0 %JDRCloseX1;


JDRComma  = (   "," )  >JDRComma0 %JDRComma1;

JDRColon  = (   ":" )  >JDRColon0 %JDRColon1;


JDROpen  = (   (JDROpenP  |  JDROpenL  |  JDROpenE  |  JDROpenX)  JDRws*  (JDRStamp  JDRws*)? )  >JDROpen0 %JDROpen1;

JDRClose  = (   (JDRCloseP  |  JDRCloseL  |  JDRCloseE  |  JDRCloseX)  JDRws* )  >JDRClose0 %JDRClose1;

JDRInter  = (   (JDRComma  |  JDRColon)  JDRws* )  >JDRInter0 %JDRInter1;


JDRdelim  = (   JDROpen  |  JDRClose  |  JDRInter );


JDRFIRST  = (   (  JDRFloat  |  JDRInt  |  JDRRef  |  JDRString  |  JDRTerm  )  (JDRws*  JDRStamp)?  JDRws* )  >JDRFIRST0 %JDRFIRST1;


JDRRoot  = (   JDRws*  (  JDRFIRST?  (  JDRdelim  <:  JDRFIRST?  )*  )   )  >JDRRoot0 %JDRRoot1;

main := JDRRoot;

}%%

%%write data;

pro(JDRlexer, JDRstate* state) {

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

    state->text[0] = p;
    if (p!=text[1] || cs < JDR_first_final) {
        return JDRfail;
    }
    done;
}
