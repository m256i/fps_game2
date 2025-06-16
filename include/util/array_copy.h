#ifndef UTIL_ARRAY_COPY_H_
#define UTIL_ARRAY_COPY_H_

#include <util/safe_string.h>
#include <common.h>
#include <string.h>
#include <assert.h>

#include <util/dbg/alloctrack.h>

static inline u0 *memclone(u0 *_src, usize _src_size) {
  u0 *ptr = TRACKED_MALLOC(_src_size);
  GAME_ASSERT(ptr);
  memcpy(ptr, _src, _src_size);
  return ptr;
}

static inline char *strclone(char *_src) {
  const usize len  = strlen(_src);
  char       *nstr = TRACKED_MALLOC(len + 1);
  GAME_ASSERT(nstr);
  strcpy(nstr, _src);
  return nstr;
}

static inline char *strnclone_s(const char *_src, usize _maxsize) {
  char *nstr = strcpy_safe(_src, _maxsize);
  return nstr;
}

#endif // UTIL_ARRAY_COPY_H_
