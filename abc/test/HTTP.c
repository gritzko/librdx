#include "HTTP.h"

#include "FILE.h"
#include "PRO.h"
#include "TEST.h"
#include "URI.h"

ok64 HTTPtest1() {
    sane(1);
    a$str(
        req,
        "GET / HTTP/1.1\r\n"
        "Host: ya.ru\r\n"
        "User-Agent: Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:73.0) "
        "Gecko/20100101 Firefox/73.0\r\n"
        "Accept: "
        "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/"
        "*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "DNT: 1\r\n"
        "Connection: keep-alive\r\n"
        "Cookie: yandexuid=5055715831606027475; is_gdpr=0; "
        "is_gdpr_b=CMmFQhD5DSgC; yp=1608619476.ygu.1; mda=0; yandex_gid=213; "
        "i=edZPZPFfsEd6HpIjWcvuW7z+f0AyBX02zdlItl9O5ZaxMFBzRn7krCU3EugUD+Hk4Mt+"
        "qUvYTCr6oR4M6k/hnDZ4Od4=\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "Cache-Control: max-age=0\r\n\r\n");
    a$str(
        res,
        "HTTP/1.1 200 Ok\r\n"
        "Accept-CH: Viewport-Width, DPR, Device-Memory, RTT, Downlink, ECT\r\n"
        "Accept-CH-Lifetime: 31536000\r\n"
        "Cache-Control: no-cache,no-store,max-age=0,must-revalidate\r\n"
        "Content-Encoding: gzip\r\n"
        "Content-Security-Policy: report-uri "
        "https://csp.yandex.net/"
        "csp?project=morda&from=morda.yaru.ru&showid=1606027548.67951.103386."
        "564645&h=stable-morda-yaru-sas-yp-10&csp=new&date=20201122&yandexuid="
        "5055715831606027475;style-src 'unsafe-inline';connect-src "
        "wss://webasr.voicetech.yandex.net https://mc.admetrica.ru "
        "https://mc.yandex.ru https://yandex.ru;default-src 'NONE';img-src "
        "https://mc.yandex.ru https://yandex.ru https://yastatic.net 'self' "
        "https://favicon.yandex.net https://avatars.mds.yandex.net "
        "https://*.verify.yandex.ru https://mc.yandex.com "
        "https://mc.admetrica.ru data:;frame-src https://mc.yandex.ru "
        "https://mc.yandex.md 'self' blob:;script-src https://mc.yandex.ru "
        "https://yandex.ru https://yastatic.net blob: 'unsafe-inline'\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Date: Sun, 22 Nov 2020 06:45:48 GMT\r\n"
        "Expires: Sun, 22 Nov 2020 06:45:49 GMT\r\n"
        "Last-Modified: Sun, 22 Nov 2020 06:45:49 GMT\r\n"
        "P3P: policyref=\"/w3c/p3p.xml\", CP=\"NON DSP ADM DEV PSD IVDo OUR "
        "IND STP PHY PRE NAV UNI\"\r\n"
        "Transfer-Encoding: chunked\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: DENY\r\n\r\n");

    // Test request parsing
    HTTPstate reqstate = {};
    aBpad2(u8cs, hdrs, 32);
    reqstate.headers = hdrsidle;
    call(HTTPutf8Drain, req, &reqstate);

    a$str(method, "GET");
    $testeq(reqstate.method, method);
    a$str(path, "/");
    $testeq(reqstate.uri, path);
    a$str(ver, "HTTP/1.1");
    $testeq(reqstate.version, ver);

    a$str(key, "Connection");
    a$str(val, "keep-alive");
    u8cs conn = {};
    call(HTTPfind, &conn, key, hdrsdata);
    $testeq(conn, val);

    // Test response parsing
    HTTPstate resstate = {};
    u8csbReset(hdrsbuf);
    resstate.headers = hdrsidle;
    call(HTTPutf8Drain, res, &resstate);

    a$str(code, "200");
    $testeq(resstate.status_code, code);
    a$str(reason, "Ok");
    $testeq(resstate.reason, reason);

    a$str(key2, "Content-Encoding");
    a$str(val2, "gzip");
    u8cs enc = {};
    call(HTTPfind, &enc, key2, hdrsdata);
    $testeq(enc, val2);

    done;
}

// Real HTTP request captured via netcat (to rfc-editor.org)
ok64 HTTPtest2() {
    sane(1);
    a$str(
        req,
        "GET /rfc/rfc3986 HTTP/1.1\r\n"
        "Host: www.rfc-editor.org\r\n"
        "Accept: text/html\r\n"
        "User-Agent: librdx/1.0\r\n\r\n");

    HTTPstate state = {};
    aBpad2(u8cs, hdrs, 16);
    state.headers = hdrsidle;
    call(HTTPutf8Drain, req, &state);

    a$str(method, "GET");
    $testeq(state.method, method);
    a$str(path, "/rfc/rfc3986");
    $testeq(state.uri, path);
    a$str(ver, "HTTP/1.1");
    $testeq(state.version, ver);

    a$str(k1, "Host");
    a$str(v1, "www.rfc-editor.org");
    u8cs host = {};
    call(HTTPfind, &host, k1, hdrsdata);
    $testeq(host, v1);

    a$str(k2, "User-Agent");
    a$str(v2, "librdx/1.0");
    u8cs ua = {};
    call(HTTPfind, &ua, k2, hdrsdata);
    $testeq(ua, v2);

    done;
}

// Real HTTP response from rfc-editor.org (RFC 3986)
ok64 HTTPtest3() {
    sane(1);
    a$str(
        res,
        "HTTP/1.1 200 OK\r\n"
        "Date: Fri, 09 Jan 2026 16:12:58 GMT\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: keep-alive\r\n"
        "CF-RAY: 9bb53822f8e71c42-FRA\r\n"
        "Vary: Accept-Encoding\r\n"
        "Strict-Transport-Security: max-age=31536000; includeSubDomains\r\n"
        "X-Frame-Options: SAMEORIGIN\r\n"
        "X-Xss-Protection: 1; mode=block\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Last-Modified: Tue, 06 Jan 2026 23:17:00 GMT\r\n"
        "CF-Cache-Status: HIT\r\n"
        "Age: 232120\r\n"
        "Expires: Fri, 09 Jan 2026 16:32:58 GMT\r\n"
        "Cache-Control: public, max-age=1200\r\n"
        "Server: cloudflare\r\n"
        "alt-svc: h3=\":443\"; ma=86400\r\n\r\n");

    HTTPstate state = {};
    aBpad2(u8cs, hdrs, 64);
    state.headers = hdrsidle;
    call(HTTPutf8Drain, res, &state);

    a$str(code, "200");
    $testeq(state.status_code, code);
    a$str(reason, "OK");
    $testeq(state.reason, reason);
    a$str(ver, "HTTP/1.1");
    $testeq(state.version, ver);

    a$str(k1, "Content-Type");
    a$str(v1, "text/html; charset=UTF-8");
    u8cs ct = {};
    call(HTTPfind, &ct, k1, hdrsdata);
    $testeq(ct, v1);

    a$str(k2, "Server");
    a$str(v2, "cloudflare");
    u8cs srv = {};
    call(HTTPfind, &srv, k2, hdrsdata);
    $testeq(srv, v2);

    a$str(k3, "Cache-Control");
    a$str(v3, "public, max-age=1200");
    u8cs cc = {};
    call(HTTPfind, &cc, k3, hdrsdata);
    $testeq(cc, v3);

    done;
}

ok64 HTTPtest() {
    sane(1);
    call(HTTPtest1);
    call(HTTPtest2);
    call(HTTPtest3);
    done;
}

TEST(HTTPtest);
