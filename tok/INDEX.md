# tok/ — Lightweight ragel-based tokenizers

Produces the same tag classification as ast/ tree-sitter parsers
(D=comment, G=string, L=number, R=keyword, P=punct, H=preproc, S=default)
but without the tree-sitter dependency.

## Headers

| Header  | Purpose |
|---------|---------|
| TOK.h   | Common callback typedef `TOKcb`, dispatch API `TOKLexer()` |
| BRCT.h  | Bracket matching and region detection on tokenized files |
| CT.h    | C tokenizer: `CTstate`, `CTLexer()` |
| CPPT.h  | C++ tokenizer: `CPPTstate`, `CPPTLexer()` |
| GOT.h   | Go tokenizer: `GOTstate`, `GOTLexer()` |
| PYT.h   | Python tokenizer: `PYTstate`, `PYTLexer()` |
| JST.h   | JavaScript tokenizer: `JSTstate`, `JSTLexer()` |
| TST.h   | TypeScript tokenizer: `TSTstate`, `TSTLexer()` |
| RST.h   | Rust tokenizer: `RSTstate`, `RSTLexer()` |
| JAT.h   | Java tokenizer: `JATstate`, `JATLexer()` |
| KTT.h   | Kotlin tokenizer: `KTTstate`, `KTTLexer()` |
| SCLT.h  | Scala tokenizer: `SCLTstate`, `SCLTLexer()` |
| CST.h   | C# tokenizer: `CSTstate`, `CSTLexer()` |
| FSHT.h  | F# tokenizer: `FSHTstate`, `FSHTLexer()` |
| SWFT.h  | Swift tokenizer: `SWFTstate`, `SWFTLexer()` |
| DARTT.h | Dart tokenizer: `DARTTstate`, `DARTTLexer()` |
| DT.h    | D tokenizer: `DTstate`, `DTLexer()` |
| ZIGT.h  | Zig tokenizer: `ZIGTstate`, `ZIGTLexer()` |
| HTMT.h  | HTML tokenizer: `HTMTstate`, `HTMTLexer()` |
| CSST.h  | CSS tokenizer: `CSSTstate`, `CSSTLexer()` |
| SCSST.h | SCSS tokenizer: `SCSSTstate`, `SCSSTLexer()` |
| JSONT.h | JSON tokenizer: `JSONTstate`, `JSONTLexer()` |
| YMLT.h  | YAML tokenizer: `YMLTstate`, `YMLTLexer()` |
| TOMLT.h | TOML tokenizer: `TOMLTstate`, `TOMLTLexer()` |
| SHT.h   | Bash tokenizer: `SHTstate`, `SHTLexer()` |
| RBT.h   | Ruby tokenizer: `RBTstate`, `RBTLexer()` |
| LUAT.h  | Lua tokenizer: `LUATstate`, `LUATLexer()` |
| PRLT.h  | Perl tokenizer: `PRLTstate`, `PRLTLexer()` |
| RT.h    | R tokenizer: `RTstate`, `RTLexer()` |
| ELXT.h  | Elixir tokenizer: `ELXTstate`, `ELXTLexer()` |
| ERLT.h  | Erlang tokenizer: `ERLTstate`, `ERLTLexer()` |
| HST.h   | Haskell tokenizer: `HSTstate`, `HSTLexer()` |
| MLT.h   | OCaml tokenizer: `MLTstate`, `MLTLexer()` |
| JLT.h   | Julia tokenizer: `JLTstate`, `JLTLexer()` |
| NIMT.h  | Nim tokenizer: `NIMTstate`, `NIMTLexer()` |
| PHPT.h  | PHP tokenizer: `PHPTstate`, `PHPTLexer()` |
| CLJT.h  | Clojure tokenizer: `CLJTstate`, `CLJTLexer()` |
| NIXT.h  | Nix tokenizer: `NIXTstate`, `NIXTLexer()` |
| SQLT.h  | SQL tokenizer: `SQLTstate`, `SQLTLexer()` |
| GQLT.h  | GraphQL tokenizer: `GQLTstate`, `GQLTLexer()` |
| PRTT.h  | Protobuf tokenizer: `PRTTstate`, `PRTTLexer()` |
| HCLT.h  | HCL/Terraform tokenizer: `HCLTstate`, `HCLTLexer()` |
| LAXT.h  | LaTeX tokenizer: `LAXTstate`, `LAXTLexer()` |
| VIMT.h  | VimL tokenizer: `VIMTstate`, `VIMTLexer()` |
| CMKT.h  | CMake tokenizer: `CMKTstate`, `CMKTLexer()` |
| DKFT.h  | Dockerfile tokenizer: `DKFTstate`, `DKFTLexer()` |
| MAKT.h  | Makefile tokenizer: `MAKTstate`, `MAKTLexer()` |
| FORT.h  | Fortran tokenizer: `FORTstate`, `FORTLexer()` |
| GLST.h  | GLSL tokenizer: `GLSTstate`, `GLSTLexer()` |
| GLMT.h  | Gleam tokenizer: `GLMTstate`, `GLMTLexer()` |
| ODNT.h  | Odin tokenizer: `ODNTstate`, `ODNTLexer()` |
| PWST.h  | PowerShell tokenizer: `PWSTstate`, `PWSTLexer()` |
| SOLT.h  | Solidity tokenizer: `SOLTstate`, `SOLTLexer()` |
| TYST.h  | Typst tokenizer: `TYSTstate`, `TYSTLexer()` |
| AGDT.h  | Agda tokenizer: `AGDTstate`, `AGDTLexer()` |
| VERT.h  | Verilog tokenizer: `VERTstate`, `VERTLexer()` |

