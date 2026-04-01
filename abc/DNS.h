#ifndef ABC_DNS_H
#define ABC_DNS_H

#include "INT.h"

con ok64 DNSFAIL = 0x1b3593ca495;
con ok64 DNSBAD = 0x1b359a25dab;
con ok64 DNSNOROOM = 0x1b35d86d8616;
con ok64 DNSNODATA = 0x1b35d834a74a;
con ok64 DNSBADNAME = 0x6cd72ca35d85ce;

#define flip16(x) ((u16)(((x) >> 8) | ((x) << 8)))

// --- wire header (12 bytes) ---

//  0  1  2  3  4  5  6  7  8  9  A  B
// [    id     ] [   flags  ] [  qdcount ]
// [ ancount   ] [ nscount  ] [ arcount  ]

#define DNS_QR 0x8000
#define DNS_AA 0x0400
#define DNS_TC 0x0200
#define DNS_RD 0x0100
#define DNS_RA 0x0080
#define DNS_AD 0x0020
#define DNS_CD 0x0010

#define DNS_OP_MASK 0x7800
#define DNS_OP_SHIFT 11
#define DNS_RC_MASK 0x000f

#define DNS_OP_QUERY 0
#define DNS_OP_IQUERY 1
#define DNS_OP_STATUS 2

#define DNS_RC_OK 0
#define DNS_RC_FMTERR 1
#define DNS_RC_SRVFAIL 2
#define DNS_RC_NXDOMAIN 3
#define DNS_RC_NOTIMP 4
#define DNS_RC_REFUSED 5

#define DNS_TYPE_A 1
#define DNS_TYPE_NS 2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_SOA 6
#define DNS_TYPE_PTR 12
#define DNS_TYPE_MX 15
#define DNS_TYPE_TXT 16
#define DNS_TYPE_AAAA 28
#define DNS_TYPE_SRV 33
#define DNS_TYPE_ANY 255

#define DNS_CLASS_IN 1

#define DNS_HDR_LEN 12
#define DNS_NAME_MAX 255
#define DNS_LABEL_MAX 63
#define DNS_MSG_MAX 512

typedef struct {
    u16 id;
    u16 flags;
    u16 qdcount;
    u16 ancount;
    u16 nscount;
    u16 arcount;
} DNShdr;

typedef struct {
    u8cs name;   // wire-encoded name in the packet
    u16 type;
    u16 class;
} DNSquest;

typedef struct {
    u8cs name;
    u16 type;
    u16 class;
    u32 ttl;
    u8cs rdata;
} DNSrr;

// --- big-endian 16/32 on wire ---

fun ok64 DNSu8sFeed16(u8s into, u16 val) {
    u16 be = flip16(val);
    return u8sFeed16(into, &be);
}

fun ok64 DNSu8sDrain16(u8cs from, u16 *val) {
    u16 be = 0;
    ok64 o = u8sDrain16(from, &be);
    if (o != OK) return o;
    *val = flip16(be);
    return OK;
}

fun ok64 DNSu8sFeed32(u8s into, u32 val) {
    u32 be = flip32(val);
    return u8sFeed32(into, &be);
}

fun ok64 DNSu8sDrain32(u8cs from, u32 *val) {
    u32 be = 0;
    ok64 o = u8sDrain32(from, &be);
    if (o != OK) return o;
    *val = flip32(be);
    return OK;
}

// --- header ---

fun ok64 DNSHdrFeed(u8s into, DNShdr const *h) {
    if ($len(into) < DNS_HDR_LEN) return DNSNOROOM;
    ok64 o = OK;
    o = DNSu8sFeed16(into, h->id);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, h->flags);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, h->qdcount);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, h->ancount);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, h->nscount);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, h->arcount);
    return o;
}

fun ok64 DNSHdrDrain(u8cs from, DNShdr *h) {
    if ($len(from) < DNS_HDR_LEN) return DNSNODATA;
    ok64 o = OK;
    o = DNSu8sDrain16(from, &h->id);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &h->flags);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &h->qdcount);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &h->ancount);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &h->nscount);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &h->arcount);
    return o;
}

// --- name encoding ---
// Wire name: sequence of (len, label...) terminated by 0x00.
// Compression: if top 2 bits of len are 11, the remaining 14 bits
// are an offset into the packet (pointer).

// Feed a dotted text name ("www.example.com") as wire labels.
fun ok64 DNSNameFeed(u8s into, u8csc text) {
    a_dup(u8c, src, text);
    while (!$empty(src)) {
        // find next dot or end
        u8c *dot = *src;
        while (dot < src[1] && *dot != '.') ++dot;
        size_t llen = dot - *src;
        if (llen == 0) {
            // trailing dot or double dot — skip
            ++*src;
            continue;
        }
        if (llen > DNS_LABEL_MAX) return DNSBADNAME;
        if ($len(into) < 1 + llen) return DNSNOROOM;
        u8sFeed1(into, (u8)llen);
        a_head(u8c, label, src, llen);
        u8sFeed(into, label);
        *src = dot;
        if (!$empty(src)) ++*src;  // skip dot
    }
    // terminating zero label
    if ($empty(into)) return DNSNOROOM;
    u8sFeed1(into, 0);
    return OK;
}

