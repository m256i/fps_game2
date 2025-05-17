#ifndef MATH_INTERP_H_
#define MATH_INTERP_H_
#include <common.h>

/* lerp between arbitrary values, make sure _step is inbetween 0 and 1 */
inline f64 lerpf64(f64 _src, f64 _dst, f64 _step) {
  return _src * (1.0 - _step) + (_dst * _step);
}
/* lerp between arbitrary values, make sure _step is inbetween 0 and 1 */
inline u64 lerpu64(u64 _src, u64 _dst, f64 _step) {
  return (u64)((f64)_src * (1.0 - _step) + ((f64)_dst * _step));
}

#endif // MATH_INTERP_H_