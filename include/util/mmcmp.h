#ifndef UTIL_MMCMP_H_
#define UTIL_MMCMP_H_

#include <common.h>

typedef struct {
  uptr start;
  usize length;
} mmcmp_msk;

/* masked memcmp, API is obvious */
static i32 mmcmp(const void *_buf0, const void *_buf1, usize _size, const mmcmp_msk *_msks, usize _nmsk) {
  const u8 *p1 = _buf0;
  const u8 *p2 = _buf1;
  for (usize i = 0; i < _size; ++i) {
    bool is_masked = false;
    for (usize m = 0; m < _nmsk; ++m) {
      if (i >= _msks[m].start && i < _msks[m].start + _msks[m].length) {
        is_masked = true;
        break;
      }
    }
    if (!is_masked && p1[i] != p2[i]) { return (i32)(i + 1); }
  }
  return 0;
}

#endif // UTIL_MMCMP_H_