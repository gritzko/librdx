_a = [0-0xff];

hline = "----";
indent = "    ";

olist = 
  [0-9] ".  " | " " [0-9] ". " | "  " [0-9] "." |
  [0-9]{2} ". " | " " [0-9]{2} "." |
  [0-9]{3} ".";
ulist = "-   " | " -  " | "  - " | "   -";

h1 = "#   " | " #  " | "  # " | "   #";
h2 = "##  " | " ## " | "  ##";
h3 = "### " | " ###";
h4 = "####"; 
h = h1 | h2 | h3 | h4;

lndx = [0-9A-Za-z];
link = "[" lndx "]:";

nest = indent | olist | ulist;
term = hline | h1 | h2 | h3 | h4 | link;
div = nest* term?;

line = div <: [^\n]* "\n";

root = line*;
