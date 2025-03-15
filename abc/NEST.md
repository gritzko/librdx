#   NEST

Virtually every programming environment has some formatted output API.
At the very least, they must allow for messages like "%i bytes sent".
Some environments, like PHP, *are* huge formatted output APIs.
The `printf` family of functions is an example of a small but still
immensely useful implementation. ABC adapts that as `$printf`.

Still, there is a need for a more sophisticated templated output API.
For example, [LEX][L] code generation needs one. A common line of thinking
is to make up a scripting language and make an interpreter for that.
The other one is to interleave the code with template chunks using
some bracket notation, then pre-compile that into normal code. Of course,
there is no need to invent these things, as we can get something by
invoking our favorite package manager. Still, that is a rather fat
dependency for a low-level system library. It is also fun to consider
that ABC needs it as a tool for the LEX/ragel parser generator combo.
Chaining three code generators does not seem wise. Also, the task of
string substitution is a modest one and [ABC][A] commandments tell us that
the code should be small, simple, orthogonal and arbitrarily composable
with the rest of ABC.

Long story short, it takes [150 lines][n] to implement a basic templating
engine on top of a plain ABC byte buffer. NEST stands for "nested templates".
Yes, differently from PHP, bash and others, a variable can be substituted
for a template that will be recursively subjected to further substitutions.
Despite that NEST is a single-pass algorithm. Yes, minimalism.
````
    NESTfeed(nestbuf, a$str("Next waypoint: $LOC. Reach by: $TIME."));
    NESTsplice(nestbuf, LOC);
    if (!$empty(locname)) {
        $u8feed(nestidle, locname);
    } else {
        NESTfeed(nestbuf, "coordinates ${LAT}:${LONG}");
    }
    //...
````

While NEST allows for [complex][c] templated output, there is a huge
advantage, relative to PHP and other options, that we do not need to reinvent
control flow or to make any new API bindings. Everything is achieved
by *reusing* the existing `$u8feedX` family of ABC byte-buffer routines
or by employing any other C API that can write into a byte buffer.

Shortcomings? NEST is not Turing-complete. It cannot stream data (as of now).

API is rather basic:

  - `NESTfeed(ct, template)` parses the template fed to the buffer and
    remembers the insertion points, like the one in `"$BYTES bytes sent"`.
    Point names are stored as u64, hence limited to 10 Base64 chars.
  - Later, we can invoke `NESTsplice(ct, BYTES)` and all the following
    output will be spliced into that insertion point, including any
    `NESTfeed` templates. `NESTspliceall` and `NESTspliceany` do multiple
    substitutions; `...all` returns an error if none is found.
  - `NESTrender(into, ct)` produces the resulting text. As NEST is
    using the usual ABC buffers, they can be saved and cloned at any
    point in time.

[L]: ./LEX.md
[A]: ./README.md
[c]: ./LEX.c
[n]: ./NEST.c
