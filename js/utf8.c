#include "abc/UTF8.h"

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/OK.h"

JSObjectRef UTF8Object = NULL;

void FreeDeallocator(void* bytes, void* deallocatorContext) { free(bytes); }

JSValueRef JABCutf8Encode(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    if (argc != 1 || !JS_ARG_IS_STRING(0)) {
        *exception = JSOfCString("utf8.Encode(string)->Uint8Array");
        return JSValueMakeUndefined(ctx);
    }
    JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* bytes = malloc(maxSize);
    size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
    JSObjectRef ta2 = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, bytes, factlen, FreeDeallocator,
        bytes, exception);
    return ta2;
}

JSValueRef JABCutf8Decode(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    JSValueRef n = JSValueMakeUndefined(ctx);
    return n;
}

ok64 JSu8BString(JSStringRef str, u8B into) {
    u8sp idle = Bidle(into);
    size_t fact = JSStringGetUTF8CString(str, (char*)*idle, $len(idle));
    if (fact < 1) return badarg;
    idle[0] += fact - 1;
    return OK;
}

ok64 JABCutf8BFeedValueRef(u8B into, JSContextRef ctx, JSValueRef val) {
    JSStringRef str = JSValueToStringCopy(ctx, val, NULL);
    ok64 o = JSu8BString(str, into);
    JSStringRelease(str);
    return o;
}

JSValueRef JSOfCString(utf8cp str) {
    JSStringRef tmp = JSStringCreateWithUTF8CString(str);
    JSValueRef n = JSValueMakeString(JS_CONTEXT, tmp);
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
