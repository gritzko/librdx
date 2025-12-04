#include "abc/INT.h"
#include "abc/PRO.h"
#include "JDR.h"

// action indices for the parser
#define JDRenum 0
enum {
	JDRNL = JDRenum+1,
	JDRUtf8cp1 = JDRenum+12,
	JDRUtf8cp2 = JDRenum+13,
	JDRUtf8cp3 = JDRenum+14,
	JDRUtf8cp4 = JDRenum+15,
	JDRInt = JDRenum+21,
	JDRFloat = JDRenum+22,
	JDRTerm = JDRenum+23,
	JDRRef = JDRenum+24,
	JDRString = JDRenum+25,
	JDRMLString = JDRenum+26,
	JDRStamp = JDRenum+27,
	JDRNoStamp = JDRenum+28,
	JDRComma = JDRenum+29,
	JDRColon = JDRenum+30,
	JDROpen = JDRenum+31,
	JDRClose = JDRenum+32,
	JDRFIRST = JDRenum+33,
	JDRInter = JDRenum+34,
	JDRRoot = JDRenum+35,
};

// user functions (callbacks) for the parser
ok64 JDRonNL (utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp1 (utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp2 (utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp3 (utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp4 (utf8cs tok, JDRstate* state);
ok64 JDRonInt (utf8cs tok, JDRstate* state);
ok64 JDRonFloat (utf8cs tok, JDRstate* state);
ok64 JDRonTerm (utf8cs tok, JDRstate* state);
ok64 JDRonRef (utf8cs tok, JDRstate* state);
ok64 JDRonString (utf8cs tok, JDRstate* state);
ok64 JDRonMLString (utf8cs tok, JDRstate* state);
ok64 JDRonStamp (utf8cs tok, JDRstate* state);
ok64 JDRonNoStamp (utf8cs tok, JDRstate* state);
ok64 JDRonComma (utf8cs tok, JDRstate* state);
ok64 JDRonColon (utf8cs tok, JDRstate* state);
ok64 JDRonOpen (utf8cs tok, JDRstate* state);
ok64 JDRonClose (utf8cs tok, JDRstate* state);
ok64 JDRonFIRST (utf8cs tok, JDRstate* state);
ok64 JDRonInter (utf8cs tok, JDRstate* state);
ok64 JDRonRoot (utf8cs tok, JDRstate* state);



%%{

machine JDR;

alphtype unsigned char;

# ragel actions
action JDRNL0 { mark0[JDRNL] = p - data[0]; }
action JDRNL1 {
    tok[0] = data[0] + mark0[JDRNL];
    tok[1] = p;
    o = JDRonNL(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRUtf8cp10 { mark0[JDRUtf8cp1] = p - data[0]; }
action JDRUtf8cp11 {
    tok[0] = data[0] + mark0[JDRUtf8cp1];
    tok[1] = p;
    o = JDRonUtf8cp1(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRUtf8cp20 { mark0[JDRUtf8cp2] = p - data[0]; }
action JDRUtf8cp21 {
    tok[0] = data[0] + mark0[JDRUtf8cp2];
    tok[1] = p;
    o = JDRonUtf8cp2(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRUtf8cp30 { mark0[JDRUtf8cp3] = p - data[0]; }
action JDRUtf8cp31 {
    tok[0] = data[0] + mark0[JDRUtf8cp3];
    tok[1] = p;
    o = JDRonUtf8cp3(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRUtf8cp40 { mark0[JDRUtf8cp4] = p - data[0]; }
action JDRUtf8cp41 {
    tok[0] = data[0] + mark0[JDRUtf8cp4];
    tok[1] = p;
    o = JDRonUtf8cp4(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRInt0 { mark0[JDRInt] = p - data[0]; }
action JDRInt1 {
    tok[0] = data[0] + mark0[JDRInt];
    tok[1] = p;
    o = JDRonInt(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRFloat0 { mark0[JDRFloat] = p - data[0]; }
action JDRFloat1 {
    tok[0] = data[0] + mark0[JDRFloat];
    tok[1] = p;
    o = JDRonFloat(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRTerm0 { mark0[JDRTerm] = p - data[0]; }
action JDRTerm1 {
    tok[0] = data[0] + mark0[JDRTerm];
    tok[1] = p;
    o = JDRonTerm(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRRef0 { mark0[JDRRef] = p - data[0]; }
action JDRRef1 {
    tok[0] = data[0] + mark0[JDRRef];
    tok[1] = p;
    o = JDRonRef(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRString0 { mark0[JDRString] = p - data[0]; }
action JDRString1 {
    tok[0] = data[0] + mark0[JDRString];
    tok[1] = p;
    o = JDRonString(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRMLString0 { mark0[JDRMLString] = p - data[0]; }
action JDRMLString1 {
    tok[0] = data[0] + mark0[JDRMLString];
    tok[1] = p;
    o = JDRonMLString(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRStamp0 { mark0[JDRStamp] = p - data[0]; }
action JDRStamp1 {
    tok[0] = data[0] + mark0[JDRStamp];
    tok[1] = p;
    o = JDRonStamp(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRNoStamp0 { mark0[JDRNoStamp] = p - data[0]; }
action JDRNoStamp1 {
    tok[0] = data[0] + mark0[JDRNoStamp];
    tok[1] = p;
    o = JDRonNoStamp(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRComma0 { mark0[JDRComma] = p - data[0]; }
action JDRComma1 {
    tok[0] = data[0] + mark0[JDRComma];
    tok[1] = p;
    o = JDRonComma(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRColon0 { mark0[JDRColon] = p - data[0]; }
action JDRColon1 {
    tok[0] = data[0] + mark0[JDRColon];
    tok[1] = p;
    o = JDRonColon(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDROpen0 { mark0[JDROpen] = p - data[0]; }
action JDROpen1 {
    tok[0] = data[0] + mark0[JDROpen];
    tok[1] = p;
    o = JDRonOpen(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRClose0 { mark0[JDRClose] = p - data[0]; }
action JDRClose1 {
    tok[0] = data[0] + mark0[JDRClose];
    tok[1] = p;
    o = JDRonClose(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRFIRST0 { mark0[JDRFIRST] = p - data[0]; }
action JDRFIRST1 {
    tok[0] = data[0] + mark0[JDRFIRST];
    tok[1] = p;
    o = JDRonFIRST(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRInter0 { mark0[JDRInter] = p - data[0]; }
action JDRInter1 {
    tok[0] = data[0] + mark0[JDRInter];
    tok[1] = p;
    o = JDRonInter(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JDRRoot0 { mark0[JDRRoot] = p - data[0]; }
action JDRRoot1 {
    tok[0] = data[0] + mark0[JDRRoot];
    tok[1] = p;
    o = JDRonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
JDRNL = (   "\n" )  >JDRNL0 %JDRNL1;
JDRws = (   [\r\t ]  |  JDRNL ); # no ws callback
JDRhex = (   [0-9a-fA-Z] ); # no hex callback
JDRron64 = (   [0-9A-Za-z_~] ); # no ron64 callback
JDRron64s = (   JDRron64+ ); # no ron64s callback
JDRdec = (   [0-9] ); # no dec callback
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
JDRid128 = (   JDRron64s  ("-"  JDRron64s)? ); # no id128 callback
JDRInt = (   [\-]?  (  [0]  |  [1-9]  JDRdec*  ) )  >JDRInt0 %JDRInt1;
JDRFloat = (   (      JDRInt 
                        ("."  JDRdec+)? 
                        ([eE]  [\-+]?  JDRdec+  )?    )  -JDRInt )  >JDRFloat0 %JDRFloat1;
JDRTerm = (   JDRron64s  -JDRInt  -JDRFloat )  >JDRTerm0 %JDRTerm1;
JDRRef = (   JDRid128  -JDRFloat  -JDRInt  -JDRTerm )  >JDRRef0 %JDRRef1;
JDRString = (   ["]  JDRutf8esc*  ["] )  >JDRString0 %JDRString1;
JDRMLString = (   "`"  (JDRutf8cp  -  [`])*  "`" )  >JDRMLString0 %JDRMLString1;
JDRStamp = (   "@"  JDRid128 )  >JDRStamp0 %JDRStamp1;
JDRNoStamp = (   "" )  >JDRNoStamp0 %JDRNoStamp1;
JDRComma = (   [,;] )  >JDRComma0 %JDRComma1;
JDRColon = (   [:] )  >JDRColon0 %JDRColon1;
JDROpen = (   [(\[{<]  JDRws*  (JDRStamp  |  JDRNoStamp) )  >JDROpen0 %JDROpen1;
JDRClose = (   [)\]}>] )  >JDRClose0 %JDRClose1;
JDRFIRST = (   (  JDRFloat  |  JDRInt  |  JDRRef  |  JDRString  |  JDRMLString  |  JDRTerm  )  JDRws*  (  JDRStamp  |  JDRNoStamp  ) )  >JDRFIRST0 %JDRFIRST1;
JDRInter = (   JDRComma  |  JDRColon  |  JDROpen  |  JDRClose  |  JDRws+ )  >JDRInter0 %JDRInter1;
JDRRoot = (   (  JDRFIRST  |  JDRInter  )**   )  >JDRRoot0 %JDRRoot1;

main := JDRRoot;

}%%

%%write data;

// the public API function
ok64 JDRlexer(JDRstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    utf8cs tok = {p, p};

    %% write init;
    %% write exec;

    state->data[0] = p;
    if (o==OK && cs < JDR_first_final) 
        o = JDRBAD;
    
    return o;
}
