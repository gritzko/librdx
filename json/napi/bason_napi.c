#include <node_api.h>
#include <stdlib.h>
#include <string.h>

#include "json/BASON.h"
#include "json/BIFF.h"
#include "abc/TLV.h"

// PRO.h globals needed by BASON.c/BIFF.c (sane/call macros)
uint8_t _pro_depth = 0;

// Buffer sizes
#define NAPI_BUF_LEN (4 * 1024 * 1024)
#define NAPI_STK_LEN 256
#define NAPI_IDX_LEN 4096

// ---------- helpers ----------

#define NAPI_THROW(env, msg)                               \
    do {                                                   \
        napi_throw_error(env, NULL, msg);                  \
        return NULL;                                       \
    } while (0)

#define NAPI_OK(env, call)                                 \
    do {                                                   \
        if ((call) != napi_ok) {                           \
            napi_throw_error(env, NULL, #call " failed");  \
            return NULL;                                   \
        }                                                  \
    } while (0)

// ---------- BasonDoc ----------

typedef struct BasonDoc {
    u8    *data;
    size_t data_len;
} BasonDoc;

static void BasonDocFinalize(napi_env env, void *raw, void *hint) {
    (void)env;
    (void)hint;
    BasonDoc *doc = (BasonDoc *)raw;
    if (doc->data) free(doc->data);
    free(doc);
}

// ---------- BasonIter ----------

typedef struct BasonIter {
    u8c      *cursor[2]; // [current, end]
    napi_ref  doc_ref;
} BasonIter;

static void BasonIterFinalize(napi_env env, void *raw, void *hint) {
    (void)hint;
    BasonIter *it = (BasonIter *)raw;
    if (it->doc_ref) napi_delete_reference(env, it->doc_ref);
    free(it);
}

// ---------- BasonView ----------

typedef struct BasonView {
    u8c      *children[2]; // [start, end]
    napi_ref  doc_ref;
} BasonView;

static void BasonViewFinalize(napi_env env, void *raw, void *hint) {
    (void)hint;
    BasonView *vw = (BasonView *)raw;
    if (vw->doc_ref) napi_delete_reference(env, vw->doc_ref);
    free(vw);
}

// Forward declarations for class constructors
static napi_ref g_doc_ctor;
static napi_ref g_iter_ctor;
static napi_ref g_view_ctor;

// ---------- Create JS objects from native data ----------

static napi_value WrapDoc(napi_env env, BasonDoc *doc) {
    napi_value ctor, obj;
    NAPI_OK(env, napi_get_reference_value(env, g_doc_ctor, &ctor));
    NAPI_OK(env, napi_new_instance(env, ctor, 0, NULL, &obj));
    NAPI_OK(env, napi_wrap(env, obj, doc, BasonDocFinalize, NULL, NULL));
    return obj;
}

static napi_value WrapIter(napi_env env, BasonIter *it) {
    napi_value ctor, obj;
    NAPI_OK(env, napi_get_reference_value(env, g_iter_ctor, &ctor));
    NAPI_OK(env, napi_new_instance(env, ctor, 0, NULL, &obj));
    NAPI_OK(env, napi_wrap(env, obj, it, BasonIterFinalize, NULL, NULL));
    return obj;
}

static napi_value WrapView(napi_env env, BasonView *vw) {
    napi_value ctor, obj;
    NAPI_OK(env, napi_get_reference_value(env, g_view_ctor, &ctor));
    NAPI_OK(env, napi_new_instance(env, ctor, 0, NULL, &obj));
    NAPI_OK(env, napi_wrap(env, obj, vw, BasonViewFinalize, NULL, NULL));
    return obj;
}

// ---------- Type char to JS string ----------

static napi_value TypeChar(napi_env env, u8 type) {
    char c;
    switch (type) {
    case 'S': c = 's'; break;
    case 'N': c = 'n'; break;
    case 'B': c = 'b'; break;
    case 'A': c = 'a'; break;
    case 'O': c = 'o'; break;
    default:  c = '?'; break;
    }
    napi_value val;
    NAPI_OK(env, napi_create_string_utf8(env, &c, 1, &val));
    return val;
}

// Convert BASON value to JS value, creating BasonView for containers
static napi_value ValueToJS(napi_env env, u8 type, u8cs val,
                            napi_value doc_js) {
    napi_value result;
    if (type == 'S') {
        NAPI_OK(env, napi_create_string_utf8(env, (const char *)val[0],
                                             (size_t)(val[1] - val[0]),
                                             &result));
    } else if (type == 'N') {
        char tmp[64];
        size_t n = (size_t)(val[1] - val[0]);
        if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
        memcpy(tmp, val[0], n);
        tmp[n] = 0;
        double d = strtod(tmp, NULL);
        NAPI_OK(env, napi_create_double(env, d, &result));
    } else if (type == 'B') {
        if ($len(val) == 0) {
            NAPI_OK(env, napi_get_null(env, &result));
        } else if (val[0][0] == 't') {
            NAPI_OK(env, napi_get_boolean(env, true, &result));
        } else {
            NAPI_OK(env, napi_get_boolean(env, false, &result));
        }
    } else if (BASONPlex(type)) {
        BasonView *vw = (BasonView *)malloc(sizeof(BasonView));
        if (!vw) NAPI_THROW(env, "out of memory");
        vw->children[0] = val[0];
        vw->children[1] = val[1];
        NAPI_OK(env, napi_create_reference(env, doc_js, 1, &vw->doc_ref));
        result = WrapView(env, vw);
    } else {
        NAPI_OK(env, napi_get_undefined(env, &result));
    }
    return result;
}

// ---------- Internal: parse JSON to BasonDoc ----------

static BasonDoc *ParseJSONToDoc(napi_env env, const char *json, size_t len) {
    u8 *buf = (u8 *)malloc(len * 4 + 4096);
    if (!buf) return NULL;
    u8b bbuf = {buf, buf, buf, buf + len * 4 + 4096};
    u64 _idx[NAPI_IDX_LEN];
    u64b idx = {_idx, _idx, _idx, _idx + NAPI_IDX_LEN};
    u8cs input = {(u8cp)json, (u8cp)json + len};
    ok64 o = BASONParseJSON(bbuf, idx, input);
    if (o != OK) {
        free(buf);
        return NULL;
    }
    size_t data_len = (size_t)(bbuf[2] - bbuf[1]);
    BasonDoc *doc = (BasonDoc *)malloc(sizeof(BasonDoc));
    if (!doc) { free(buf); return NULL; }
    // Copy data to tight allocation
    doc->data = (u8 *)malloc(data_len);
    if (!doc->data) { free(doc); free(buf); return NULL; }
    memcpy(doc->data, bbuf[1], data_len);
    doc->data_len = data_len;
    free(buf);
    return doc;
}

// ---------- Module-level functions ----------

// bason.parse(jsonString) → BasonDoc
static napi_value BasonParse(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 1) NAPI_THROW(env, "parse: expected 1 argument");

    size_t len = 0;
    NAPI_OK(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &len));
    char *json = (char *)malloc(len + 1);
    if (!json) NAPI_THROW(env, "out of memory");
    NAPI_OK(env, napi_get_value_string_utf8(env, argv[0], json, len + 1, &len));

    BasonDoc *doc = ParseJSONToDoc(env, json, len);
    free(json);
    if (!doc) NAPI_THROW(env, "parse: invalid JSON");

    return WrapDoc(env, doc);
}

