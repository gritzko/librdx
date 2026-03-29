#ifndef ABC_MIME_H
#define ABC_MIME_H

#include "PATH.h"

//  MIME type lookup by file extension or path.
//  ~105 common types: web, media, documents, programming languages.
//  Binary search on a sorted table, case-insensitive.

#define MIMEdefault "application/octet-stream"

// clang-format off
con char *MIMEtab[][2] = {
    {"7z",     "application/x-7z-compressed"},
    {"aac",    "audio/aac"},
    {"apng",   "image/apng"},
    {"asm",    "text/x-asm"},
    {"avi",    "video/x-msvideo"},
    {"avif",   "image/avif"},
    {"bin",    "application/octet-stream"},
    {"bmp",    "image/bmp"},
    {"bz2",    "application/x-bzip2"},
    {"c",      "text/x-c"},
    {"cc",     "text/x-c"},
    {"cpp",    "text/x-c"},
    {"cs",     "text/x-csharp"},
    {"css",    "text/css"},
    {"csv",    "text/csv"},
    {"d",      "text/x-d"},
    {"dart",   "text/x-dart"},
    {"diff",   "text/x-diff"},
    {"doc",    "application/msword"},
    {"docx",   "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"eot",    "application/vnd.ms-fontobject"},
    {"epub",   "application/epub+zip"},
    {"erl",    "text/x-erlang"},
    {"ex",     "text/x-elixir"},
    {"flac",   "audio/flac"},
    {"gif",    "image/gif"},
    {"go",     "text/x-go"},
    {"gz",     "application/gzip"},
    {"h",      "text/x-c"},
    {"hpp",    "text/x-c"},
    {"hs",     "text/x-haskell"},
    {"htm",    "text/html"},
    {"html",   "text/html"},
    {"ico",    "image/vnd.microsoft.icon"},
    {"jar",    "application/java-archive"},
    {"java",   "text/x-java"},
    {"jl",     "text/x-julia"},
    {"jpeg",   "image/jpeg"},
    {"jpg",    "image/jpeg"},
    {"js",     "text/javascript"},
    {"json",   "application/json"},
    {"jsonld", "application/ld+json"},
    {"kt",     "text/x-kotlin"},
    {"less",   "text/x-less"},
    {"lisp",   "text/x-lisp"},
    {"lua",    "text/x-lua"},
    {"m4a",    "audio/mp4"},
    {"md",     "text/markdown"},
    {"mid",    "audio/midi"},
    {"midi",   "audio/midi"},
    {"mjs",    "text/javascript"},
    {"mkv",    "video/x-matroska"},
    {"ml",     "text/x-ocaml"},
    {"mov",    "video/quicktime"},
    {"mp3",    "audio/mpeg"},
    {"mp4",    "video/mp4"},
    {"mpeg",   "video/mpeg"},
    {"nim",    "text/x-nim"},
    {"ogg",    "audio/ogg"},
    {"ogv",    "video/ogg"},
    {"opus",   "audio/opus"},
    {"otf",    "font/otf"},
    {"pas",    "text/x-pascal"},
    {"pdf",    "application/pdf"},
    {"php",    "application/x-httpd-php"},
    {"pl",     "text/x-perl"},
    {"png",    "image/png"},
    {"ppt",    "application/vnd.ms-powerpoint"},
    {"pptx",   "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"py",     "text/x-python"},
    {"r",      "text/x-r"},
    {"rb",     "text/x-ruby"},
    {"rl",     "text/x-ragel"},
    {"rs",     "text/x-rust"},
    {"rtf",    "application/rtf"},
    {"sass",   "text/x-sass"},
    {"scala",  "text/x-scala"},
    {"scss",   "text/x-scss"},
    {"sh",     "application/x-sh"},
    {"sql",    "application/sql"},
    {"svg",    "image/svg+xml"},
    {"swift",  "text/x-swift"},
    {"tar",    "application/x-tar"},
    {"tex",    "text/x-tex"},
    {"tgz",    "application/gzip"},
    {"tif",    "image/tiff"},
    {"tiff",   "image/tiff"},
    {"toml",   "text/x-toml"},
    {"ts",     "text/typescript"},
    {"tsx",    "text/typescript"},
    {"ttf",    "font/ttf"},
    {"txt",    "text/plain"},
    {"wasm",   "application/wasm"},
    {"wav",    "audio/wav"},
    {"webm",   "video/webm"},
    {"webp",   "image/webp"},
    {"woff",   "font/woff"},
    {"woff2",  "font/woff2"},
    {"xhtml",  "application/xhtml+xml"},
    {"xls",    "application/vnd.ms-excel"},
    {"xlsx",   "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"xml",    "application/xml"},
    {"yaml",   "application/yaml"},
    {"yml",    "application/yaml"},
    {"zig",    "text/x-zig"},
    {"zip",    "application/zip"},
};
// clang-format on

#define MIMEtablen ((int)(sizeof(MIMEtab) / sizeof(MIMEtab[0])))

// Case-insensitive compare u8cs against a C string.
fun int MIMEcmpext(u8cs ext, const char *s) {
    size_t i = 0;
    size_t elen = $len(ext);
    while (i < elen && s[i] != 0) {
        u8 ca = $at(ext, i) | 0x20;
        u8 cs = (u8)s[i] | 0x20;
        if (ca != cs) return (int)ca - (int)cs;
        i++;
    }
    if (i < elen) return 1;
    if (s[i] != 0) return -1;
    return 0;
}

// Lookup MIME type by file extension (without leading dot).
// Case-insensitive.  Returns MIMEdefault for unknown extensions.
fun const char *MIMEByExt(u8cs ext) {
    if ($empty(ext)) return MIMEdefault;
    int lo = 0, hi = MIMEtablen - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = MIMEcmpext(ext, MIMEtab[mid][0]);
        if (cmp < 0)
            hi = mid - 1;
        else if (cmp > 0)
            lo = mid + 1;
        else
            return MIMEtab[mid][1];
    }
    return MIMEdefault;
}

// Lookup MIME type by file path (uses path8sExt).
fun const char *MIMEByPath(u8cs path) {
    if ($empty(path)) return MIMEdefault;
    u8cs ext = {};
    path8sExt(ext, path);
    return MIMEByExt(ext);
}

#endif
