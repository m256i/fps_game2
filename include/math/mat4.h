#ifndef MATH_MAT4_H_
#define MATH_MAT4_H_

/*
TODO:


view_matrix4(pos, forward, up)
perspective_matrix4(fov_radians, f32 aspect_ratio, znear, zfar)


*/

#include <common.h>
#include <math/vec3.h>
#include <string.h>

typedef struct {
  /* we want this to be packed so we wont use alignas(32) or __m256s */
  union {
    _Alignas(16) struct {
      f32 m00, m10, m20, m30;
      f32 m01, m11, m21, m31;
      f32 m02, m12, m22, m32;
      f32 m03, m13, m23, m33;
    };
    _Alignas(16) f32 data[16];
    __m128 vec4_data[4];
  };
} mat4;

static inline f32 *
mat4_at(mat4 *const _m0, const usize _row, const usize _col) {
  return &_m0->data[_row * 4 + _col];
}

static inline u0 mat4_identity(mat4 *const _m0) {
  memset(_m0, 0, sizeof *_m0);
  _m0->m00 = 1.f;
  _m0->m11 = 1.f;
  _m0->m22 = 1.f;
  _m0->m33 = 1.f;
}

static inline u0 mat4_clear(mat4 *const _m0) { memset(_m0, 0, sizeof *_m0); }

static inline u0
mat4_look_at(mat4 *m, const vec3 *eye, const vec3 *center, const vec3 *up) {
  vec3 f;
  vec3_sub_inplace(&f, center, eye);
  vec3_normalize_inplace(&f);

  vec3 u = vec3_normalize(up);
  vec3 s = vec3_cross(&f, &u);
  vec3_normalize_inplace(&s);

  // Negative forward for RH
  vec3 nf = {-f.x, -f.y, -f.z};

  // Column 0
  m->data[0 * 4 + 0] = s.x;
  m->data[0 * 4 + 1] = u.x;
  m->data[0 * 4 + 2] = nf.x;
  m->data[0 * 4 + 3] = 0.f;

  // Column 1
  m->data[1 * 4 + 0] = s.y;
  m->data[1 * 4 + 1] = u.y;
  m->data[1 * 4 + 2] = nf.y;
  m->data[1 * 4 + 3] = 0.f;

  // Column 2
  m->data[2 * 4 + 0] = s.z;
  m->data[2 * 4 + 1] = u.z;
  m->data[2 * 4 + 2] = nf.z;
  m->data[2 * 4 + 3] = 0.f;

  // Column 3 (translation)
  m->data[3 * 4 + 0] = -vec3_dot(&s, eye);
  m->data[3 * 4 + 1] = -vec3_dot(&u, eye);
  m->data[3 * 4 + 2] = vec3_dot(&f, eye);
  m->data[3 * 4 + 3] = 1.f;
}

static inline u0 mat4_perspective(
  mat4 *const _result,
  f32         _fov,
  f32         _aspect,
  f32         _znear,
  f32         _zfar
) {
  const f32 tan_half_fov = tanf(_fov / 2.f);
  _result->m00           = 1.f / (_aspect * tan_half_fov);
  _result->m11           = 1.f / tan_half_fov;
  _result->m22           = -(_zfar + _znear) / (_zfar - _znear);
  _result->m23           = -(2.f * _zfar * _znear) / (_zfar - _znear);
  _result->m32           = -1.f;
}

#endif // MATH_MAT4_H_
