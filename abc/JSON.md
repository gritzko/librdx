#   JSON rework

Objective: make a "binary JSON" format, nested TLKV (see TLV.h) with
SLOG (see SLOG.md, SLOG.h).

The iterator structure and the API:
````
typedef struct {
    u8bp  buf;
    u64bp stack;

    u8cs key;
    u8cs val;

    u8 lit;  // value type
    u8 plit; // parent O A N S B
} slit;

ok64 JSONNext(slit *it);
ok64 JSONInto(slit *it);
ok64 JSONOuto(slit *it);

ok64 JSONWriteNext(slit *it);
ok64 JSONWriteInto(slit *it);
ok64 JSONWriteOuto(slit *it);
````

The Ragel lexer should use *Write* calls to build a doc in a buffer.
Read should use Next/Into/Outo API. Note that Into() with `key` and
`lit` set should position on an element having >= the provided key
or return NONE.

API entry points:
````
ok64 JSONParse(u8bp buf, u8cs json);
ok64 JSONExport(u8s json, u8bp buf);
````
Keys and values are always text. Strings are unescaped (escaped back
on export). Record types (lits): Object, Array, Number, String,
Boolean, Null.
For array contents, the key is the index in RON base 64 (see RON.h).
