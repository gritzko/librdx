Space = [ \t\r\n] ;
ws = [ \t\r\n] ;
hex = "0x" [0-9a-f]+;
dec = [0-9]+;
word =  [A-Za-z_] [A-Z0-9a-z_]**;  
Name = word;
Rep = "{" [0-9]* ("," [0-9]*)? "}" - "{}";
Op = Space | [()+*\-?><:|\.] | Rep;
Class = "[" ([^\]]|"\\]")* "]" ;
Range = "(" (hex|dec) ".." (hex|dec) | (hex|dec) ")";
String = "\"" ([^"]|"\\\"")* "\"" ;
QString = "\'" ([^']|"\\\'")* "\'" ;
Entity = Class | Name | String | QString | Range;
Expr = (Op+ Entity)* Op* ;
RuleName = word;
Eq = ws* "=";
Line = ws* RuleName Eq Expr ";" ;
Root = Line* ws* ;

