#ifndef UTIL_DBG_ALLOCTRACK_H_
#define UTIL_DBG_ALLOCTRACK_H_

#include <common.h>
#ifdef GAME_DEBUG
#define TRACKED_MALLOC(_size) tracked_malloc(_size, __PRETTY_FUNCTION__)
#define TRACKED_ALIGNED_MALLOC(_size, _alignment)                              \
  tracked_aligned_malloc(_size, _alignment, __PRETTY_FUNCTION__)
#define TRACKED_ALIGNED_REALLOC(_ptr, _size, _alignment)                       \
  tracked_aligned_realloc(_ptr, _size, _alignment, __PRETTY_FUNCTION__)
#define TRACKED_REALLOC(_ptr, _size)                                           \
  tracked_realloc(_ptr, _size, __PRETTY_FUNCTION__)
#define TRACKED_CALLOC(_count, _size)                                          \
  tracked_calloc(_count, _size, __PRETTY_FUNCTION__)
#define TRACKED_FREE(_ptr) tracked_free(_ptr, __PRETTY_FUNCTION__)
#define TRACKED_ALIGNED_FREE(_ptr)                                             \
  tracked_aligned_free(_ptr, __PRETTY_FUNCTION__)

#else
#ifdef _WIN64
#include <stdlib.h>
#define TRACKED_MALLOC(_size) malloc(_size)
#define TRACKED_ALIGNED_MALLOC(_size, _alignment)                              \
  _aligned_malloc(_size, _alignment)
#define TRACKED_ALIGNED_REALLOC(_ptr, _size, _alignment)                       \
  _aligned_malloc(_ptr, _size, _alignment)
#define TRACKED_REALLOC(_ptr, _size)  realloc(_ptr, _size)
#define TRACKED_CALLOC(_count, _size) calloc(_count, _size)
#define TRACKED_FREE(_ptr)            free(_ptr)
#define TRACKED_ALIGNED_FREE(_ptr)    _aligned_free(_ptr)
#else

#include <stdlib.h>
#define TRACKED_MALLOC(_size) malloc(_size)
#define TRACKED_ALIGNED_MALLOC(_size, _alignment)                              \
  aligned_alloc(_alignment, _size)
#define TRACKED_ALIGNED_REALLOC(_ptr, _size, _alignment)                       \
  assert(false) /* TODO: implement properly */
#define TRACKED_REALLOC(_ptr, _size)  realloc(_ptr, _size)
#define TRACKED_CALLOC(_count, _size) calloc(_count, _size)
#define TRACKED_FREE(_ptr)            free(_ptr)
#define TRACKED_ALIGNED_FREE(_ptr)    free(_ptr)
#endif
#endif

u0 *tracked_malloc(usize _size, const char *_function);
u0 *tracked_aligned_malloc(
  usize       _size,
  usize       _alignment,
  const char *_function
);
u0 *tracked_aligned_realloc(
  u0         *_ptr,
  usize       _size,
  usize       _alignment,
  const char *_function
);

u0 *tracked_realloc(u0 *_ptr, usize _size, const char *_function);
u0 *tracked_calloc(usize _count, usize _size, const char *_function);
u0  tracked_free(u0 *_ptr, const char *_function);
u0  tracked_aligned_free(u0 *_ptr, const char *_function);
u0  dump_alloc_statistics(u0);

#endif // UTIL_DBG_ALLOCTRACK_H_