// bason.from(uint8Array) → BasonDoc
static napi_value BasonFrom(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 1) NAPI_THROW(env, "from: expected 1 argument");

    bool is_typedarray;
    NAPI_OK(env, napi_is_typedarray(env, argv[0], &is_typedarray));
    if (!is_typedarray) NAPI_THROW(env, "from: expected Uint8Array");

    u8 *buf;
    size_t len;
    napi_typedarray_type atype;
    NAPI_OK(env, napi_get_typedarray_info(env, argv[0], &atype, &len,
                                          (void **)&buf, NULL, NULL));
    if (atype != napi_uint8_array)
        NAPI_THROW(env, "from: expected Uint8Array");

    BasonDoc *doc = (BasonDoc *)malloc(sizeof(BasonDoc));
    if (!doc) NAPI_THROW(env, "out of memory");
    doc->data = (u8 *)malloc(len);
    if (!doc->data) { free(doc); NAPI_THROW(env, "out of memory"); }
    memcpy(doc->data, buf, len);
    doc->data_len = len;

    return WrapDoc(env, doc);
}

// bason.stringify(doc) → string
static napi_value BasonStringify(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 1) NAPI_THROW(env, "stringify: expected 1 argument");

    BasonDoc *doc;
    NAPI_OK(env, napi_unwrap(env, argv[0], (void **)&doc));

    u8 *out = (u8 *)malloc(doc->data_len * 4 + 4096);
    if (!out) NAPI_THROW(env, "out of memory");
    u8s outslice = {out, out + doc->data_len * 4 + 4096};
    u64 _stk[NAPI_STK_LEN];
    u64b stk = {_stk, _stk, _stk, _stk + NAPI_STK_LEN};
    u8cs data = {(u8cp)doc->data, (u8cp)doc->data + doc->data_len};
    ok64 o = BASONExportJSON(outslice, stk, data);
    if (o != OK) { free(out); NAPI_THROW(env, "stringify: export failed"); }

    size_t json_len = (size_t)(outslice[0] - out);
    napi_value result;
    NAPI_OK(env, napi_create_string_utf8(env, (const char *)out, json_len,
                                         &result));
    free(out);
    return result;
}

