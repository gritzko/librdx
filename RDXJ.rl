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
action RDXJOpenObject0 { mark0[RDXJOpenObject] = p - text[0]; }
action RDXJOpenObject1 {
    tok[0] = text[0] + mark0[RDXJOpenObject];
    tok[1] = p;
    call(RDXJonOpenObject, tok, state); 
}
action RDXJCloseObject0 { mark0[RDXJCloseObject] = p - text[0]; }
action RDXJCloseObject1 {
    tok[0] = text[0] + mark0[RDXJCloseObject];
    tok[1] = p;
    call(RDXJonCloseObject, tok, state); 
}
action RDXJOpenArray0 { mark0[RDXJOpenArray] = p - text[0]; }
action RDXJOpenArray1 {
    tok[0] = text[0] + mark0[RDXJOpenArray];
    tok[1] = p;
    call(RDXJonOpenArray, tok, state); 
}
action RDXJCloseArray0 { mark0[RDXJCloseArray] = p - text[0]; }
action RDXJCloseArray1 {
    tok[0] = text[0] + mark0[RDXJCloseArray];
    tok[1] = p;
    call(RDXJonCloseArray, tok, state); 
}
action RDXJOpenVector0 { mark0[RDXJOpenVector] = p - text[0]; }
action RDXJOpenVector1 {
    tok[0] = text[0] + mark0[RDXJOpenVector];
    tok[1] = p;
    call(RDXJonOpenVector, tok, state); 
}
action RDXJCloseVector0 { mark0[RDXJCloseVector] = p - text[0]; }
action RDXJCloseVector1 {
    tok[0] = text[0] + mark0[RDXJCloseVector];
    tok[1] = p;
    call(RDXJonCloseVector, tok, state); 
}
action RDXJStamp0 { mark0[RDXJStamp] = p - text[0]; }
action RDXJStamp1 {
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
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


RDXJid128  = (   [0-9a-fA-F]+  "-"  [0-9a-fA-F]+ );


RDXJInt  = (   [\-]?  (  [0]  |  [1-9]  [0-9]*  ) )  >RDXJInt0 %RDXJInt1;

RDXJFloat  = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  )

                        ("."  [0-9]+)?

                        ([eE]  [\-+]?  [0-9]+  )?  )  -  RDXJInt )  >RDXJFloat0 %RDXJFloat1;

RDXJRef  = (   RDXJid128  -  RDXJFloat )  >RDXJRef0 %RDXJRef1;

RDXJString  = (   ["]  RDXJutf8esc*  ["] )  >RDXJString0 %RDXJString1;

RDXJTerm  = (   [a-zA-Z]  [a-zA-Z0-9_]* )  >RDXJTerm0 %RDXJTerm1;


RDXJOpenObject  = (   "{" )  >RDXJOpenObject0 %RDXJOpenObject1;

RDXJCloseObject  = (   "}" )  >RDXJCloseObject0 %RDXJCloseObject1;

RDXJOpenArray  = (   "[" )  >RDXJOpenArray0 %RDXJOpenArray1;

RDXJCloseArray  = (   "]" )  >RDXJCloseArray0 %RDXJCloseArray1;

RDXJOpenVector  = (   "(" )  >RDXJOpenVector0 %RDXJOpenVector1;

RDXJCloseVector  = (   ")" )  >RDXJCloseVector0 %RDXJCloseVector1;


RDXJStamp  = (   "@"  RDXJid128 )  >RDXJStamp0 %RDXJStamp1;


RDXJComma  = (   "," )  >RDXJComma0 %RDXJComma1;

RDXJColon  = (   ":" )  >RDXJColon0 %RDXJColon1;


RDXJdelimiter  = (   RDXJOpenObject  |  RDXJCloseObject  |

                        RDXJOpenArray  |  RDXJCloseArray  |

                        RDXJOpenVector  |  RDXJCloseVector  |

                        RDXJComma  |  RDXJColon );


RDXJFIRST  = (   (  RDXJFloat  |  RDXJInt  |  RDXJRef  |  RDXJString  |  RDXJTerm  )  (RDXJws*  RDXJStamp)?  RDXJws* )  >RDXJFIRST0 %RDXJFIRST1;


RDXJRoot  = (   RDXJws*  (  RDXJFIRST?  (  RDXJdelimiter  RDXJws*  RDXJFIRST?  )*  )   )  >RDXJRoot0 %RDXJRoot1;

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
        state->text[0] = p;
        fail(RDXJfail);
    }
    done;
}
