# tok/ — Lightweight ragel-based tokenizers

Produces the same tag classification as ast/ tree-sitter parsers
(D=comment, G=string, L=number, R=keyword, P=punct, H=preproc, S=default)
but without the tree-sitter dependency.

## Headers

| Header  | Purpose |
|---------|---------|
| TOK.h   | Common callback typedef `TOKcb`, dispatch API `TOKLexer()` |
| CT.h    | C tokenizer: `CTstate`, `CTLexer()` |
| GOT.h   | Go tokenizer: `GOTstate`, `GOTLexer()` |
| PYT.h   | Python tokenizer: `PYTstate`, `PYTLexer()` |
| JST.h   | JavaScript tokenizer: `JSTstate`, `JSTLexer()` |
| RST.h   | Rust tokenizer: `RSTstate`, `RSTLexer()` |
| JAT.h   | Java tokenizer: `JATstate`, `JATLexer()` |
| CPPT.h  | C++ tokenizer: `CPPTstate`, `CPPTLexer()` |
| CST.h   | C# tokenizer: `CSTstate`, `CSTLexer()` |
| HTMT.h  | HTML tokenizer: `HTMTstate`, `HTMTLexer()` |
| CSST.h  | CSS tokenizer: `CSSTstate`, `CSSTLexer()` |
| JSONT.h | JSON tokenizer: `JSONTstate`, `JSONTLexer()` |
| SHT.h   | Bash tokenizer: `SHTstate`, `SHTLexer()` |
| RBT.h   | Ruby tokenizer: `RBTstate`, `RBTLexer()` |
| HST.h   | Haskell tokenizer: `HSTstate`, `HSTLexer()` |
| MLT.h   | OCaml tokenizer: `MLTstate`, `MLTLexer()` |
| JLT.h   | Julia tokenizer: `JLTstate`, `JLTLexer()` |
| PHPT.h  | PHP tokenizer: `PHPTstate`, `PHPTLexer()` |
| AGDT.h  | Agda tokenizer: `AGDTstate`, `AGDTLexer()` |
| VERT.h  | Verilog tokenizer: `VERTstate`, `VERTLexer()` |

## Extension dispatch (TOK.c)

| Extension(s)                    | Tokenizer |
|---------------------------------|-----------|
| c h rl                          | CT        |
| cpp cc cxx hpp hh hxx           | CPPT      |
| go                              | GOT       |
| py                              | PYT       |
| js jsx mjs                      | JST       |
| rs                              | RST       |
| java                            | JAT       |
| cs                              | CST       |
| html htm                        | HTMT      |
| css                             | CSST      |
| json                            | JSONT     |
| sh bash                         | SHT       |
| rb                              | RBT       |
| hs                              | HST       |
| ml mli                          | MLT       |
| jl                              | JLT       |
| php                             | PHPT      |
| agda                            | AGDT      |
| v sv                            | VERT      |

## Tag mapping

| Tag | Meaning | Color |
|-----|---------|-------|
| D   | Comment | gray  |
| G   | String  | green |
| L   | Number  | cyan  |
| H   | Preproc/annotation/pragma | pink |
| R   | Keyword | red   |
| P   | Punctuation | gray |
| S   | Default (identifier, whitespace) | default |

## Regenerating

```sh
cd tok/
ragel -C XX.c.rl -o XX.rl.c -L
```

where XX is the module prefix (CT, GOT, PYT, JST, RST, JAT, etc).