// bason.merge(doc1, doc2) → BasonDoc
static napi_value BasonMerge(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 2) NAPI_THROW(env, "merge: expected 2 arguments");

    BasonDoc *ldoc, *rdoc;
    NAPI_OK(env, napi_unwrap(env, argv[0], (void **)&ldoc));
    NAPI_OK(env, napi_unwrap(env, argv[1], (void **)&rdoc));

    size_t out_cap = ldoc->data_len + rdoc->data_len + 4096;
    u8 *obuf = (u8 *)malloc(out_cap);
    if (!obuf) NAPI_THROW(env, "out of memory");
    u8b out = {obuf, obuf, obuf, obuf + out_cap};
    u64 _idx[NAPI_IDX_LEN];
    u64b idx = {_idx, _idx, _idx, _idx + NAPI_IDX_LEN};
    u64 _lstk[NAPI_STK_LEN], _rstk[NAPI_STK_LEN];
    u64b lstk = {_lstk, _lstk, _lstk, _lstk + NAPI_STK_LEN};
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + NAPI_STK_LEN};
    u8cs ldata = {(u8cp)ldoc->data, (u8cp)ldoc->data + ldoc->data_len};
    u8cs rdata = {(u8cp)rdoc->data, (u8cp)rdoc->data + rdoc->data_len};

    ok64 o = BASONMerge(out, idx, lstk, ldata, rstk, rdata);
    if (o != OK) { free(obuf); NAPI_THROW(env, "merge failed"); }

    size_t dlen = (size_t)(out[2] - out[1]);
    BasonDoc *doc = (BasonDoc *)malloc(sizeof(BasonDoc));
    if (!doc) { free(obuf); NAPI_THROW(env, "out of memory"); }
    doc->data = (u8 *)malloc(dlen);
    if (!doc->data) { free(doc); free(obuf); NAPI_THROW(env, "out of memory"); }
    memcpy(doc->data, out[1], dlen);
    doc->data_len = dlen;
    free(obuf);

    return WrapDoc(env, doc);
}

