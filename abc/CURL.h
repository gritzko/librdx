#ifndef ABC_CURL_H
#define ABC_CURL_H

#include <curl/curl.h>

#include "OK.h"
#include "BUF.h"

con ok64 CURLBAD = 0xc79b54b28d;
con ok64 CURLFAIL = 0x31e6d53ca495;

// Callback when request completes: (request, http_status, response_body)
typedef struct CURLreq CURLreq;
typedef void (*CURLcb)(CURLreq *req, long status, u8cs body);

typedef struct CURLreq {
    CURL *easy;
    char *url;
    u8b headers;
    u8b response;
    CURLcb callback;
    void *userdata;
} CURLreq;

// Initialize curl subsystem, integrate with POL
ok64 CURLInit();

// Cleanup curl subsystem
ok64 CURLFree();

// Start a GET request (async, calls callback when complete)
ok64 CURLGet(const char *url, CURLcb cb, void *userdata);

// Start a POST request
ok64 CURLPost(const char *url, u8cs body, const char *content_type,
              CURLcb cb, void *userdata);

// Drive curl (called automatically by POL integration)
ok64 CURLTick();

#endif
