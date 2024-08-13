_a = [0-9A-Za-z];
_sp = [ \t];
_nl = "\n";
_nonl = [^\n];
plain = [^\n\r \t]+;
ref = "[" plain "][" _a "]";
tostress = plain :>> ref;
stress = "*" tostress (_sp+ tostress)* "*";
toemph = plain :>> (stress | ref);
emph = "_" toemph (_sp+ toemph*) "_";
inline = plain :>> (ref);

root = inline*;
