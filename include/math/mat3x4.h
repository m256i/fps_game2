#ifndef MATH_MAT3X4_H_
#define MATH_MAT3X4_H_

#include <common.h>
#include <immintrin.h>
#include <string.h>
#include <math/vec3.h>

/*
this exists because for most matrix math you only actually need
the three rows and the fourth is implicitly 0,0,0,1
we can safe alot of space and cache this way.
*/

typedef struct {
  /* we want this to be packed so we wont use alignas(32) or __m256s */
  union {
    struct {
      f32 x0, y0, z0, w0;
      f32 x1, y1, z1, w1;
      f32 x2, y2, z2, w2;
      /* x3 = 0, y3 = 0, z3 = 0, w3 = 1 */
    };
    struct {
      f32 data[12];
    };
  };
} mat3x4;

__attribute__((const)) static inline mat3x4 mat3x4_identity(u0) {
  mat3x4 out = {0};
  out.x0     = 1.f;
  out.y1     = 1.f;
  out.z2     = 1.f;
  return out;
}

static inline u0 mat3x4_to_identity(mat3x4 *const _m) {
  memset(_m->data, 0, sizeof _m->data);
  _m->x0 = 1.f;
  _m->y1 = 1.f;
  _m->z2 = 1.f;
}

static inline u0 mat3x4_assign(
  mat3x4 *const restrict _mdest,
  const mat3x4 *const restrict _msrc
) {
  memcpy(_mdest, _msrc, sizeof *_mdest);
}

static inline u0 mat3x4_from_transform(
  mat3x4 *const restrict _m,
  const vec3 *const restrict _scale,
  const vec3 *const restrict _rotation, /* euler angles in radians */
  const vec3 *const restrict _translation
) {
  const f32 sx = sinf(_rotation->x), cx = cosf(_rotation->x);
  const f32 sy = sinf(_rotation->y), cy = cosf(_rotation->y);
  const f32 sz = sinf(_rotation->z), cz = cosf(_rotation->z);

  const f32 m00 = cy * cz;
  const f32 m01 = cy * sz;
  const f32 m02 = -sy;

  const f32 m10 = sx * sy * cz - cx * sz;
  const f32 m11 = sx * sy * sz + cx * cz;
  const f32 m12 = sx * cy;

  const f32 m20 = cx * sy * cz + sx * sz;
  const f32 m21 = cx * sy * sz - sx * cz;
  const f32 m22 = cx * cy;

  const f32 s0 = _scale->x;
  const f32 s1 = _scale->y;
  const f32 s2 = _scale->z;

  _m->x0 = m00 * s0;
  _m->y0 = m01 * s0;
  _m->z0 = m02 * s0;
  _m->w0 = _translation->x;

  _m->x1 = m10 * s1;
  _m->y1 = m11 * s1;
  _m->z1 = m12 * s1;
  _m->w1 = _translation->y;

  _m->x2 = m20 * s2;
  _m->y2 = m21 * s2;
  _m->z2 = m22 * s2;
  _m->w2 = _translation->z;
}

static inline u0 mat3x4_mul(
  mat3x4 *const restrict _out,
  const mat3x4 *const restrict _m0,
  const mat3x4 *const restrict _m1
) {
  GAME_ASSERT(_out != _m0 && _out != _m1);
  GAME_ASSERT(_m0 != _m1);

  const f32 a00 = _m0->x0, a01 = _m0->y0, a02 = _m0->z0, a03 = _m0->w0;
  const f32 a10 = _m0->x1, a11 = _m0->y1, a12 = _m0->z1, a13 = _m0->w1;
  const f32 a20 = _m0->x2, a21 = _m0->y2, a22 = _m0->z2, a23 = _m0->w2;

  const f32 b00 = _m1->x0, b01 = _m1->y0, b02 = _m1->z0, b03 = _m1->w0;
  const f32 b10 = _m1->x1, b11 = _m1->y1, b12 = _m1->z1, b13 = _m1->w1;
  const f32 b20 = _m1->x2, b21 = _m1->y2, b22 = _m1->z2, b23 = _m1->w2;

  _out->x0 = a00 * b00 + a01 * b10 + a02 * b20;
  _out->y0 = a00 * b01 + a01 * b11 + a02 * b21;
  _out->z0 = a00 * b02 + a01 * b12 + a02 * b22;
  _out->w0 = a00 * b03 + a01 * b13 + a02 * b23 + a03;

  _out->x1 = a10 * b00 + a11 * b10 + a12 * b20;
  _out->y1 = a10 * b01 + a11 * b11 + a12 * b21;
  _out->z1 = a10 * b02 + a11 * b12 + a12 * b22;
  _out->w1 = a10 * b03 + a11 * b13 + a12 * b23 + a13;

  _out->x2 = a20 * b00 + a21 * b10 + a22 * b20;
  _out->y2 = a20 * b01 + a21 * b11 + a22 * b21;
  _out->z2 = a20 * b02 + a21 * b12 + a22 * b22;
  _out->w2 = a20 * b03 + a21 * b13 + a22 * b23 + a23;
}

