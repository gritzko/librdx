#include "abc/INT.h"
#include "abc/PRO.h"
#include "SMI.h"

// user functions (callbacks) for the parser
ok64 SMIonStars (u8cs tok, SMIstate* state);
ok64 SMIonTildes (u8cs tok, SMIstate* state);
ok64 SMIonCode (u8cs tok, SMIstate* state);
ok64 SMIonWord (u8cs tok, SMIstate* state);
ok64 SMIonSpace (u8cs tok, SMIstate* state);
ok64 SMIonOpen (u8cs tok, SMIstate* state);
ok64 SMIonClose (u8cs tok, SMIstate* state);

%%{

machine SMI;

alphtype unsigned char;

ws = [ \t];
wc = [^\n \t*`~\[\]];

main := |*
    "*"+ => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonStars(tok, state);
        if (o != OK) goto _out;
    };
    "~"+ => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonTildes(tok, state);
        if (o != OK) goto _out;
    };
    "`" => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonCode(tok, state);
        if (o != OK) goto _out;
    };
    "[" => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonOpen(tok, state);
        if (o != OK) goto _out;
    };
    "]" => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonClose(tok, state);
        if (o != OK) goto _out;
    };
    wc+ => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonWord(tok, state);
        if (o != OK) goto _out;
    };
    ws+ => {
        tok[0] = ts;
        tok[1] = te;
        o = SMIonSpace(tok, state);
        if (o != OK) goto _out;
    };
*|;

}%%

%%write data;

// the public API function
ok64 SMILexer(SMIstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    int act = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u8c *ts = NULL;
    u8c *te = NULL;
    ok64 o = OK;

    u8cs tok = {p, p};

    %% write init;
    %% write exec;

    state->data[0] = p;
    if (o==OK && cs < SMI_first_final)
        o = SMIBAD;

    return o;
}
