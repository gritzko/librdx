//
// Created by gritzko on 11/12/25.
//
#include <cmath>

#include "JABC.hpp"
#include "abc/ABC.hpp"
#include "abc/PRO.h"
#include "rdx/RDX.hpp"

a_cstr(RDX_EMPTY_TUPLE, "p\01\00");

ok64 JABCu8bImport(u8bp builder, JSContextRef ctx, JSValueRef val,
                   JSValueRef* exception) {
    //u8bAllocate();
    // u8bReserve()
    sane(u8bOK(builder) && ctx != nullptr && val != nullptr);
    JSType t = JSValueGetType(ctx, val);
    switch (t) {
        case kJSTypeUndefined:
            call(u8bFeed, builder, RDX_EMPTY_TUPLE);
            done;
        case kJSTypeNull:
            break;
        case kJSTypeBoolean:
            break;
        case kJSTypeNumber: {
            double d = JSValueToNumber(ctx, val, exception);
            rdx r = {};
            if (floor(d) != d) {
                r.type = RDX_FLOAT;
                r.f = d;
                call(RDXu8sFeedF, u8bIdle(builder), &r);
            } else {
                r.type = RDX_INT;
                r.i = (i64)d;
                call(RDXu8sFeedI, u8bIdle(builder), &r);
            }
            done;
        }
        case kJSTypeString:
            break;
        case kJSTypeObject:
            break;
    }
    done;
}

ok64 JABCrdxExport(rdxb reader, JSContextRef ctx, JSValueRef* val,
                   JSValueRef* exception) {
    sane(reader != nullptr && rdxbDataLen(reader) > 0);
    rdxp top = rdxbLast(reader);
    switch (top->type) {
        case RDX_FLOAT:
            *val = JSValueMakeNumber(ctx, top->f);
            done;
        case RDX_INT:
            *val = JSValueMakeNumber(ctx, top->i);
            done;
        default:
            fail(notimplyet);
    }
    done;
}