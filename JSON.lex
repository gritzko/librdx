ws = [\r\n\t ];
hex = [0-9a-fA-F];

Literal = "true" | "false" | "null";

safeCP = (0x20..0xff) - ["\\];
esc = "\\" ["\\/bfnrt];
uEsc =  "\\u" hex{4};
cp = safeCP | esc | uEsc;

String = ["] cp* ["];

Number = ( [\-]? ( [0] | [1-9] [0-9]* )
            ("." [0-9]+)?
            ([eE] [\-+]? [0-9]+ )?
            );

OpenObject = "{";
CloseObject = "}";

OpenArray = "[";
CloseArray = "]";

Comma = ",";

Colon = ":";

delimiter = OpenObject | CloseObject | OpenArray | CloseArray | Comma | Colon;

primitive = Number | String | Literal;

JSON = ws* ( primitive? ( ws* delimiter ws* primitive? )* ) ws*;

Root = JSON;
