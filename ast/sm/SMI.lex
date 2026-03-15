ws = [ \t];
wc = [^\n \t*`~];

Stars = "*"+;
Tildes = "~"+;
Code = "`";
Word = wc+;
Space = ws+;

token = Stars | Code | Word | Space | Tildes;
Root = token*;
