#   MARK: deterministic Markdown dialect

The MARK is a minimalistic markup format based on [Markdown][m].
Markup languages have a long and boring history; we can 
mention `man`, `TeX` or even `HTML` that now requires some 
million lines of code to properly parse and display itself.
The huge upside of `Markdown` is that it is simple and intuitive.
The key downside is ambiguity and incompatibility of parsers.

MARK started as a subset of Markdown with a formal grammar,
hence zero ambiguity. If two MARK parsers are incompatible,
that is a bug in at least one of them. MARK is made to tolerate
mishaps to some degree, such as merges of concurrent edits.

MARK parsing rules are made to be easily implemented with 
regular expressions of any kind.

##  Principles for the formal grammar

There is a legendary unsolvable computer science problem:
should we use spaces or tabs? The logic of MARK is to have 
only one way to do something. So, can spaces replace tabs?
Yes. Can tabs replace spaces? No. Hence, spaces!

That logic also applies to the MARK's way of minimizing 
Markdown: there must be one format for headers, one format 
for bullet lists, etc. The overlap between markup elements 
must absolutely be minimized. For example, using `*` both
for emphasis and lists is a no-no.

To summarize that,

 1. Intuitive syntax,
 2. One way only,
 3. No ambiguity.

##  The formal grammar

MARK formal grammar consists of two orthogonal parts that can
and should be parsed separately: block-level and inline. All
block-level formatting goes in the beginning of a line, four
characters per one level of nesting - be it 

  - numbered lists ` 1. `,
  - bulleted lists `  - `,
  - block quotes ` >  `,
  - code blocks `\`\`\`\``,
  - headers (four levels ` #  ` ` ## ` `### ` `####`),
  - hyperlinks `[a]:`,
  - or suchlike.

Block level elements can allow for arbitrary nesting
(lists, block quotes), only allow for inline formatting 
inside them (headers) or no formatting at all (code, links).

Inline elements are either one-piece or *bracket-like pairs*.
Either way, the syntax of the element is defined by a regular 
expression. Bracket-matching algorithm is the same for all 
bracketed elements, be it emphasis, strong emphasis, inline 
code, or hyperlinks.

##  CommonMark critique

[CommonMark][c] was a notable effort to standardize Markdown.
After 10 years, the spec is still changing and there is no 
formal grammar, only a human-language description.
As the [CommonMark spec][c] says, 

 >  la la la

That precedent-based intuitive syntax of Markdown is likely the problem here. 
If we implement a Markdown parser with neural nets, that approach might 
be OK. Except that another neural net might see things differently.
Instead, MARK is based on very clear formal rules that are easy
to understand, remember and implement in code.

[c]: http://commonmark.org
[m]: fireball
