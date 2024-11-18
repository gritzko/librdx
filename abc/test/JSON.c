#include "JSON.h"

#include "B.h"
#include "TEST.h"

pro(RDXtest1) {
    sane(1);
    a$str(json, "{\"a\":1,\"b\":[2]}");
    aBcpad(u64, pad, 1024);
    aBcpad(u32, stack, 1024);

    JSONstate state = {.text = {json[0], json[1]},
                       .json = (u64B)padbuf,
                       .stack = (u32B)stackbuf};

    js64 child = {.node = JSON_NODE_ROOT};
    call(Bu64feedp, state.json, (u64*)&child);
    call(Bu32feed1, state.stack, 0);

    call(JSONlexer, &state);

    for (u64* p = padbuf[0]; p < padbuf[2]; ++p) {
        js64* j = (js64*)p;
        fprintf(stderr, "%u\t%s\t%u\n", j->pos, JSON_NODE_NAMES[j->node],
                j->toks);
    }

    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXtest1);
    done;
}

TEST(RDXtest);
