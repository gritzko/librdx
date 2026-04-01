#include <sys/socket.h>

#include "DNS.h"

#include "FILE.h"
#include "PRO.h"
#include "TEST.h"
#include "UDP.h"

// --- wire format unit tests ---

ok64 DNSTestNameRoundtrip() {
    sane(1);
    a$str(name, "www.example.com");
    a_pad(u8, wire, 256);
    call(DNSNameFeed, wire_idle, name);

    // wire should be: 3www7example3com0
    u8 *d = *wire_data;
    want(d[0] == 3);
    want(d[1] == 'w');
    want(d[4] == 7);
    want(d[12] == 3);
    want(d[16] == 0);

    a_pad(u8, text, 256);
    call(DNSNameText, text_idle, wire_datac);

    $testeq(name, text_data);
    done;
}

ok64 DNSTestHdrRoundtrip() {
    sane(1);
    DNShdr h = {
        .id = 0x1234,
        .flags = DNS_RD | DNS_QR,
        .qdcount = 1,
        .ancount = 2,
        .nscount = 0,
        .arcount = 0,
    };
    a_pad(u8, buf, 64);
    call(DNSHdrFeed, buf_idle, &h);

    DNShdr h2 = {};
    u8cs hdr_from = {};
    u8csMv(hdr_from, buf_datac);
    call(DNSHdrDrain, hdr_from, &h2);
    testeq(h.id, h2.id);
    testeq(h.flags, h2.flags);
    testeq(h.qdcount, h2.qdcount);
    testeq(h.ancount, h2.ancount);
    done;
}

ok64 DNSTestQueryBuild() {
    sane(1);
    a_pad(u8, pkt, DNS_MSG_MAX);
    a$str(name, "google.com");
    call(DNSQueryBuild, pkt_idle, 0xBEEF, name, DNS_TYPE_A);

    // parse it back
    u8cs from = {};
    u8csMv(from, pkt_datac);
    DNShdr h = {};
    call(DNSHdrDrain, from, &h);
    testeq(h.id, 0xBEEF);
    testeq(h.flags, DNS_RD);
    testeq(h.qdcount, 1);

    a_pad(u8, nb, 256);
    DNSquest q = {};
    call(DNSQuestDrain, from, pkt_datac, &q, nb_idle);
    testeq(q.type, DNS_TYPE_A);
    testeq(q.class, DNS_CLASS_IN);

    a_pad(u8, txt, 256);
    call(DNSNameText, txt_idle, q.name);
    a$str(expected, "google.com");
    $testeq(expected, txt_data);
    done;
}

ok64 DNSTestCompression() {
    sane(1);
    // Build a packet with a name, then a pointer back to it
    a_pad(u8, pkt, 256);

    // header placeholder (12 bytes)
    DNShdr h = {.id = 1, .qdcount = 1, .ancount = 1};
    call(DNSHdrFeed, pkt_idle, &h);

    // question: "example.com" at offset 12
    a$str(name, "example.com");
    call(DNSNameFeed, pkt_idle, name);
    call(DNSu8sFeed16, pkt_idle, DNS_TYPE_A);
    call(DNSu8sFeed16, pkt_idle, DNS_CLASS_IN);

    // answer: compressed name pointing to offset 12
    u8sFeed1(pkt_idle, 0xc0);
    u8sFeed1(pkt_idle, 12);
    call(DNSu8sFeed16, pkt_idle, DNS_TYPE_A);
    call(DNSu8sFeed16, pkt_idle, DNS_CLASS_IN);
    call(DNSu8sFeed32, pkt_idle, 300);
    // rdata: 4 bytes IP
    u16 rdlen = 4;
    call(DNSu8sFeed16, pkt_idle, rdlen);
    u8sFeed1(pkt_idle, 1);
    u8sFeed1(pkt_idle, 2);
    u8sFeed1(pkt_idle, 3);
    u8sFeed1(pkt_idle, 4);

    // now parse the answer RR
    u8cs from = {};
    u8csMv(from, pkt_datac);
    // skip header
    from[0] += DNS_HDR_LEN;
    // skip question
    DNSquest q = {};
    a_pad(u8, nb, 256);
    call(DNSQuestDrain, from, pkt_datac, &q, nb_idle);

    // parse answer RR with compression
    a_pad(u8, nb2, 256);
    DNSrr rr = {};
    call(DNSRRDrain, from, pkt_datac, &rr, nb2_idle);

    testeq(rr.type, DNS_TYPE_A);
    testeq(rr.ttl, 300);
    testeq($len(rr.rdata), 4);
    want(*rr.rdata[0] == 1);

    // decompressed name should be "example.com"
    a_pad(u8, txt, 256);
    call(DNSNameText, txt_idle, rr.name);
    $testeq(name, txt_data);

    done;
}

