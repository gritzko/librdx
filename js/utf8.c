#include "abc/UTF8.h"

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/OK.h"

void FreeDeallocator(void* bytes, void* deallocatorContext) { free(bytes); }

JSValueRef UTF8En(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                  size_t argc, const JSValueRef args[], JSValueRef* exception) {
    if (argc != 1 || !JS_ARG_IS_STRING(0))
        JS_THROW("utf8.Encode(string)->Uint8Array");
    JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* bytes = malloc(maxSize);
    size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
    JSObjectRef ta2 = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, bytes, factlen, FreeDeallocator,
        bytes, exception);
    return ta2;
}

JSValueRef UTF8De(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                  size_t argc, const JSValueRef args[], JSValueRef* exception) {
    JSValueRef n = JSValueMakeUndefined(ctx);
    return n;
}

ok64 UTF8Install() {
    JS_API_OBJECT(utf8, "utf8");
    JS_SET_PROPERTY_FN(utf8, "en", UTF8En);
    JS_SET_PROPERTY_FN(utf8, "Encode", UTF8En);
    JS_SET_PROPERTY_FN(utf8, "de", UTF8De);
    JS_SET_PROPERTY_FN(utf8, "Decode", UTF8De);
    return OK;
}
