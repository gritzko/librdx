#include "MARQ.rl.h"


%%{

machine MARQ;

alphtype unsigned char;

action MARQRef00 { mark0[MARQRef0] = p - text[0]; }
action MARQRef01 {
    tok[0] = text[0] + mark0[MARQRef0];
    tok[1] = p;
    call(MARQonRef0, tok, state); 
}
action MARQRef10 { mark0[MARQRef1] = p - text[0]; }
action MARQRef11 {
    tok[0] = text[0] + mark0[MARQRef1];
    tok[1] = p;
    call(MARQonRef1, tok, state); 
}
action MARQEm00 { mark0[MARQEm0] = p - text[0]; }
action MARQEm01 {
    tok[0] = text[0] + mark0[MARQEm0];
    tok[1] = p;
    call(MARQonEm0, tok, state); 
}
action MARQEm10 { mark0[MARQEm1] = p - text[0]; }
action MARQEm11 {
    tok[0] = text[0] + mark0[MARQEm1];
    tok[1] = p;
    call(MARQonEm1, tok, state); 
}
action MARQEm0 { mark0[MARQEm] = p - text[0]; }
action MARQEm1 {
    tok[0] = text[0] + mark0[MARQEm];
    tok[1] = p;
    call(MARQonEm, tok, state); 
}
action MARQCode010 { mark0[MARQCode01] = p - text[0]; }
action MARQCode011 {
    tok[0] = text[0] + mark0[MARQCode01];
    tok[1] = p;
    call(MARQonCode01, tok, state); 
}
action MARQSt00 { mark0[MARQSt0] = p - text[0]; }
action MARQSt01 {
    tok[0] = text[0] + mark0[MARQSt0];
    tok[1] = p;
    call(MARQonSt0, tok, state); 
}
action MARQSt10 { mark0[MARQSt1] = p - text[0]; }
action MARQSt11 {
    tok[0] = text[0] + mark0[MARQSt1];
    tok[1] = p;
    call(MARQonSt1, tok, state); 
}
action MARQSt0 { mark0[MARQSt] = p - text[0]; }
action MARQSt1 {
    tok[0] = text[0] + mark0[MARQSt];
    tok[1] = p;
    call(MARQonSt, tok, state); 
}
action MARQRoot0 { mark0[MARQRoot] = p - text[0]; }
action MARQRoot1 {
    tok[0] = text[0] + mark0[MARQRoot];
    tok[1] = p;
    call(MARQonRoot, tok, state); 
}

MARQalpha  = (   [0-9A-Za-z] );

MARQws  = (   [ \t\n\r] );

MARQany  = (   [0-0xff] );

MARQnonws  = (   [^ \t\n\r] );

MARQpunkt  = (   [,.;:!?\-"'()"] );

MARQwsp  = (   MARQws  |  MARQpunkt );

MARQword  = (   MARQnonws  + );

MARQwords  = (   MARQword  (  MARQws+  MARQword  )* );


MARQRef0  = (   MARQwsp  "["  MARQnonws )  >MARQRef00 %MARQRef01;

MARQRef1  = (   MARQnonws  "]["  MARQalpha  "]" )  >MARQRef10 %MARQRef11;


MARQEm0  = (   MARQwsp  "_"  MARQnonws )  >MARQEm00 %MARQEm01;

MARQEm1  = (   MARQnonws  "_"  MARQwsp )  >MARQEm10 %MARQEm11;

MARQEm  = (   "_"  (MARQword  MARQws+)*  MARQword?  (MARQnonws-"\\")  :>>  "_" )  >MARQEm0 %MARQEm1;


MARQCode01  = (   [^\\]  "`" )  >MARQCode010 %MARQCode011;


MARQSt0  = (   MARQwsp  "*"  MARQnonws )  >MARQSt00 %MARQSt01;

MARQSt1  = (   [^\t\r\n *]  "*" )  >MARQSt10 %MARQSt11;

MARQSt  = (   "*"  (MARQword  MARQws+)*  MARQword?  (MARQnonws-"\\")  :>>  "*" )  >MARQSt0 %MARQSt1;


MARQinline  = (   MARQwords  |  MARQEm0  |  MARQEm1  |  MARQEm  |  MARQSt0  |  MARQSt1  |  MARQRef0  |  MARQRef1  |  MARQCode01 );

MARQRoot  = (   (MARQws*  MARQinline)*  MARQws* )  >MARQRoot0 %MARQRoot1;

main := MARQRoot;

}%%

%%write data;

pro(MARQlexer, MARQstate* state) {

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

    if (p!=text[1] || cs < MARQ_first_final) {
        fail(MARQfail);
        state->text[0] = p;
    }
    done;
}
