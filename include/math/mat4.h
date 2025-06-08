#ifndef MATH_MAT4_H_
#define MATH_MAT4_H_

#include <common.h>
#include <math/vec3.h>
#include <string.h>

typedef struct {
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
mat4_mul(mat4 *const _result, const mat4 *const _m0, const mat4 *const _m1) {
  for (int col = 0; col < 4; ++col) {
    __m128 b0 = _mm_set1_ps(_m1->data[col * 4 + 0]);
    __m128 b1 = _mm_set1_ps(_m1->data[col * 4 + 1]);
    __m128 b2 = _mm_set1_ps(_m1->data[col * 4 + 2]);
    __m128 b3 = _mm_set1_ps(_m1->data[col * 4 + 3]);

    __m128 result_col = _mm_mul_ps(_m0->vec4_data[0], b0);
    result_col = _mm_add_ps(result_col, _mm_mul_ps(_m0->vec4_data[1], b1));
    result_col = _mm_add_ps(result_col, _mm_mul_ps(_m0->vec4_data[2], b2));
    result_col = _mm_add_ps(result_col, _mm_mul_ps(_m0->vec4_data[3], b3));

    _result->vec4_data[col] = result_col;
  }
}

static inline u0 mat4_look_at(
  mat4       *m,
  const vec3 *position,
  const vec3 *target,
  const vec3 *up
) {
  vec3 tmp_forward;
  vec3 tmp_side;
  vec3 tmp_up;
  tmp_forward = vec3_sub(target, position);
  tmp_forward = vec3_normalize(&tmp_forward);
  tmp_side    = vec3_cross(&tmp_forward, up);
  tmp_side    = vec3_normalize(&tmp_side);
  tmp_up      = vec3_cross(&tmp_side, &tmp_forward);
  m->data[0]  = tmp_side.x;
  m->data[1]  = tmp_up.x;
  m->data[2]  = -tmp_forward.x;
  m->data[3]  = 0;
  m->data[4]  = tmp_side.y;
  m->data[5]  = tmp_up.y;
  m->data[6]  = -tmp_forward.y;
  m->data[7]  = 0;
  m->data[8]  = tmp_side.z;
  m->data[9]  = tmp_up.z;
  m->data[10] = -tmp_forward.z;
  m->data[11] = 0;
  m->data[12] = -vec3_dot(&tmp_side, position);
  m->data[13] = -vec3_dot(&tmp_up, position);
  m->data[14] = vec3_dot(&tmp_forward, position);
  m->data[15] = 1.0;
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
