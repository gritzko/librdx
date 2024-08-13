space = [ \t\r\n] ;
name = [A-Za-z_] [A-Z0-9a-z_]** ;
_rep = "{" [0-9]* ("," [0-9]*)? "}" - "{}";
op = space | [()+*\-?><:|] | _rep;
class = "[" ([^\]]|"\\]")* "]" ;
string = "\"" ([^"]|"\\\"")* "\"" ;
entity = class | name | string ;
expr = (op+ entity)* op* ;
rulename = name ;
eq = space* "=";
line = space* rulename eq expr ";" space** ;
root = line* ;

