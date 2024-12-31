#include "abc/INT.h"
#include "abc/PRO.h"
#include "JDR.h"

// action indices for the parser
#define JDRenum 0
enum {
	JDRUtf8cp1 = JDRenum+8,
	JDRUtf8cp2 = JDRenum+9,
	JDRUtf8cp3 = JDRenum+10,
	JDRUtf8cp4 = JDRenum+11,
	JDRInt = JDRenum+17,
	JDRFloat = JDRenum+18,
	JDRRef = JDRenum+19,
	JDRString = JDRenum+20,
	JDRMLString = JDRenum+21,
	JDRTerm = JDRenum+22,
	JDRStamp = JDRenum+23,
	JDROpenP = JDRenum+24,
	JDRCloseP = JDRenum+25,
	JDROpenL = JDRenum+26,
	JDRCloseL = JDRenum+27,
	JDROpenE = JDRenum+28,
	JDRCloseE = JDRenum+29,
	JDROpenX = JDRenum+30,
	JDRCloseX = JDRenum+31,
	JDRComma = JDRenum+32,
	JDRColon = JDRenum+33,
	JDROpen = JDRenum+34,
	JDRClose = JDRenum+35,
	JDRInter = JDRenum+36,
	JDRFIRST = JDRenum+38,
	JDRRoot = JDRenum+39,
};

// user functions (callbacks) for the parser
ok64 JDRonUtf8cp1 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp2 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp3 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp4 ($cu8c tok, JDRstate* state);
ok64 JDRonInt ($cu8c tok, JDRstate* state);
ok64 JDRonFloat ($cu8c tok, JDRstate* state);
ok64 JDRonRef ($cu8c tok, JDRstate* state);
ok64 JDRonString ($cu8c tok, JDRstate* state);
ok64 JDRonMLString ($cu8c tok, JDRstate* state);
ok64 JDRonTerm ($cu8c tok, JDRstate* state);
ok64 JDRonStamp ($cu8c tok, JDRstate* state);
ok64 JDRonOpenP ($cu8c tok, JDRstate* state);
ok64 JDRonCloseP ($cu8c tok, JDRstate* state);
ok64 JDRonOpenL ($cu8c tok, JDRstate* state);
ok64 JDRonCloseL ($cu8c tok, JDRstate* state);
ok64 JDRonOpenE ($cu8c tok, JDRstate* state);
ok64 JDRonCloseE ($cu8c tok, JDRstate* state);
ok64 JDRonOpenX ($cu8c tok, JDRstate* state);
ok64 JDRonCloseX ($cu8c tok, JDRstate* state);
ok64 JDRonComma ($cu8c tok, JDRstate* state);
ok64 JDRonColon ($cu8c tok, JDRstate* state);
ok64 JDRonOpen ($cu8c tok, JDRstate* state);
ok64 JDRonClose ($cu8c tok, JDRstate* state);
ok64 JDRonInter ($cu8c tok, JDRstate* state);
ok64 JDRonFIRST ($cu8c tok, JDRstate* state);
ok64 JDRonRoot ($cu8c tok, JDRstate* state);



