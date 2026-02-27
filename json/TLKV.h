#ifndef LIBRDX_TLKV_H
#define LIBRDX_TLKV_H

#include "abc/TLV.h"

// Can we use the short (2-byte header) form?
fun b8 TLKVShort(size_t klen, size_t vlen) {
    return klen <= 15 && vlen <= 15;
}

// Total record size (header + key + value).
fun size_t TLKVLen(size_t klen, size_t vlen) {
    return (TLKVShort(klen, vlen) ? 2 : 6) + klen + vlen;
}

// Write a complete TLKV record.
//  Short: [tag|0x20][klen_hi:vlen_lo][key][val]   2B overhead
//  Long:  [TAG     ][vlen 4B LE     ][klen 1B][key][val]  6B overhead
fun ok64 TLKVFeed(u8s into, u8 type, u8csc key, u8csc val) {
    size_t klen = $len(key);
    size_t vlen = $len(val);
    if (klen > 255) return TLVBADARG;
    size_t total = TLKVLen(klen, vlen);
    if ($len(into) < total) return TLVnoroom;
    if (TLKVShort(klen, vlen)) {
        u8sFeed2(into, type | TLVaA, (u8)((klen << 4) | vlen));
    } else {
        u8sFeed1(into, type & ~TLVaA);
        u32 vl = (u32)vlen;
        u8sFeed32(into, &vl);
        u8sFeed1(into, (u8)klen);
    }
    u8sCopy(into, key);
    *into += klen;
    u8sCopy(into, val);
    *into += vlen;
    return OK;
}

// Read a complete TLKV record.
// Sets *type (without case bit), key slice, val slice; advances from.
fun ok64 TLKVDrain(u8cs from, u8p type, u8cs key, u8cs val) {
    if ($len(from) < 2) return TLVNODATA;
    u8 tag = **from;
    size_t klen, vlen;
    if (tag & TLVaA) {
        // short form
        *type = tag & ~TLVaA;
        u8 nibs = *(*from + 1);
        klen = nibs >> 4;
        vlen = nibs & 0x0f;
        *from += 2;
    } else {
        // long form
        if ($len(from) < 6) return TLVNODATA;
        *type = tag;
        u32 vl = 0;
        *from += 1;
        u8sDrain32(from, &vl);
        vlen = vl;
        klen = **from;
        *from += 1;
    }
    if ($len(from) < klen + vlen) return TLVNODATA;
    key[0] = *from;
    key[1] = *from + klen;
    *from += klen;
    val[0] = *from;
    val[1] = *from + vlen;
    *from += vlen;
    return OK;
}

// Open a container record (backpatching pattern).
// Always writes a 6-byte long-form header, then the key.
// Header: [TAG 1B][saved_dl 4B][klen 1B]
fun ok64 TLKVInto(u8bp buf, u8 type, u8csc key) {
    size_t klen = $len(key);
    if (klen > 255) return TLVBADARG;
    if (u8bIdleLen(buf) < 6 + klen) return TLVnoroom;
    size_t dl = u8bDataLen(buf);
    if (unlikely(dl > u32max)) return TLVtoolong;
    u8bFeed1(buf, type & ~TLVaA);
    u8sFeed32(u8bIdle(buf), (u32*)&dl);
    u8bFeed1(buf, (u8)klen);
    ((u8**)buf)[1] = buf[2]; // push header to past
    if (u8bFeed(buf, key) != OK) return TLVnoroom;
    return OK;
}

// Close a container record (backpatch lengths, optionally shrink to short).
fun ok64 TLKVOuto(u8bp buf) {
    if (u8bPastLen(buf) < 6) return TLVNODATA;
    size_t ndl = u8bDataLen(buf);
    a_tail(u8c, hdr, u8bPast(buf), 6);
    // extract saved values from header
    u8 tag = *hdr[0];
    u32 saved_dl = 0;
    hdr[0] += 1;
    memcpy(&saved_dl, hdr[0], 4);
    hdr[0] += 4;
    u8 klen = *hdr[0];
    // vlen = total new data minus key bytes
    size_t vlen = ndl - klen;
    // restore data pointer
    ((u8**)buf)[1] = buf[1] - 6 - saved_dl;
    // patch vlen in long-form header
    u32* vlenp = (u32*)(buf[1] + saved_dl + 1);
    *vlenp = (u32)vlen;
    // try to shrink to short form
    if (klen <= 15 && vlen <= 15) {
        u8* p = buf[1] + saved_dl;
        *p = tag | TLVaA;
        *(p + 1) = (klen << 4) | (u8)vlen;
        u8cs empty = {};
        u8bSplice(buf, saved_dl + 2, 4, empty); // remove 4 bytes
    }
    return OK;
}

#endif
