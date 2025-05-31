#ifndef MATH_VEC3_BATCH_H_
#define MATH_VEC3_BATCH_H_

#include <common.h>
#include <util/dbg/alloctrack.h>
#include <math/vec3.h>
#include <immintrin.h>

typedef struct {
  f32 *restrict x;
  f32 *restrict y;
  f32 *restrict z;
  usize count;
} vec3_batch;

static inline vec3_batch create_vec3_batch(usize _s) {
  vec3_batch b;
  /* needed for avx2 instructions */
  GAME_ASSERT(b.count % 8 == 0);

  b.count = _s;
  b.x     = TRACKED_ALIGNED_MALLOC(sizeof(f32) * _s, _Alignof(__m256));
  b.y     = TRACKED_ALIGNED_MALLOC(sizeof(f32) * _s, _Alignof(__m256));
  b.z     = TRACKED_ALIGNED_MALLOC(sizeof(f32) * _s, _Alignof(__m256));
  return b;
}

static inline u0 destroy_vec3_batch(vec3_batch *_b) {
  TRACKED_ALIGNED_FREE(_b->x);
  TRACKED_ALIGNED_FREE(_b->y);
  TRACKED_ALIGNED_FREE(_b->z);
  _b->x     = NULL;
  _b->y     = NULL;
  _b->z     = NULL;
  _b->count = 0;
}

static inline vec3 extract(const vec3_batch *const _batch, usize _index) {
  GAME_ASSERT(_index < _batch->count);
  // const __m256i indices = _mm256_set_epi64x(
  //   _index,                                   /* index in x array */
  //   sizeof(f32) * _batch->count + _index,     /* index in y array */
  //   sizeof(f32) * _batch->count * 2 + _index, /* index in z array */
  //   0
  // );
  // const __m128 res = _mm256_mask_i64gather_ps(
  //   _mm_setzero_si128(), /* we dont need any default data */
  //   _batch->x,           /* start indexing at the x array */
  //   indices,
  //   _mm_set_epi32(
  //     (u32)-1,
  //     (u32)-1,
  //     (u32)-1,
  //     (u32)0
  //   ), /*  set first three mask slots to all 1s and the last one to 0 */
  //   sizeof(f32)
  // );
  // return (vec3){.vec128 = res};
  return (vec3){.x = _batch->x[_index],
                .y = _batch->y[_index],
                .z = _batch->z[_index]};
}

static inline u0
set_one(const vec3 *const _v0, const vec3_batch *const _batch, usize _index) {
  GAME_ASSERT(_index < _batch->count);
  _batch->x[_index] = _v0->x;
  _batch->y[_index] = _v0->y;
  _batch->z[_index] = _v0->z;
}

#endif // MATH_VEC3_BATCH_H_
