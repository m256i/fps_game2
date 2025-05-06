#ifndef COMMON_H_
#define COMMON_H_
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <immintrin.h>
#include <log/log.h>

/* game client build version */
#define GAME_CLIENT_VER 0.1a
// #define GAME_DEBUG

/* macro impl stuff */
#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)
#define GAME_CLIENT_VER_STR EXPAND_AND_STRINGIFY(GAME_CLIENT_VER)

/* just the usual suspects */
// clang-format off
typedef void          u0;
typedef size_t        usize;
typedef uintptr_t     uptr;
typedef ptrdiff_t     ptrdiff;

typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;

typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;

typedef float         f32;
typedef double        f64;
typedef long double   f87;

#ifdef GAME_DEBUG
#define GAME_LOGF(...) log_info(__VA_ARGS__)
#define GAME_WARNF(...) log_warn(__VA_ARGS__)
#define GAME_CRITICALF(...) log_error(__VA_ARGS__)
#else
#define GAME_LOGF(...) (u0)0
#define GAME_WARNF(...) log_warn(__VA_ARGS__)
#define GAME_CRITICALF(...) log_error(__VA_ARGS__)
#endif

// clang-format on
#endif // COMMON_H_
