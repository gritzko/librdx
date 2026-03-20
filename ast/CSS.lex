ws       = [ \t];
ident    = [a-zA-Z_] [a-zA-Z0-9_]*;
number   = [0-9]+;

Ident    = ident;
Dot      = ".";
Star     = "*";
Child    = ">";
Adjacent = "+";
Sibling  = "~";
Has      = ":has(";
Not      = ":not(";
Close    = ")";
Line     = "L" number ("-" number)?;

punct = Dot | Star | Child | Adjacent | Sibling | Has | Not | Close;
first = Ident | Line;
sep   = ws* punct ws* | ws+;

Root = ws* first? (sep first?)* ws*;
