//
// Created by gritzko on 10/27/25.
//
#include "JABC.hpp"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "rdx/RDX.hpp"

JSClassRef JABCBrixClassStore = NULL;
JSClassRef JABCBrixClassEuler = NULL;
JSClassRef JABCBrixClassTuple = NULL;

JSStringRef JABC_PROP_PRNT = NULL;
const unsigned JABC_RO_PROP_ATTS = kJSPropertyAttributeDontDelete |
                                   kJSPropertyAttributeDontEnum |
                                   kJSPropertyAttributeReadOnly;

int JABC_BRIX_HOME = FILE_CLOSED;

static u32 JABC_BRIX_GEN = 0;

/*
 *   brix.Open("hello") // ?
 *   var obj1 = brix.Get("2s-18b") // ? replica-1, 0-1 -> {"title":id}
 *   var obj = brix.GetByTag("servers")
 *   io.log(obj.message)
 *   obj.time = io.now()
 *   obj.addr = {host:"google.com",port:443}
 *   io.log(""+obj)
 *   var hash = brix.Seal()
 *
 *   brix.Join("bonjour")
 *
 *   brix.Merge()
 *
 *   brix.TagPrivate()
 *   brix.TagPublic()
 */

typedef struct {
    u8bb brix;
    u32b refs;
    u16b prevs;
    int home;
} JABCBrix;

void JABCBrixInit(JABCBrix* brix) {
    // ptrs;
    u8b** b = (u8b**)brix->brix;
    b[0] = b[1] = b[2] = brix->_brix;
    b[3] = (u8b*)brix->_brix + BRIX_MAX_STACK;
    // home
}

ok64 JABCBrixOpen(JABCBrix* store, u8csc path, u8csc tip) { return notimplyet; }

ok64 JABCBrixRecords(JABCBrix* store, u8cssp recs, void* desc) {
    return notimplyet;
}

ok64 JABCBrixDesc(JABCBrix* store, void** desc, u8cscsc recs) { return 0; }

JABCBrix* JABCBrixStoreOf(JSContextRef ctx, JSObjectRef object,
                          JSValueRef* exception) {
    JSObjectRef i = object;
    while (JSObjectHasProperty(ctx, i, JABC_PROP_PRNT)) {
        JSValueRef v = JSObjectGetProperty(ctx, i, JABC_PROP_PRNT, exception);
        if (!JSValueIsObject(ctx, v)) break;
        i = (JSObjectRef)v;
    }
    if (!JSValueIsObjectOfClass(ctx, i, JABCBrixClassStore)) return nullptr;
    JABCBrix* brix = (JABCBrix*)JSObjectGetPrivate(i);
    return brix;
}

ok64 JABCBrixWrap(JSContextRef ctx, JSObjectRef jso, u8b rec) {
    return notimplyet;
}

JSObjectRef JABCBrixStoreConstructor(JSContextRef ctx, JSObjectRef constructor,
                                     size_t argumentCount,
                                     const JSValueRef arguments[],
                                     JSValueRef* exception) {
    u64 handl = 0;
    JSObjectRef obj = JSObjectMake(ctx, JABCBrixClassStore, (void*)handl);
    return obj;
}

bool JABCBrixStoreHas(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName) {
    return false;
}

JSValueRef JABCBrixStoreP(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef thisObject, size_t argumentCount,
                          const JSValueRef arguments[], JSValueRef* exception) {
    JABCBrix* store = (JABCBrix*)JSObjectGetPrivate(thisObject);
    a_pad(u8, rec, PAGESIZE);  // ?
    // see args
    ok64 o = BRIXu8bbAdd(store->brix, rec);
    void* desc;
    JSClassRef lass = JABCBrixClassEuler;
    u8ss inputs_datac;  // fixme
    o = JABCBrixDesc(store, &desc, inputs_datac);
    JSObjectRef ret = JSObjectMake(ctx, lass, desc);
    JSObjectSetProperty(ctx, ret, JABC_PROP_PRNT, thisObject, JABC_RO_PROP_ATTS,
                        exception);
    return ret;
}