// bason.mergeN(arrayOfDocs) → BasonDoc
static napi_value BasonMergeN(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 1) NAPI_THROW(env, "mergeN: expected 1 argument");

    bool is_array;
    NAPI_OK(env, napi_is_array(env, argv[0], &is_array));
    if (!is_array) NAPI_THROW(env, "mergeN: expected array");

    uint32_t n;
    NAPI_OK(env, napi_get_array_length(env, argv[0], &n));
    if (n == 0) NAPI_THROW(env, "mergeN: empty array");
    if (n > 64) NAPI_THROW(env, "mergeN: too many inputs (max 64)");

    // Collect data slices
    u8cs inputs[64];
    BasonDoc *docs[64];
    size_t total = 0;
    for (uint32_t i = 0; i < n; i++) {
        napi_value elem;
        NAPI_OK(env, napi_get_element(env, argv[0], i, &elem));
        NAPI_OK(env, napi_unwrap(env, elem, (void **)&docs[i]));
        inputs[i][0] = (u8cp)docs[i]->data;
        inputs[i][1] = (u8cp)docs[i]->data + docs[i]->data_len;
        total += docs[i]->data_len;
    }

    size_t out_cap = total + 4096;
    u8 *obuf = (u8 *)malloc(out_cap);
    if (!obuf) NAPI_THROW(env, "out of memory");
    u8b out = {obuf, obuf, obuf, obuf + out_cap};
    u64 _idx[NAPI_IDX_LEN];
    u64b idx = {_idx, _idx, _idx, _idx + NAPI_IDX_LEN};
    u8css ins = {inputs, inputs + n};

    ok64 o = BASONMergeN(out, idx, ins);
    if (o != OK) { free(obuf); NAPI_THROW(env, "mergeN failed"); }

    size_t dlen = (size_t)(out[2] - out[1]);
    BasonDoc *doc = (BasonDoc *)malloc(sizeof(BasonDoc));
    if (!doc) { free(obuf); NAPI_THROW(env, "out of memory"); }
    doc->data = (u8 *)malloc(dlen);
    if (!doc->data) { free(doc); free(obuf); NAPI_THROW(env, "out of memory"); }
    memcpy(doc->data, out[1], dlen);
    doc->data_len = dlen;
    free(obuf);

    return WrapDoc(env, doc);
}

// bason.diff(oldDoc, newDoc) → BasonDoc (patch)
static napi_value BasonDiff(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < 2) NAPI_THROW(env, "diff: expected 2 arguments");

    BasonDoc *odoc, *ndoc;
    NAPI_OK(env, napi_unwrap(env, argv[0], (void **)&odoc));
    NAPI_OK(env, napi_unwrap(env, argv[1], (void **)&ndoc));

    size_t out_cap = odoc->data_len + ndoc->data_len + 4096;
    u8 *obuf = (u8 *)malloc(out_cap);
    if (!obuf) NAPI_THROW(env, "out of memory");
    u8b out = {obuf, obuf, obuf, obuf + out_cap};
    u64 _idx[NAPI_IDX_LEN];
    u64b idx = {_idx, _idx, _idx, _idx + NAPI_IDX_LEN};
    u64 _ostk[NAPI_STK_LEN], _nstk[NAPI_STK_LEN];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + NAPI_STK_LEN};
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + NAPI_STK_LEN};
    u8cs odata = {(u8cp)odoc->data, (u8cp)odoc->data + odoc->data_len};
    u8cs ndata = {(u8cp)ndoc->data, (u8cp)ndoc->data + ndoc->data_len};

    ok64 o = BASONDiff(out, idx, ostk, odata, nstk, ndata, NULL);
    if (o != OK) { free(obuf); NAPI_THROW(env, "diff failed"); }

    size_t dlen = (size_t)(out[2] - out[1]);
    BasonDoc *doc = (BasonDoc *)malloc(sizeof(BasonDoc));
    if (!doc) { free(obuf); NAPI_THROW(env, "out of memory"); }
    doc->data = (u8 *)malloc(dlen > 0 ? dlen : 1);
    if (!doc->data) { free(doc); free(obuf); NAPI_THROW(env, "out of memory"); }
    if (dlen > 0) memcpy(doc->data, out[1], dlen);
    doc->data_len = dlen;
    free(obuf);

    return WrapDoc(env, doc);
}

// ---------- BasonDoc methods ----------

