//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef UNTITLED_OK_H
#define UNTITLED_OK_H

#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* . . . . . . . .  Integer types . . . . . . . .  */

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;
typedef uint64_t ok64;
typedef ok64 ok;
typedef u64 pos64;

#define as64(v) (*(u64*)&(v))

#define WORDS(k) union { \
    u64 _64[k];          \
    u32 _32[k*2];        \
    u16 _16[k*4];        \
    u8 _8[k*8];          \
}

typedef WORDS(2) u128;
typedef WORDS(4) u256;
typedef WORDS(8) u512;

#define LOG2PAGE 12
#define PAGE_SIZE (1<<LOG2PAGE)
#define PAGE_MASK (PAGE_SIZE-1)

typedef uint8_t b8;
typedef u128 u120;
con b8 FALSE = 0;
con b8 TRUE = 1;

#define max(a, b) ((a)>(b)?(a):(b))
#define min(a, b) ((a)<(b)?(a):(b))

#define OK 0UL

#define proc ok
#define fun static inline
#define funi static __always_inline
#define inl static inline
#define con con
#define record typedef struct

/* . . . . . . . .  Tracing . . . . . . . .  */

#ifndef NDEBUG
extern int traced;
#define trace traced = 1;
#define notrace traced = 0;
#define checked 1
#else
#define traced 0
#define trace
#define notrace
#define checked 0
#endif

extern int __NESTING__;

extern const char *__TRACE_INDENTS__[32];

#define __INDENT__ __TRACE_INDENTS__[__NESTING__&0x1f]

#define traces(msg) \
    if (traced) fprintf(stderr, "%s%s:%d %s", __INDENT__, __func__, __LINE__, msg);

#define tracef(fmt, ...) \
    if (traced) { fprintf(stderr, "%s%s:%d " fmt, __INDENT__, __func__, __LINE__, __VA_ARGS__); }

#define inf(fmt, ...) \
    if (traced) { fprintf(stderr, "%s>%s:%d " fmt, __INDENT__, __func__, __LINE__, __VA_ARGS__); }

#ifdef __GNUC__
#define unlikely(x) (__builtin_expect(x, 0))
#define likely(x) (__builtin_expect(!!(x), 1))
#else
#define unlikely(x) (x)
#define likely(x) (x)
#endif

/* . . . . . . . .  Control flow . . . . . . . .  */

#define done { \
    over:         \
    if (traced) fprintf(stderr, "%s<%s:%d %s\n", __INDENT__, __func__, __LINE__, ok64str(_));  \
    __NESTING__--;              \
    return _;  \
}

#define nedo(fixes) { \
    over:             \
    fixes;              \
    if (traced) fprintf(stderr, "%s<%s:%d %s\n", __INDENT__, __func__, __LINE__, ok64str(_));  \
    __NESTING__--;              \
    return _; \
}

#define fail(code) { \
    _ = code;        \
    goto over;                 \
}

#define skip  { _=OK; goto over; }

#define args(cond, fmt, ...) \
    __NESTING__++;              \
    ok _ = OK;               \
    if (traced) fprintf(stderr, "%s>%s:%d " fmt "\n", __INDENT__, __func__, __LINE__, __VA_ARGS__); \
    if (checked && !likely(cond)) { \
        if (traced) fprintf(stderr, "%s!%s:%d fails: %s\n", __INDENT__, __func__, __LINE__, #cond); \
        fail(BADARGUMNT);    \
    }                        \

con ok BADARG0000 = 201031024430284800UL;

#define sane(cond, ...)                                                              \
    __NESTING__++;                                                                   \
    ok _ = OK;                                                                       \
    if (traced) Pile(), Pstr(__INDENT__), Pu8('>'), Pstr(__func__), Pu8(':'),                 \
        Pint(__LINE__), Pu8('('), __VA_ARGS__, Pu8(')'), Pnl(), Perr();               \
    if (checked && !likely(cond)) {                                                   \
        if (traced) Pile(), Pstr(__INDENT__), Pstr("bad args: "), Pstr(#cond), Pnl(), Perr(); \
        fail(BADARG0000 + __LINE__);                                                 \
    }

#define quiet(cond)                                                              \
    __NESTING__++;                                                                   \
    ok _ = OK;                                                                       \
    if (checked && !likely(cond)) {                                                   \
        if (traced) Pstr(__INDENT__), Pstr("bad args: "), Pstr(#cond), Pnl(), Perr(); \
        fail(BADARG0000 + __LINE__);                                                 \
    }

// TODO nowarn
//#define quiet int traced = 0;

#define noargs \
    __NESTING__++;              \
    ok _ = OK; \
    if (traced) fprintf(stderr, "%s>%s:%d\n", __INDENT__, __func__, __LINE__);

#define try(x) { \
    _ = (x);         \
}

#define fix(x) if (_==(x) && !(_=OK))
#define fixany(x) if ((_&~63)==x && !(_=OK))
#define fixfail if (_ && !(_=OK))

#define on(x) if (_==x)
#define onany(x) if ((_&~63)==x)
#define onfail if (_)

#define call(x) {         \
    try(x)                \
    if (_!=OK) fail(_)  \
}

#define callcv(x, fail) {         \
    if (!(x)) { failcv(fail); } \
}

