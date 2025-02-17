
#include "OK.h"
#include "SST.h"
#include "TLV.h"

typedef Bu8 X(SST, );
typedef X(, ) Key;

static const u32 X(SST, magic) = (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) |
                                 ((u32)('0' + (63 - clz64(sizeof(Key)))) << 24);

fun ok64 X(SST, init)(X(SST, ) sst, int* fd, $u8c path, size_t size) {
    if (size < sizeof(SSTheader)) return SSTbadhead;
    ok64 o = FILEmapnew(sst, fd, path, size);
    if (o == OK) {
        u8** s = (u8**)sst;
        s[1] += sizeof(SSTheader);
        s[2] = s[1];
        ((SSTheader*)*sst)->magic = X(SST, magic);
    }
    return o;
}

fun ok64 X(SST, open)(X(SST, ) sst, $u8c path) {
    int fd = FILE_CLOSED;
    ok64 o = FILEmapro(sst, &fd, path);
    if (o != OK) return o;
    SSTheader const* head = (SSTheader const*)sst[0];
    if (Blen(sst) < sizeof(SSTheader) || head->magic != X(SST, magic) ||
        Blen(sst) < head->metalen + head->datalen)
        return SSTbadhead;
    u8** s = (u8**)sst;
    s[1] = s[0] + head->metalen;
    s[2] = s[1] + head->datalen;
    return FILEclose(&fd);
}

fun ok64 X(SST, hasindex)(X(SST, ) sst) { return Bidlelen(sst) != 0; }

fun ok64 X(SST, feed)(X(SST, ) sst, SSTab* tab, u8 type, Key const* key,
                      $u8c value) {
    a$rawcp(raw, key);
    ok64 o = TLVfeedkv(Bu8idle(sst), type, raw, value);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun ok64 X(SST, initshort)(X(SST, ) sst, u8 type, Key const* key, Bu8p stack) {
    u8$ into = Bu8idle(sst);
    a$rawcp(raw, key);
    TLVinitshort(into, type, stack);
    $u8feed1(into, sizeof(Key));
    return $u8feedall(into, raw);
}

fun ok64 X(SST, initlong)(X(SST, ) sst, u8 type, Key const* key, Bu8p stack) {
    u8$ into = Bu8idle(sst);
    a$rawcp(raw, key);
    TLVinitlong(Bu8idle(sst), type, stack);
    $u8feed1(into, sizeof(Key));
    return $u8feedall(into, raw);
}

fun ok64 X(SST, endany)(X(SST, ) sst, u8 type, SSTab* tab, Bu8p stack) {
    ok64 o = TLVendany(Bu8idle(sst), type, stack);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun ok64 X(SST, feedkv)(X(SST, ) sst, SSTab* tab, $u8c rec) {
    ok64 o = $u8feedall(Bu8idle(sst), rec);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun int X(SST, cmp)($cc a, $cc b) {
    u8 ta = 0, tb = 0;
    $u8c ka = {}, va = {}, kb = {}, vb = {};
    a$dup(u8c, aa, a);
    a$dup(u8c, bb, b);  // TODO fast and robust
    TLVdrainkv(&ta, ka, va, aa);
    TLVdrainkv(&tb, kb, vb, bb);
    int z = X(, cmp)((Key*)*ka, (Key*)*kb);
    if (z == 0) z = u8cmp(&ta, &tb);
    return z;
}

fun ok64 X(SST, locate)(u8c$ rest, X(SST, ) sst, u8 type, Key const* key) {
    u8 t = (type ? type : 'A') | TLVaA;
    an$u8(needle, sizeof(Key) + 3, t, sizeof(Key) + 1, sizeof(Key));
    a$rawcp(keyraw, key);
    a$tail(u8, into, needle, 3);
    $u8move(into, keyraw);
    return SKIPu8find(rest, sst, (u8c**)needle, X(SST, cmp));
}

fun ok64 X(SST, next)(u8* t, Key* key, u8c$ val, $u8c rest) {
    if (!$empty(rest) && (**rest & ~TLVaA) == SKIP_TLV_TYPE) {
        $u8c rec;
        TLVdrain$(rec, rest);
    }
    a$rawp(keyraw, key);
    $u8c k = {};
    ok64 o = TLVdrainkv(t, k, val, rest);
    if (o != OK) return o;
    if ($len(k) > sizeof(Key)) return SSTbadrec;
    keyraw[0] += sizeof(Key) - $len(k);
    $copy(keyraw, k);
    return OK;
}

// Get a record by its type, id. Set type to 0 if insignificant.
fun ok64 X(SST, get)(u8* type, u8c$ val, X(SST, ) sst, Key const* key) {
    $u8c rest = {};
    ok64 o = X(SST, locate)(rest, sst, *type, key);
    while (o == OK) {
        Key k = {};
        u8 t = 0;
        o = X(SST, next)(&t, &k, val, rest);
        if (o != OK) break;
        int z = X(, cmp)(&k, key);
        if (z < 0) {
            continue;
        } else if (z == 0) {
            if (*type == 0) {
                *type = t;
            } else if (*type != t) {
                continue;
            }
            break;
        } else {
            o = SSTnone;
            break;
        }
    }
    return o;
}

fun ok64 X(SST, end)(X(SST, ) sst, int* fd, SSTab* tab) {
    sane(Bok(sst) && fd != nil && *fd != FILE_CLOSED && tab != nil);
    call(SKIPu8finish, sst, tab);
    call(FILEresize, fd, Busysize(sst));
    SSTheader* head = (SSTheader*)*sst;
    head->metalen = Bpastlen(sst);
    head->datalen = Bdatalen(sst);
    call(FILEclose, fd);
    done;
}

fun ok64 X(SST, close)(X(SST, ) sst) { return FILEunmap(sst); }
