ws = [\r\n\t ];
hex = [0-9a-fA-F];

cp = (0x20..0xff) - ["\\];
UTF8 = cp+;
Esc = "\\" ["\\/bfnrt];
HexEsc =  "\\u" hex{4};
utf8esc = UTF8 | Esc | HexEsc;

id128 = [0-9a-fA-F]+ "-" [0-9a-fA-F]+;

Int = [\-]? ( [0] | [1-9] [0-9]* );
Float = ( [\-]? ( [0] | [1-9] [0-9]* )
            ("." [0-9]+)?
            ([eE] [\-+]? [0-9]+ )? ) - Int;
Ref = id128 - Float;
String = ["] utf8esc ["];
Term = [a-zA-Z] [a-zA-Z0-9_]*;

OpenObject = "{";
CloseObject = "}";
OpenArray = "[";
CloseArray = "]";
OpenVector = "(";
CloseVector = ")";

Stamp = "@" id128;

Comma = ",";
Colon = ":";

delimiter = OpenObject | CloseObject |
            OpenArray | CloseArray |
            OpenVector | CloseVector |
            Comma | Colon;

FIRST = ( Float | Int | Ref | String | Term ) (ws* Stamp)? ws*;

Root = ws* ( FIRST? ( delimiter ws* FIRST? )* ) ;
