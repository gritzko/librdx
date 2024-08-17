Space = [ \t\r\n] ;
Name = [A-Za-z_] [A-Z0-9a-z_]** ;
Rep = "{" [0-9]* ("," [0-9]*)? "}" - "{}";
Op = Space | [()+*\-?><:|] | Rep;
Class = "[" ([^\]]|"\\]")* "]" ;
String = "\"" ([^"]|"\\\"")* "\"" ;
Entity = Class | Name | String ;
Expr = (Op+ Entity)* Op* ;
RuleName = Name ;
Eq = Space* "=";
Line = Space* RuleName Eq Expr ";" ;
Root = Line* Space* ;

