#include "MARK.rl.h"


%%{

machine MARK;

alphtype unsigned char;

action MARKHLine0 { mark0[MARKHLine] = p - text[0]; }
action MARKHLine1 {
    tok[0] = text[0] + mark0[MARKHLine];
    tok[1] = p;
    call(MARKonHLine, tok, state); 
}
action MARKIndent0 { mark0[MARKIndent] = p - text[0]; }
action MARKIndent1 {
    tok[0] = text[0] + mark0[MARKIndent];
    tok[1] = p;
    call(MARKonIndent, tok, state); 
}
action MARKOList0 { mark0[MARKOList] = p - text[0]; }
action MARKOList1 {
    tok[0] = text[0] + mark0[MARKOList];
    tok[1] = p;
    call(MARKonOList, tok, state); 
}
action MARKUList0 { mark0[MARKUList] = p - text[0]; }
action MARKUList1 {
    tok[0] = text[0] + mark0[MARKUList];
    tok[1] = p;
    call(MARKonUList, tok, state); 
}
action MARKH10 { mark0[MARKH1] = p - text[0]; }
action MARKH11 {
    tok[0] = text[0] + mark0[MARKH1];
    tok[1] = p;
    call(MARKonH1, tok, state); 
}
action MARKH20 { mark0[MARKH2] = p - text[0]; }
action MARKH21 {
    tok[0] = text[0] + mark0[MARKH2];
    tok[1] = p;
    call(MARKonH2, tok, state); 
}
action MARKH30 { mark0[MARKH3] = p - text[0]; }
action MARKH31 {
    tok[0] = text[0] + mark0[MARKH3];
    tok[1] = p;
    call(MARKonH3, tok, state); 
}
action MARKH40 { mark0[MARKH4] = p - text[0]; }
action MARKH41 {
    tok[0] = text[0] + mark0[MARKH4];
    tok[1] = p;
    call(MARKonH4, tok, state); 
}
action MARKH0 { mark0[MARKH] = p - text[0]; }
action MARKH1 {
    tok[0] = text[0] + mark0[MARKH];
    tok[1] = p;
    call(MARKonH, tok, state); 
}
action MARKQuote0 { mark0[MARKQuote] = p - text[0]; }
action MARKQuote1 {
    tok[0] = text[0] + mark0[MARKQuote];
    tok[1] = p;
    call(MARKonQuote, tok, state); 
}
action MARKLink0 { mark0[MARKLink] = p - text[0]; }
action MARKLink1 {
    tok[0] = text[0] + mark0[MARKLink];
    tok[1] = p;
    call(MARKonLink, tok, state); 
}
action MARKDiv0 { mark0[MARKDiv] = p - text[0]; }
action MARKDiv1 {
    tok[0] = text[0] + mark0[MARKDiv];
    tok[1] = p;
    call(MARKonDiv, tok, state); 
}
action MARKLine0 { mark0[MARKLine] = p - text[0]; }
action MARKLine1 {
    tok[0] = text[0] + mark0[MARKLine];
    tok[1] = p;
    call(MARKonLine, tok, state); 
}
action MARKRoot0 { mark0[MARKRoot] = p - text[0]; }
action MARKRoot1 {
    tok[0] = text[0] + mark0[MARKRoot];
    tok[1] = p;
    call(MARKonRoot, tok, state); 
}

MARK_a  = (   [0-0xff] );


MARKHLine  = (   "----" )  >MARKHLine0 %MARKHLine1;

MARKIndent  = (   "    " )  >MARKIndent0 %MARKIndent1;


MARKOList  = (   

    [0-9]  ".  "  |  " "  [0-9]  ". "  |  "  "  [0-9]  "."  |

    [0-9]{2}  ". "  |  " "  [0-9]{2}  "."  |

    [0-9]{3}  "." )  >MARKOList0 %MARKOList1;

MARKUList  = (   "-   "  |  " -  "  |  "  - "  |  "   -" )  >MARKUList0 %MARKUList1;


MARKH1  = (   "#   "  |  " #  "  |  "  # "  |  "   #" )  >MARKH10 %MARKH11;

MARKH2  = (   "##  "  |  " ## "  |  "  ##" )  >MARKH20 %MARKH21;

MARKH3  = (   "### "  |  " ###" )  >MARKH30 %MARKH31;

MARKH4  = (   "####" )  >MARKH40 %MARKH41;
 
MARKH  = (   MARKH1  |  MARKH2  |  MARKH3  |  MARKH4 )  >MARKH0 %MARKH1;


MARKQuote  = (   ">   "  |  " >  "  |  "  > "  |  "   >" )  >MARKQuote0 %MARKQuote1;


MARKlndx  = (   [0-9A-Za-z] );

MARKLink  = (   "["  MARKlndx  "]:" )  >MARKLink0 %MARKLink1;


MARKnest  = (   MARKIndent  |  MARKQuote );

MARKterm  = (   MARKHLine  |  MARKH1  |  MARKH2  |  MARKH3  |  MARKH4  |  MARKLink  |  MARKOList  |  MARKUList );

MARKDiv  = (   MARKnest*  MARKterm? )  >MARKDiv0 %MARKDiv1;


MARKLine  = (   MARKDiv  [^\n]*  "\n" )  >MARKLine0 %MARKLine1;


MARKRoot  = (   MARKLine* )  >MARKRoot0 %MARKRoot1;

main := MARKRoot;

}%%

%%write data;

pro(MARKlexer, MARKstate* state) {

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

    if (p!=text[1] || cs <= MARK_first_final) {
        fail(MARKfail);
        state->text[0] = p;
    }
    done;
}