// doc.toJSON() → string
static napi_value BasonDocToJSON(napi_env env, napi_callback_info info) {
    napi_value this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL));

    BasonDoc *doc;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&doc));

    u8 *out = (u8 *)malloc(doc->data_len * 4 + 4096);
    if (!out) NAPI_THROW(env, "out of memory");
    u8s outslice = {out, out + doc->data_len * 4 + 4096};
    u64 _stk[NAPI_STK_LEN];
    u64b stk = {_stk, _stk, _stk, _stk + NAPI_STK_LEN};
    u8cs data = {(u8cp)doc->data, (u8cp)doc->data + doc->data_len};
    ok64 o = BASONExportJSON(outslice, stk, data);
    if (o != OK) { free(out); NAPI_THROW(env, "toJSON: export failed"); }

    size_t json_len = (size_t)(outslice[0] - out);
    napi_value result;
    NAPI_OK(env, napi_create_string_utf8(env, (const char *)out, json_len,
                                         &result));
    free(out);
    return result;
}

// doc.get(key) → value or undefined
static napi_value BasonDocGet(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1], this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, &argc, argv, &this_val, NULL));
    if (argc < 1) NAPI_THROW(env, "get: expected 1 argument");

    BasonDoc *doc;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&doc));

    size_t klen = 0;
    NAPI_OK(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &klen));
    char kbuf[256];
    if (klen >= sizeof(kbuf)) NAPI_THROW(env, "get: key too long");
    NAPI_OK(env, napi_get_value_string_utf8(env, argv[0], kbuf, klen + 1, &klen));

    u64 _stk[NAPI_STK_LEN];
    u64b stk = {_stk, _stk, _stk, _stk + NAPI_STK_LEN};
    u8cs data = {(u8cp)doc->data, (u8cp)doc->data + doc->data_len};
    ok64 o = BASONOpen(stk, data);
    if (o != OK) { napi_value undef; napi_get_undefined(env, &undef); return undef; }

    // Read top-level element
    u8 type; u8cs key, val;
    o = BASONDrain(stk, data, &type, key, val);
    if (o != OK || !BASONPlex(type)) {
        napi_value undef; napi_get_undefined(env, &undef); return undef;
    }

    o = BASONInto(stk, data, val);
    if (o != OK) { napi_value undef; napi_get_undefined(env, &undef); return undef; }

    u8cs target = {(u8cp)kbuf, (u8cp)kbuf + klen};
    BASONSeek(stk, data, target);

    // Scan for exact match
    while (BASONDrain(stk, data, &type, key, val) == OK) {
        if ((size_t)$len(key) == klen &&
            memcmp(key[0], kbuf, klen) == 0) {
            return ValueToJS(env, type, val, this_val);
        }
        // Past target key in sorted order
        if ($len(key) > 0 && memcmp(key[0], kbuf,
                klen < (size_t)$len(key) ? klen : (size_t)$len(key)) > 0) {
            break;
        }
    }

    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
}

// doc.buffer → Uint8Array (copy)
static napi_value BasonDocBuffer(napi_env env, napi_callback_info info) {
    napi_value this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL));

    BasonDoc *doc;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&doc));

    void *buf_data;
    napi_value arraybuf, result;
    NAPI_OK(env, napi_create_arraybuffer(env, doc->data_len, &buf_data,
                                         &arraybuf));
    memcpy(buf_data, doc->data, doc->data_len);
    NAPI_OK(env, napi_create_typedarray(env, napi_uint8_array, doc->data_len,
                                        arraybuf, 0, &result));
    return result;
}

// doc[Symbol.iterator]() → BasonIter
static napi_value BasonDocIterator(napi_env env, napi_callback_info info) {
    napi_value this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL));

    BasonDoc *doc;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&doc));

    // Read top-level element to get children range
    u64 _stk[NAPI_STK_LEN];
    u64b stk = {_stk, _stk, _stk, _stk + NAPI_STK_LEN};
    u8cs data = {(u8cp)doc->data, (u8cp)doc->data + doc->data_len};
    ok64 o = BASONOpen(stk, data);
    if (o != OK) NAPI_THROW(env, "iterator: open failed");

    u8 type; u8cs key, val;
    o = BASONDrain(stk, data, &type, key, val);
    if (o != OK) NAPI_THROW(env, "iterator: drain failed");

    BasonIter *it = (BasonIter *)malloc(sizeof(BasonIter));
    if (!it) NAPI_THROW(env, "out of memory");

    if (BASONPlex(type)) {
        it->cursor[0] = val[0];
        it->cursor[1] = val[1];
    } else {
        // Scalar: iterate the top-level stream
        it->cursor[0] = data[0];
        it->cursor[1] = data[1];
    }
    NAPI_OK(env, napi_create_reference(env, this_val, 1, &it->doc_ref));

    return WrapIter(env, it);
}

