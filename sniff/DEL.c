//  DEL: append `delete <path>` rows to sniff's ULOG.
//
#include "DEL.h"

#include "abc/BUF.h"
#include "abc/PRO.h"

#include "AT.h"

ok64 DELStage(u32 nuris, uri const *uris) {
    sane(SNIFF.h && (nuris == 0 || uris != NULL));

    ron60 verb = SNIFFAtVerbDelete();
    for (u32 i = 0; i < nuris; i++) {
        u8cs raw = {};
        SNIFFAtPathBytes(&uris[i], raw);
        if (u8csEmpty(raw)) continue;

        uri urow = {};
        urow.path[0] = raw[0];
        urow.path[1] = raw[1];

        call(SNIFFAtAppend, verb, &urow);
    }
    done;
}
