   # LEX

LEX is essentially a boilerplate generator for [Ragel][r].
Ragel is an immensely useful and highly performant parser generator for regular languages.
If your grammar can be parsed with a regex, Ragel alone can manage that.
For an example, see the [URI parser][U].
The typical use though is to use Ragel as a lexer tokenizing the byte stream.
The higher-level parser is built on top of that.
For an example, see the [JSON parser][J].

As of now, LEX supports C and Go.

[r]: http://www.colm.net/open-source/ragel
[U]: ./URI.md
[J]: ./JSON.md
