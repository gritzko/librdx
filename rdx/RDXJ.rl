#include "RDXJ.rl.h"


%%{

machine RDXJ;

alphtype unsigned char;

action RDXJInt0 { mark0[RDXJInt] = p - text[0]; }
action RDXJInt1 {
    tok[0] = text[0] + mark0[RDXJInt];
    tok[1] = p;
    call(RDXJonInt, tok, state); 
}
action RDXJFloat0 { mark0[RDXJFloat] = p - text[0]; }
action RDXJFloat1 {
    tok[0] = text[0] + mark0[RDXJFloat];
    tok[1] = p;
    call(RDXJonFloat, tok, state); 
}
action RDXJRef0 { mark0[RDXJRef] = p - text[0]; }
action RDXJRef1 {
    tok[0] = text[0] + mark0[RDXJRef];
    tok[1] = p;
    call(RDXJonRef, tok, state); 
}
action RDXJString0 { mark0[RDXJString] = p - text[0]; }
action RDXJString1 {
    tok[0] = text[0] + mark0[RDXJString];
    tok[1] = p;
    call(RDXJonString, tok, state); 
}
action RDXJTerm0 { mark0[RDXJTerm] = p - text[0]; }
action RDXJTerm1 {
    tok[0] = text[0] + mark0[RDXJTerm];
    tok[1] = p;
    call(RDXJonTerm, tok, state); 
}
action RDXJStamp0 { mark0[RDXJStamp] = p - text[0]; }
action RDXJStamp1 {
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
}
action RDXJOpenP0 { mark0[RDXJOpenP] = p - text[0]; }
action RDXJOpenP1 {
    tok[0] = text[0] + mark0[RDXJOpenP];
    tok[1] = p;
    call(RDXJonOpenP, tok, state); 
}
action RDXJCloseP0 { mark0[RDXJCloseP] = p - text[0]; }
action RDXJCloseP1 {
    tok[0] = text[0] + mark0[RDXJCloseP];
    tok[1] = p;
    call(RDXJonCloseP, tok, state); 
}
action RDXJOpenL0 { mark0[RDXJOpenL] = p - text[0]; }
action RDXJOpenL1 {
    tok[0] = text[0] + mark0[RDXJOpenL];
    tok[1] = p;
    call(RDXJonOpenL, tok, state); 
}
action RDXJCloseL0 { mark0[RDXJCloseL] = p - text[0]; }
action RDXJCloseL1 {
    tok[0] = text[0] + mark0[RDXJCloseL];
    tok[1] = p;
    call(RDXJonCloseL, tok, state); 
}
action RDXJOpenE0 { mark0[RDXJOpenE] = p - text[0]; }
action RDXJOpenE1 {
    tok[0] = text[0] + mark0[RDXJOpenE];
    tok[1] = p;
    call(RDXJonOpenE, tok, state); 
}
action RDXJCloseE0 { mark0[RDXJCloseE] = p - text[0]; }
action RDXJCloseE1 {
    tok[0] = text[0] + mark0[RDXJCloseE];
    tok[1] = p;
    call(RDXJonCloseE, tok, state); 
}
action RDXJOpenX0 { mark0[RDXJOpenX] = p - text[0]; }
action RDXJOpenX1 {
    tok[0] = text[0] + mark0[RDXJOpenX];
    tok[1] = p;
    call(RDXJonOpenX, tok, state); 
}
action RDXJCloseX0 { mark0[RDXJCloseX] = p - text[0]; }
action RDXJCloseX1 {
    tok[0] = text[0] + mark0[RDXJCloseX];
    tok[1] = p;
    call(RDXJonCloseX, tok, state); 
}
action RDXJComma0 { mark0[RDXJComma] = p - text[0]; }
action RDXJComma1 {
    tok[0] = text[0] + mark0[RDXJComma];
    tok[1] = p;
    call(RDXJonComma, tok, state); 
}
action RDXJColon0 { mark0[RDXJColon] = p - text[0]; }
action RDXJColon1 {
    tok[0] = text[0] + mark0[RDXJColon];
    tok[1] = p;
    call(RDXJonColon, tok, state); 
}
action RDXJOpen0 { mark0[RDXJOpen] = p - text[0]; }
action RDXJOpen1 {
    tok[0] = text[0] + mark0[RDXJOpen];
    tok[1] = p;
    call(RDXJonOpen, tok, state); 
}
action RDXJClose0 { mark0[RDXJClose] = p - text[0]; }
action RDXJClose1 {
    tok[0] = text[0] + mark0[RDXJClose];
    tok[1] = p;
    call(RDXJonClose, tok, state); 
}
action RDXJInter0 { mark0[RDXJInter] = p - text[0]; }
action RDXJInter1 {
    tok[0] = text[0] + mark0[RDXJInter];
    tok[1] = p;
    call(RDXJonInter, tok, state); 
}
action RDXJFIRST0 { mark0[RDXJFIRST] = p - text[0]; }
action RDXJFIRST1 {
    tok[0] = text[0] + mark0[RDXJFIRST];
    tok[1] = p;
    call(RDXJonFIRST, tok, state); 
}
action RDXJRoot0 { mark0[RDXJRoot] = p - text[0]; }
action RDXJRoot1 {
    tok[0] = text[0] + mark0[RDXJRoot];
    tok[1] = p;
    call(RDXJonRoot, tok, state); 
}

RDXJws  = (   [\r\n\t ] );

RDXJhex  = (   [0-9a-fA-F] );


RDXJcp  = (   (0x20..0xff)  -  ["\\] );

RDXJutf8  = (   RDXJcp+ );

RDXJesc  = (   [\\]  ["\\/bfnrt] );

RDXJhexEsc  = (     "\\u"  RDXJhex{4} );

RDXJutf8esc  = (   RDXJutf8  |  RDXJesc  |  RDXJhexEsc );


RDXJid128  = (   [0-9a-fA-F]+  ("-"  [0-9a-fA-F]+)? );


RDXJInt  = (   [\-]?  (  [0]  |  [1-9]  [0-9]*  ) )  >RDXJInt0 %RDXJInt1;

RDXJFloat  = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  )

                        ("."  [0-9]+)?

                        ([eE]  [\-+]?  [0-9]+  )?  )  -  RDXJInt )  >RDXJFloat0 %RDXJFloat1;

RDXJRef  = (   RDXJid128  -RDXJFloat  -RDXJInt )  >RDXJRef0 %RDXJRef1;

RDXJString  = (   ["]  RDXJutf8esc*  ["] )  >RDXJString0 %RDXJString1;

RDXJTerm  = (   [a-zA-Z0-9_~]+  -RDXJInt  -RDXJFloat )  >RDXJTerm0 %RDXJTerm1;


RDXJStamp  = (   "@"  RDXJid128 )  >RDXJStamp0 %RDXJStamp1;


RDXJOpenP  = (   "<" )  >RDXJOpenP0 %RDXJOpenP1;

RDXJCloseP  = (   ">" )  >RDXJCloseP0 %RDXJCloseP1;

RDXJOpenL  = (   "[" )  >RDXJOpenL0 %RDXJOpenL1;

RDXJCloseL  = (   "]" )  >RDXJCloseL0 %RDXJCloseL1;

RDXJOpenE  = (   "{" )  >RDXJOpenE0 %RDXJOpenE1;

RDXJCloseE  = (   "}" )  >RDXJCloseE0 %RDXJCloseE1;

RDXJOpenX  = (   "(" )  >RDXJOpenX0 %RDXJOpenX1;

RDXJCloseX  = (   ")" )  >RDXJCloseX0 %RDXJCloseX1;


RDXJComma  = (   "," )  >RDXJComma0 %RDXJComma1;

RDXJColon  = (   ":" )  >RDXJColon0 %RDXJColon1;


RDXJOpen  = (   (RDXJOpenP  |  RDXJOpenL  |  RDXJOpenE  |  RDXJOpenX)  RDXJws*  (RDXJStamp  RDXJws*)? )  >RDXJOpen0 %RDXJOpen1;

RDXJClose  = (   (RDXJCloseP  |  RDXJCloseL  |  RDXJCloseE  |  RDXJCloseX)  RDXJws* )  >RDXJClose0 %RDXJClose1;

RDXJInter  = (   (RDXJComma  |  RDXJColon)  RDXJws* )  >RDXJInter0 %RDXJInter1;


RDXJdelim  = (   RDXJOpen  |  RDXJClose  |  RDXJInter );


RDXJFIRST  = (   (  RDXJFloat  |  RDXJInt  |  RDXJRef  |  RDXJString  |  RDXJTerm  )  (RDXJws*  RDXJStamp)?  RDXJws* )  >RDXJFIRST0 %RDXJFIRST1;


RDXJRoot  = (   RDXJws*  (  RDXJFIRST?  (  RDXJdelim  <:  RDXJFIRST?  )*  )   )  >RDXJRoot0 %RDXJRoot1;

main := RDXJRoot;

}%%

%%write data;

pro(RDXJlexer, RDXJstate* state) {

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

    if (p!=text[1] || cs < RDXJ_first_final) {
        fail(RDXJfail);
    }
    nedo(state->text[0] = p;);
}
