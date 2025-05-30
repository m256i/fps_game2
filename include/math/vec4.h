#ifndef MATH_VEC4_H_
#define MATH_VEC4_H_

#include <common.h>
#include <immintrin.h>
#include <math.h>

#define VEC4_NORM_TOLERANCE 1e-8f * 1e-8f

typedef struct {
  _Alignas(32) union {
    struct {
      f32 x, y, z, w;
    };
    struct {
      f32 r, g, b, a;
    };
    __m128 vec128;
  };
} vec4;

static inline vec4 make_vec4(f32 _x, f32 _y, f32 _z, f32 _w) {
  return (vec4){.x = _x, .y = _y, .z = _z, .w = _w};
}

#endif // MATH_VEC4_H_
