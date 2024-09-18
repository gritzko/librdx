#ifndef ABC_BITS_H
#define ABC_BITS_H
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define fun static inline
#define funi static __always_inline
#define con static const
#define record typedef struct

#define must(c) assert(c)

typedef void *$[2];
typedef void *const $c[2];
typedef void const *$_c[2];
typedef void const *const $cc[2];

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u8 const u8c;
typedef u16 const u16c;
typedef u32 const u32c;
typedef u64 const u64c;

typedef u8 *u8p;
typedef u16 *u16p;
typedef u32 *u32p;
typedef u64 *u64p;

typedef u8 const *u8cp;
typedef u16 const *u16cp;
typedef u32 const *u32cp;
typedef u64 const *u64cp;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u8 bl00[1];
typedef u8 bl01[2];
typedef u8 bl02[4];
typedef u8 bl03[8];
typedef u8 bl04[0x10];
typedef u8 bl05[0x20];
typedef u8 bl06[0x40];
typedef u8 bl07[0x80];
typedef u8 bl08[0x100];
typedef u8 bl09[0x200];
typedef u8 bl0A[0x400];
typedef u8 bl0B[0x800];
typedef u8 bl0C[0x1000];
typedef u8 bl0D[0x2000];
typedef u8 bl0E[0x4000];
typedef u8 bl0F[0x8000];
typedef u8 bl10[0x10000];

typedef u8 const *$u8c[2];
typedef u8 const *const $cu8c[2];

// boolean, strictly 0 or 1
typedef uint8_t b8;
con b8 NO = 0;
con b8 YES = 1;

fun int b8ok(b8 b) { return (b & ~1) == 0; }

typedef uint64_t ok64;

#define OK 0
#define FAIL 0xffffffffffffffffUL

con ok64 $noroom = 0x31cf3db3cbf;
con ok64 $badarg = 0x2bda5a259bf;

#define nil NULL
#define WORDS(k)        \
    union {             \
        u64 _64[k];     \
        u32 _32[k * 2]; \
        u16 _16[k * 4]; \
        u8 _8[k * 8];   \
    }

typedef WORDS(1) w64;
typedef WORDS(2) w128;
typedef WORDS(4) w256;
typedef WORDS(8) w512;
typedef w128 u128;
typedef w256 u256;
typedef w512 u512;

#ifndef PAGESIZE
#define PAGESIZE (1 << 12)
#endif

// size_t, 6 bytes; two upper bytes are free for reuse
typedef u64 pos48;
fun u16 pos48hi16(pos48 u) { return u >> 48; }

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// bit counting
#ifdef _MSC_VER
#include <intrin.h>
uint32_t __inline clz64(u64 value) {
    DWORD leading_zero = 0;
    _BitScanReverse64(&leading_zero, value);
    return 63 - leading_zero;
}
// TODO ctz popc
#elif __GNUC__
#define clz64(x) __builtin_clzll(x)
#define ctz64(x) __builtin_ctzll(x)
#define clz32(x) __builtin_clz(x)
#define ctz32(x) __builtin_ctz(x)
#define popc32(x) __builtin_popcount(x)
#define popc64(x) __builtin_popcount(x)
#endif

// flipping byte order (we imply CPU is little endian)
// from Google's CityHash
#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else
#include <byteswap.h>
#endif

#define flip32(x) bswap_32(x)
#define flip64(x) bswap_64(x)

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
    if (is_power_of_2(a)) return a;
    int p = clz64(a);
    return 1UL << (64 - p);
}

fun u8 upper_log_2(u64 val) {
    u8 pow = 64 - clz64(val);
    if (!is_power_of_2(val)) ++pow;
    return pow;
}

fun u8 log_2(size_t x) {
    u8 z = clz64(x | 1);
    return 63 - z;
}

fun u64 roundup(u64 val, u64 page) {
    u64 mask = page - 1;
    if (val & mask) val = (val & ~mask) + page;
    return val;
}

