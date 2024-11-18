#include "JSON.rl.h"


%%{

machine JSON;

alphtype unsigned char;

action JSONLiteral0 { mark0[JSONLiteral] = p - text[0]; }
action JSONLiteral1 {
    tok[0] = text[0] + mark0[JSONLiteral];
    tok[1] = p;
    call(JSONonLiteral, tok, state); 
}
action JSONString0 { mark0[JSONString] = p - text[0]; }
action JSONString1 {
    tok[0] = text[0] + mark0[JSONString];
    tok[1] = p;
    call(JSONonString, tok, state); 
}
action JSONNumber0 { mark0[JSONNumber] = p - text[0]; }
action JSONNumber1 {
    tok[0] = text[0] + mark0[JSONNumber];
    tok[1] = p;
    call(JSONonNumber, tok, state); 
}
action JSONOpenObject0 { mark0[JSONOpenObject] = p - text[0]; }
action JSONOpenObject1 {
    tok[0] = text[0] + mark0[JSONOpenObject];
    tok[1] = p;
    call(JSONonOpenObject, tok, state); 
}
action JSONCloseObject0 { mark0[JSONCloseObject] = p - text[0]; }
action JSONCloseObject1 {
    tok[0] = text[0] + mark0[JSONCloseObject];
    tok[1] = p;
    call(JSONonCloseObject, tok, state); 
}
action JSONOpenArray0 { mark0[JSONOpenArray] = p - text[0]; }
action JSONOpenArray1 {
    tok[0] = text[0] + mark0[JSONOpenArray];
    tok[1] = p;
    call(JSONonOpenArray, tok, state); 
}
action JSONCloseArray0 { mark0[JSONCloseArray] = p - text[0]; }
action JSONCloseArray1 {
    tok[0] = text[0] + mark0[JSONCloseArray];
    tok[1] = p;
    call(JSONonCloseArray, tok, state); 
}
action JSONComma0 { mark0[JSONComma] = p - text[0]; }
action JSONComma1 {
    tok[0] = text[0] + mark0[JSONComma];
    tok[1] = p;
    call(JSONonComma, tok, state); 
}
action JSONColon0 { mark0[JSONColon] = p - text[0]; }
action JSONColon1 {
    tok[0] = text[0] + mark0[JSONColon];
    tok[1] = p;
    call(JSONonColon, tok, state); 
}
action JSONJSON0 { mark0[JSONJSON] = p - text[0]; }
action JSONJSON1 {
    tok[0] = text[0] + mark0[JSONJSON];
    tok[1] = p;
    call(JSONonJSON, tok, state); 
}
action JSONRoot0 { mark0[JSONRoot] = p - text[0]; }
action JSONRoot1 {
    tok[0] = text[0] + mark0[JSONRoot];
    tok[1] = p;
    call(JSONonRoot, tok, state); 
}

JSONws  = (   [\r\n\t ] );

JSONhex  = (   [0-9a-fA-F] );


JSONLiteral  = (   "true"  |  "false"  |  "null" )  >JSONLiteral0 %JSONLiteral1;


JSONsafeCP  = (   (0x20..0xff)  -  ["\\] );

JSONesc  = (   "\\"  ["\\/bfnrt] );

JSONuEsc  = (     "\\u"  JSONhex{4} );

JSONcp  = (   JSONsafeCP  |  JSONesc  |  JSONuEsc );


JSONString  = (   ["]  JSONcp*  ["] )  >JSONString0 %JSONString1;


JSONNumber  = (   (  [\-]?  (  [0]  |  [1-9]  [0-9]*  )

                        ("."  [0-9]+)?

                        ([eE]  [\-+]?  [0-9]+  )?

                        ) )  >JSONNumber0 %JSONNumber1;


JSONOpenObject  = (   "{" )  >JSONOpenObject0 %JSONOpenObject1;

JSONCloseObject  = (   "}" )  >JSONCloseObject0 %JSONCloseObject1;


JSONOpenArray  = (   "[" )  >JSONOpenArray0 %JSONOpenArray1;

JSONCloseArray  = (   "]" )  >JSONCloseArray0 %JSONCloseArray1;


JSONComma  = (   "," )  >JSONComma0 %JSONComma1;


JSONColon  = (   ":" )  >JSONColon0 %JSONColon1;


JSONdelimiter  = (   JSONOpenObject  |  JSONCloseObject  |  JSONOpenArray  |  JSONCloseArray  |  JSONComma  |  JSONColon );


JSONprimitive  = (   JSONNumber  |  JSONString  |  JSONLiteral );


JSONJSON  = (   JSONws*  (  JSONprimitive?  (  JSONws*  JSONdelimiter  JSONws*  JSONprimitive?  )*  )  JSONws* )  >JSONJSON0 %JSONJSON1;


JSONRoot  = (   JSONJSON )  >JSONRoot0 %JSONRoot1;

main := JSONRoot;

}%%

%%write data;

pro(JSONlexer, JSONstate* state) {

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

    if (p!=text[1] || cs < JSON_first_final) {
        state->text[0] = p;
        fail(JSONfail);
    }
    done;
}