JSValueRef JABCBrixStoreE(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef thisObject, size_t argumentCount,
                          const JSValueRef arguments[], JSValueRef* exception) {
    // see args
    return nullptr;
}

JSValueRef JABCBrixStoreGet(JSContextRef ctx, JSObjectRef object,
                            JSStringRef propertyName, JSValueRef* exception) {
    // pick from every
    // special case: E P2
    JABCBrix* store = (JABCBrix*)JSObjectGetPrivate(object);
    a_pad(u8cs, inputs, BRIX_MAX_STACK);
    // todo parse
    ref128 ref;
    ok64 o = BRIXu8bbGets(store->brix, ref, inputs_idle);
    u8 type;
    void* desc;
    JSClassRef lass = JABCBrixClassEuler;
    o = JABCBrixDesc(store, &desc, inputs_datac);
    JSObjectRef ret = JSObjectMake(ctx, lass, desc);
    JSObjectSetProperty(ctx, ret, JABC_PROP_PRNT, object, JABC_RO_PROP_ATTS,
                        exception);
    return ret;
}

bool JABCBrixStoreSet(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef value,
                      JSValueRef* exception) {
    return false;
}

bool JABCBrixStoreDel(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef* exception) {
    return false;
}

JSValueRef JABCBrixStoreConvert(JSContextRef ctx, JSObjectRef object,
                                JSType type, JSValueRef* exception) {
    return nullptr;
}

JSObjectRef JABCBrixTupleConstructor(JSContextRef ctx, JSObjectRef constructor,
                                     size_t argumentCount,
                                     const JSValueRef arguments[],
                                     JSValueRef* exception) {
    return nullptr;
}

bool JABCBrixTupleHas(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName) {
    return false;
}

JSValueRef JABCBrixTupleGet(JSContextRef ctx, JSObjectRef object,
                            JSStringRef propertyName, JSValueRef* exception) {
    // pick from every
    // special case: E P2
    return nullptr;
}

bool JABCBrixTupleSet(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef value,
                      JSValueRef* exception) {
    return false;
}

bool JABCBrixTupleDel(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef* exception) {
    return false;
}

JSValueRef JABCBrixTupleConvert(JSContextRef ctx, JSObjectRef object,
                                JSType type, JSValueRef* exception) {
    return nullptr;
}

JSObjectRef JABCBrixEulerConstructor(JSContextRef ctx, JSObjectRef constructor,
                                     size_t argumentCount,
                                     const JSValueRef arguments[],
                                     JSValueRef* exception) {
    JABCBrix* store;
    JSObjectRef storeRef;
    void* desc = nullptr;
    a_pad(u8cs, recs, BRIX_MAX_STACK);
    ok64 o = JABCBrixDesc(store, &desc, u8csbData(recs));
    if (o != OK) {
        *exception = JSOfCString("io.connect('tcp://google.com:80', function)");
    }
    JSObjectRef euler = JSObjectMake(ctx, JABCBrixClassEuler, desc);
    JSObjectSetProperty(ctx, euler, JABC_PROP_PRNT, storeRef, JABC_RO_PROP_ATTS,
                        exception);
    return euler;
}

bool JABCBrixEulerHas(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName) {
    return false;
}

JSValueRef JABCBrixEulerGet(JSContextRef ctx, JSObjectRef object,
                            JSStringRef propertyName, JSValueRef* exception) {
    // pick from every
    // special case: E P2
    return nullptr;
}

bool JABCBrixEulerSet(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef value,
                      JSValueRef* exception) {
    JABCBrix* brix;
    u8bp top = {};
    // prnt ->
    a_pad(u8cs, inputs, RDX_MAX_INPUTS);  //!!!
    u8b rec = {};
    JSObjectRef prnt;
    ok64 o = JABCBrixWrap(ctx, prnt, rec);
    // top ->
    // ? o = BRIXu8bAppend(top, inputs);  // 1 idle!
    // gc press! realloc x2
    // refit
    return false;
}

bool JABCBrixEulerDel(JSContextRef ctx, JSObjectRef object,
                      JSStringRef propertyName, JSValueRef* exception) {
    return false;
}