static inline f32 mat3x4_det(const mat3x4 *const m) {
  return m->x0 * (m->y1 * m->z2 - m->y2 * m->z1) -
         m->y0 * (m->x1 * m->z2 - m->x2 * m->z1) +
         m->z0 * (m->x1 * m->y2 - m->x2 * m->y1);
}

static inline bool
mat3x4_inverse(mat3x4 *const restrict out, const mat3x4 *const restrict m) {
  const f32 x0 = m->x0, y0 = m->y0, z0 = m->z0;
  const f32 x1 = m->x1, y1 = m->y1, z1 = m->z1;
  const f32 x2 = m->x2, y2 = m->y2, z2 = m->z2;

  const f32 det = mat3x4_det(m);
  if (fabsf(det) < 1e-8f) return false;
  const f32 inv_det = 1.0f / det;

  out->x0 = (y1 * z2 - y2 * z1) * inv_det;
  out->y0 = -(y0 * z2 - y2 * z0) * inv_det;
  out->z0 = (y0 * z1 - y1 * z0) * inv_det;
  out->x1 = -(x1 * z2 - x2 * z1) * inv_det;
  out->y1 = (x0 * z2 - x2 * z0) * inv_det;
  out->z1 = -(x0 * z1 - x1 * z0) * inv_det;
  out->x2 = (x1 * y2 - x2 * y1) * inv_det;
  out->y2 = -(x0 * y2 - x2 * y0) * inv_det;
  out->z2 = (x0 * y1 - x1 * y0) * inv_det;

  const f32 tx = m->w0;
  const f32 ty = m->w1;
  const f32 tz = m->w2;

  out->w0 = -(out->x0 * tx + out->y0 * ty + out->z0 * tz);
  out->w1 = -(out->x1 * tx + out->y1 * ty + out->z1 * tz);
  out->w2 = -(out->x2 * tx + out->y2 * ty + out->z2 * tz);

  return true;
}

/*
does not check if the matrix can even be inverted
*/
static inline u0 mat3x4_inverse_unsafe(
  mat3x4 *const restrict out,
  const mat3x4 *const restrict m
) {
  const f32 x0 = m->x0, y0 = m->y0, z0 = m->z0;
  const f32 x1 = m->x1, y1 = m->y1, z1 = m->z1;
  const f32 x2 = m->x2, y2 = m->y2, z2 = m->z2;

  const f32 det     = mat3x4_det(m);
  const f32 inv_det = 1.0f / det;

  out->x0 = (y1 * z2 - y2 * z1) * inv_det;
  out->y0 = -(y0 * z2 - y2 * z0) * inv_det;
  out->z0 = (y0 * z1 - y1 * z0) * inv_det;
  out->x1 = -(x1 * z2 - x2 * z1) * inv_det;
  out->y1 = (x0 * z2 - x2 * z0) * inv_det;
  out->z1 = -(x0 * z1 - x1 * z0) * inv_det;
  out->x2 = (x1 * y2 - x2 * y1) * inv_det;
  out->y2 = -(x0 * y2 - x2 * y0) * inv_det;
  out->z2 = (x0 * y1 - x1 * y0) * inv_det;

  const f32 tx = m->w0;
  const f32 ty = m->w1;
  const f32 tz = m->w2;

  out->w0 = -(out->x0 * tx + out->y0 * ty + out->z0 * tz);
  out->w1 = -(out->x1 * tx + out->y1 * ty + out->z1 * tz);
  out->w2 = -(out->x2 * tx + out->y2 * ty + out->z2 * tz);
}

#endif // MATH_MAT3X4_H_
