#include "DOG.h"

#include <string.h>

#include "abc/PRO.h"

ok64 DOGParseURI(urip uri, u8csc text) {
    sane(uri != NULL);
    memset(uri, 0, sizeof(*uri));
    uri->data[0] = text[0];
    uri->data[1] = text[1];
    call(URILexer, uri);

    // Dog normalization: bare `host:path` with no `//` ends up
    // as scheme="host", path="path".  Promote scheme to authority
    // when authority is empty and the path has no leading slash
    // (proper RFC schemes have either an authority or a path
    // starting with '/').
    if (!$empty(uri->scheme) && $empty(uri->authority)) {
        b8 rooted = !$empty(uri->path) && $at(uri->path, 0) == '/';
        if (!rooted) {
            u8csMv(uri->authority, uri->scheme);
            u8csMv(uri->host, uri->scheme);
            uri->scheme[0] = NULL;
            uri->scheme[1] = NULL;
        }
    }

    done;
}
