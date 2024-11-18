alpha = [0-9A-Za-z];
ws = [ \t\n\r];
any = [0-0xff];
nonws = [^ \t\n\r];
punkt = [,.;:!?\-"'()"];
wsp = ws | punkt;
word = nonws +;
words = word ( ws+ word )*;

Ref0 = wsp "[" nonws;
Ref1 = nonws "][" alpha "]";

Em0 = wsp "_" nonws;
Em1 = nonws "_" wsp;
Em = "_" (word ws+)* word? (nonws-"\\") :>> "_";

Code01 = [^\\] "`";

St0 = wsp "*" nonws;
St1 = [^\t\r\n *] "*";
St = "*" (word ws+)* word? (nonws-"\\") :>> "*";

inline = words | Em0 | Em1 | Em | St0 | St1 | Ref0 | Ref1 | Code01;
Root = (ws* inline)* ws*;
