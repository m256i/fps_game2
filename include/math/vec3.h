#ifndef MATH_VECTOR_H_
#define MATH_VECTOR_H_

#include <common.h>
#include <immintrin.h>
#include <math.h>

#define VEC3_NORM_TOLERANCE 1e-8f * 1e-8f

typedef struct {
  _Alignas(32) union {
    struct {
      f32 x, y, z;
    };
    struct {
      f32 r, g, b;
    };
    struct {
      f32 width, height, depth;
    };
    __m128 vec128;
  };
} vec3;

static inline vec3 make_vec3(f32 _x, f32 _y, f32 _z) {
  return (vec3){.x = _x, .y = _y, .z = _z};
}

static inline vec3 vec3_add(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.vec128 = _mm_add_ps(_v0->vec128, _v1->vec128)};
}

static inline vec3 vec3_sub(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.vec128 = _mm_sub_ps(_v0->vec128, _v1->vec128)};
}

static inline vec3 vec3_scale(const vec3 *const _v0, f32 _s) {
  return (vec3){.vec128 = _mm_mul_ps(_v0->vec128, _mm_load_ps1(&_s))};
}

static inline f32 vec3_dot(const vec3 *const _v0, const vec3 *const _v1) {
  const __m128 mul  = _mm_mul_ps(_v0->vec128, _v1->vec128);
  __m128       shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
  __m128       sums = _mm_add_ps(mul, shuf);
  shuf              = _mm_movehl_ps(shuf, sums);
  sums              = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

static inline vec3 vec3_cross(const vec3 *const _v0, const vec3 *const _v1) {
  __m128 tmp0 =
    _mm_shuffle_ps(_v0->vec128, _v0->vec128, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 =
    _mm_shuffle_ps(_v1->vec128, _v1->vec128, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2 = _mm_mul_ps(tmp0, _v1->vec128);
  __m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
  __m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  return (vec3){.vec128 = _mm_sub_ps(tmp3, tmp4)};
}

static inline f32 vec3_length(const vec3 *const _v0) {
  return sqrtf(vec3_dot(_v0, _v0));
}

static inline f32 vec3_length_squared(const vec3 *const _v0) {
  return vec3_dot(_v0, _v0);
}

static inline f32 vec3_dist(const vec3 *const _v0, const vec3 *const _v1) {
  const vec3 diff = vec3_sub(_v1, _v0);
  return vec3_length(&diff);
}

static inline f32
vec3_dist_squared(const vec3 *const _v0, const vec3 *const _v1) {
  const vec3 diff = vec3_sub(_v1, _v0);
  return vec3_length_squared(&diff);
}

/*
...yes, gcc 15.0.1+ optimizes basically perfectly on every single machine we
tested this on
*/
static inline vec3 vec3_normalize(const vec3 *const _v) {
  const f32 len = 1.f / sqrtf(_v->x * _v->x + _v->y * _v->y + _v->z * _v->z);
  return (vec3){.x = _v->x * len, .y = _v->y * len, .z = _v->z * len};
}

static inline vec3 vec3_normalize_safe(const vec3 *const _v) {
  const f32 lensqrd = (_v->x * _v->x + _v->y * _v->y + _v->z * _v->z);
  if (lensqrd > VEC3_NORM_TOLERANCE) {
    const f32 len = 1.f / sqrtf(lensqrd);
    return (vec3){.x = _v->x * len, .y = _v->y * len, .z = _v->z * len};
  }
  return (vec3){0};
}

static inline f32
vec3_angle_between(const vec3 *const _v0, const vec3 *const _v1) {
#ifdef GAME_DEBUG
  GAME_ASSERT(vec3_length(_v0) > 0.f);
  GAME_ASSERT(vec3_length(_v1) > 0.f);
#endif
  return acosf(vec3_dot(_v0, _v1) / (vec3_length(_v0) * vec3_length(_v1)));
}

static inline f32
vec3_angle_between_normalized(const vec3 *const _v0, const vec3 *const _v1) {
  return acosf(vec3_dot(_v0, _v1));
}

static inline bool
vec3_approx_eq(const vec3 *const _v0, const vec3 *const _v1, f32 _tol) {
  return vec3_dist(_v0, _v1) <= _tol;
}

static inline bool vec3_equal(const vec3 *const _v0, const vec3 *const _v1) {
  return vec3_dist(_v0, _v1) <= __FLT_EPSILON__;
}

static inline u0 vec3_add_inplace(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  _res->vec128 = _mm_add_ps(_v0->vec128, _v1->vec128);
}

static inline u0
vec3_scale_inplace(vec3 *const _res, const vec3 *const _v0, f32 _s) {
  _res->vec128 = _mm_mul_ps(_v0->vec128, _mm_load1_ps(&_s));
}

static inline u0 vec3_sub_inplace(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  _res->vec128 = _mm_sub_ps(_v0->vec128, _v1->vec128);
}

static inline u0 vec3_cross_inplace(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  __m128 tmp0 =
    _mm_shuffle_ps(_v0->vec128, _v0->vec128, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 =
    _mm_shuffle_ps(_v1->vec128, _v1->vec128, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2  = _mm_mul_ps(tmp0, _v1->vec128);
  __m128 tmp3  = _mm_mul_ps(tmp0, tmp1);
  __m128 tmp4  = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  _res->vec128 = _mm_sub_ps(tmp3, tmp4);
}

static inline u0 vec3_normalize_inplace(vec3 *const _v0) {
  const f32 len =
    1.f / sqrtf(_v0->x * _v0->x + _v0->y * _v0->y + _v0->z * _v0->z);
  vec3_scale(_v0, len);
}

#endif // MATH_VECTOR_H_
