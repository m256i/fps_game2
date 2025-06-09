#ifndef MATH_COLLIDERS_AABB_H_
#define MATH_COLLIDERS_AABB_H_

#include <common.h>
#include <math/vec3.h>
#include <math/vec2.h>

typedef struct {
  vec3 min, max;
} aabb3d;

typedef struct {
  vec3 min, max;
} aabb2d;

#endif // MATH_COLLIDERS_AABB_H_