%%{

machine JDR;

alphtype unsigned char;

# ragel actions
action JDRUtf8cp10 { mark0[JDRUtf8cp1] = p - text[0]; }
action JDRUtf8cp11 {
    tok[0] = text[0] + mark0[JDRUtf8cp1];
    tok[1] = p;
    call(JDRonUtf8cp1, tok, state); 
}
action JDRUtf8cp20 { mark0[JDRUtf8cp2] = p - text[0]; }
action JDRUtf8cp21 {
    tok[0] = text[0] + mark0[JDRUtf8cp2];
    tok[1] = p;
    call(JDRonUtf8cp2, tok, state); 
}
action JDRUtf8cp30 { mark0[JDRUtf8cp3] = p - text[0]; }
action JDRUtf8cp31 {
    tok[0] = text[0] + mark0[JDRUtf8cp3];
    tok[1] = p;
    call(JDRonUtf8cp3, tok, state); 
}
action JDRUtf8cp40 { mark0[JDRUtf8cp4] = p - text[0]; }
action JDRUtf8cp41 {
    tok[0] = text[0] + mark0[JDRUtf8cp4];
    tok[1] = p;
    call(JDRonUtf8cp4, tok, state); 
}
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
action JDRMLString0 { mark0[JDRMLString] = p - text[0]; }
action JDRMLString1 {
    tok[0] = text[0] + mark0[JDRMLString];
    tok[1] = p;
    call(JDRonMLString, tok, state); 
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

# ragel grammar rules
JDRws = (   [\r\n\t ] ); # no ws callback
JDRhex = (   [0-9a-fA-F] ); # no hex callback
JDRutf8cont = (     (0x80..0xbf) ); # no utf8cont callback
JDRutf8lead1 = (   (0x00..0x7f) ); # no utf8lead1 callback
JDRutf8lead2 = (   (0xc0..0xdf) ); # no utf8lead2 callback
JDRutf8lead3 = (   (0xe0..0xef) ); # no utf8lead3 callback
JDRutf8lead4 = (   (0xf0..0xf7) ); # no utf8lead4 callback
JDRUtf8cp1 = (     JDRutf8lead1 )  >JDRUtf8cp10 %JDRUtf8cp11;
JDRUtf8cp2 = (     JDRutf8lead2  JDRutf8cont )  >JDRUtf8cp20 %JDRUtf8cp21;
JDRUtf8cp3 = (     JDRutf8lead3  JDRutf8cont  JDRutf8cont )  >JDRUtf8cp30 %JDRUtf8cp31;
JDRUtf8cp4 = (     JDRutf8lead4  JDRutf8cont  JDRutf8cont  JDRutf8cont )  >JDRUtf8cp40 %JDRUtf8cp41;
JDRutf8cp = (   JDRUtf8cp1  |  JDRUtf8cp2  |  JDRUtf8cp3  |  JDRUtf8cp4 ); # no utf8cp callback
JDResc = (   [\\]  ["\\/bfnrt] ); # no esc callback
JDRhexEsc = (     "\\u"  JDRhex{4} ); # no hexEsc callback
JDRutf8esc = (   (JDRutf8cp  -  ["\\\r\n])  |  JDResc  |  JDRhexEsc ); # no utf8esc callback
JDRid128 = (   [0-9a-fA-F]+  ("-"  [0-9a-fA-F]+)? ); # no id128 callback
JDRInt = (   [\-]?  (  [0]  |  [1-9]  [0-9]*  ) )  >JDRInt0 %JDRInt1;
JDRFloat = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  ) 
                        ("."  [0-9]+)? 
                        ([eE]  [\-+]?  [0-9]+  )?  )  -  JDRInt )  >JDRFloat0 %JDRFloat1;
JDRRef = (   JDRid128  -JDRFloat  -JDRInt )  >JDRRef0 %JDRRef1;
JDRString = (   ["]  JDRutf8esc*  ["] )  >JDRString0 %JDRString1;
JDRMLString = (   "```"  (JDRutf8cp  -  [`])*  "```" )  >JDRMLString0 %JDRMLString1;
JDRTerm = (   [a-zA-Z0-9_~]+  -JDRInt  -JDRFloat )  >JDRTerm0 %JDRTerm1;
JDRStamp = (   "@"  JDRid128 )  >JDRStamp0 %JDRStamp1;
JDROpenP = (   "<" )  >JDROpenP0 %JDROpenP1;
JDRCloseP = (   ">" )  >JDRCloseP0 %JDRCloseP1;
JDROpenL = (   "[" )  >JDROpenL0 %JDROpenL1;
JDRCloseL = (   "]" )  >JDRCloseL0 %JDRCloseL1;
JDROpenE = (   "{" )  >JDROpenE0 %JDROpenE1;
JDRCloseE = (   "}" )  >JDRCloseE0 %JDRCloseE1;
JDROpenX = (   "(" )  >JDROpenX0 %JDROpenX1;
JDRCloseX = (   ")" )  >JDRCloseX0 %JDRCloseX1;
JDRComma = (   "," )  >JDRComma0 %JDRComma1;
JDRColon = (   ":" )  >JDRColon0 %JDRColon1;
JDROpen = (   (JDROpenP  |  JDROpenL  |  JDROpenE  |  JDROpenX)  JDRws*  (JDRStamp  JDRws*)? )  >JDROpen0 %JDROpen1;
JDRClose = (   (JDRCloseP  |  JDRCloseL  |  JDRCloseE  |  JDRCloseX)  JDRws* )  >JDRClose0 %JDRClose1;
JDRInter = (   (JDRComma  |  JDRColon)  JDRws* )  >JDRInter0 %JDRInter1;
JDRdelim = (   JDROpen  |  JDRClose  |  JDRInter ); # no delim callback
JDRFIRST = (   (  JDRFloat  |  JDRInt  |  JDRRef  |  JDRString  |  JDRMLString  |  JDRTerm  )  (JDRws*  JDRStamp)?  JDRws* )  >JDRFIRST0 %JDRFIRST1;
JDRRoot = (   JDRws*  (  JDRFIRST?  (  JDRdelim  <:  JDRFIRST?  )*  )   )  >JDRRoot0 %JDRRoot1;

main := JDRRoot;

}%%

%%write data;

// the public API function
pro(JDRlexer, JDRstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u64 mark0[64] = {};

    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    state->text[0] = p;
    if (p!=text[1] || cs < JDR_first_final) {
        return JDRfail;
    }
    done;
}
