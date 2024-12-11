#include "CT.h"

typedef struct {
    u64 var;
    u32 pos;
    u32 ins;
} mark128;

#define CTlog(ct) ((mark128**)ct + 2)

fun mark128* CTmark(Bu8 ct, u32 ndx) {
    mark128** log = CTlog(ct);
    assert(ndx > 0 && ndx <= $len(log));
    return log[1] - ndx;
}

fun u32 CTloglen(Bu8 ct) { return $len(CTlog(ct)); }

fun ok64 CTaddmark(Bu8 ct, mark128 const* rec) {
    mark128** log = CTlog(ct);
    u8$ idle = CTidle(ct);
    if ($size(idle) < sizeof(u128)) return CTnoroom;
    --log[0];
    **log = *rec;
    return OK;
}

fun ok64 CTaddvar(Bu8 ct, u64 var) {
    mark128 mark = {.var = var, .pos = $len(CTdata(ct))};
    return CTaddmark(ct, &mark);
}

fun u32 CTfind(Bu8 ct, u64 var) {
    for (u32 i = $len(CTlog(ct)); i > 0; --i)
        if (CTmark(ct, i)->var == var) return i;
    return 0;
}

ok64 CTsplice(Bu8 ct, u64 var) {
    u32 at = CTfind(ct, var);
    if (at == 0) return CTnone;
    mark128 mark = {.pos = $len(CTdata(ct))};
    call(CTaddmark, ct, &mark);
    while (CTmark(ct, at)->ins != 0) at = CTmark(ct, at)->ins;
    CTmark(ct, at)->ins = $len(CTlog(ct));
    return OK;
}

ok64 CTsplicemany(Bu8 ct, u64 var, b8 some) {
    mark128 mark = {.pos = $len(CTdata(ct))};
    call(CTaddmark, ct, &mark);
    ok64 o = some ? CTnone : OK;
    for (u32 i = $len(CTlog(ct)); i > 0; --i) {
        mark128* m = CTmark(ct, i);
        if (m->var == var && m->ins == 0) {
            m->ins = $len(CTlog(ct));
            o = OK;
        }
    }
    return o;
}

// $1 $var ${var}
ok64 CTscanvar(ok64* var, $u8c input) {
    sane(**input == '$' && $len(input) > 1 && var != nil);
    u8c* p = input[0];
    ++p;
    if (*p == '$') {
        ++input[0];
        return CTnone;
    }
    b8 bracket = (*p == '{');
    if (bracket) ++p;
    $u8c name = {p};
    while (p < input[1] && BASEron64rev[*p] != 0xff) ++p;
    if (bracket) {
        test(p < input[1] && *p == '}', CTbad);
        name[1] = p;
        ++p;
    } else {
        name[1] = p;
    }
    test($len(name) > 0 && $len(name) <= 10, CTbad);
    OKscan(var, name);
    input[0] = p;
    done;
}

ok64 CTfeed(Bu8 ct, $u8c insert) {
    u8$ idle = CTidle(ct);
    u8c$ data = CTdata(ct);
    if ($len(idle) < $len(insert)) return CTnoroom;
    a$dup(u8c, ins, insert);
    while (!$empty(ins)) {
        if (**ins != '$' || $len(ins) <= 1) {
            **idle = **ins, ++*idle, ++*ins;
            continue;
        }
        ok64 var = 0;
        ok64 o = CTscanvar(&var, ins);
        if (o != OK) {
            **idle = **ins, ++*idle, ++*ins;
        } else {
            call(CTaddvar, ct, var);
        }
    }
    return OK;
}

ok64 CTrendertree($u8 into, Bu8 ct, u32 ndx) {
    u8c$ data = CTdata(ct);
    mark128 zero = {};
    do {
        mark128* mark = ndx ? CTmark(ct, ndx) : &zero;
        u32 from = mark->pos;
        u32 n = ndx + 1;
        mark128* next = CTmark(ct, n);
        while (next->var != 0) {
            u32 till = next->pos;
            a$part(u8c, part, data, from, till - from);  // TODO
            call($u8feedall, into, part);
            from = till;
            if (next->ins != 0) {
                call(CTrendertree, into, ct, next->ins);
            }
            next = CTmark(ct, ++n);
        }
        u32 till = next->pos;
        a$part(u8c, part, data, from, till - from);  // TODO
        call($u8feedall, into, part);
        ndx = mark->ins;
    } while (ndx != 0);
    done;
}

ok64 CTrender($u8 into, Bu8 ct) {
    u32 from = 0;
    mark128 mark = {.pos = $len(CTdata(ct))};
    call(CTaddmark, ct, &mark);
    return CTrendertree(into, ct, 0);
}
