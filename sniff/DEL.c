//  DEL: append `delete <path>` rows to sniff's ULOG.
//
#include "DEL.h"

#include "abc/BUF.h"
#include "abc/PRO.h"

#include "AT.h"

//  Same path-extraction fallback as PUT: CLI classifies bare names as
//  query, so path is often empty.  Prefer path → query → fragment → data.
static void del_path_bytes(uri const *u, u8cs out) {
    if (!u8csEmpty(u->path))     { out[0] = u->path[0];     out[1] = u->path[1];     return; }
    if (!u8csEmpty(u->query))    { out[0] = u->query[0];    out[1] = u->query[1];    return; }
    if (!u8csEmpty(u->fragment)) { out[0] = u->fragment[0]; out[1] = u->fragment[1]; return; }
    out[0] = u->data[0]; out[1] = u->data[1];
}

ok64 DELStage(u32 nuris, uri const *uris) {
    sane(SNIFF.h && (nuris == 0 || uris != NULL));

    ron60 verb = SNIFFAtVerbDelete();
    for (u32 i = 0; i < nuris; i++) {
        u8cs raw = {};
        del_path_bytes(&uris[i], raw);
        if (u8csEmpty(raw)) continue;

        //  Compose the row URI directly via its path component; ULOG
        //  serializes via URIutf8Feed, no hand-built text.
        uri urow = {};
        urow.path[0] = raw[0];
        urow.path[1] = raw[1];

        call(SNIFFAtAppend, verb, &urow);
    }
    done;
}
