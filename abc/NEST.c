#include "NEST.h"

#include "PRO.h"

typedef struct {
    u64 var;
    u32 pos;
    u32 ins;
} mark128;

#define NESTlog(ct) ((mark128**)ct + 2)

fun mark128* NESTmark(Bu8 ct, u32 ndx) {
    mark128** log = NESTlog(ct);
    assert(ndx > 0 && ndx <= $len(log));
    return log[1] - ndx;
}

fun u32 NESTloglen(Bu8 ct) { return $len(NESTlog(ct)); }

fun ok64 NESTaddmark(Bu8 ct, mark128 const* rec) {
    mark128** log = NESTlog(ct);
    u8$ idle = NESTidle(ct);
    if ($size(idle) < sizeof(u128)) return NESTnoroom;
    --log[0];
    **log = *rec;
    return OK;
}

fun ok64 NESTaddvar(Bu8 ct, u64 var) {
    mark128 mark = {.var = var, .pos = $len(NESTdata(ct))};
    return NESTaddmark(ct, &mark);
}

fun u32 NESTfind(Bu8 ct, u64 var) {
    for (u32 i = $len(NESTlog(ct)); i > 0; --i)
        if (NESTmark(ct, i)->var == var) return i;
    return 0;
}

ok64 NESTsplice(Bu8 ct, u64 var) {
    sane(Bok(ct));
    u32 at = NESTfind(ct, var);
    if (at == 0) return NESTnone;
    mark128 mark = {.pos = $len(NESTdata(ct))};
    call(NESTaddmark, ct, &mark);
    while (NESTmark(ct, at)->ins != 0) at = NESTmark(ct, at)->ins;
    NESTmark(ct, at)->ins = $len(NESTlog(ct));
    done;
}

ok64 NESTsplicemany(Bu8 ct, u64 var, b8 some) {
    sane(Bok(ct));
    mark128 mark = {.pos = $len(NESTdata(ct))};
    call(NESTaddmark, ct, &mark);
    int found = 0;
    for (u32 i = $len(NESTlog(ct)); i > 0; --i) {
        mark128* m = NESTmark(ct, i);
        if (m->var == var && m->ins == 0) {
            m->ins = $len(NESTlog(ct));
            ++found;
        }
    }
    if (!found) fail(NESTnone);
    done;
}

// $1 $var ${var}
ok64 NESTscanvar(ok64* var, u8cs input) {
    sane(**input == '$' && $len(input) > 1 && var != NULL);
    u8c* p = input[0];
    ++p;
    if (*p == '$') {
        ++input[0];
        return NESTnone;
    }
    b8 bracket = (*p == '{');
    if (bracket) ++p;
    u8cs name = {p};
    while (p < input[1] && BASEron64rev[*p] != 0xff) ++p;
    if (bracket) {
        test(p < input[1] && *p == '}', NESTbad);
        name[1] = p;
        ++p;
    } else {
        name[1] = p;
    }
    test($len(name) > 0 && $len(name) <= 10, NESTbad);
    OKscan(var, name);
    input[0] = p;
    done;
}

ok64 NESTfeed(Bu8 ct, u8cs insert) {
    sane(Bok(ct) && $ok(insert));
    u8$ idle = NESTidle(ct);
    u8c$ data = NESTdata(ct);
    if ($len(idle) < $len(insert)) return NESTnoroom;
    a$dup(u8c, ins, insert);
    while (!$empty(ins)) {
        if (**ins != '$' || $len(ins) <= 1) {
            **idle = **ins, ++*idle, ++*ins;
            continue;
        }
        ok64 var = 0;
        ok64 o = NESTscanvar(&var, ins);
        if (o != OK) {
            **idle = **ins, ++*idle, ++*ins;
        } else {
            call(NESTaddvar, ct, var);
        }
    }
    done;
}

ok64 NESTrendertree($u8 into, Bu8 ct, u32 ndx) {
    sane(Bok(ct) && $ok(into));
    u8c$ data = NESTdata(ct);
    mark128 zero = {};
    do {
        mark128* mark = ndx ? NESTmark(ct, ndx) : &zero;
        u32 from = mark->pos;
        u32 n = ndx + 1;
        mark128* next = NESTmark(ct, n);
        while (next->var != 0) {
            u32 till = next->pos;
            a$part(u8c, part, data, from, till - from);  // TODO
            call(u8sFeed, into, part);
            from = till;
            if (next->ins != 0) {
                call(NESTrendertree, into, ct, next->ins);
            }
            next = NESTmark(ct, ++n);
        }
        u32 till = next->pos;
        a$part(u8c, part, data, from, till - from);  // TODO
        call(u8sFeed, into, part);
        ndx = mark->ins;
    } while (ndx != 0);
    done;
}

ok64 NESTrender($u8 into, Bu8 ct) {
    sane($ok(into) && Bok(ct));
    u32 from = 0;
    mark128 mark = {.pos = $len(NESTdata(ct))};
    try(NESTaddmark, ct, &mark);
    then try(NESTrendertree, into, ct, 0);
    done;
}
