#include "PRO.h"
#include "MARK.h"

enum {
	MARK = 0,
	MARKhline = MARK+2,
	MARKindent = MARK+3,
	MARKolist = MARK+4,
	MARKulist = MARK+5,
	MARKh1 = MARK+6,
	MARKh2 = MARK+7,
	MARKh3 = MARK+8,
	MARKh4 = MARK+9,
	MARKh = MARK+10,
	MARKlndx = MARK+11,
	MARKlink = MARK+12,
	MARKnest = MARK+13,
	MARKterm = MARK+14,
	MARKdiv = MARK+15,
	MARKline = MARK+16,
	MARKroot = MARK+17,
};

#define MARKmaxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : MARKfail;
}

#define lexpush(t) { \
    if (sp>=MARKmaxnest) fail(MARKfail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _MARKhline ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKindent ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKolist ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKulist ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh1 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh2 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh3 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh4 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKlndx ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKlink ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKnest ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKterm ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKdiv ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKline ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKroot ($cu8c text, $cu8c tok, MARKstate* state);


%%{

machine MARK;

alphtype unsigned char;

action MARKhline0 { lexpush(MARKhline); }
action MARKhline1 { lexpop(MARKhline); call(_MARKhline, text, tok, state); }
action MARKindent0 { lexpush(MARKindent); }
action MARKindent1 { lexpop(MARKindent); call(_MARKindent, text, tok, state); }
action MARKolist0 { lexpush(MARKolist); }
action MARKolist1 { lexpop(MARKolist); call(_MARKolist, text, tok, state); }
action MARKulist0 { lexpush(MARKulist); }
action MARKulist1 { lexpop(MARKulist); call(_MARKulist, text, tok, state); }
action MARKh10 { lexpush(MARKh1); }
action MARKh11 { lexpop(MARKh1); call(_MARKh1, text, tok, state); }
action MARKh20 { lexpush(MARKh2); }
action MARKh21 { lexpop(MARKh2); call(_MARKh2, text, tok, state); }
action MARKh30 { lexpush(MARKh3); }
action MARKh31 { lexpop(MARKh3); call(_MARKh3, text, tok, state); }
action MARKh40 { lexpush(MARKh4); }
action MARKh41 { lexpop(MARKh4); call(_MARKh4, text, tok, state); }
action MARKh0 { lexpush(MARKh); }
action MARKh1 { lexpop(MARKh); call(_MARKh, text, tok, state); }
action MARKlndx0 { lexpush(MARKlndx); }
action MARKlndx1 { lexpop(MARKlndx); call(_MARKlndx, text, tok, state); }
action MARKlink0 { lexpush(MARKlink); }
action MARKlink1 { lexpop(MARKlink); call(_MARKlink, text, tok, state); }
action MARKnest0 { lexpush(MARKnest); }
action MARKnest1 { lexpop(MARKnest); call(_MARKnest, text, tok, state); }
action MARKterm0 { lexpush(MARKterm); }
action MARKterm1 { lexpop(MARKterm); call(_MARKterm, text, tok, state); }
action MARKdiv0 { lexpush(MARKdiv); }
action MARKdiv1 { lexpop(MARKdiv); call(_MARKdiv, text, tok, state); }
action MARKline0 { lexpush(MARKline); }
action MARKline1 { lexpop(MARKline); call(_MARKline, text, tok, state); }
action MARKroot0 { lexpush(MARKroot); }
action MARKroot1 { lexpop(MARKroot); call(_MARKroot, text, tok, state); }

MARK_a  = (   [0-0xff]

 );
MARKhline  = (   "----"
 ) >MARKhline0 %MARKhline1;
MARKindent  = (   "    "

 ) >MARKindent0 %MARKindent1;
MARKolist  = (   

    [0-9]  ".  "  |  " "  [0-9]  ". "  |  "  "  [0-9]  "."  |

    [0-9]{2}  ". "  |  " "  [0-9]{2}  "."  |

    [0-9]{3}  "."
 ) >MARKolist0 %MARKolist1;
MARKulist  = (   "-   "  |  " -  "  |  "  - "  |  "   -"

 ) >MARKulist0 %MARKulist1;
MARKh1  = (   "#   "  |  " #  "  |  "  # "  |  "   #"
 ) >MARKh10 %MARKh11;
MARKh2  = (   "##  "  |  " ## "  |  "  ##"
 ) >MARKh20 %MARKh21;
MARKh3  = (   "### "  |  " ###"
 ) >MARKh30 %MARKh31;
MARKh4  = (   "####" 
 ) >MARKh40 %MARKh41;
MARKh  = (   MARKh1  |  MARKh2  |  MARKh3  |  MARKh4

 ) >MARKh0 %MARKh1;
MARKlndx  = (   [0-9A-Za-z]
 ) >MARKlndx0 %MARKlndx1;
MARKlink  = (   "["  MARKlndx  "]:"

 ) >MARKlink0 %MARKlink1;
MARKnest  = (   MARKindent  |  MARKolist  |  MARKulist
 ) >MARKnest0 %MARKnest1;
MARKterm  = (   MARKhline  |  MARKh1  |  MARKh2  |  MARKh3  |  MARKh4  |  MARKlink
 ) >MARKterm0 %MARKterm1;
MARKdiv  = (   MARKnest*  MARKterm?

 ) >MARKdiv0 %MARKdiv1;
MARKline  = (   MARKdiv  <:  [^\n]*  "\n"

 ) >MARKline0 %MARKline1;
MARKroot  = (   MARKline*
 ) >MARKroot0 %MARKroot1;
main := MARKroot;

}%%

%%write data;

pro(MARKlexer, MARKstate* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[MARKmaxnest] = {0, MARK};
    u32 sp = 2;
    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    test(p==text[1], MARKfail);

    if (state->tbc) {
        test(cs != MARK_error, MARKfail);
        state->cs = cs;
    } else {
        test(cs >= MARK_first_final, MARKfail);
    }

    nedo(
        state->text[0] = p;
    );
}