con u64 PRIME1 = 11400714785074694791ULL;
con u64 PRIME2 = 14029467366897019727ULL;
con u64 PRIME3 = 1609587929392839161ULL;
con u64 PRIME4 = 9650029242287828579ULL;
con u64 PRIME5 = 2870177450012600261ULL;

typedef uint64_t h64;

fun h64 mix32(u32 a) { return (a * PRIME1) >> 32; }

fun h64 mix64(u64 a) {
    h64 mix1 = a * PRIME1;
    h64 mix2 = flip64(mix1) ^ a;
    return mix2 * PRIME2;
}

#define XB (1UL << 60)
#define PB (1UL << 50)
#define TB (1UL << 40)
#define GB (1UL << 30)
#define MB (1UL << 20)
#define KB (1UL << 10)

#define O1join32(lo, hi) (((u64)lo) | (((u64)hi) << 32))
#define O1low32(lohi) (((u64)lohi) & 0xffffffff)
#define O1high32(lohi) (((u64)lohi) >> 32)

#define bitmask(l) ((1UL << l) - 1)
#define bitpack(a, h, l) (((u64)(a) & bitmask(l)) << (h))
#define bitpick(u, h, l) ((u >> h) & bitmask(l))
#define bitpack4816(a, b) (bitpack(a, 0, 48) | bitpack(b, 48, 16))
#define bitpick4816a(u) (u & 0xffffffffffffUL)
#define bitpick4816b(u) (u >> 48)

#define bitpack4888(a, b, c) \
    (bitpack(a, 0, 48) | bitpack(b, 48, 8) | bitpack(c, 56, 8))
#define bitpick4888a(u) bitpick(u, 0, 48)
#define bitpick4888b(u) bitpick(u, 48, 8)
#define bitpick4888c(u) bitpick(u, 56, 8)

fun u8 u8bytelen(u8 u) { return u == 0 ? 0 : 1; }

fun u8 u16bytelen(u16 u) { return u > 0xff ? 2 : u8bytelen((u8)u); }

fun u8 u32bytelen(u32 u) {
    return u > 0xffff ? 2 + u16bytelen(u >> 16) : u16bytelen((u16)u);
}

fun u8 u64bytelen(u64 u) {
    if (u == 0) return 0;
    u8 b = clz64(u) >> 3;
    return 8 - b;
}

fun u8 u64byte(u64 u, u8 b) { return u >> (b << 3); }
fun u8 u64getbyte(u64 u, u8 b) { return u64byte(u, b); }

fun u8 u64topbyte(u64 u) {
    u8 l = u64bytelen(u);
    if (l == 0) return 0;
    return u64byte(u, l - 1);
}

fun u64 u64setbyte(u64 u, u8 b, u8 ndx) {
    u8 shift = ndx << 3;
    u64 mask = 0xffUL << shift;
    u64 newval = ((u64)b) << shift;
    return (u & ~mask) | newval;
}

fun u64 u64lowbytes(u64 u, u8 keep) {
    if (keep == 0) return 0;
    return u & (UINT64_MAX >> ((8 - keep) << 3));
}

fun u8 w64bytelen(w64 w) { return u64bytelen(w._64[0]); }

fun u8 u128bytelen(u128 u) {
    return u._64[1] ? 8 + u64bytelen(u._64[1]) : u64bytelen(u._64[0]);
}

#ifdef __GNUC__
#define unlikely(x) (__builtin_expect(x, 0))
#define likely(x) (__builtin_expect(!!(x), 1))
#else
#define unlikely(x) (x)
#define likely(x) (x)
#endif

fun ok64 u64decfeed(u8 **dec, u64 x) {
    u8 into[32];
    u8 *e = into + 32;
    u8 *to = e;
    do {
        u8 digit = x % 10;
        *--to = '0' + digit;
        x /= 10;
    } while (x);
    size_t sz = e - to;
    if (dec[1] - dec[0] < sz) return $noroom;
    memcpy(*dec, to, sz);
    *dec += sz;
    return OK;
}

#endif