#define xcall(S, V, args) call(X(S,V)args)

#define test(cond, err) { \
    if (!likely(cond)) {        \
        fail(err)                  \
    }                      \
}

#define failv(code) \
        fprintf(stderr, "%s at %s:%d\n", ok64str(code), __FILE__, __LINE__); \
        fail((code) + errno)

#define testv(cond, err) { \
    if (!likely(cond)) {        \
        failv(err)                  \
    }                      \
}

#define failcv(code) { \
        fprintf(stderr, "%s at %s:%d\n", strerror(errno), __FILE__, __LINE__); \
        fail((code) + errno); \
}

#define testcv(cond, fail)              \
    if (!likely(cond)) {                \
        failcv(fail)                   \
    }


#define mute(cond, err) { \
    _ = (cond);        \
    if (_==err) _ = OK;   \
    if (_) fail(_)                      \
}

#define testeq(a, b) { \
    if (!likely((a)==(b))) {        \
        fail(FAILEQTEST)                  \
    }                      \
}

#define testeqv(a, b) { \
    if (!likely((a)==(b))) {        \
        failv(FAILEQTEST)                  \
    }                      \
}

#define $testeq(a, b) { \
    if (! likely( $size(a)==$size(b) && 0 == memcmp(*a, *b, $size(a)) ) ) {        \
        fail(FAILEQTEST)                  \
    }                      \
}

// sanity checks; crash on fail
#define must(cond)  if (!likely(cond)) { \
    tracef("condition %s failed\n", #cond);  \
    abort(); \
}

// hash number (like hashtable hash)
typedef uint64_t h64;

/* . . . . . . . .  Bitwise magic . . . . . . . .  */

#define MIN(x, y) ((x)<(y)?(x):(y))

#ifdef _MSC_VER
#include <intrin.h>
uint32_t __inline clz64(u64 value) {
    DWORD leading_zero = 0;
    _BitScanReverse64(&leading_zero, value);
    return 63 - leading_zero;
}
#elif __GNUC__
#define clz64(x) __builtin_clzll(x)
#define ctz64(x) __builtin_ctzll(x)
#define clz32(x) __builtin_clz(x)
#define ctz32(x) __builtin_ctz(x)
#define popc32(x) __builtin_popcount(x)
#define popc64(x) __builtin_popcount(x)
#endif

#if defined(__linux__) || defined(__CYGWIN__)

#include <endian.h>

#define flip64(x) __bswap_64(x)
#define flip32(x) __bswap_32(x)

#elif defined(__APPLE__)

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#define flip64(x) OSSwapInt64(x)

#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __PDP_ENDIAN PDP_ENDIAN

#elif defined(__OpenBSD__)

#include <sys/endian.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#include <sys/endian.h>

#define be16toh(x) betoh16(x)
#define le16toh(x) letoh16(x)

#define be32toh(x) betoh32(x)
#define le32toh(x) letoh32(x)

#define be64toh(x) betoh64(x)
#define le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#include <sys/param.h>
#include <winsock2.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#elif BYTE_ORDER == BIG_ENDIAN

/* that would be xbox 360 */
#define htobe16(x) (x)
#define htole16(x) __builtin_bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) __builtin_bswap16(x)

#define htobe32(x) (x)
#define htole32(x) __builtin_bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) __builtin_bswap32(x)

#define htobe64(x) (x)
#define htole64(x) __builtin_bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) __builtin_bswap64(x)

#else

#error byte order not supported

#endif

#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __PDP_ENDIAN PDP_ENDIAN

#else

#error platform not supported

#endif

fun u64 rotr64(u64 val, uint8_t len) {
    return (val >> len) | (val << (64U - len));
}

fun u64 rotl64(u64 val, uint8_t len) {
    return (val << len) | (val >> (64U - len));
}

fun u32 rotr32(u32 val, uint8_t len) {
    return (val >> len) | (val << (32U - len));
}

fun u32 rotl32(u32 val, uint8_t len) {
    return (val << len) | (val >> (32U - len));
}

fun int is_power_of_2(u64 w) { return 0 == ((w - 1U) & w); }

fun u64 round_power_of_2(u64 a) {
    if (is_power_of_2(a))
        return a;
    int p = clz64(a);
    return 1UL << (64 - p);
}

