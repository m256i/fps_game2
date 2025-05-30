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
  _Alignas(32) union {
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

vec2 vec2_add(const vec2 _v0, const vec2 _v1) {
  return (vec2){.x = _v0.x + _v1.x, .y = _v0.y + _v1.y};
}

/*
TODO: implement
*/

#endif // MATH_VEC2_H_