// Drain a wire name, handling compression pointers.
// `pkt` is the full packet (for pointer chasing).
// `from` is advanced past the name bytes in the packet.
// `into` receives the decompressed wire name (no pointers).
fun ok64 DNSNameDrain(u8s into, u8cs from, u8csc pkt) {
    u8c *p = *from;
    b8 jumped = NO;
    int hops = 0;
    while (p < pkt[1]) {
        u8 len = *p;
        if (len == 0) {
            if (!jumped) *from = p + 1;
            if ($empty(into)) return DNSNOROOM;
            u8sFeed1(into, 0);
            return OK;
        }
        if ((len & 0xc0) == 0xc0) {
            // compression pointer
            if (p + 1 >= pkt[1]) return DNSBAD;
            u16 off = ((u16)(len & 0x3f) << 8) | *(p + 1);
            if (!jumped) *from = p + 2;
            jumped = YES;
            if (off >= $len(pkt)) return DNSBAD;
            p = *pkt + off;
            if (++hops > DNS_NAME_MAX) return DNSBAD;  // loop guard
            continue;
        }
        if (len > DNS_LABEL_MAX) return DNSBADNAME;
        if (p + 1 + len > pkt[1]) return DNSNODATA;
        if ($len(into) < 1 + len) return DNSNOROOM;
        u8sFeed1(into, len);
        a_head(u8c, label, ((u8cs){p + 1, pkt[1]}), len);
        u8sFeed(into, label);
        p += 1 + len;
    }
    return DNSBAD;
}

// Drain a wire name to dotted text ("www.example.com").
fun ok64 DNSNameText(u8s into, u8csc wire) {
    u8 *start = *into;
    a_dup(u8c, src, wire);
    while (!$empty(src)) {
        u8 len = **src;
        ++*src;
        if (len == 0) return OK;
        if (len > DNS_LABEL_MAX) return DNSBADNAME;
        if ($len(src) < len) return DNSNODATA;
        if (*into > start) {
            if ($empty(into)) return DNSNOROOM;
            u8sFeed1(into, '.');
        }
        a_head(u8c, label, src, len);
        u8sFeed(into, label);
        *src += len;
    }
    return OK;
}

// --- question section ---

fun ok64 DNSQuestFeed(u8s into, u8csc name_text, u16 type, u16 class) {
    ok64 o = DNSNameFeed(into, name_text);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, type);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, class);
    return o;
}

fun ok64 DNSQuestDrain(u8cs from, u8csc pkt, DNSquest *q, u8s namebuf) {
    u8 *save = *namebuf;
    ok64 o = DNSNameDrain(namebuf, from, pkt);
    if (o != OK) return o;
    q->name[0] = save;
    q->name[1] = *namebuf;
    o = DNSu8sDrain16(from, &q->type);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &q->class);
    return o;
}

// --- resource record ---

fun ok64 DNSRRFeed(u8s into, u8csc name_text, u16 type, u16 class,
                   u32 ttl, u8csc rdata) {
    ok64 o = DNSNameFeed(into, name_text);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, type);
    if (o != OK) return o;
    o = DNSu8sFeed16(into, class);
    if (o != OK) return o;
    o = DNSu8sFeed32(into, ttl);
    if (o != OK) return o;
    u16 rdlen = (u16)$len(rdata);
    o = DNSu8sFeed16(into, rdlen);
    if (o != OK) return o;
    return u8sFeed(into, rdata);
}

fun ok64 DNSRRDrain(u8cs from, u8csc pkt, DNSrr *rr, u8s namebuf) {
    u8 *save = *namebuf;
    ok64 o = DNSNameDrain(namebuf, from, pkt);
    if (o != OK) return o;
    rr->name[0] = save;
    rr->name[1] = *namebuf;
    o = DNSu8sDrain16(from, &rr->type);
    if (o != OK) return o;
    o = DNSu8sDrain16(from, &rr->class);
    if (o != OK) return o;
    o = DNSu8sDrain32(from, &rr->ttl);
    if (o != OK) return o;
    u16 rdlen = 0;
    o = DNSu8sDrain16(from, &rdlen);
    if (o != OK) return o;
    if ($len(from) < rdlen) return DNSNODATA;
    rr->rdata[0] = *from;
    *from += rdlen;
    rr->rdata[1] = *from;
    return OK;
}

// --- convenience: build a query packet ---

fun ok64 DNSQueryBuild(u8s into, u16 id, u8csc name, u16 type) {
    DNShdr h = {.id = id, .flags = DNS_RD, .qdcount = 1};
    ok64 o = DNSHdrFeed(into, &h);
    if (o != OK) return o;
    return DNSQuestFeed(into, name, type, DNS_CLASS_IN);
}

// --- convenience: feed an A record (4 bytes rdata) ---

fun ok64 DNSRRFeedA(u8s into, u8csc name, u32 ttl, u32 ip4) {
    u32 be = flip32(ip4);
    u8c *rd[2] = {(u8c *)&be, (u8c *)&be + 4};
    return DNSRRFeed(into, name, DNS_TYPE_A, DNS_CLASS_IN, ttl, rd);
}

// --- convenience: feed an AAAA record (16 bytes rdata) ---

fun ok64 DNSRRFeedAAAA(u8s into, u8csc name, u32 ttl, u8csc ip6) {
    return DNSRRFeed(into, name, DNS_TYPE_AAAA, DNS_CLASS_IN, ttl, ip6);
}

#endif
