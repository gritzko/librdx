#include "URI.h"

#include <stdlib.h>

#include "B.h"
#include "FILE.h"
#include "PRO.h"
#include "TEST.h"

pro(URItest1) {
    sane(1);
#define LEN1 5
    u8cs inputs[LEN1] = {
        $u8str("http://mit.edu"),
        $u8str("git+ssh://git@github.com/gritzko/librdx"),
        $u8str("ftp://1.2.3.4/some/path"),
        $u8str("//1.2.3.4/some/path"),
        $u8str("http://myserver:123/path?query#fragment"),
    };

    for (int i = 0; i < LEN1; ++i) {
        URIstate state = {};
        $mv(state.text, inputs[i]);
        call(URIlexer, &state);
    }
    done;
}

pro(URItest2) {
    sane(1);
    a$str(uri, "http://myserver:123/path?query#fragment");
    a$str(scheme, "http");
    a$str(host, "myserver");
    a$str(port, "123");
    a$str(path, "/path");
    a$str(query, "query");
    a$str(fragment, "fragment");
    URIstate state = {};
    $mv(state.text, uri);
    call(URIlexer, &state);
    $println(state.scheme);
    $testeq(state.scheme, scheme);
    $testeq(state.host, host);
    $testeq(state.port, port);
    $testeq(state.path, path);
    $testeq(state.query, query);
    $testeq(state.fragment, fragment);
    done;
}

pro(URItest) {
    sane(1);
    call(URItest1);
    call(URItest2);
    done;
}

TEST(URItest);
