#ifndef MATH_MATH_H_
#define MATH_MATH_H_

#include <common.h>

static inline f64 clamp(f64 _d, f64 _min, f64 _max) {
  const f64 t = _d < _min ? _min : _d;
  return t > _max ? _max : t;
}

static inline f64 clampf(f32 _d, f32 _min, f32 _max) {
  const f32 t = _d < _min ? _min : _d;
  return t > _max ? _max : t;
}

#define CLAMP(value, low, high)                                                \
  (value < low ? low : (value > high ? high : value))

#endif // MATH_MATH_H_
