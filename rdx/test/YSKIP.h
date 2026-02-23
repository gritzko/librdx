// Table-driven tests for rdxSkipJDR
// Each entry: {jdr_input, expected_element_count_at_top_level}
// Tests that rdxNext can iterate through all elements correctly
// (Skip is used internally by Next to advance past plex elements)

#include "abc/BUF.h"

typedef struct {
    u8cs jdr;
    u32 count;
} SkipTest;

SkipTest SKIP_TESTS[] = {
    // Simple tuples with primitive values
    {u8csOf("(1,2,3)"), 3},
    {u8csOf("(a,b,c)"), 3},
    {u8csOf("(1)"), 1},
    {u8csOf("()"), 0},

    // Nested bracket tuples
    {u8csOf("((1),(2),(3))"), 3},
    {u8csOf("((a,b),(c,d),(e,f))"), 3},
    {u8csOf("(((1)))"), 1},

    // Colon tuples (inline PIN format)
    {u8csOf("(a:1,b:2,c:3)"), 3},
    {u8csOf("(key:value)"), 1},
    {u8csOf("(a:b:c)"), 1},  // a:(b:c) or (a:b):c?

    // Mixed bracket and colon tuples
    {u8csOf("((1),a:b,\"str\")"), 3},
    {u8csOf("(a:1,(b,c),d:2)"), 3},

    // Euler sets
    {u8csOf("{1,2,3}"), 3},
    {u8csOf("{(a,b),(c,d)}"), 2},
    {u8csOf("{a:1,b:2}"), 2},

    // Linear arrays
    {u8csOf("[1,2,3]"), 3},
    {u8csOf("[(a),(b)]"), 2},

    // Strings in tuples
    {u8csOf("(\"hello\",\"world\")"), 2},
    {u8csOf("(\"a:b\",c:d)"), 2},  // string contains colon

    // Refs
    {u8csOf("(a-1,b-2,c-3)"), 3},

    // Syslog-style tuples (from syslog2rdx)
    {u8csOf("{(2608B1U000-4124,\"host\",\"proc\",\"msg\")}"), 1},
    {u8csOf("{(a-1,\"h1\",\"p1\",\"m1\"),(b-2,\"h2\",\"p2\",\"m2\")}"), 2},

    // Complex nested structures
    {u8csOf("{(1:a),(2:b),(3:c)}"), 3},

    // End marker
    {{}, 0},
};