// ---------- BasonIter methods ----------

// iter.next() → {value: {key, type, value}, done: false} | {done: true}
static napi_value BasonIterNext(napi_env env, napi_callback_info info) {
    napi_value this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL));

    BasonIter *it;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&it));

    napi_value result;
    NAPI_OK(env, napi_create_object(env, &result));

    if (it->cursor[0] >= it->cursor[1]) {
        napi_value is_done;
        NAPI_OK(env, napi_get_boolean(env, true, &is_done));
        NAPI_OK(env, napi_set_named_property(env, result, "done", is_done));
        return result;
    }

    // Skip index records
    u8 type;
    u8cs key, val;
    for (;;) {
        if (it->cursor[0] >= it->cursor[1]) {
            napi_value done;
            NAPI_OK(env, napi_get_boolean(env, true, &done));
            NAPI_OK(env, napi_set_named_property(env, result, "done", done));
            return result;
        }
        u8 tag = *it->cursor[0];
        u8 raw = (tag & 0x20) ? (tag & ~0x20) : tag;
        if (raw == 'X' || raw == 'K' || raw == 'C') {
            // Skip metadata record
            u8cs from = {it->cursor[0], it->cursor[1]};
            u8 lit;
            u8cs body;
            if (TLVu8sDrain(from, &lit, body) != OK) {
                napi_value done;
                NAPI_OK(env, napi_get_boolean(env, true, &done));
                NAPI_OK(env, napi_set_named_property(env, result, "done", done));
                return result;
            }
            it->cursor[0] = from[0];
            continue;
        }
        break;
    }

    u8cs from = {it->cursor[0], it->cursor[1]};
    ok64 o = TLKVDrain(from, &type, key, val);
    if (o != OK) {
        napi_value is_done;
        NAPI_OK(env, napi_get_boolean(env, true, &is_done));
        NAPI_OK(env, napi_set_named_property(env, result, "done", is_done));
        return result;
    }
    it->cursor[0] = from[0];

    // Get the doc JS object from ref
    napi_value doc_js;
    NAPI_OK(env, napi_get_reference_value(env, it->doc_ref, &doc_js));

    // Build {key, type, value}
    napi_value entry;
    NAPI_OK(env, napi_create_object(env, &entry));

    // key
    napi_value js_key;
    if ($len(key) > 0) {
        NAPI_OK(env, napi_create_string_utf8(
            env, (const char *)key[0], (size_t)(key[1] - key[0]), &js_key));
    } else {
        NAPI_OK(env, napi_get_null(env, &js_key));
    }
    NAPI_OK(env, napi_set_named_property(env, entry, "key", js_key));

    // type
    NAPI_OK(env, napi_set_named_property(env, entry, "type",
                                          TypeChar(env, type)));

    // value
    napi_value js_val = ValueToJS(env, type, val, doc_js);
    NAPI_OK(env, napi_set_named_property(env, entry, "value", js_val));

    NAPI_OK(env, napi_set_named_property(env, result, "value", entry));
    napi_value is_done;
    NAPI_OK(env, napi_get_boolean(env, false, &is_done));
    NAPI_OK(env, napi_set_named_property(env, result, "done", is_done));
    return result;
}

// ---------- BasonView methods ----------

// view[Symbol.iterator]() → BasonIter
static napi_value BasonViewIterator(napi_env env, napi_callback_info info) {
    napi_value this_val;
    NAPI_OK(env, napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL));

    BasonView *vw;
    NAPI_OK(env, napi_unwrap(env, this_val, (void **)&vw));

    BasonIter *it = (BasonIter *)malloc(sizeof(BasonIter));
    if (!it) NAPI_THROW(env, "out of memory");
    it->cursor[0] = vw->children[0];
    it->cursor[1] = vw->children[1];

    // Share the doc ref
    napi_value doc_js;
    NAPI_OK(env, napi_get_reference_value(env, vw->doc_ref, &doc_js));
    NAPI_OK(env, napi_create_reference(env, doc_js, 1, &it->doc_ref));

    return WrapIter(env, it);
}

