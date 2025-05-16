#ifndef UTIL_ZREALLOC_H_
#define UTIL_ZREALLOC_H_

#include <common.h>
#include <string.h>

/* realloc that 0s out the new buffer part */
static u0 *zrealloc(u0 *_mem, usize _oldsize, usize _newsize) {
  if (_oldsize == _newsize) { return _mem; }
  u0 *newbuf = realloc(_mem, _newsize);
  if (!newbuf) {
    GAME_CRITICALF("OOM");
    exit(1);
  }
  if (_newsize > _oldsize) {
    ptrdiff dif = _newsize - _oldsize;
    u0 *new_start = ((u8 *)newbuf) + _oldsize;
    memset(new_start, 0, dif);
  }
  return newbuf;
}

#endif // UTIL_ZREALLOC_H_