fun u8 upper_log_2(u64 val) {
    u8 pow = 64 - clz64(val);
    if (!is_power_of_2(val))
        ++pow;
    return pow;
}

fun u8 log_2(size_t x) {
    u8 z = clz64(x | 1);
    return 63 - z;
}

con u64 PRIME1 = 11400714785074694791ULL;
con u64 PRIME2 = 14029467366897019727ULL;
con u64 PRIME3 = 1609587929392839161ULL;
con u64 PRIME4 = 9650029242287828579ULL;
con u64 PRIME5 = 2870177450012600261ULL;

fun h64 mix32(u32 a) {
    return (a * PRIME1) >> 32;
}

fun h64 mix64(u64 a) {
    h64 mix1 = a * PRIME1;
    h64 mix2 = flip64(mix1) ^ a;
    return mix2 * PRIME2;
}

/* . . . . . .   RON Base64, hex and other codings . . . . . . */

con char *BASE16 = "0123456789abcdef";

con char *BASE64 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";

extern const u8 BASE64rev[256];

extern const u8 BASE16rev[256];

enum BASE64_DIGITS {
    BASE_0,
    BASE_1,
    BASE_2,
    BASE_3,
    BASE_4,
    BASE_5,
    BASE_6,
    BASE_7,
    BASE_8,
    BASE_9,
    BASE_A,
    BASE_B,
    BASE_C,
    BASE_D,
    BASE_E,
    BASE_F,
    BASE_G,
    BASE_H,
    BASE_I,
    BASE_J,
    BASE_K,
    BASE_L,
    BASE_M,
    BASE_N,
    BASE_O,
    BASE_P,
    BASE_Q,
    BASE_R,
    BASE_S,
    BASE_T,
    BASE_U,
    BASE_V,
    BASE_W,
    BASE_X,
    BASE_Y,
    BASE_Z,
    BASE_UNDER,
    BASE_a,
    BASE_b,
    BASE_c,
    BASE_d,
    BASE_e,
    BASE_f,
    BASE_g,
    BASE_h,
    BASE_i,
    BASE_j,
    BASE_k,
    BASE_l,
    BASE_m,
    BASE_n,
    BASE_o,
    BASE_p,
    BASE_q,
    BASE_r,
    BASE_s,
    BASE_t,
    BASE_u,
    BASE_v,
    BASE_w,
    BASE_x,
    BASE_y,
    BASE_z,
    BASE_TILDE
};

typedef void *$void[2];
typedef const void *$voidc[2];
typedef u8 *$u8[2];
typedef const u8 *$u8c[2];

typedef u64 u60;
#define MASK60 ((1UL<<60)-1)
#define MASK30 ((1UL<<30)-1)
#define MASK30h (((1UL<<30)-1)<<30)

fun void Pstr(const char *s);

fun void Pnl();

fun void Perr();

proc u60rondrain(u60 *word, $u8 base64);

proc u120rondrain(u128 *word, $u8 base64);

proc ok64parse(ok *word, $u8 base64);

proc u60ronfeed($u8 base64, u60 word);

proc u120ronfeed($u8 base64, u128 words);

// full length base64
proc u60ronfeedn($u8 base64, u60 word, u8 n);

extern const char *ok64str(ok code);

proc u64hexfeed($u8 hex, pos64 pos);

proc u64hexfeedn($u8 hex, pos64 pos, u8 n);

proc u64hexdrain(pos64 *pos, $u8 hex);

proc u64decdrain(u64 *num, $u8 dec);

proc u64decfeed($u8 dec, u64 x);

con ok NOSPACEYET = 421211439390925725UL;
con ok NOTOPENYET = 421215784860001181UL;
con ok BAD_SYNTAX = 201032812518560417UL;
con ok BADARGUMNT = 201031024438240733UL;
con ok FAILSANITY = 273111365594195810UL;
con ok NOTIMPLYET = 421215369505940381UL;

con ok FAILEQTEST = 273111350831802141UL;
con ok FAILMUTEOK = 273111359488845332UL;

#define RUN_TESTS \
    int main(int argn, char **argc) { \
        char *tmp = getenv("TMP"); \
        if (!tmp) tmp = "/tmp"; \
        char __tmp_path[1024]; \
        sprintf(__tmp_path, "%s/test-XXXXXX", tmp); \
        mkdtemp(__tmp_path); \
        assert(0 == chdir(__tmp_path)); \
        ok _ = Tests(); \
        if (_) printf("abnormal finish: %s\n", ok64str(_)); \
        Frmrf(__tmp_path); \
        return (int) _; \
    }

/* . . . . . . . .  Colors . . . . . . . .  */

