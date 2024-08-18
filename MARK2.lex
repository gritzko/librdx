alpha = [0-9A-Za-z];
ws = [ \t\n\r];
any = [0-0xff];
nonws = [^ \t\n\r];
punkt = [,.;:!?\-"'()"];
word = nonws +;
words = word ( ws+ word )*;

Ref0 = ws "[" nonws;
Ref1 = nonws "][" alpha "]";

Em = "_" (word ws+)* word? (nonws-"\\") :>> "_";

StA0 = ws "*" nonws;
StA1 = [^\t\r\n *] "*";

inline = words | Em | StA0 | StA1 | Ref0 | Ref1;
Root = (ws* inline)* ws*;
