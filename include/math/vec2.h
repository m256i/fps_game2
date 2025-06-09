#ifndef MATH_VEC2_H_
#define MATH_VEC2_H_

#include <common.h>
#include <immintrin.h>
#include <math.h>

#define VEC2_NORM_TOLERANCE 1e-8f * 1e-8f

/*
here we actually always pass by value and also never done inplace
because we can pass like 4 vec2s just through registers
*/
typedef struct {
  union {
    struct {
      f32 x, y;
    };
    struct {
      f32 r, a;
    };
    struct {
      f32 width, height;
    };
    __m64 vec64;
  };
} vec2;

static inline vec2 make_vec2(f32 _x, f32 _y) {
  return (vec2){.x = _x, .y = _y};
}

static inline vec2 vec2_add(const vec2 _v0, const vec2 _v1) {
  return (vec2){.x = _v0.x + _v1.x, .y = _v0.y + _v1.y};
}

static inline vec2 vec2_sub(const vec2 _v0, const vec2 _v1) {
  return (vec2){.x = _v0.x - _v1.x, .y = _v0.y - _v1.y};
}

static inline vec2 vec2_mul(const vec2 _v0, const vec2 _v1) {
  return (vec2){.x = _v0.x * _v1.x, .y = _v0.y * _v1.y};
}

static inline vec2 vec2_div(const vec2 _v0, const vec2 _v1) {
  return (vec2){.x = _v0.x / _v1.x, .y = _v0.y / _v1.y};
}

static inline vec2 vec2_scale(const vec2 _v0, f32 _s) {
  return (vec2){.x = _v0.x * _s, .y = _v0.y * _s};
}

static inline f32 vec2_length(const vec2 _v0) {
  return sqrtf(_v0.x * _v0.x + _v0.y * _v0.y);
}

static inline f32 vec2_length_squared(const vec2 _v0) {
  return _v0.x * _v0.x + _v0.y * _v0.y;
}

static inline f32 vec2_distance(const vec2 _v0, const vec2 _v1) {
  return vec2_length(vec2_sub(_v1, _v0));
}

static inline f32 vec2_distance_squared(const vec2 _v0, const vec2 _v1) {
  return vec2_length_squared(vec2_sub(_v1, _v0));
}

static inline vec2 vec2_normalized(const vec2 _v0) {
  const f32 l = 1.f / vec2_length(_v0);
  return (vec2){.x = _v0.x * l, .y = _v0.y * l};
}

static inline u0 vec2_normalize(vec2 *const _v0) {
  *_v0 = vec2_normalized(*_v0);
}

static inline vec2 vec2_lerp(const vec2 _v0, const vec2 _v1, f32 _t) {
  return (vec2){.x = _v0.x + (_v1.x - _v0.x) * _t,
                .y = _v0.y + (_v1.y - _v1.y) * _t};
}

#endif // MATH_VEC2_H_
