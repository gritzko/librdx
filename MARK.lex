_a = [0-0xff];

HLine = "----";
Indent = "    ";

OList = 
  [0-9] ".  " | " " [0-9] ". " | "  " [0-9] "." |
  [0-9]{2} ". " | " " [0-9]{2} "." |
  [0-9]{3} ".";
UList = "-   " | " -  " | "  - " | "   -";

H1 = "#   " | " #  " | "  # " | "   #";
H2 = "##  " | " ## " | "  ##";
H3 = "### " | " ###";
H4 = "####"; 
H = H1 | H2 | H3 | H4;

lndx = [0-9A-Za-z];
Link = "[" lndx "]:";

nest = Indent | OList | UList;
term = HLine | H1 | H2 | H3 | H4 | Link;
Div = nest* term?;

Line = Div <: [^\n]* "\n";

Root = Line*;
