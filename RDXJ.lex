ws = [\r\n\t ];
hex = [0-9a-fA-F];

cp = (0x20..0xff) - ["\\];
utf8 = cp+;
esc = [\\] ["\\/bfnrt];
hexEsc =  "\\u" hex{4};
utf8esc = utf8 | esc | hexEsc;

id128 = [0-9a-fA-F]+ "-" [0-9a-fA-F]+;

Int = [\-]? ( [0] | [1-9] [0-9]* );
Float = ( [\-]? ( [0] | [1-9] [0-9]* )
            ("." [0-9]+)?
            ([eE] [\-+]? [0-9]+ )? ) - Int;
Ref = id128 - Float;
String = ["] utf8esc* ["];
Term = [a-zA-Z0-9_~]+ -Int -Float;

Stamp = "@" id128;

OpenP = "<";
CloseP = ">";
OpenL = "[";
CloseL = "]";
OpenE = "{";
CloseE = "}";
OpenX = "(";
CloseX = ")";

Comma = ",";
Colon = ":";

Open = (OpenP | OpenL | OpenE | OpenX) ws* (Stamp ws*)?;
Close = (CloseP | CloseL | CloseE | CloseX) ws*;
Inter = (Comma | Colon) ws*;

delim = Open | Close | Inter;

FIRST = ( Float | Int | Ref | String | Term ) (ws* Stamp)? ws*;

Root = ws* ( FIRST? ( delim <: FIRST? )* ) ;
