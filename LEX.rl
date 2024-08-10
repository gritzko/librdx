#include "PRO.h"
#include "LEX.h"

enum {
	LEX = 0,
	LEXspace = LEX+1,
	LEXname = LEX+2,
	LEXop = LEX+4,
	LEXclass = LEX+5,
	LEXstring = LEX+6,
	LEXentity = LEX+7,
	LEXexpr = LEX+8,
	LEXrulename = LEX+9,
	LEXeq = LEX+10,
	LEXline = LEX+11,
	LEXroot = LEX+12,
};

#define LEXmaxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : LEXfail;
}

#define lexpush(t) { \
    if (sp>=LEXmaxnest) fail(LEXfail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _LEXspace ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXname ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXop ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXclass ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXstring ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXentity ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXexpr ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXrulename ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXeq ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXline ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXroot ($cu8c text, $cu8c tok, LEXstate* state);


%%{

machine LEX;

alphtype unsigned char;

action LEXspace0 { lexpush(LEXspace); }
action LEXspace1 { lexpop(LEXspace); call(_LEXspace, text, tok, state); }
action LEXname0 { lexpush(LEXname); }
action LEXname1 { lexpop(LEXname); call(_LEXname, text, tok, state); }
action LEXop0 { lexpush(LEXop); }
action LEXop1 { lexpop(LEXop); call(_LEXop, text, tok, state); }
action LEXclass0 { lexpush(LEXclass); }
action LEXclass1 { lexpop(LEXclass); call(_LEXclass, text, tok, state); }
action LEXstring0 { lexpush(LEXstring); }
action LEXstring1 { lexpop(LEXstring); call(_LEXstring, text, tok, state); }
action LEXentity0 { lexpush(LEXentity); }
action LEXentity1 { lexpop(LEXentity); call(_LEXentity, text, tok, state); }
action LEXexpr0 { lexpush(LEXexpr); }
action LEXexpr1 { lexpop(LEXexpr); call(_LEXexpr, text, tok, state); }
action LEXrulename0 { lexpush(LEXrulename); }
action LEXrulename1 { lexpop(LEXrulename); call(_LEXrulename, text, tok, state); }
action LEXeq0 { lexpush(LEXeq); }
action LEXeq1 { lexpop(LEXeq); call(_LEXeq, text, tok, state); }
action LEXline0 { lexpush(LEXline); }
action LEXline1 { lexpop(LEXline); call(_LEXline, text, tok, state); }
action LEXroot0 { lexpush(LEXroot); }
action LEXroot1 { lexpop(LEXroot); call(_LEXroot, text, tok, state); }

LEXspace  = (   [ \t\r\n]  
 ) >LEXspace0 %LEXspace1;
LEXname  = (   [A-Za-z_]  [A-Z0-9a-z_]**  
 ) >LEXname0 %LEXname1;
LEX_rep  = (   "{"  [0-9]*  (","  [0-9]*)?  "}"  -  "{}"
 );
LEXop  = (   LEXspace  |  [()+*\-?><:|]  |  LEX_rep
 ) >LEXop0 %LEXop1;
LEXclass  = (   "["  ([^\]]|"\\]")*  "]"  
 ) >LEXclass0 %LEXclass1;
LEXstring  = (   "\""  ([^"]|"\\\"")*  "\""  
 ) >LEXstring0 %LEXstring1;
LEXentity  = (   LEXclass  |  LEXname  |  LEXstring  
 ) >LEXentity0 %LEXentity1;
LEXexpr  = (   (LEXop+  LEXentity)*  LEXop*  
 ) >LEXexpr0 %LEXexpr1;
LEXrulename  = (   LEXname  
 ) >LEXrulename0 %LEXrulename1;
LEXeq  = (   LEXspace*  "="
 ) >LEXeq0 %LEXeq1;
LEXline  = (   LEXrulename  LEXeq  LEXexpr  ";"  LEXspace*  
 ) >LEXline0 %LEXline1;
LEXroot  = (   LEXline*  

 ) >LEXroot0 %LEXroot1;
main := LEXroot;

}%%

%%write data;

pro(LEXlexer, LEXstate* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[LEXmaxnest] = {0, LEX};
    u32 sp = 2;
    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    test(p==text[1], LEXfail);

    if (state->tbc) {
        test(cs != LEX_error, LEXfail);
        state->cs = cs;
    } else {
        test(cs >= LEX_first_final, LEXfail);
    }

    nedo(
        state->text[0] = p;
    );
}


