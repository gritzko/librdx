// Table-driven tests for merging syslog-style Euler sets
// This tests rdxMerge with the kind of data that syslog2rdx produces
// Format: {(time-pid,"host","process","message"), ...}

#include "abc/BUF.h"

u8cs YSYS_TEST[][8] = {
    // Two entries from different sources - should merge by time ordering
    {
        u8csOf("{(a-1,\"h1\",\"p1\",\"m1\"),(b-2,\"h2\",\"p2\",\"m2\")}"),
        u8csOf("{(a-1,\"h1\",\"p1\",\"m1\")}"),
        u8csOf("{(b-2,\"h2\",\"p2\",\"m2\")}"),
        0,
    },
    // Same timestamp, different content - should keep both
    {
        u8csOf("{(a-1,\"h1\",\"p1\",\"m1\"),(a-10,\"h2\",\"p2\",\"m2\")}"),
        u8csOf("{(a-1,\"h1\",\"p1\",\"m1\")}"),
        u8csOf("{(a-10,\"h2\",\"p2\",\"m2\")}"),
        0,
    },
    // Three-way merge
    {
        u8csOf("{(a-1,\"h\",\"p\",\"m\"),(b-2,\"h\",\"p\",\"m\"),(c-3,\"h\","
               "\"p\",\"m\")}"),
        u8csOf("{(a-1,\"h\",\"p\",\"m\")}"),
        u8csOf("{(b-2,\"h\",\"p\",\"m\")}"),
        u8csOf("{(c-3,\"h\",\"p\",\"m\")}"),
        0,
    },
    0,
};
