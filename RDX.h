#ifndef LIBRDX_RDX_H
#define LIBRDX_RDX_H

#include "$.h"
#include "B.h"
#include "LINE.h"
#include "ZINT.h"

con ok64 RDXnospace = 0xa67974df3ca135b;

typedef enum {
    Float = 'F',
    Integer = 'I',
    Reference = 'R',
    String = 'S',
    Term = 'T',
    Eulerian = 'E',
    Linear = 'L',
    Mapping = 'M',
} RDXtype;

typedef int64_t RDXint;
typedef double RDXfloat;
typedef struct {
    u64 src;
    int64_t seq;
} id128;
typedef id128 RDXref;
typedef $u8c RDXstring;
typedef $u8c RDXterm;

typedef $u8 rdx$;

typedef ok64 RDXmergefn($u8 into, $u8cp from);

fun pro(RDXmerge1, line* into, $cline from) {
    test($len(from) > 0, OK);
    linec* top = *from;
    $for(linec, p, from) {
        $u8c val = {};
        a$dup(u8c, body, p->body);
        call(TLVtake, 'T', val, body);  // todo errors
        u128 id = {};
        call(ZINTu128drain, &id, val);
        if (u128cmp(&top->id, &id) < 0) top = p;
    }
    call(LINEfeed, into, top);
    done;
}

fun ok64 RDXmergeF(line* into, $cline from) { return RDXmerge1(into, from); }
fun ok64 RDXmergeI(line* into, $cline from) { return RDXmerge1(into, from); }
fun ok64 RDXmergeR(line* into, $cline from) { return RDXmerge1(into, from); }
fun ok64 RDXmergeS(line* into, $cline from) { return RDXmerge1(into, from); }
fun ok64 RDXmergeT(line* into, $cline from) { return RDXmerge1(into, from); }

fun pro(RDXmergeE, line* into, $line from) {
    aBpad(line, sets, LINEmaxmergelen);
    $for(line, p, from) {
        aLINE(next, p->body);
        call(Blinefeed1, sets, next);
    }
    // FIXME header
    call(LINEmerge, into, Blinedata(sets), &linebodycmp, &RDXmerge1);
    // call(LINEfeed)
    done;
}

fun ok64 RDXmergeN(line* into, $line from) {
    return LINEmerge(into, from, &lineidhicmp, &RDXmerge1);
}

fun ok64 RDXmergeZ(line* into, $line from) {
    return LINEmerge(into, from, &lineidhicmp, &RDXmerge1);
}

fun ok64 _Mmerge(line* into, $cline from) {
    // todo both
    return OK;
}

fun ok64 RDXmergeM(line* into, $line from) {
    return LINEmerge(into, from, &linebodycmp, &_Mmerge);
}

fun pro(RDXmerge, Bu8 into, $u8c$ from) {
    aBpad(line, lines, LINEmaxmergelen);
    line$ f = Blinedata(lines);
    u8 type;
    $for(u8c$, p, from) {
        aLINE(l, *p);  // rest?!!
        Blinefeedp(lines, &l);
    }
    aLINE(inl, Bu8cidle(into));
    switch (type) {
        case 'F':
        case 'I':
        case 'R':
        case 'S':
        case 'T':
            call(RDXmerge1, &inl, f);
    }
    done;
}

//-----------
//

fun ok64 RDXfeedid($u8 into, RDXref id) {
    return ZINTu128feed(into, ZINTzigzag(id.seq), id.src);
}

ok64 RDXfeedF($u8 into, RDXref id, RDXfloat val);
ok64 RDXfeedI($u8 into, RDXref id, RDXint val);
ok64 RDXfeedR($u8 into, RDXref id, RDXref val);
ok64 RDXfeedS($u8 into, RDXref id, RDXstring val) {
    aBpad(u8, tmp, 16);
    RDXfeedid(Bu8idle(tmp), id);
    u8c** idb = Bu8cdata(tmp);
    u32 len = $len(idb) + $len(val) + 1;
    if ($len(idb) > 9) ++len;
    if (len > $len(into)) return RDXnospace;
    TLVhead(into, String, len);
    TLVtinyhead(into, 'T', $len(idb));
    $u8feed(into, idb);
    $u8feed(into, val);
    return OK;
}
ok64 RDXfeedT($u8 into, RDXref id, RDXterm val);

ok64 RDXdrainF(RDXref* id, RDXfloat* val, $u8c from);
ok64 RDXdrainI(RDXref* id, RDXint* val, $u8c from);
ok64 RDXdrainR(RDXref* id, RDXref* val, $u8c from);
fun pro(RDXdrainS, RDXref* id, RDXstring val, $u8c from) {
    $u8c body = {}, idb = {};
    call(TLVtake, String, body, from);
    call(TLVtake, 'T', idb, body);
    call(RDXdrainid, id, idb);
    $mv(val, body);
    done;
}
ok64 RDXdrainT(RDXref* id, RDXterm val, $u8c from);

ok64 RDXpush(Brdx$ heap, rdx$ it, rdx$cmpfn fn);
ok64 RDXpop(rdx$ next, Brdx$ heap, rdx$cmpfn fn);

#endif
