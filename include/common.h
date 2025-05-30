#ifndef COMMON_H_
#define COMMON_H_
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <immintrin.h>
#include <log/log.h>

/* game client build version */
#define GAME_CLIENT_VER 0.1a
// #define GAME_DEBUG

/* macro impl stuff */
#define STRINGIFY(x)              #x
#define STRINGIFY_L(x)            L## #x
#define EXPAND_AND_STRINGIFY(x)   STRINGIFY(x)
#define EXPAND_AND_STRINGIFY_L(x) STRINGIFY_L(x)
#define GAME_CLIENT_VER_STR       EXPAND_AND_STRINGIFY(GAME_CLIENT_VER)
#define GAME_CLIENT_VER_STR_L     EXPAND_AND_STRINGIFY_L(GAME_CLIENT_VER)

/* just the usual suspects */
typedef void      u0;
typedef size_t    usize;
typedef uintptr_t uptr;
typedef ptrdiff_t ptrdiff;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float       f32;
typedef double      f64;
typedef long double f87;

/*
if attributed to a callback signalizes that the callback may get called
asynchronously. so:

GASYNC_CALLBACK bool my_callback(u0* data) {
  ... modifies external state unsynchronized ...
}

will almost certainly lead to a race condition
*/
#define GASYNC_CALLBACK

#ifdef GAME_DEBUG
#define GAME_ASSERT(_expr)  assert(_expr)
#define GAME_LOGF(...)      log_info(__VA_ARGS__)
#define GAME_WARNF(...)     log_warn(__VA_ARGS__)
#define GAME_CRITICALF(...) log_error(__VA_ARGS__)
#else
#define GAME_ASSERT(_expr)  (u0)0
#define GAME_LOGF(...)      (u0)0
#define GAME_WARNF(...)     log_warn(__VA_ARGS__)
#define GAME_CRITICALF(...) log_error(__VA_ARGS__)
#endif

#endif // COMMON_H_
