#include "abc/INT.h"
#include "abc/PRO.h"
#include "LEX.h"

// action indices for the parser
#define LEXenum 0
enum {
	LEXSpace = LEXenum+1,
	LEXName = LEXenum+6,
	LEXRep = LEXenum+7,
	LEXOp = LEXenum+8,
	LEXClass = LEXenum+9,
	LEXRange = LEXenum+10,
	LEXString = LEXenum+11,
	LEXQString = LEXenum+12,
	LEXEntity = LEXenum+13,
	LEXExpr = LEXenum+14,
	LEXRuleName = LEXenum+15,
	LEXEq = LEXenum+16,
	LEXLine = LEXenum+17,
	LEXRoot = LEXenum+18,
};

// user functions (callbacks) for the parser
ok64 LEXonSpace ($cu8c tok, LEXstate* state);
ok64 LEXonName ($cu8c tok, LEXstate* state);
ok64 LEXonRep ($cu8c tok, LEXstate* state);
ok64 LEXonOp ($cu8c tok, LEXstate* state);
ok64 LEXonClass ($cu8c tok, LEXstate* state);
ok64 LEXonRange ($cu8c tok, LEXstate* state);
ok64 LEXonString ($cu8c tok, LEXstate* state);
ok64 LEXonQString ($cu8c tok, LEXstate* state);
ok64 LEXonEntity ($cu8c tok, LEXstate* state);
ok64 LEXonExpr ($cu8c tok, LEXstate* state);
ok64 LEXonRuleName ($cu8c tok, LEXstate* state);
ok64 LEXonEq ($cu8c tok, LEXstate* state);
ok64 LEXonLine ($cu8c tok, LEXstate* state);
ok64 LEXonRoot ($cu8c tok, LEXstate* state);



%%{

machine LEX;

alphtype unsigned char;

# ragel actions
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
action LEXRange0 { mark0[LEXRange] = p - text[0]; }
action LEXRange1 {
    tok[0] = text[0] + mark0[LEXRange];
    tok[1] = p;
    call(LEXonRange, tok, state); 
}
action LEXString0 { mark0[LEXString] = p - text[0]; }
action LEXString1 {
    tok[0] = text[0] + mark0[LEXString];
    tok[1] = p;
    call(LEXonString, tok, state); 
}
action LEXQString0 { mark0[LEXQString] = p - text[0]; }
action LEXQString1 {
    tok[0] = text[0] + mark0[LEXQString];
    tok[1] = p;
    call(LEXonQString, tok, state); 
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

# ragel grammar rules
LEXSpace = (   [ \t\r\n]   )  >LEXSpace0 %LEXSpace1;
LEXws = (   [ \t\r\n]   ); # no ws callback
LEXhex = (   "0x"  [0-9a-f]+ ); # no hex callback
LEXdec = (   [0-9]+ ); # no dec callback
LEXword = (     [A-Za-z_]  [A-Z0-9a-z_]** ); # no word callback
LEXName = (   LEXword )  >LEXName0 %LEXName1;
LEXRep = (   "{"  [0-9]*  (","  [0-9]*)?  "}"  -  "{}" )  >LEXRep0 %LEXRep1;
LEXOp = (   LEXSpace  |  [()+*\-?><:|\.]  |  LEXRep )  >LEXOp0 %LEXOp1;
LEXClass = (   "["  ([^\]]|"\\]")*  "]"   )  >LEXClass0 %LEXClass1;
LEXRange = (   "("  (LEXhex|LEXdec)  ".."  (LEXhex|LEXdec)  |  (LEXhex|LEXdec)  ")" )  >LEXRange0 %LEXRange1;
LEXString = (   "\""  ([^"]|"\\\"")*  "\""   )  >LEXString0 %LEXString1;
LEXQString = (   "\'"  ([^']|"\\\'")*  "\'"   )  >LEXQString0 %LEXQString1;
LEXEntity = (   LEXClass  |  LEXName  |  LEXString  |  LEXQString  |  LEXRange )  >LEXEntity0 %LEXEntity1;
LEXExpr = (   (LEXOp+  LEXEntity)*  LEXOp*   )  >LEXExpr0 %LEXExpr1;
LEXRuleName = (   LEXword )  >LEXRuleName0 %LEXRuleName1;
LEXEq = (   LEXws*  "=" )  >LEXEq0 %LEXEq1;
LEXLine = (   LEXws*  LEXRuleName  LEXEq  LEXExpr  ";"   )  >LEXLine0 %LEXLine1;
LEXRoot = (   LEXLine*  LEXws*   )  >LEXRoot0 %LEXRoot1;

main := LEXRoot;

}%%

%%write data;

// the public API function
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

    state->text[0] = p;
    if (p!=text[1] || cs < LEX_first_final) {
        return LEXfail;
    }
    done;
}
