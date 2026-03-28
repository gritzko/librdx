# tok/ — Lightweight ragel-based tokenizers

Fast, dependency-free tokenizers for syntax highlighting. Each tokenizer is a
hand-written [Ragel](http://www.colm.net/open-source/ragel/) scanner that
classifies source bytes into tagged tokens: **D**=comment, **G**=string,
**L**=number, **R**=keyword, **P**=punct, **H**=preproc, **S**=default.

Produces output compatible with the `ast/` tree-sitter based tokenizers,
but without the tree-sitter dependency (~100KB static lib vs ~10MB).

## Supported languages

| Module | Language   | Extensions              |
|--------|-----------|-------------------------|
| CT     | C         | `.c` `.h` `.rl`         |
| CPPT   | C++       | `.cpp` `.cc` `.cxx` `.hpp` `.hh` `.hxx` |
| GOT    | Go        | `.go`                   |
| PYT    | Python    | `.py`                   |
| JST    | JavaScript| `.js` `.jsx` `.mjs`     |
| RST    | Rust      | `.rs`                   |
| JAT    | Java      | `.java`                 |
| CST    | C#        | `.cs`                   |
| HTMT   | HTML      | `.html` `.htm`          |
| CSST   | CSS       | `.css`                  |
| JSONT  | JSON      | `.json`                 |
| SHT    | Bash      | `.sh` `.bash`           |
| RBT    | Ruby      | `.rb`                   |
| HST    | Haskell   | `.hs`                   |
| MLT    | OCaml     | `.ml` `.mli`            |
| JLT    | Julia     | `.jl`                   |
| PHPT   | PHP       | `.php`                  |
| AGDT   | Agda      | `.agda`                 |
| VERT   | Verilog   | `.v` `.sv`              |

## Usage

### CLI tool

```sh
toktok file.go          # prints TAG\tTOKEN per line
toktok file.py
toktok file.rs
```

### C API

```c
#include "TOK.h"

ok64 my_callback(u8 tag, u8cs tok, void *ctx) {
    // tag: 'D','G','L','R','P','H','S'
    // tok: [start, end) byte slice
    return OK;
}

// Dispatch by extension
TOKstate state = {
    .data = {src_start, src_end},
    .cb = my_callback,
    .ctx = NULL,
};
u8csc ext = {(u8c*)"py", (u8c*)"py" + 2};
ok64 o = TOKLexer(&state, ext);

// Or call a specific lexer directly
GOTstate go = {
    .data = {src_start, src_end},
    .cb = my_callback,
    .ctx = NULL,
};
ok64 o = GOTLexer(&go);
```

## Architecture

Each language tokenizer follows the same pattern:

- **XX.h** — Public header: state typedef, lexer declaration, error codes
- **XX.c** — Keyword table + callback functions (tag mapping)
- **XX.c.rl** — Ragel scanner source (hand-written `|* ... *|` scanner)
- **XX.rl.c** — Generated C (do not edit; regenerate with `ragel -C XX.c.rl -o XX.rl.c -L`)

The dispatcher **TOK.c** routes file extensions to the appropriate lexer.

## Tag mapping

| Tag | Meaning                 | ANSI Color |
|-----|------------------------|------------|
| D   | Comment                | gray       |
| G   | String literal         | green      |
| L   | Number literal         | cyan       |
| H   | Preprocessor/annotation| pink       |
| R   | Keyword                | red        |
| P   | Punctuation            | gray       |
| S   | Default (id/whitespace)| default    |

## Regenerating ragel output

```sh
cd tok/
ragel -C CT.c.rl -o CT.rl.c -L
ragel -C GOT.c.rl -o GOT.rl.c -L
# etc.
```

## Testing

```sh
ninja -C build-debug tok/test/TOK01test
build-debug/tok/test/TOK01test
```

Compare with tree-sitter output:
```sh
build-debug/tok/toktok sample.go > /tmp/tok.out
build-debug/ast/astok sample.go > /tmp/ast.out
diff /tmp/tok.out /tmp/ast.out
```
