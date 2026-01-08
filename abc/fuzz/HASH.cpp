//
// (c) Victor Grishchenko, 2020-2023
//
#include <cassert>
#include <unordered_map>

#include "INT.h"
#include "KV.h"
#include "PRO.h"

extern "C" {

#define X(M, name) M##kv32##name
#include "HASHx.h"
#undef X
}

static bool init = false;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    Bkv32 hashbuf = {};
    kv32bAlloc(hashbuf, 1024);
    kv32sp hashmap = kv32bIdle(hashbuf);
    ok64 status;

    std::unordered_map<uint32_t, uint32_t> ref{};

    char *str = (char *)malloc(Size + 1);
    memcpy(str, Data, Size);
    str[Size] = 0;
    const char *p = str;
    const char *e = p + Size;

    while (p < e) {
        kv32 e;
        int rd;
        int ret = sscanf(p, "%u %u %n", &e.key, &e.val, &rd);
        if (ret < 2 || e.key == 0) {
            break;
        }
        if (e.val) {
            status = HASHkv32put(hashmap, &e);
            if (OK != status) {
                if (status == HASHnoroom) break;
                fprintf(stderr, "\n\nPUT %u,%u failed %lx\n\n", e.key, e.val,
                        status);
            }
            assert(OK == status);
            ref[e.key] = e.val;
        } else {
            HASHkv32del(hashmap, &e);
            ref.erase(e.key);
        }
        p += rd;
    }

    for (auto &pair : ref) {
        uint32_t key = pair.first;
        uint32_t val = pair.second;
        kv32 fact = {.key = key, .val = 0};
        status = HASHkv32get(&fact, hashmap);
        if (status != OK || fact.val != val)
            fprintf(stderr, "? %u: %u!=%u (%lx)\n", key, val, fact.val, status);
        assert(status == OK);
        assert(fact.val == val);
    }

    /*for (uint64_t ndx = 0; ndx < $len(hashmap); ndx += 2) {
        if ((*hashmap)[ndx + VAL] == 0 || (*hashmap)[ndx + KEY] == 0) continue;
        auto i = ref.find((*hashmap)[ndx + KEY]);
        assert(i != ref.end());
    }*/

    free(str);
    kv32bFree(hashbuf);
    ref.clear();

    return 0;
}
