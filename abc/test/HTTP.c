#include "HTTP.h"

#include "FILE.h"
#include "PRO.h"
#include "TEST.h"
#include "URI.h"

pro(HTTPtest1) {
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

    HTTPstate reqstate = {};
    aBpad2(u8cs, parse, 32);
    $mv(reqstate.text, req);
    reqstate.parsed = parseidle;
    call(HTTPlexer, &reqstate);
    a$str(method, "GET");
    $testeq(method, $at(parsedata, 0));
    a$str(path, "/");
    $testeq(path, $at(parsedata, 1));
    a$str(key, "Connection");
    a$str(val, "keep-alive");
    u8cs conn = {};
    call(HTTPfind, conn, key, parsedata);
    $testeq(conn, val);

    HTTPstate resstate = {};
    u8csbReset(parsebuf);
    $mv(resstate.text, res);
    resstate.parsed = parseidle;
    call(HTTPlexer, &resstate);
    a$str(key2, "Content-Encoding");
    a$str(val2, "gzip");
    u8cs conn2 = {};
    call(HTTPfind, conn2, key2, parsedata);
    $testeq(conn2, val2);

    done;
}

pro(HTTPtest) {
    sane(1);
    call(HTTPtest1);
    done;
}

TEST(HTTPtest);
