#include "RDX.h"
#include "abc/01.h"
#include "abc/KV.h"
#include "abc/MMAP.h"
#include "abc/PRO.h"

typedef struct {
    u32 pos;
    u8 ptype;
    u8 type;
    u8 deep;
    b8 keep;
} diff64;

int diff64cmp(diff64 const* a, diff64 const* b) { return 0; }

#define X(M, name) M##diff64##name
#include "abc/Bx.h"
#undef X

fun b8 diff64Eq(diff64cp a, diff64cp b) { return *(u64c*)a == *(u64c*)b; }
fun b8 diff64In(diff64cp a, diff64cp b) {}
fun b8 diff64Rm(diff64cp a, diff64cp b) {}
fun b8 diff64Nx(diff64cp a, diff64cp b) {}

ok64 diff64bDrawMap(diff64b map, rdxp it, u8 deep) {
    sane(diff64bOK(map) && it);
    scan(rdxNext, it) {
        diff64 n = {
            .pos = u8csLen(it->data),
            .ptype = it->ptype,
            .type = it->type,
            .deep = deep,
        };
        if (rdxTypePlex(it)) {
            call(diff64bFeed1, map, n);
            rdx inner = {};
            call(rdxInto, &inner, it);
            call(diff64bDrawMap, map, &inner, deep + 1);
            call(rdxOuto, &inner, it);
            call(diff64bFeed1, map, n);
        } else {
            call(diff64bFeed1, map, n);
        }
    }
    seen(END);
    done;
}

ok64 u64bFindPath(u64cs oldmap, u64cs neumap, kv32bp wave) {
    sane(u64csOK(oldmap) && u64csOK(neumap) && kv32bOK(wave));
    u32 ol = u64csLen(oldmap);
    u32 nl = u64csLen(neumap);
    i32 d = (i64)ol - (i64)nl;
    u32 u = i64Zig(d);
    call(kv32bFed, wave, u + 1);
    kv32bAtP(wave, 0)->key = nl;
    kv32bAtP(wave, d)->val = nl;
    u64c* om = *oldmap;
    u64c* nm = *neumap;
    kv32* wv = *kv32bData(wave);
    for (u32 c = 0; c < ol + nl; c++) {
        u32 zl = kv32bDataLen(wave);
        for (u64 u = 0; u <= zl; ++u) {
            kv32 dfwd = kv32bAt(wave, u);
            d = u64Zag(u);
            u32 fwdneux = nl - dfwd.key;
            u32 fwdoldx = dfwd.val;
            while (om[nl - wv[u].key] == nm[wv[u].key]) wv[u].key++;  // 1 1
            diff64 od = *(diff64*)(om + nl - wv[u].key);
            diff64 nd = *(diff64*)(nm + wv[u].key);
            // for eq
            if (od.deep < nd.deep) {
                // 0 0 0 0 0
            } else if (od.deep > nd.deep) {
                // 0 0 0 0 0
            } else if (1) {
            }

            // for rmin
        }
        call(kv32bFed, wave, 2);
    }
    done;
}

// 30x30
ok64 u64sMarkByRange(u64sp whole, u64sp part, u64p mark) {}
ok64 u64SetRangeByMark(u64sp whole, u64sp part, u64cp mark) {}

ok64 rdxDiffMakeMap(u64b map, rdxp was, rdxp is) {
    sane(u64bOK(map) && was && is);
    // todo copy it
    // scan, recur
    // scan, make
    done;
}

ok64 rdxDiffFindPath(u64p weight, u64b map, rdxp was, rdxp is) {
    sane(weight && u64bOK(map));
    // myers
    // todo HEAP and FRONT
    // lefts
    // rights
    // heap

    done;
}

ok64 rdxDiffDrawPath(rdxp into, u64b map, rdxp was, rdxp is) {
    sane(into && (into->format & RDX_FMT_WRITE) && u64bOK(map));
    //
    done;
}

ok64 rdxDiff(rdxp into, rdxp was, rdxp is) {
    sane(into && was && is);
    u64b map = {};
    u64 weight = 0;
    u64 len = 0;  // fixme max(u8cgDoneLen(was->datag), u8cgDoneLen(is->datag));
    len = roundup(len, PAGESIZE / 8);
    // call(MMAPu64open, map, len);

    ok64 o = rdxDiffMakeMap(map, was, is);
    if (o == OK) o = rdxDiffFindPath(&weight, map, was, is);
    if (o == OK) o = rdxDiffDrawPath(into, map, was, is);

    // call(MMAPu64close, map);
    return o;
}
