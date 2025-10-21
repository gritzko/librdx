
#include "OK.h"
#include "SST.h"
#include "TLV.h"
#include "abc/B.h"

typedef Bu8 X(SST, );
typedef X(, ) Key;

static const u32 X(SST, magic) = (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) |
                                 ((u32)('0' + (63 - clz64(sizeof(Key)))) << 24);

fun ok64 X(SST, meta)(X(SST, ) sst, SSTheader const** head, u8c$ meta) {
    SSTheader const* h = (SSTheader const*)*sst;
    if (h->metalen > Bpastlen(sst)) return Bmiss;
    if (head != NULL) *head = h;
    meta[0] = *sst + sizeof(SSTheader);
    meta[1] = *sst + h->metalen;
    return OK;
}

fun ok64 X(SST, init)(X(SST, ) sst, int* fd, u8cs path, size_t size) {
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

fun ok64 X(SST, open)(X(SST, ) sst, u8cs path) {
    ok64 o = FILEmapro(sst, path);
    if (o != OK) return o;
    SSTheader const* head = (SSTheader const*)sst[0];
    if (Blen(sst) < sizeof(SSTheader) || head->magic != X(SST, magic) ||
        Blen(sst) < head->metalen + head->datalen)
        return SSTbadhead;
    u8** s = (u8**)sst;
    s[1] = s[0] + head->metalen;
    s[2] = s[1] + head->datalen;
    return OK;
}

fun ok64 X(SST, hasindex)(X(SST, ) sst) { return Bidlelen(sst) != 0; }

fun ok64 X(SST, feed)(X(SST, ) sst, SSTab* tab, u8 type, Key const* key,
                      u8cs value) {
    aBcpad(u8, raw, sizeof(Key));
    X(, pack)(rawidle, key);
    ok64 o = TLVFeedkv(u8bIdle(sst), type, rawdata, value);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun ok64 X(SST, initshort)(X(SST, ) sst, u8 type, Key const* key, Bu8p stack) {
    u8$ into = u8bIdle(sst);
    aBcpad(u8, raw, sizeof(Key));
    X(, pack)(rawidle, key);
    TLVinitshort(into, type, stack);
    u8sFeed1(into, sizeof(Key));
    return u8sFeed(into, rawdata);
}

fun ok64 X(SST, initlong)(X(SST, ) sst, u8 type, Key const* key, Bu8p stack) {
    u8$ into = u8bIdle(sst);
    aBcpad(u8, raw, sizeof(Key));
    X(, pack)(rawidle, key);
    TLVinitlong(u8bIdle(sst), type, stack);
    u8sFeed1(into, sizeof(Key));
    return u8sFeed(into, rawdata);
}

fun ok64 X(SST, endany)(X(SST, ) sst, u8 type, SSTab* tab, Bu8p stack) {
    ok64 o = TLVendany(u8bIdle(sst), type, stack);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun ok64 X(SST, feedkv)(X(SST, ) sst, SSTab* tab, u8cs rec) {
    ok64 o = u8sFeed(u8bIdle(sst), rec);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun int X(SST, cmp)($cc a, $cc b) {
    u8 ta = 0, tb = 0;
    u8cs ka = {}, va = {}, kb = {}, vb = {};
    a$dup(u8c, aa, a);
    a$dup(u8c, bb, b);  // TODO fast and robust
    TLVDrainKeyVal(&ta, ka, va, aa);
    TLVDrainKeyVal(&tb, kb, vb, bb);
    Key keya = X(, max), keyb = X(, max);
    X(, unpack)(&keya, ka);
    X(, unpack)(&keyb, kb);
    int z = X(, cmp)(&keya, &keyb);
    if (z == 0) z = u8cmp(&ta, &tb);
    return z;
}

fun ok64 X(SST, locate)(u8c$ rest, X(SST, ) sst, u8 type, Key const* key) {
    u8 t = (type ? type : 'A') | TLVaA;
    aBcpad(u8, raw, sizeof(Key) + 3);
    u8sFeed1(rawidle, t);
    u8sFeed2(rawidle, 0, 0);
    X(, pack)(rawidle, key);
    *Bu8atp(rawbuf, 1) = $len(rawdata) - 2;
    *Bu8atp(rawbuf, 2) = $len(rawdata) - 3;
    return SKIPu8find(rest, sst, rawdata, X(SST, cmp));
}

fun ok64 X(SST, next)(u8* t, Key* key, u8c$ val, u8cs rest) {
    if (!$empty(rest) && (**rest & ~TLVaA) == SKIP_TLV_TYPE) {
        u8cs rec;
        TLVDrain$(rec, rest);
    }
    u8cs k = {};
    ok64 o = TLVDrainKeyVal(t, k, val, rest);
    if (o == OK) o = X(, unpack)(key, k);
    if (o != OK) return o;
    return OK;
}

fun ok64 X(SST, getkv)(u8c$ rec, X(SST, ) sst, u8 type, Key const* key) {
    u8cs rest = {};
    ok64 o = X(SST, locate)(rest, sst, type, key);
    while (o == OK) {
        u8 t = 0;
        Key k = {};
        u8cs v;
        a$dup(u8c, dup, rest);
        o = X(SST, next)(&t, &k, v, rest);
        if (o != OK) break;
        int z = X(, cmp)(&k, key);
        if (z < 0) {
            continue;
        } else if (z == 0) {
            if (type != 0 && type != t) {
                continue;
            }
            u8scSup(dup, rest);
            $mv(rec, dup);
            break;
        } else {
            o = SSTnone;
            break;
        }
    }
    return o;
}

// Get a record by its type, id. Set type to 0 if insignificant.
fun ok64 X(SST, get)(u8* type, u8c$ val, X(SST, ) sst, Key const* key) {
    u8cs rest = {};
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
    sane(Bok(sst) && fd != NULL && *fd != FILE_CLOSED && tab != NULL);
    call(SKIPu8finish, sst, tab);
    call(FILEresize, fd, Busysize(sst));
    SSTheader* head = (SSTheader*)*sst;
    head->metalen = Bpastlen(sst);
    head->datalen = Bdatalen(sst);
    call(FILEclose, fd);
    done;
}

fun ok64 X(SST, close)(X(SST, ) sst) { return FILEunmap(sst); }