## Extension dispatch (TOK.c)

| Extension(s)                    | Tokenizer |
|---------------------------------|-----------|
| c h                              | CT        |
| cpp cc cxx hpp hh hxx           | CPPT      |
| go                              | GOT       |
| py                              | PYT       |
| js jsx mjs                      | JST       |
| ts tsx                          | TST       |
| rs                              | RST       |
| java                            | JAT       |
| kt kts                          | KTT       |
| scala sc                        | SCLT      |
| cs                              | CST       |
| fs fsi fsx                      | FSHT      |
| swift                           | SWFT      |
| dart                            | DARTT     |
| d                               | DT        |
| zig                             | ZIGT      |
| html htm                        | HTMT      |
| css                             | CSST      |
| scss                            | SCSST     |
| json                            | JSONT     |
| yml yaml                        | YMLT      |
| toml                            | TOMLT     |
| sh bash                         | SHT       |
| rb                              | RBT       |
| lua                             | LUAT      |
| pl pm                           | PRLT      |
| r R                             | RT        |
| ex exs                          | ELXT      |
| erl hrl                         | ERLT      |
| hs                              | HST       |
| ml mli                          | MLT       |
| jl                              | JLT       |
| nim nims                        | NIMT      |
| php                             | PHPT      |
| clj cljs cljc edn              | CLJT      |
| nix                             | NIXT      |
| sql                             | SQLT      |
| graphql gql                     | GQLT      |
| proto                           | PRTT      |
| hcl tf                          | HCLT      |
| tex sty cls                     | LAXT      |
| vim                             | VIMT      |
| cmake                           | CMKT      |
| dockerfile                      | DKFT      |
| mk                              | MAKT      |
| f90 f95 f03 f08                 | FORT      |
| glsl vert frag geom comp        | GLST      |
| gleam                           | GLMT      |
| odin                            | ODNT      |
| ps1 psm1 psd1                   | PWST      |
| sol                             | SOLT      |
| typ                             | TYST      |
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

where XX is the module prefix (CT, GOT, PYT, JST, RST, etc).
