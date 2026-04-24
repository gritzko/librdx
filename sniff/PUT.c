//  PUT: append `put <path>` rows to sniff's ULOG.
//
#include "PUT.h"

#include "abc/BUF.h"
#include "abc/PRO.h"

#include "AT.h"

//  Pick the best bytes to treat as the path for this URI.  CLI's
//  DOGNormalizeArg classifies bare `a.txt`-style tokens into `query`
//  (ref-safe heuristic), so `uris[i].path` is often empty for the
//  sniff put/delete CLI.  Fall back through path → query → data.
static void put_path_bytes(uri const *u, u8cs out) {
    if (!u8csEmpty(u->path))     { out[0] = u->path[0];     out[1] = u->path[1];     return; }
    if (!u8csEmpty(u->query))    { out[0] = u->query[0];    out[1] = u->query[1];    return; }
    if (!u8csEmpty(u->fragment)) { out[0] = u->fragment[0]; out[1] = u->fragment[1]; return; }
    out[0] = u->data[0]; out[1] = u->data[1];
}

ok64 PUTStage(u32 nuris, uri const *uris) {
    sane(SNIFF.h && (nuris == 0 || uris != NULL));

    ron60 verb = SNIFFAtVerbPut();
    for (u32 i = 0; i < nuris; i++) {
        u8cs raw = {};
        put_path_bytes(&uris[i], raw);
        if (u8csEmpty(raw)) continue;

        //  Construct the row URI directly: `put` rows carry a single
        //  component (the path), so we set uri.path from raw bytes
        //  and let ULOGAppend → URIutf8Feed serialize it.
        uri urow = {};
        urow.path[0] = raw[0];
        urow.path[1] = raw[1];

        call(SNIFFAtAppend, verb, &urow);
    }
    done;
}