JSValueRef JABCBrixEulerConvert(JSContextRef ctx, JSObjectRef object,
                                JSType type, JSValueRef* exception) {
    return nullptr;
}

JABC_FN_DEFINE(JABCBrixHome) {
    JABC_FN_ARG_STRING(0, homepath, 1024, "no tip tag specified");
    a_path(defhome, ".rdx/");
    //if ($len(homepath) == 0) u8csDup(homepath, defhome);
    if (JABC_BRIX_HOME != FILE_CLOSED) FILEClose(&JABC_BRIX_HOME);
    JABC_FN_CALL(BRIXOpenHome, &JABC_BRIX_HOME, defhome);
    JABC_FN_RETURN_UNDEFINED;
}

JABC_FN_DEFINE(JABCBrixOpen) {
    JABC_FN_ARG_STRING(0, tiptag, 128, "no tip tag specified");
    if (JABC_BRIX_HOME == FILE_CLOSED) {
        a_path(defhome, ".rdx/");
        JABC_FN_CALL(BRIXOpenHome, &JABC_BRIX_HOME, defhome);
    }
    JABCBrix brix{};
    JABCBrixInit(&brix);
    JABC_FN_CALL(BRIXu8bbOpenTip, brix.brix, &brix.fd, JABC_BRIX_HOME, tiptag);
    JABCBrix* allocd = (JABCBrix*)malloc(sizeof(JABCBrix));
    memcpy(allocd, &brix, sizeof(JABCBrix));
    return JSObjectMake(ctx, JABCBrixClassStore, allocd);
}

JABC_FN_DEFINE(JABCBrixCreate) {
    JABC_FN_ARG_STRING(0, tiptag, 128, "no tip tag specified");
    if (JABC_BRIX_HOME == FILE_CLOSED) {
        a_path(defhome, ".rdx/");
        JABC_FN_CALL(BRIXOpenHome, &JABC_BRIX_HOME, defhome);
    }
    JABCBrix brix{};
    JABCBrixInit(&brix);
    sha256 nobase{};
    JABC_FN_CALL(BRIXu8bbCreateTip, brix.brix, &brix.fd, brix.home, &nobase,
                 tiptag);
    JABCBrix* allocd = (JABCBrix*)malloc(sizeof(JABCBrix));
    memcpy(allocd, &brix, sizeof(JABCBrix));
    return JSObjectMake(ctx, JABCBrixClassStore, allocd);
}

JSValueRef JABCBrixModeRW, JABCBrixModeRO;

JABC_FN_DEFINE(JABCBrixStoreMode) {
    JABCBrix* brixp = (JABCBrix*)JSObjectGetPrivate(self);
    if (!JSValueIsObjectOfClass(ctx, self, JABCBrixClassStore) || !brixp)
        JABC_FN_THROW("not a store");
    u8bp tip = Blast(brixp->brix);
    return BRIXu8bIndexType(tip) == BRIX_INDEX_LSMHASH ? JABCBrixModeRW
                                                       : JABCBrixModeRO;
}

JABC_FN_DEFINE(JABCBrixStoreLength) {
    JABCBrix* brixp = (JABCBrix*)JSObjectGetPrivate(self);
    if (!JSValueIsObjectOfClass(ctx, self, JABCBrixClassStore) || !brixp)
        JABC_FN_THROW("not a store");
    return JSValueMakeNumber(ctx, (double)u8bbDataLen(brixp->brix));
}

void JABCBrixStoreFin(JSObjectRef self) {
    JABCBrix* brixp = (JABCBrix*)JSObjectGetPrivate(self);
    BRIXu8bbClose(brixp->brix);
}

