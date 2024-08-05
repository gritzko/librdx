space = [ \t\r\n] ;
name = [A-Za-z] [A-Z0-9a-z_]** ;
op = space | [()+*\-?|] ;
class = "[" ([^\]]|"\\]")* "]" ;
string = "\"" ([^"]|"\\\"")* "\"" ;
entity = class | name | string ;
expr = (op+ entity)* op* ;
rulename = name ;
eq = space* "=";
line = rulename eq expr ";" space* ;
root = line* ;

