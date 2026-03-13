#include "abc/INT.h"
#include "abc/PRO.h"
#include "SM.h"

// action indices for the parser
#define SMenum 0
enum {
	SMHLine = SMenum+2,
	SMIndent = SMenum+3,
	SMOList = SMenum+4,
	SMUList = SMenum+5,
	SMH1 = SMenum+6,
	SMH2 = SMenum+7,
	SMH3 = SMenum+8,
	SMH4 = SMenum+9,
	SMH = SMenum+10,
	SMQuote = SMenum+11,
	SMCode = SMenum+12,
	SMTodo = SMenum+13,
	SMLink = SMenum+15,
	SMDiv = SMenum+18,
	SMLine = SMenum+19,
	SMRoot = SMenum+20,
};

// user functions (callbacks) for the parser
ok64 SMonHLine (u8cs tok, SMstate* state);
ok64 SMonIndent (u8cs tok, SMstate* state);
ok64 SMonOList (u8cs tok, SMstate* state);
ok64 SMonUList (u8cs tok, SMstate* state);
ok64 SMonH1 (u8cs tok, SMstate* state);
ok64 SMonH2 (u8cs tok, SMstate* state);
ok64 SMonH3 (u8cs tok, SMstate* state);
ok64 SMonH4 (u8cs tok, SMstate* state);
ok64 SMonH (u8cs tok, SMstate* state);
ok64 SMonQuote (u8cs tok, SMstate* state);
ok64 SMonCode (u8cs tok, SMstate* state);
ok64 SMonTodo (u8cs tok, SMstate* state);
ok64 SMonLink (u8cs tok, SMstate* state);
ok64 SMonDiv (u8cs tok, SMstate* state);
ok64 SMonLine (u8cs tok, SMstate* state);
ok64 SMonRoot (u8cs tok, SMstate* state);



%%{

machine SM;

alphtype unsigned char;

# ragel actions
action SMHLine0 { mark0[SMHLine] = p - data[0]; }
action SMHLine1 {
    tok[0] = data[0] + mark0[SMHLine];
    tok[1] = p;
    o = SMonHLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMIndent0 { mark0[SMIndent] = p - data[0]; }
action SMIndent1 {
    tok[0] = data[0] + mark0[SMIndent];
    tok[1] = p;
    o = SMonIndent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMOList0 { mark0[SMOList] = p - data[0]; }
action SMOList1 {
    tok[0] = data[0] + mark0[SMOList];
    tok[1] = p;
    o = SMonOList(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMUList0 { mark0[SMUList] = p - data[0]; }
action SMUList1 {
    tok[0] = data[0] + mark0[SMUList];
    tok[1] = p;
    o = SMonUList(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMH10 { mark0[SMH1] = p - data[0]; }
action SMH11 {
    tok[0] = data[0] + mark0[SMH1];
    tok[1] = p;
    o = SMonH1(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMH20 { mark0[SMH2] = p - data[0]; }
action SMH21 {
    tok[0] = data[0] + mark0[SMH2];
    tok[1] = p;
    o = SMonH2(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMH30 { mark0[SMH3] = p - data[0]; }
action SMH31 {
    tok[0] = data[0] + mark0[SMH3];
    tok[1] = p;
    o = SMonH3(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMH40 { mark0[SMH4] = p - data[0]; }
action SMH41 {
    tok[0] = data[0] + mark0[SMH4];
    tok[1] = p;
    o = SMonH4(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMH0 { mark0[SMH] = p - data[0]; }
action SMH1 {
    tok[0] = data[0] + mark0[SMH];
    tok[1] = p;
    o = SMonH(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMQuote0 { mark0[SMQuote] = p - data[0]; }
action SMQuote1 {
    tok[0] = data[0] + mark0[SMQuote];
    tok[1] = p;
    o = SMonQuote(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMCode0 { mark0[SMCode] = p - data[0]; }
action SMCode1 {
    tok[0] = data[0] + mark0[SMCode];
    tok[1] = p;
    o = SMonCode(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMTodo0 { mark0[SMTodo] = p - data[0]; }
action SMTodo1 {
    tok[0] = data[0] + mark0[SMTodo];
    tok[1] = p;
    o = SMonTodo(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMLink0 { mark0[SMLink] = p - data[0]; }
action SMLink1 {
    tok[0] = data[0] + mark0[SMLink];
    tok[1] = p;
    o = SMonLink(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMDiv0 { mark0[SMDiv] = p - data[0]; }
action SMDiv1 {
    tok[0] = data[0] + mark0[SMDiv];
    tok[1] = p;
    o = SMonDiv(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMLine0 { mark0[SMLine] = p - data[0]; }
action SMLine1 {
    tok[0] = data[0] + mark0[SMLine];
    tok[1] = p;
    o = SMonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action SMRoot0 { mark0[SMRoot] = p - data[0]; }
action SMRoot1 {
    tok[0] = data[0] + mark0[SMRoot];
    tok[1] = p;
    o = SMonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
SM_a = (   [0-0xff] ); # no _a callback
SMHLine = (   "----" )  >SMHLine0 %SMHLine1;
SMIndent = (   "    " )  >SMIndent0 %SMIndent1;
SMOList = (  
    [0-9]  ".  "  |  " "  [0-9]  ". "  |  "  "  [0-9]  "."  | 
    [0-9]{2}  ". "  |  " "  [0-9]{2}  "."  | 
    [0-9]{3}  "." )  >SMOList0 %SMOList1;
SMUList = (   "-   "  |  " -  "  |  "  - "  |  "   -" )  >SMUList0 %SMUList1;
SMH1 = (   "#   "  |  " #  "  |  "  # "  |  "   #" )  >SMH10 %SMH11;
SMH2 = (   "##  "  |  " ## "  |  "  ##" )  >SMH20 %SMH21;
SMH3 = (   "### "  |  " ###" )  >SMH30 %SMH31;
SMH4 = (   "####" )  >SMH40 %SMH41;
SMH = (   SMH1  |  SMH2  |  SMH3  |  SMH4 )  >SMH0 %SMH1;
SMQuote = (   ">   "  |  " >  "  |  "  > "  |  "   >" )  >SMQuote0 %SMQuote1;
SMCode = (   "````"  |  "``` " )  >SMCode0 %SMCode1;
SMTodo = (   "[ ] "  |  "[x] "  |  "[X] " )  >SMTodo0 %SMTodo1;
SMlndx = (   [0-9A-Za-z] ); # no lndx callback
SMLink = (   "["  SMlndx  "]:" )  >SMLink0 %SMLink1;
SMnest = (   SMIndent  |  SMQuote ); # no nest callback
SMterm = (   SMHLine  |  SMH1  |  SMH2  |  SMH3  |  SMH4  |  SMLink  |  SMOList  |  SMUList  |  SMCode  |  SMTodo ); # no term callback
SMDiv = (   SMnest*  SMterm? )  >SMDiv0 %SMDiv1;
SMLine = (   SMDiv  [^\n]*  "\n" )  >SMLine0 %SMLine1;
SMRoot = (   SMLine* )  >SMRoot0 %SMRoot1;

main := SMRoot;

}%%

%%write data;

// the public API function
ok64 SMLexer(SMstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    u8cs tok = {p, p};

    %% write init;
    %% write exec;

_out:
    state->data[0] = p;
    if (o==OK && cs < SM_first_final) 
        o = SMBAD;
    
    return o;
}
