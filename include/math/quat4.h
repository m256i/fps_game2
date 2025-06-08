#ifndef MATH_QUAT_H_
#define MATH_QUAT_H_

#include <common.h>
#include <math/vec3.h>

typedef struct {
  union {
    _Alignas(16) struct {
      f32 x, y, z, w;
    };
    __m128 vec128;
    _Alignas(16) f32 data[4];
  };
} quat4;

static inline quat4
quat4_from_axis_angle(const vec3 *const _axis, f32 _angle_rad) {
  const vec3 axis = vec3_normalize(_axis);

  const f32 half = 0.5f * _angle_rad;
  const f32 s    = sinf(half);
  const f32 c    = cosf(half);

  quat4 q;
  q.x = axis.x * s;
  q.y = axis.y * s;
  q.z = axis.z * s;
  q.w = c;
  return q;
}

static inline quat4 quat4_normalize(const quat4 *const _q0) {
  const f32 len = sqrtf(
    _q0->x * _q0->x + _q0->y * _q0->y + _q0->z * _q0->z + _q0->w * _q0->w
  );
  quat4 out = *_q0;
  if (len > 0.0f) {
    f32 inv = 1.0f / len;
    out.x   = _q0->x * inv;
    out.y   = _q0->y * inv;
    out.z   = _q0->z * inv;
    out.w   = _q0->w * inv;
  }
  return out;
}

static inline quat4 quat4_mul(const quat4 *const q0, const quat4 *const q1) {
  vec3 v0 = make_vec3(q0->x, q0->y, q0->z);
  vec3 v1 = make_vec3(q1->x, q1->y, q1->z);

  const f32 w = q0->w * q1->w - (v0.x * v1.x + v0.y * v1.y + v0.z * v1.z);
  vec3      v;

  v.x = q0->w * v1.x;
  v.y = q0->w * v1.y;
  v.z = q0->w * v1.z;

  v.x += q1->w * v0.x;
  v.y += q1->w * v0.y;
  v.z += q1->w * v0.z;

  v.x += v0.y * v1.z - v0.z * v1.y;
  v.y += v0.z * v1.x - v0.x * v1.z;
  v.z += v0.x * v1.y - v0.y * v1.x;

  quat4 out;
  out.x = v.x;
  out.y = v.y;
  out.z = v.z;
  out.w = w;
  return out;
}

static inline vec3
quat4_rotate_vec3(const quat4 *const q, const vec3 *const v) {
  const quat4 vq     = {.x = v->x, .y = v->y, .z = v->z, .w = 0.0f};
  const quat4 q_conj = {.x = -q->x, .y = -q->y, .z = -q->z, .w = q->w};
  const quat4 tmp    = quat4_mul(q, &vq);
  const quat4 res    = quat4_mul(&tmp, &q_conj);
  return make_vec3(res.x, res.y, res.z);
}

#endif // MATH_QUAT_H_