// --- live resolution tests ---

ok64 DNSResolve(u8cs server, u8csc name, u16 type,
                DNShdr *rh, DNSrr *rr, u8s rrbuf) {
    sane(6);
    // build query
    a_pad(u8, qpkt, DNS_MSG_MAX);
    call(DNSQueryBuild, qpkt_idle, 0xABCD, name, type);

    // connected UDP: send/recv without addr
    int fd = -1;
    ok64 o = UDPConnect(&fd, server);
    if (o != OK) return o;
    o = FILEFeedall(fd, qpkt_datac);
    if (o != OK) { UDPClose(fd); return o; }

    // receive with timeout
    struct timeval tv = {.tv_sec = 3};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    a_pad(u8, rpkt, DNS_MSG_MAX);
    ssize_t nr = recv(fd, *rpkt_idle, $len(rpkt_idle), 0);
    UDPClose(fd);
    if (nr <= DNS_HDR_LEN) return DNSFAIL;
    *rpkt_idle += nr;

    // parse header
    u8cs from = {};
    u8csMv(from, rpkt_datac);
    call(DNSHdrDrain, from, rh);
    testeq(rh->id, 0xABCD);
    want(rh->flags & DNS_QR);

    // skip questions
    for (u16 i = 0; i < rh->qdcount; i++) {
        a_pad(u8, nb, 256);
        DNSquest q = {};
        call(DNSQuestDrain, from, rpkt_datac, &q, nb_idle);
    }

    // parse first answer
    want(rh->ancount > 0);
    call(DNSRRDrain, from, rpkt_datac, rr, rrbuf);
    done;
}

ok64 DNSTestResolveGoogle() {
    sane(1);
    a_cstr(server, "udp://1.1.1.1:53");
    a$str(name, "google.com");

    DNShdr rh = {};
    DNSrr rr = {};
    a_pad(u8, nb, 256);
    ok64 o = DNSResolve(server, name, DNS_TYPE_A, &rh, &rr, nb_idle);
    if (o == DNSFAIL) {
        fprintf(stderr, "  google.com via 1.1.1.1: no network, skipped\n");
        return OK;
    }
    if (o != OK) return o;

    // may get CNAME first, but eventually an A
    want(rr.type == DNS_TYPE_A || rr.type == DNS_TYPE_CNAME);
    if (rr.type == DNS_TYPE_A) {
        testeq($len(rr.rdata), 4);
    }

    fprintf(stderr, "  google.com via 1.1.1.1: type=%u rcode=%u answers=%u\n",
            rr.type, rh.flags & DNS_RC_MASK, rh.ancount);
    done;
}

ok64 DNSTestResolveCloudflare() {
    sane(1);
    a_cstr(server, "udp://8.8.8.8:53");
    a$str(name, "cloudflare.com");

    DNShdr rh = {};
    DNSrr rr = {};
    a_pad(u8, nb, 256);
    ok64 o = DNSResolve(server, name, DNS_TYPE_A, &rh, &rr, nb_idle);
    if (o == DNSFAIL) {
        fprintf(stderr, "  cloudflare.com via 8.8.8.8: no network, skipped\n");
        return OK;
    }
    if (o != OK) return o;

    want(rr.type == DNS_TYPE_A || rr.type == DNS_TYPE_CNAME);
    if (rr.type == DNS_TYPE_A) {
        testeq($len(rr.rdata), 4);
    }

    fprintf(stderr, "  cloudflare.com via 8.8.8.8: type=%u rcode=%u answers=%u\n",
            rr.type, rh.flags & DNS_RC_MASK, rh.ancount);
    done;
}

ok64 DNStest() {
    sane(1);
    call(DNSTestNameRoundtrip);
    call(DNSTestHdrRoundtrip);
    call(DNSTestQueryBuild);
    call(DNSTestCompression);
    call(DNSTestResolveGoogle);
    call(DNSTestResolveCloudflare);
    done;
}

TEST(DNStest);
