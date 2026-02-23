// Table-driven round-trip tests for JDR <-> TLV conversion
// Each entry: JDR input string
// Property: JDR -> TLV -> JDR should produce identical output

#include "abc/BUF.h"

u8cs JDRTLV_TESTS[] = {
    // Primitive types: integers
    u8csOf("0"),
    u8csOf("1"),
    u8csOf("-1"),
    u8csOf("123"),
    u8csOf("-456"),
    u8csOf("999999999"),

    // Primitive types: floats
    u8csOf("0E0"),
    u8csOf("1E0"),
    u8csOf("3.14E0"),
    u8csOf("-2.5E0"),
    u8csOf("1.23E10"),

    // Primitive types: terms
    u8csOf("a"),
    u8csOf("abc"),
    u8csOf("foo123"),
    u8csOf("CamelCase"),

    // Primitive types: refs
    u8csOf("a-1"),
    u8csOf("abc-123"),
    u8csOf("0-0"),

    // Primitive types: strings
    u8csOf("\"\""),
    u8csOf("\"hello\""),
    u8csOf("\"hello world\""),
    u8csOf("\"with\\nnewline\""),
    u8csOf("\"with\\ttab\""),
    u8csOf("\"with\\\"quote\""),
    u8csOf("\"with\\\\backslash\""),

    // Tuples: empty and simple
    u8csOf("()"),
    u8csOf("(1)"),
    u8csOf("(1,2)"),
    u8csOf("(1,2,3)"),
    u8csOf("(a,b,c)"),
    u8csOf("(\"x\",\"y\")"),

    // Tuples: mixed types
    u8csOf("(1,a,\"str\")"),
    u8csOf("(a-1,123,\"text\")"),
    u8csOf("(1E0,2,three)"),

    // Tuples: nested
    u8csOf("((1))"),
    u8csOf("((1,2))"),
    u8csOf("((1),(2))"),
    u8csOf("((1,2),(3,4))"),
    u8csOf("(((1)))"),

    // Linear arrays
    u8csOf("[]"),
    u8csOf("[1]"),
    u8csOf("[1,2,3]"),
    u8csOf("[a,b,c]"),
    u8csOf("[[1],[2]]"),
    u8csOf("[\"a\",\"b\"]"),

    // Euler sets
    u8csOf("{}"),
    u8csOf("{1}"),
    u8csOf("{1,2,3}"),
    u8csOf("{a,b,c}"),
    u8csOf("{\"x\",\"y\"}"),

    // Euler sets with tuples (maps)
    u8csOf("{(a,1)}"),
    u8csOf("{(a,1),(b,2)}"),
    u8csOf("{(\"key\",\"val\")}"),

    // Multix (version vectors)
    u8csOf("<>"),
    u8csOf("<1@a-1>"),
    u8csOf("<1@a-1,2@b-2>"),

    // Complex nested structures
    u8csOf("(1,[2,3],{4,5})"),
    u8csOf("{(a,[1,2]),(b,[3,4])}"),
    u8csOf("[(1,2),(3,4)]"),
    u8csOf("{[1],[2],[3]}"),

    // With stamps
    u8csOf("(@1 1)"),
    u8csOf("(@1-2 a)"),
    u8csOf("(1@1,2@2)"),
    u8csOf("{(@1 a,1),(@2 b,2)}"),

    // tricky: inline tuples nested in Euler sets
    u8csOf("{(a,1),e}"),
    u8csOf("{(a,{(b,1)}),c}"),
    u8csOf("{(a,1),(b,2),c}"),

    // End marker
    {},
};

// Inline mode tests: tuples use colon notation (a:b instead of (a,b))
// Empty and single-element tuples still use brackets
u8cs JDRTLV_INLINE_TESTS[] = {
    // Primitives (same as bracket mode)
    u8csOf("{a:{b:{c:d}},e}"),
    u8csOf("1:2:(3,4):5"),
    u8csOf("1"),
    u8csOf("a"),
    u8csOf("\"hello\""),

    // Empty and single-element tuples use brackets
    u8csOf("()"),
    u8csOf("1:"),

    // Two+ element tuples use colon notation
    u8csOf("1:2"),
    u8csOf("a:b:c"),
    u8csOf("1:\"str\":a"),

    // Nested inline
    u8csOf("1:2:3"),
    u8csOf("{a:1,b:2}"),
    u8csOf("{a:1,b:2,c}"),
    u8csOf("[1:a,2:b]"),

    // trickies
    u8csOf("{a:{b:1},e}"),
    u8csOf("{a:{b:2,{c:3}},d:4}"),
    // End marker
    {},
};
