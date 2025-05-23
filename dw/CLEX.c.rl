#include "abc/INT.h"
#include "abc/PRO.h"
#include "CLEX.h"

// action indices for the parser
#define CLEXenum 0
enum {
	CLEXToken = CLEXenum+40,
	CLEXRoot = CLEXenum+41,
};

// user functions (callbacks) for the parser
ok64 CLEXonToken ($cu8c tok, CLEXstate* state);
ok64 CLEXonRoot ($cu8c tok, CLEXstate* state);



%%{

machine CLEX;

alphtype unsigned char;

# ragel actions
action CLEXToken0 { mark0[CLEXToken] = p - text[0]; }
action CLEXToken1 {
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        fbreak;
    }
}
action CLEXRoot0 { mark0[CLEXRoot] = p - text[0]; }
action CLEXRoot1 {
    tok[0] = text[0] + mark0[CLEXRoot];
    tok[1] = p;
    o = CLEXonRoot(tok, state); 
    if (o!=OK) {
        fbreak;
    }
}

# ragel grammar rules
CLEXoctal_digit = (   [0-7] ); # no octal_digit callback
CLEXdecimal_constant = (   [1-9]  [0-9]* ); # no decimal_constant callback
CLEXoctal_constant = (   "0"  CLEXoctal_digit* ); # no octal_constant callback
CLEXhexadecimal_digit = (   [0-9a-fA-F] ); # no hexadecimal_digit callback
CLEXhexadecimal_digit_sequence = (   [0-9a-fA-F]+ ); # no hexadecimal_digit_sequence callback
CLEXhexadecimal_constant = (   "0"  [xX]  CLEXhexadecimal_digit_sequence ); # no hexadecimal_constant callback
CLEXunsigned_suffix = (   [uU] ); # no unsigned_suffix callback
CLEXlong_suffix = (   [lL] ); # no long_suffix callback
CLEXlong_long_suffix = (   "ll"  |  "LL" ); # no long_long_suffix callback
CLEXinteger_suffix = (  
        CLEXunsigned_suffix  CLEXlong_suffix?  | 
        CLEXunsigned_suffix  CLEXlong_long_suffix  | 
        CLEXlong_suffix  CLEXunsigned_suffix?  | 
        CLEXlong_long_suffix  CLEXunsigned_suffix?   ); # no integer_suffix callback
CLEXinteger_constant = (   (  CLEXdecimal_constant  |  CLEXoctal_constant  |  CLEXhexadecimal_constant  )  CLEXinteger_suffix?   ); # no integer_constant callback
CLEXdigit_sequence = (   [0-9]+ ); # no digit_sequence callback
CLEXfractional_constant = (  
        CLEXdigit_sequence?  "."  CLEXdigit_sequence  | 
        CLEXdigit_sequence  "." ); # no fractional_constant callback
CLEXsign = (   [+\-] ); # no sign callback
CLEXexponent_part = (   [eE]  CLEXsign?  CLEXdigit_sequence ); # no exponent_part callback
CLEXfloating_suffix = (   [FLfl] ); # no floating_suffix callback
CLEXdecimal_floating_constant = (  
        CLEXfractional_constant  CLEXexponent_part?  CLEXfloating_suffix?  | 
        CLEXdigit_sequence  CLEXexponent_part  CLEXfloating_suffix? ); # no decimal_floating_constant callback
CLEXhexadecimal_fractional_constant = (  
        CLEXhexadecimal_digit_sequence?  "."  CLEXhexadecimal_digit_sequence  | 
        CLEXhexadecimal_digit_sequence  "." ); # no hexadecimal_fractional_constant callback
CLEXbinary_exponent_part = (   [pP]  CLEXsign?  CLEXdigit_sequence ); # no binary_exponent_part callback
CLEXhexadecimal_prefix = (   "0x" ); # no hexadecimal_prefix callback
CLEXhexadecimal_floating_constant = (  
        CLEXhexadecimal_prefix  CLEXhexadecimal_fractional_constant 
        CLEXbinary_exponent_part  CLEXfloating_suffix?  | 
        CLEXhexadecimal_prefix  CLEXhexadecimal_digit_sequence 
        CLEXbinary_exponent_part  CLEXfloating_suffix? ); # no hexadecimal_floating_constant callback
