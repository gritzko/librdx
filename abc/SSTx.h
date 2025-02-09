#include <fcntl.h>

#include "B.h"
#include "LSM.h"
#include "SST.h"
#include "TLV.h"
#include "abc/FILE.h"
#include "abc/SKIP.h"

typedef Bu8 X(SST, );
typedef X(, ) Key;

static const u32 X(SST, magic) = (u32)'S' | ((u32)'S' << 8) | ((u32)'T' << 16) |
                                 ((u32)('0' + (63 - clz64(sizeof(Key)))) << 24);

fun ok64 X(SST, create)(X(SST, ) sst, int* fd, $u8c path, size_t size) {
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

fun ok64 X(SST, feedkv)(X(SST, ) sst, SSTab* tab, $u8c rec) {
    ok64 o = $u8feedall(Bu8idle(sst), rec);
    if (o == OK) o = SKIPu8mayfeed(sst, tab);
    return o;
}

fun int X(SST, cmp)($cc needle, $cc sub) {
    Key* nk = (Key*)*needle;
    u8 t = 0;
    $u8c k = {}, v = {};
    TLVdrainkv(&t, k, v, (u8c**)sub);
    Key* vk = (Key*)*k;
    return X(, cmp)(nk, vk);
}

fun ok64 X(SST, locate)(u8c$ rest, X(SST, ) sst, Key const* key) {
    a$rawcp(keyraw, key);
    return SKIPu8find(rest, sst, keyraw, X(SST, cmp));
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

fun ok64 X(SST, get)(u8* t, u8c$ val, X(SST, ) sst, Key const* key) {
    $u8c rest = {};
    ok64 o = X(SST, locate)(rest, sst, key);
    while (o == OK) {
        Key k = {};
        o = X(SST, next)(t, &k, val, rest);
        int z = X(, cmp)(&k, key);
        if (o != OK || z == 0) break;
        if (z > 0) {
            o = SSTnone;
            break;
        }
    }
    return o;
}

ok64 X(SST, mget)($u8 into, BBu8 inputs, Key const* key, $u8cYfn y);
ok64 X(SST, merge)(X(SST, ) into, BBu8 inputs, $u8cYfn y);

fun ok64 X(SST, closenew)(X(SST, ) sst, int* fd, SSTab* tab) {
    sane(Bok(sst) && fd != nil && *fd != FILE_CLOSED && tab != nil);
    call(SKIPu8finish, sst, tab);
    call(FILEresize, fd, Busysize(sst));
    SSTheader* head = (SSTheader*)*sst;
    head->metalen = Bpastlen(sst);
    head->datalen = Bdatalen(sst);
    call(FILEclose, fd);
    call(FILEunmap, sst);
    done;
}

fun ok64 X(SST, close)(X(SST, ) sst) { return FILEunmap(sst); }