// No-op constructor for napi_define_class
static napi_value NapiNoopCtor(napi_env env, napi_callback_info info) {
    napi_value this_val;
    napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL);
    return this_val;
}

// ---------- Module init ----------

static napi_value Init(napi_env env, napi_value exports) {
    // --- BasonDoc class ---
    napi_property_descriptor doc_props[] = {
        {"toJSON", NULL, BasonDocToJSON, NULL, NULL, NULL, napi_enumerable, NULL},
        {"get", NULL, BasonDocGet, NULL, NULL, NULL, napi_enumerable, NULL},
        {"buffer", NULL, NULL, BasonDocBuffer, NULL, NULL, napi_enumerable, NULL},
    };
    napi_value doc_ctor;
    napi_define_class(env, "BasonDoc", NAPI_AUTO_LENGTH, NapiNoopCtor,
                      NULL, 3, doc_props, &doc_ctor);

    // Add Symbol.iterator to BasonDoc prototype
    napi_value sym_iter;
    napi_get_global(env, &sym_iter);
    napi_value symbol_val;
    napi_get_named_property(env, sym_iter, "Symbol", &symbol_val);
    napi_value iter_sym;
    napi_get_named_property(env, symbol_val, "iterator", &iter_sym);

    napi_value iter_fn;
    napi_create_function(env, "[Symbol.iterator]", NAPI_AUTO_LENGTH,
                         BasonDocIterator, NULL, &iter_fn);
    napi_set_property(env, doc_ctor, iter_sym, iter_fn);

    // We need to set it on the prototype, not the constructor
    napi_value doc_proto;
    napi_get_named_property(env, doc_ctor, "prototype", &doc_proto);
    napi_set_property(env, doc_proto, iter_sym, iter_fn);

    napi_create_reference(env, doc_ctor, 1, &g_doc_ctor);

    // --- BasonIter class ---
    napi_property_descriptor iter_props[] = {
        {"next", NULL, BasonIterNext, NULL, NULL, NULL, napi_enumerable, NULL},
    };
    napi_value it_ctor;
    napi_define_class(env, "BasonIter", NAPI_AUTO_LENGTH, NapiNoopCtor,
                      NULL, 1, iter_props, &it_ctor);
    napi_create_reference(env, it_ctor, 1, &g_iter_ctor);

    // --- BasonView class ---
    napi_value view_ctor;
    napi_define_class(env, "BasonView", NAPI_AUTO_LENGTH, NapiNoopCtor,
                      NULL, 0, NULL, &view_ctor);

    // Add Symbol.iterator to BasonView prototype
    napi_value view_proto;
    napi_get_named_property(env, view_ctor, "prototype", &view_proto);
    napi_value view_iter_fn;
    napi_create_function(env, "[Symbol.iterator]", NAPI_AUTO_LENGTH,
                         BasonViewIterator, NULL, &view_iter_fn);
    napi_set_property(env, view_proto, iter_sym, view_iter_fn);

    napi_create_reference(env, view_ctor, 1, &g_view_ctor);

    // --- Module exports ---
    napi_property_descriptor mod_props[] = {
        {"parse", NULL, BasonParse, NULL, NULL, NULL, napi_enumerable, NULL},
        {"from", NULL, BasonFrom, NULL, NULL, NULL, napi_enumerable, NULL},
        {"stringify", NULL, BasonStringify, NULL, NULL, NULL, napi_enumerable, NULL},
        {"merge", NULL, BasonMerge, NULL, NULL, NULL, napi_enumerable, NULL},
        {"mergeN", NULL, BasonMergeN, NULL, NULL, NULL, napi_enumerable, NULL},
        {"diff", NULL, BasonDiff, NULL, NULL, NULL, napi_enumerable, NULL},
    };
    napi_define_properties(env, exports, 6, mod_props);

    return exports;
}

NAPI_MODULE(bason, Init)
