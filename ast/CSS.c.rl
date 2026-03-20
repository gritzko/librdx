#include "abc/INT.h"
#include "abc/PRO.h"
#include "CSS.h"

// action indices for the parser
#define CSSenum 0
enum {
	CSSIdent = CSSenum+4,
	CSSDot = CSSenum+5,
	CSSStar = CSSenum+6,
	CSSChild = CSSenum+7,
	CSSAdjacent = CSSenum+8,
	CSSSibling = CSSenum+9,
	CSSHas = CSSenum+10,
	CSSNot = CSSenum+11,
	CSSClose = CSSenum+12,
	CSSLine = CSSenum+13,
	CSSRoot = CSSenum+17,
};

// user functions (callbacks) for the parser
ok64 CSSonIdent (u8cs tok, CSSstate* state);
ok64 CSSonDot (u8cs tok, CSSstate* state);
ok64 CSSonStar (u8cs tok, CSSstate* state);
ok64 CSSonChild (u8cs tok, CSSstate* state);
ok64 CSSonAdjacent (u8cs tok, CSSstate* state);
ok64 CSSonSibling (u8cs tok, CSSstate* state);
ok64 CSSonHas (u8cs tok, CSSstate* state);
ok64 CSSonNot (u8cs tok, CSSstate* state);
ok64 CSSonClose (u8cs tok, CSSstate* state);
ok64 CSSonLine (u8cs tok, CSSstate* state);
ok64 CSSonRoot (u8cs tok, CSSstate* state);



%%{

machine CSS;

alphtype unsigned char;

# ragel actions
action CSSIdent0 { mark0[CSSIdent] = p - data[0]; }
action CSSIdent1 {
    tok[0] = data[0] + mark0[CSSIdent];
    tok[1] = p;
    o = CSSonIdent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSDot0 { mark0[CSSDot] = p - data[0]; }
action CSSDot1 {
    tok[0] = data[0] + mark0[CSSDot];
    tok[1] = p;
    o = CSSonDot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSStar0 { mark0[CSSStar] = p - data[0]; }
action CSSStar1 {
    tok[0] = data[0] + mark0[CSSStar];
    tok[1] = p;
    o = CSSonStar(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSChild0 { mark0[CSSChild] = p - data[0]; }
action CSSChild1 {
    tok[0] = data[0] + mark0[CSSChild];
    tok[1] = p;
    o = CSSonChild(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSAdjacent0 { mark0[CSSAdjacent] = p - data[0]; }
action CSSAdjacent1 {
    tok[0] = data[0] + mark0[CSSAdjacent];
    tok[1] = p;
    o = CSSonAdjacent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSSibling0 { mark0[CSSSibling] = p - data[0]; }
action CSSSibling1 {
    tok[0] = data[0] + mark0[CSSSibling];
    tok[1] = p;
    o = CSSonSibling(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSHas0 { mark0[CSSHas] = p - data[0]; }
action CSSHas1 {
    tok[0] = data[0] + mark0[CSSHas];
    tok[1] = p;
    o = CSSonHas(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSNot0 { mark0[CSSNot] = p - data[0]; }
action CSSNot1 {
    tok[0] = data[0] + mark0[CSSNot];
    tok[1] = p;
    o = CSSonNot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSClose0 { mark0[CSSClose] = p - data[0]; }
action CSSClose1 {
    tok[0] = data[0] + mark0[CSSClose];
    tok[1] = p;
    o = CSSonClose(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSLine0 { mark0[CSSLine] = p - data[0]; }
action CSSLine1 {
    tok[0] = data[0] + mark0[CSSLine];
    tok[1] = p;
    o = CSSonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action CSSRoot0 { mark0[CSSRoot] = p - data[0]; }
action CSSRoot1 {
    tok[0] = data[0] + mark0[CSSRoot];
    tok[1] = p;
    o = CSSonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
CSSws = (   [ \t] ); # no ws callback
CSSident = (   [a-zA-Z_]  [a-zA-Z0-9_]* ); # no ident callback
CSSnumber = (   [0-9]+ ); # no number callback
CSSIdent = (   CSSident )  >CSSIdent0 %CSSIdent1;
CSSDot = (   "." )  >CSSDot0 %CSSDot1;
CSSStar = (   "*" )  >CSSStar0 %CSSStar1;
CSSChild = (   ">" )  >CSSChild0 %CSSChild1;
CSSAdjacent = (   "+" )  >CSSAdjacent0 %CSSAdjacent1;
CSSSibling = (   "~" )  >CSSSibling0 %CSSSibling1;
CSSHas = (   ":has(" )  >CSSHas0 %CSSHas1;
CSSNot = (   ":not(" )  >CSSNot0 %CSSNot1;
CSSClose = (   ")" )  >CSSClose0 %CSSClose1;
CSSLine = (   "L"  CSSnumber  ("-"  CSSnumber)? )  >CSSLine0 %CSSLine1;
CSSpunct = (   CSSDot  |  CSSStar  |  CSSChild  |  CSSAdjacent  |  CSSSibling  |  CSSHas  |  CSSNot  |  CSSClose ); # no punct callback
CSSfirst = (   CSSIdent  |  CSSLine ); # no first callback
CSSsep = (   CSSws*  CSSpunct  CSSws*  |  CSSws+ ); # no sep callback
CSSRoot = (   CSSws*  CSSfirst?  (CSSsep  CSSfirst?)*  CSSws* )  >CSSRoot0 %CSSRoot1;

main := CSSRoot;

}%%

%%write data;

// the public API function
ok64 CSSLexer(CSSstate* state) {

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

    state->data[0] = p;
    if (o==OK && cs < CSS_first_final) 
        o = CSSBAD;
    
    return o;
}