ok64 JABCbrixInstall() {
    JABC_API_OBJECT(brix);
    JABC_API_FN(brix, "home", JABCBrixHome);
    JABC_API_FN(brix, "open", JABCBrixOpen);
    JABC_API_FN(brix, "create", JABCBrixCreate);

    JSStaticFunction brix_store_fns[] = {{.name = "length",
                                          .callAsFunction = JABCBrixStoreLength,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = "mode",
                                          .callAsFunction = JABCBrixStoreMode,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = "hash",
                                          .callAsFunction = nullptr,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = "P",
                                          .callAsFunction = JABCBrixStoreP,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = "E",
                                          .callAsFunction = JABCBrixStoreE,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = "toString",
                                          .callAsFunction = nullptr,
                                          .attributes = JABC_RO_PROP_ATTS},
                                         {.name = nullptr}};
    JSClassDefinition def_store = {
        .className = "Store",
        .staticFunctions = brix_store_fns,
        .finalize = JABCBrixStoreFin,
        .hasProperty = JABCBrixStoreHas,
        .getProperty = JABCBrixStoreGet,
        .setProperty = JABCBrixStoreSet,
        .deleteProperty = JABCBrixStoreDel,
        .callAsConstructor = JABCBrixStoreConstructor,
        .convertToType = JABCBrixStoreConvert,
    };
    JABCBrixClassStore = JSClassCreate(&def_store);
    JSClassRetain(JABCBrixClassStore);
    JSObjectRef store_constr = JSObjectMakeConstructor(
        JABC_CONTEXT, JABCBrixClassStore, JABCBrixStoreConstructor);

    JSStaticFunction brix_elem_fns[] = {{.name = "Type",
                                         .callAsFunction = nullptr,
                                         .attributes = JABC_RO_PROP_ATTS},
                                        {.name = "Id",
                                         .callAsFunction = nullptr,
                                         .attributes = JABC_RO_PROP_ATTS},
                                        {.name = "String",
                                         .callAsFunction = nullptr,
                                         .attributes = JABC_RO_PROP_ATTS},
                                        {.name = nullptr}};
    JSClassDefinition def = {
        .className = "Euler",
        .staticFunctions = brix_elem_fns,
        .hasProperty = JABCBrixEulerHas,
        .getProperty = JABCBrixEulerGet,
        .setProperty = JABCBrixEulerSet,
        .deleteProperty = JABCBrixEulerDel,
        .callAsConstructor = JABCBrixEulerConstructor,
        .convertToType = JABCBrixEulerConvert,
    };
    JABCBrixClassEuler = JSClassCreate(&def);
    JSClassRetain(JABCBrixClassEuler);

    JSClassDefinition tuple_def = {
        .className = "Tuple",
        .staticFunctions = brix_elem_fns,
        .hasProperty = JABCBrixTupleHas,
        .getProperty = JABCBrixTupleGet,
        .setProperty = JABCBrixTupleSet,
        .deleteProperty = JABCBrixTupleDel,
        .callAsConstructor = JABCBrixTupleConstructor,
        .convertToType = JABCBrixTupleConvert,
    };
    JABCBrixClassTuple = JSClassCreate(&tuple_def);
    JSClassRetain(JABCBrixClassTuple);

    JABC_PROP_PRNT = JSStringCreateWithUTF8CString("_prnt");

    JSStringRef ro = JSStringCreateWithUTF8CString("ro");
    JSStringRef rw = JSStringCreateWithUTF8CString("rw");
    JABCBrixModeRW = JSValueMakeString(JABC_CONTEXT, rw);
    JABCBrixModeRO = JSValueMakeString(JABC_CONTEXT, ro);
    JSValueProtect(JABC_CONTEXT, JABCBrixModeRO);
    JSValueProtect(JABC_CONTEXT, JABCBrixModeRW);
    JSStringRelease(ro);
    JSStringRelease(rw);

    return OK;
}

ok64 JABCbrixUninstall() {
    JSClassRelease(JABCBrixClassStore);
    JABCBrixClassStore = nullptr;
    JSClassRelease(JABCBrixClassEuler);
    JABCBrixClassEuler = nullptr;
    JSClassRelease(JABCBrixClassTuple);
    JABCBrixClassTuple = nullptr;
    JSValueUnprotect(JABC_CONTEXT, JABCBrixModeRO);
    JSValueUnprotect(JABC_CONTEXT, JABCBrixModeRW);
    JABCBrixModeRO = nullptr;
    JABCBrixModeRW = nullptr;
    return OK;
}