CLEXfloating_constant = (   CLEXdecimal_floating_constant  |  CLEXhexadecimal_floating_constant ); # no floating_constant callback
CLEXidentifier = (   [a-zA-Z_$]  [0-9a-zA-Z_$]* ); # no identifier callback
CLEXenum_constant = (   CLEXidentifier ); # no enum_constant callback
CLEXhexadecimal_escape_sequence = (   "\\x"  CLEXhexadecimal_digit+ ); # no hexadecimal_escape_sequence callback
CLEXoctal_escape_sequence = (   "\\"  CLEXoctal_digit{1,3} ); # no octal_escape_sequence callback
CLEXsimple_escape_sequence = (   "\\"  "\\" ['"['"?\\abfnrtv] ); # no simple_escape_sequence callback
CLEXescape_sequence = (   CLEXsimple_escape_sequence  | 
        CLEXoctal_escape_sequence  | 
        CLEXhexadecimal_escape_sequence   ); # no escape_sequence callback
CLEXc_char = (   [^'\\\n]  |  CLEXescape_sequence ); # no c_char callback
CLEXc_char_sequence = (   CLEXc_char+ ); # no c_char_sequence callback
CLEXcharacter_constant = (  
        "'"  CLEXc_char_sequence  "'"  | 
        "L'"  CLEXc_char_sequence  "'" ); # no character_constant callback
CLEXconstant = (   CLEXinteger_constant  |  CLEXfloating_constant  |  CLEXenum_constant  |  CLEXcharacter_constant ); # no constant callback
CLEXs_char = (   [^"\\\n]  |  CLEXescape_sequence ); # no s_char callback
CLEXs_char_sequence = (   CLEXs_char+ ); # no s_char_sequence callback
CLEXstring_literal = (  
        "L"?  ["]  CLEXs_char_sequence?  ["]   ); # no string_literal callback
CLEXpunctuator = (   [\[\]()[\[\](){}\.&*+-~!/%><^|?:;=,#]  |  "->"  |  "++"  |  "--"  |  "<<"  |  ">>"  |   
                        "<="  |  ">="  |  "=="  |  "!="  |  "&&"  |  "||"  |  "..."  |  "*="  | 
                        "/="  |  "%="  |  "+="  |  "-="  |  "<<="  |  ">>="  |  "&="  |  "^="  |  "|="  |  "##"  | 
                        "<:"  |  ":>"  |  "<%"  |  "%>"  |  "%:"  |  "%:%:"   ); # no punctuator callback
CLEXcomment = (   "//"  [^\n]* ); # no comment callback
CLEXkeyword = (  
        "auto"  | 
        "break"  | 
        "case"  | 
        "char"  | 
        "const"  | 
        "continue"  | 
        "default"  | 
        "do"  | 
        "double"  | 
        "else"  | 
        "enum"  | 
        "extern"  | 
        "float"  | 
        "for"  | 
        "goto"  | 
        "if"  | 
        "inline"  | 
        "int"  | 
        "long"  | 
        "register"  | 
        "restrict"  | 
        "return"  | 
        "short"  | 
        "signed"  | 
        "sizeof"  | 
        "static"  | 
        "struct"  | 
        "switch"  | 
        "typedef"  | 
        "union"  | 
        "unsigned"  | 
        "void"  | 
        "volatile"  | 
        "while"  | 
        "_Bool"  | 
        "_Complex"  | 
        "_Imaginary" ); # no keyword callback
CLEXws = (   [ \t\r\n]+ ); # no ws callback
CLEXToken = (   CLEXkeyword  |  CLEXidentifier  |  CLEXconstant  |  CLEXstring_literal  |  CLEXpunctuator  |  CLEXcomment )  >CLEXToken0 %CLEXToken1;
CLEXRoot = (   (CLEXws  |  CLEXToken)** )  >CLEXRoot0 %CLEXRoot1;

main := CLEXRoot;

}%%

%%write data;

// the public API function
ok64 CLEXlexer(CLEXstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    state->text[0] = p;
    if (p!=text[1] || cs < CLEX_first_final || o!=OK) {
        return CLEXfail;
    }
    return o;
}
