#include "JABC.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"

JSObjectRef UTF8Object = NULL;

void FreeDeallocator(void* bytes, void* deallocatorContext) { free(bytes); }

JSValueRef JABCutf8Encode(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef self, size_t argc,
                          const JSValueRef args[], JSValueRef* exception) {
    if (argc != 1 || !JS_ARG_IS_STRING(0)) {
        *exception = JSOfCString("utf8.Encode(string)->Uint8Array");
        return JSValueMakeUndefined(ctx);
    }
    JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* bytes = (char*)malloc(maxSize);
    size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
    JSObjectRef ta2 = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, bytes, factlen, FreeDeallocator,
        bytes, exception);
    return ta2;
}

JSValueRef JABCutf8Decode(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef self, size_t argc,
                          const JSValueRef args[], JSValueRef* exception) {
    JSValueRef n = JSValueMakeUndefined(ctx);
    return n;
}

ok64 JSu8bString(JSStringRef str, u8bp into) {
    u8sp idle = u8bIdle(into);
    size_t fact = JSStringGetUTF8CString(str, (char*)*idle, $len(idle));
    if (fact < 1) return badarg;
    idle[0] += fact - 1;
    return OK;
}

ok64 JABCutf8bFeedValueRef(u8bp into, JSContextRef ctx, JSValueRef val) {
    JSStringRef str = JSValueToStringCopy(ctx, val, NULL);
    ok64 o = JSu8bString(str, into);
    JSStringRelease(str);
    return o;
}

#define JABproc(n, params...) \
    JSValueRef n(JSContextRef ctx, params, JSValueRef* exception)
#define JABsane(cond)  \
    {                  \
        if (!(cond)) { \
        }              \
    }
#define JABcall(fn, params...)            \
    fn(ctx, params, exception);           \
    if (exception != NULL) {              \
        return JSValueMakeUndefined(ctx); \
    }

JABproc(JABCutf8CopyStringValue, u8sp into, JSValueRef val) {
    JABsane(into != NULL && val != NULL);
    JSStringRef str = JSValueToStringCopy(ctx, val, exception);
    $u8alloc(into, JSStringGetMaximumUTF8CStringSize(str));
    size_t fact = JSStringGetUTF8CString(str, (char*)*into, $len(into));
    into[0] += fact;
    JSStringRelease(str);
    return JSValueMakeUndefined(ctx);
}

JABproc(JAButf8CreateValue, utf8cp str) {
    JABsane(str != NULL);
    JSStringRef tmp = JSStringCreateWithUTF8CString((const char*)str);
    JSValueRef n = JSValueMakeString(ctx, tmp);
    JSStringRelease(tmp);
    return n;
}

JSValueRef JSOfCString(const char* str) {
    JSStringRef tmp = JSStringCreateWithUTF8CString(str);
    JSValueRef n = JSValueMakeString(JABC_CONTEXT, tmp);
    JSStringRelease(tmp);
    return n;
}

// Utility: Convert JSStringRef to null-terminated C string
char* JSStringRefToCString(JSStringRef str) {
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* buffer = (char*)malloc(maxSize);
    if (!buffer) return NULL;
    JSStringGetUTF8CString(str, buffer, maxSize);
    return buffer;
}

ok64 JABCutf8Install() {
    JS_API_OBJECT(utf8, "utf8");
    JS_SET_PROPERTY_FN(utf8, "en", JABCutf8Encode);
    JS_SET_PROPERTY_FN(utf8, "Encode", JABCutf8Encode);
    JS_SET_PROPERTY_FN(utf8, "de", JABCutf8Decode);
    JS_SET_PROPERTY_FN(utf8, "Decode", JABCutf8Decode);
    UTF8Object = utf8;
    return OK;
}

ok64 JABCutf8Uninstall() { return OK; }
