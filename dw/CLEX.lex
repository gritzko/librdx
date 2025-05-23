octal_digit = [0-7];
decimal_constant = [1-9] [0-9]*;
octal_constant = "0" octal_digit*;
hexadecimal_digit = [0-9a-fA-F];
hexadecimal_digit_sequence = [0-9a-fA-F]+;
hexadecimal_constant = "0" [xX] hexadecimal_digit_sequence;

unsigned_suffix = [uU];
long_suffix = [lL];
long_long_suffix = "ll" | "LL";
integer_suffix =
    unsigned_suffix long_suffix? |
    unsigned_suffix long_long_suffix |
    long_suffix unsigned_suffix? |
    long_long_suffix unsigned_suffix? ;
integer_constant = ( decimal_constant | octal_constant | hexadecimal_constant ) integer_suffix? ;
digit_sequence = [0-9]+;
fractional_constant =
    digit_sequence? "." digit_sequence |
    digit_sequence ".";
sign = [+\-];
exponent_part = [eE] sign? digit_sequence;
floating_suffix = [FLfl];
decimal_floating_constant =
    fractional_constant exponent_part? floating_suffix? |
    digit_sequence exponent_part floating_suffix?;
hexadecimal_fractional_constant =
    hexadecimal_digit_sequence? "." hexadecimal_digit_sequence |
    hexadecimal_digit_sequence ".";
binary_exponent_part = [pP] sign? digit_sequence;
hexadecimal_prefix = "0x";
hexadecimal_floating_constant =
    hexadecimal_prefix hexadecimal_fractional_constant
    binary_exponent_part floating_suffix? |
    hexadecimal_prefix hexadecimal_digit_sequence
    binary_exponent_part floating_suffix?;
floating_constant = decimal_floating_constant | hexadecimal_floating_constant;
identifier = [a-zA-Z_$] [0-9a-zA-Z_$]*;
enum_constant = identifier;
hexadecimal_escape_sequence = "\\x" hexadecimal_digit+;
octal_escape_sequence = "\\" octal_digit{1,3};
simple_escape_sequence = "\\" ['"?\\abfnrtv];
escape_sequence  = simple_escape_sequence |
    octal_escape_sequence |
    hexadecimal_escape_sequence ;
c_char = [^'\\\n] | escape_sequence;
c_char_sequence = c_char+;
character_constant =
    "'" c_char_sequence "'" |
    "L'" c_char_sequence "'";
constant = integer_constant | floating_constant | enum_constant | character_constant;
s_char = [^"\\\n] | escape_sequence;
s_char_sequence = s_char+;
string_literal =
    "L"? ["] s_char_sequence? ["] ;
punctuator = [\[\](){}\.&*+-~!/%><^|?:;=,#] | "->" | "++" | "--" | "<<" | ">>" | 
            "<=" | ">=" | "==" | "!=" | "&&" | "||" | "..." | "*=" |
            "/=" | "%=" | "+=" | "-=" | "<<=" | ">>=" | "&=" | "^=" | "|=" | "##" |
            "<:" | ":>" | "<%" | "%>" | "%:" | "%:%:" ;

comment = "//" [^\n]*;

keyword =
    "auto" |
    "break" |
    "case" |
    "char" |
    "const" |
    "continue" |
    "default" |
    "do" |
    "double" |
    "else" |
    "enum" |
    "extern" |
    "float" |
    "for" |
    "goto" |
    "if" |
    "inline" |
    "int" |
    "long" |
    "register" |
    "restrict" |
    "return" |
    "short" |
    "signed" |
    "sizeof" |
    "static" |
    "struct" |
    "switch" |
    "typedef" |
    "union" |
    "unsigned" |
    "void" |
    "volatile" |
    "while" |
    "_Bool" |
    "_Complex" |
    "_Imaginary";
ws = [ \t\r\n]+;
Token = keyword | identifier | constant | string_literal | punctuator | comment;
Root = (ws | Token)**;
