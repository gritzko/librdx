#include "abc/INT.h"
#include "abc/PRO.h"
#include "JSON.h"

// action indices for the parser
#define JSONenum 0
enum {
	JSONLiteral = JSONenum+7,
	JSONString = JSONenum+8,
	JSONNumber = JSONenum+9,
	JSONOpenObject = JSONenum+10,
	JSONCloseObject = JSONenum+11,
	JSONOpenArray = JSONenum+12,
	JSONCloseArray = JSONenum+13,
	JSONComma = JSONenum+14,
	JSONColon = JSONenum+15,
	JSONJSON = JSONenum+18,
	JSONRoot = JSONenum+19,
};

// user functions (callbacks) for the parser
ok64 JSONonLiteral (u8cs tok, JSONstate* state);
ok64 JSONonString (u8cs tok, JSONstate* state);
ok64 JSONonNumber (u8cs tok, JSONstate* state);
ok64 JSONonOpenObject (u8cs tok, JSONstate* state);
ok64 JSONonCloseObject (u8cs tok, JSONstate* state);
ok64 JSONonOpenArray (u8cs tok, JSONstate* state);
ok64 JSONonCloseArray (u8cs tok, JSONstate* state);
ok64 JSONonComma (u8cs tok, JSONstate* state);
ok64 JSONonColon (u8cs tok, JSONstate* state);
ok64 JSONonJSON (u8cs tok, JSONstate* state);
ok64 JSONonRoot (u8cs tok, JSONstate* state);



%%{

machine JSON;

alphtype unsigned char;

# ragel actions
action JSONLiteral0 { mark0[JSONLiteral] = p - data[0]; }
action JSONLiteral1 {
    tok[0] = data[0] + mark0[JSONLiteral];
    tok[1] = p;
    o = JSONonLiteral(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONString0 { mark0[JSONString] = p - data[0]; }
action JSONString1 {
    tok[0] = data[0] + mark0[JSONString];
    tok[1] = p;
    o = JSONonString(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONNumber0 { mark0[JSONNumber] = p - data[0]; }
action JSONNumber1 {
    tok[0] = data[0] + mark0[JSONNumber];
    tok[1] = p;
    o = JSONonNumber(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONOpenObject0 { mark0[JSONOpenObject] = p - data[0]; }
action JSONOpenObject1 {
    tok[0] = data[0] + mark0[JSONOpenObject];
    tok[1] = p;
    o = JSONonOpenObject(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONCloseObject0 { mark0[JSONCloseObject] = p - data[0]; }
action JSONCloseObject1 {
    tok[0] = data[0] + mark0[JSONCloseObject];
    tok[1] = p;
    o = JSONonCloseObject(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONOpenArray0 { mark0[JSONOpenArray] = p - data[0]; }
action JSONOpenArray1 {
    tok[0] = data[0] + mark0[JSONOpenArray];
    tok[1] = p;
    o = JSONonOpenArray(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONCloseArray0 { mark0[JSONCloseArray] = p - data[0]; }
action JSONCloseArray1 {
    tok[0] = data[0] + mark0[JSONCloseArray];
    tok[1] = p;
    o = JSONonCloseArray(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONComma0 { mark0[JSONComma] = p - data[0]; }
action JSONComma1 {
    tok[0] = data[0] + mark0[JSONComma];
    tok[1] = p;
    o = JSONonComma(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONColon0 { mark0[JSONColon] = p - data[0]; }
action JSONColon1 {
    tok[0] = data[0] + mark0[JSONColon];
    tok[1] = p;
    o = JSONonColon(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONJSON0 { mark0[JSONJSON] = p - data[0]; }
action JSONJSON1 {
    tok[0] = data[0] + mark0[JSONJSON];
    tok[1] = p;
    o = JSONonJSON(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action JSONRoot0 { mark0[JSONRoot] = p - data[0]; }
action JSONRoot1 {
    tok[0] = data[0] + mark0[JSONRoot];
    tok[1] = p;
    o = JSONonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
JSONws = (   [\r\n\t ] ); # no ws callback
JSONhex = (   [0-9a-fA-F] ); # no hex callback
JSONsafeCP = (   (0x20..0xff)  -  ["\\] ); # no safeCP callback
JSONesc = (   "\\"  ["\\/bfnrt] ); # no esc callback
JSONuEsc = (     "\\u"  JSONhex{4} ); # no uEsc callback
JSONcp = (   JSONsafeCP  |  JSONesc  |  JSONuEsc ); # no cp callback
JSONLiteral = (   "true"  |  "false"  |  "null" )  >JSONLiteral0 %JSONLiteral1;
JSONString = (   ["]  JSONcp*  ["] )  >JSONString0 %JSONString1;
JSONNumber = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  ) 
                        ("."  [0-9]+)? 
                        ([eE]  [\-+]?  [0-9]+  )? 
                        ) )  >JSONNumber0 %JSONNumber1;
JSONOpenObject = (   "{" )  >JSONOpenObject0 %JSONOpenObject1;
JSONCloseObject = (   "}" )  >JSONCloseObject0 %JSONCloseObject1;
JSONOpenArray = (   "[" )  >JSONOpenArray0 %JSONOpenArray1;
JSONCloseArray = (   "]" )  >JSONCloseArray0 %JSONCloseArray1;
JSONComma = (   "," )  >JSONComma0 %JSONComma1;
JSONColon = (   ":" )  >JSONColon0 %JSONColon1;
JSONdelimiter = (   JSONOpenObject  |  JSONCloseObject  |  JSONOpenArray  |  JSONCloseArray  |  JSONComma  |  JSONColon ); # no delimiter callback
JSONprimitive = (   JSONNumber  |  JSONString  |  JSONLiteral ); # no primitive callback
JSONJSON = (   JSONws*  (  JSONprimitive?  (  JSONws*  JSONdelimiter  JSONws*  JSONprimitive?  )*  )  JSONws* )  >JSONJSON0 %JSONJSON1;
JSONRoot = (   JSONJSON )  >JSONRoot0 %JSONRoot1;

main := JSONRoot;

}%%

%%write data;

// the public API function
ok64 JSONLexer(JSONstate* state) {

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
    if (o==OK && cs < JSON_first_final) 
        o = JSONBAD;
    
    return o;
}
