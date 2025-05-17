#ifndef MATH_VECTOR_H_
#define MATH_VECTOR_H_

#include <common.h>
#include <intrin.h>

#define VEC3_NORM_TOLERANCE 1e-8f * 1e-8f

/* init the math library */
u0 init_mathlib(u0);

/* call when you go from vec3 instructions to matrix instructions */
u0 hint_from_sse_to_avx(u0);
u0 hint_from_avx_to_sse(u0);

u0 unit_test_mathlib(u0);

typedef struct {
  _Alignas(32) union {
    struct {
      f32 x, y, z;
    };
    struct {
      f32 r, g, b;
    };
    struct {
      f32 width, height, depth;
    };
    __m128 vec128;
  };
} vec3;

/*
ONLY USE IN NON-PERFORMANCE-CRITICAL SCENARIOS FOR CLEANER SYNTAX
*/
typedef vec3          (*VEC3_ADDPROC)(const vec3 *const, const vec3 *const);
static VEC3_ADDPROC   vec3_add;
typedef vec3          (*VEC3_SUBPROC)(const vec3 *const, const vec3 *const);
static VEC3_SUBPROC   vec3_sub;
typedef vec3          (*VEC3_SCALEPROC)(const vec3 *const, const f32);
static VEC3_SCALEPROC vec3_scale;
typedef f32           (*VEC3_DOTPROC)(const vec3 *const, const vec3 *const);
static VEC3_DOTPROC   vec3_dot;
typedef vec3          (*VEC3_CROSSROC)(const vec3 *const, const vec3 *const);
static VEC3_CROSSROC  vec3_cross;
typedef f32           (*VEC3_LENGTHROC)(const vec3 *const);
static VEC3_LENGTHROC vec3_length;
typedef f32           (*VEC3_DISTROC)(const vec3 *const, const vec3 *const);
static VEC3_DISTROC   vec3_dist;
typedef vec3          (*VEC3_NORMROC)(const vec3 *const);
static VEC3_NORMROC   vec3_normalize;
typedef vec3          (*VEC3_NORMROC)(const vec3 *const);
static VEC3_NORMROC   vec3_normalize_safe;
typedef bool (*VEC3_APPROXEQPROC)(const vec3 *const, const vec3 *const, f32);
static VEC3_APPROXEQPROC vec3_approx_eq;
typedef bool             (*VEC3_EQPROC)(const vec3 *const, const vec3 *const);
static VEC3_EQPROC       vec3_equal;
/*
INPLACE OPERATIONS = BETTER CODEGEN = BETTER PERORMANCE
*/
typedef u0               (*VEC3_ADD_IP_PROC)(
  vec3 *const,
  const vec3 *const,
  const vec3 *const
);
static VEC3_ADD_IP_PROC vec3_add_inplace;
typedef u0              (*VEC3_SUB_IP_PROC)(
  vec3 *const,
  const vec3 *const,
  const vec3 *const
);
static VEC3_SUB_IP_PROC vec3_sub_inplace;
typedef u0 (*VEC3_SCALE_IP_PROC)(vec3 *const, const vec3 *const, f32);
static VEC3_SCALE_IP_PROC vec3_scale_inplace;
typedef u0                (*VEC3_CROSS_IP_PROC)(
  vec3 *const,
  const vec3 *const,
  const vec3 *const
);
static VEC3_CROSS_IP_PROC vec3_cross_inplace;
typedef u0                (*VEC3_NORM_IP_PROC)(vec3 *const);
static VEC3_NORM_IP_PROC  vec3_normalize_inplace;
#endif // MATH_VECTOR_H_