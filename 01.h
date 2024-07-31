#ifndef ABC_BITS_H
#define ABC_BITS_H
#include <stdint.h>
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

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t b8;
con b8 NO = 0;
con b8 YES = 1;

typedef uint64_t ok64;

#define OK 0
#define FAIL 0xffffffffffffffffUL

con ok64 $noroom = 0x31cf3db3cbf;
con ok64 $badarg = 0x2bda5a259bf;

#define nil NULL
#define WORDS(k)                                                               \
  union {                                                                      \
    u64 _64[k];                                                                \
    u32 _32[k * 2];                                                            \
    u16 _16[k * 4];                                                            \
    u8 _8[k * 8];                                                              \
  }

typedef WORDS(2) u128;
typedef WORDS(4) u256;
typedef WORDS(8) u512;

#define PAGELOG2 12
#define PAGESIZE (1 << LOG2PAGE)
#define PAGEMASK (PAGE_SIZE - 1)

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
#if defined(__linux__) || defined(__CYGWIN__)
#include <endian.h>
#define flip64(x) __bswap_64(x)
#define flip32(x) __bswap_32(x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define flip64(x) OSSwapInt64(x)
#define flip32(x) OSSwapInt32(x)
#elif defined(__OpenBSD__)
#include <sys/endian.h>
// TODO
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/endian.h>
// TODO
#elif defined(__WINDOWS__)
#include <sys/param.h>
#include <winsock2.h>
#if BYTE_ORDER == LITTLE_ENDIAN
// TODO
#elif BYTE_ORDER == BIG_ENDIAN
// TODO xbox?
#else
#error unknown platform
#endif
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

#endif
