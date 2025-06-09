#ifndef MATH_COLLIDERS_OBB_H_
#define MATH_COLLIDERS_OBB_H_

#include <common.h>
#include <math/vec3.h>
#include <math/vec2.h>

typedef struct {
  vec3 center;
  vec3 axes[3];
  vec3 min, max;
} obb3d;

typedef struct {
  vec2 center;
  vec2 axes[2];
  vec2 min, max;
} obb2d;

#endif // MATH_COLLIDERS_OBB_H_
