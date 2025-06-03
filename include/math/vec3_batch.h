#ifndef MATH_VEC3_BATCH_H_
#define MATH_VEC3_BATCH_H_

#include <common.h>
#include <util/dbg/alloctrack.h>
#include <math/vec3.h>
#include <immintrin.h>

typedef struct {
  f32 *restrict data;
  usize count;
} vec3_batch;

static inline vec3_batch create_vec3_batch(usize _s) {
  vec3_batch b;
  /* needed for avx2 instructions */
  GAME_ASSERT(b.count % 8 == 0);
  b.count = _s;
  b.data  = TRACKED_ALIGNED_MALLOC(3 * sizeof(f32) * _s, _Alignof(__m256));
  return b;
}

static inline u0 destroy_vec3_batch(vec3_batch *_b) {
  TRACKED_ALIGNED_FREE(_b->data);
  _b->data  = NULL;
  _b->count = 0;
}

static inline vec3 extract(const vec3_batch *const _batch, usize _index) {
  GAME_ASSERT(_index < _batch->count);
  return (vec3){.x = _batch->data[_index + 0 * _batch->count],
                .y = _batch->data[_index + 1 * _batch->count],
                .z = _batch->data[_index + 2 * _batch->count]};
}

static inline u0
set_one(const vec3 *const _v0, const vec3_batch *const _batch, usize _index) {
  GAME_ASSERT(_index < _batch->count);
  _batch->data[_index + 0 * _batch->count] = _v0->x;
  _batch->data[_index + 1 * _batch->count] = _v0->y;
  _batch->data[_index + 2 * _batch->count] = _v0->z;
}

#endif // MATH_VEC3_BATCH_H_