typedef enum {
    BOLD = 1,

    WEAK = 2,
    HIGHLIGHT = 3,
    UNDERLINE = 4,
    BLACK = 30,
    DARK_RED = 31,
    DARK_GREEN = 32,
    DARK_YELLOW = 33,
    DARK_BLUE = 34,
    DARK_PINK = 35,
    DARK_CYAN = 36,
    BLACK_BG = 40,
    DARK_RED_BG = 41,
    DARK_GREEN_BG = 42,
    DARK_YELLOW_BG = 43,
    DARK_BLUE_BG = 44,
    DARK_PINK_BG = 45,
    DARK_CYAN_BG = 46,
    GRAY = 90,
    LIGHT_RED = 91,
    LIGHT_GREEN = 92,
    LIGHT_YELLOW = 93,
    LIGHT_BLUE = 94,
    LIGHT_PINK = 95,
    LIGHT_CYAN = 96,
    LIGHT_GRAY = 97,
    GRAY_BG = 100,
    LIGHT_RED_BG = 101,
    LIGHT_GREEN_BG = 102,
    LIGHT_YELLOW_BG = 103,
    LIGHT_BLUE_BG = 104,
    LIGHT_PINK_BG = 105,
    LIGHT_CYAN_BG = 106,
    LIGHT_GRAY_BG = 107,
} ANSI_COLOR;

proc escfeed($u8 data, u8 esc);

fun proc u8hexfeed($u8 hex, u8 b) {
    **hex = BASE16[b >> 4];
    ++*hex;
    **hex = BASE16[b & 0xf];
    ++*hex;
    return OK;
}

/* . . . . . . . .  Easy printing . . . . . . . .  */

extern u8 *Pad[4];
#define iPad (Pad + 2)

void Pflush(int fd);

fun void Pout() { Pflush(STDOUT_FILENO); }

fun void Perr() { Pflush(STDERR_FILENO); }

// todo realloc
#define Prep(i) if (Pad[3]-Pad[2] < (i)) return;
#define Push(c) *(Pad[2]++) = (c);

fun void Phexn(u64 num, u8 len) {
    Prep(len);
    u64hexfeedn(iPad, num, len);
}

fun void Ptr(const void *p) {
    Prep(18);
    Push('0');
    Push('x');
    Phexn((u64) p, 12);
}

fun void Pint(i64 i) {
    Prep(21);
    u64 u = i;
    if (i < 0) {
        Push('-');
        u = -i;
    }
    u64decfeed(iPad, u);
}

fun void Pfeed(const $u8 data) {
    u64 sz = data[1] - data[0];
    Prep(sz);
    memcpy(*iPad, data[0], sz);
    *iPad += sz;
}

fun void P$u8($u8 str) {
    Pfeed(str);
}

fun void Pu8(u8 c) {
    Prep(1);
    Push(c);
}

#define Pcoma Pu8(',')
#define Ptab Pu8('\t')
#define Psp Pu8(' ')
#define Pnl() Pu8('\n')

fun void Pesc(u8 esc) {
    Prep(8);
    escfeed(Pad + 2, esc);
}

fun void Phexraw(u64 num) {
    Prep(16);
    u64hexfeedn(iPad, flip64(num), 16);
}

fun void Phex128(u128 num) {
    Prep(32);
    Phexraw(num._64[0]);
    Phexraw(num._64[1]);
}

fun void Phex256(u256 num) {
    Prep(64);
    Phexraw(num._64[0]);
    Phexraw(num._64[1]);
    Phexraw(num._64[2]);
    Phexraw(num._64[3]);
}

fun void Phex(u64 num) {
    Prep(16);
    u64hexfeed(iPad, num);
}

fun void Pu8hex(u8 num) {
    Prep(2);
    u8hexfeed(iPad, num);
}

fun void Pok(ok64 _) {
    Prep(24);
    u60ronfeed(iPad, _);
}

fun void Pstr(const char *str) {
    $u8 s = {(u8 *) str, (u8 *) str + strlen(str)};
    Pfeed(s);
}

fun void Pu32(u32 u) {
    Prep(10);
    u64decfeed(iPad, u);
}

fun void Pu64(u64 u) {
    Prep(16);
    u64decfeed(iPad, u);
}

fun void Pron(u64 word) {
    Prep(24);
    u60ronfeed(iPad, word);
}

fun void Pron120(u128 w) {
    Prep(48);
    u120ronfeed(iPad, w);
}

proc $u8hexfeed($u8 hex, $u8 bin);

proc $u8hexdrain($u8 bin, $u8 hex);

funi void P$hex($u8 data) {
    Prep((data[1] - data[0]) * 2);
    $u8hexfeed(iPad, data);
}

void Pile();

extern int Piled;

#endif //UNTITLED_OK_H
