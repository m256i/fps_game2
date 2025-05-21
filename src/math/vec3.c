#include <math.h>
#include <math/vec3.h>

/* define the extern variables here */
VEC3_ADDPROC       vec3_add               = NULL;
VEC3_SUBPROC       vec3_sub               = NULL;
VEC3_SCALEPROC     vec3_scale             = NULL;
VEC3_DOTPROC       vec3_dot               = NULL;
VEC3_CROSSROC      vec3_cross             = NULL;
VEC3_LENGTHROC     vec3_length            = NULL;
VEC3_DISTROC       vec3_dist              = NULL;
VEC3_NORMROC       vec3_normalize         = NULL;
VEC3_NORMROC       vec3_normalize_safe    = NULL;
VEC3_APPROXEQPROC  vec3_approx_eq         = NULL;
VEC3_EQPROC        vec3_equal             = NULL;
VEC3_ADD_IP_PROC   vec3_add_inplace       = NULL;
VEC3_SUB_IP_PROC   vec3_sub_inplace       = NULL;
VEC3_SCALE_IP_PROC vec3_scale_inplace     = NULL;
VEC3_CROSS_IP_PROC vec3_cross_inplace     = NULL;
VEC3_NORM_IP_PROC  vec3_normalize_inplace = NULL;

u0 hint_from_sse_to_avx(u0) { _mm256_zeroupper(); }
u0 hint_from_avx_to_sse(u0) { _mm256_zeroupper(); }

/* forward decls */
u0 vec3_add_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_add_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_sub_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_sub_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_scale_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  f32               _s
);
u0 vec3_scale_inplace_impl_SSE(vec3 *const _res, const vec3 *const _v0, f32 _s);
u0 vec3_cross_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_cross_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
);
u0 vec3_normalize_inplace_impl_noSIMD(vec3 *const _v0);
u0 vec3_normalize_inplace_impl_SSE(vec3 *const _v0);

vec3 vec3_add_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.r = _v0->r + _v1->r, .g = _v0->g + _v1->g, .b = _v0->b + _v1->b
  };
}

vec3 vec3_add_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.vec128 = _mm_add_ps(_v0->vec128, _v1->vec128)};
}

vec3 vec3_sub_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.r = _v0->r - _v1->r, .g = _v0->g - _v1->g, .b = _v0->b - _v1->b
  };
}

vec3 vec3_sub_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.vec128 = _mm_sub_ps(_v0->vec128, _v1->vec128)};
}

vec3 vec3_scale_impl_noSIMD(const vec3 *const _v0, f32 _s) {
  return (vec3){.x = _v0->x * _s, .y = _v0->y * _s, .z = _v0->z * _s};
}

vec3 vec3_scale_impl_SSE(const vec3 *const _v0, f32 _s) {
  return (vec3){.vec128 = _mm_mul_ps(_v0->vec128, _mm_load_ps1(&_s))};
}

f32 vec3_dot_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  return _v0->x * _v1->x + _v0->y * _v1->y + _v0->z * _v1->z;
}

f32 vec3_dot_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  const __m128 mul  = _mm_mul_ps(_v0->vec128, _v1->vec128);
  __m128       shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
  __m128       sums = _mm_add_ps(mul, shuf);
  shuf              = _mm_movehl_ps(shuf, sums);
  sums              = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

vec3 vec3_cross_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.x = _v0->y * _v1->z - _v0->z * _v1->y,
                .y = _v0->z * _v1->x - _v0->x * _v1->z,
                .z = _v0->x * _v1->y - _v0->y * _v1->x};
}

vec3 vec3_cross_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  __m128 tmp0 =
    _mm_shuffle_ps(_v0->vec128, _v0->vec128, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 =
    _mm_shuffle_ps(_v1->vec128, _v1->vec128, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2 = _mm_mul_ps(tmp0, _v1->vec128);
  __m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
  __m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  return (vec3){.vec128 = _mm_sub_ps(tmp3, tmp4)};
}

f32 vec3_length_impl_noSIMD(const vec3 *const _v0) {
  return sqrtf(vec3_dot_impl_noSIMD(_v0, _v0));
}

f32 vec3_length_impl_SSE(const vec3 *const _v0) {
  return sqrtf(vec3_dot_impl_SSE(_v0, _v0));
}

f32 vec3_dist_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  const vec3 diff = vec3_sub_impl_noSIMD(_v1, _v0);
  return vec3_length_impl_noSIMD(&diff);
}

f32 vec3_dist_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  const vec3 diff = vec3_sub_impl_SSE(_v1, _v0);
  return vec3_length_impl_SSE(&diff);
}

/* returns (0,0,0) if length is zero */
vec3 vec3_normalize_impl_noSIMD(const vec3 *const _v0) {
  f32 len = vec3_length_impl_noSIMD(_v0);
  if (len > VEC3_NORM_TOLERANCE) {
    f32 inv = 1.0f / len;
    return vec3_scale_impl_noSIMD(_v0, inv);
  }
  return (vec3){0};
}

vec3 vec3_normalize_safe_impl_noSIMD(const vec3 *const _v0) {
  f32 len = vec3_length_impl_noSIMD(_v0);
  if (len > VEC3_NORM_TOLERANCE) {
    f32 inv = 1.0f / len;
    return vec3_scale_impl_noSIMD(_v0, inv);
  }
  return (vec3){0};
}

vec3 vec3_normalize_impl_SSE(const vec3 *const _v) {
  __m128 dot     = _mm_mul_ps(_v->vec128, _v->vec128);
  __m128 shuf    = _mm_shuffle_ps(dot, dot, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 sums    = _mm_add_ps(dot, shuf);
  shuf           = _mm_movehl_ps(shuf, sums);
  sums           = _mm_add_ss(sums, shuf);
  __m128 inv_len = _mm_rsqrt_ss(sums);
  inv_len        = _mm_shuffle_ps(inv_len, inv_len, 0x00);
  return (vec3){.vec128 = _mm_mul_ps(_v->vec128, inv_len)};
}

vec3 vec3_normalize_safe_impl_SSE(const vec3 *const _v) {
  f32 len = vec3_length_impl_SSE(_v);
  if (len > VEC3_NORM_TOLERANCE) {
    f32 inv = 1.0f / len;
    return vec3_scale_impl_SSE(_v, inv);
  }
  return (vec3){0};
}

bool vec3_approx_eq_impl_noSIMD(
  const vec3 *const _v0,
  const vec3 *const _v1,
  f32               _tol
) {
  return vec3_dist_impl_noSIMD(_v0, _v1) <= _tol;
}

bool vec3_approx_eq_impl_SSE(
  const vec3 *const _v0,
  const vec3 *const _v1,
  f32               _tol
) {
  return vec3_dist_impl_SSE(_v0, _v1) <= _tol;
}

bool vec3_equal_impl_noSIMD(const vec3 *const _v0, const vec3 *const _v1) {
  return vec3_dist_impl_noSIMD(_v0, _v1) <= __FLT_EPSILON__;
}

bool vec3_equal_impl_SSE(const vec3 *const _v0, const vec3 *const _v1) {
  return vec3_dist_impl_SSE(_v0, _v1) <= __FLT_EPSILON__;
}

u0 vec3_add_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  *_res =
    (vec3){.x = _v0->x + _v1->x, .y = _v0->y + _v1->y, .z = _v0->z + _v1->z};
}

u0 vec3_add_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  _res->vec128 = _mm_add_ps(_v0->vec128, _v1->vec128);
}

u0 vec3_sub_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  *_res =
    (vec3){.x = _v0->x - _v1->x, .y = _v0->y - _v1->y, .z = _v0->z - _v1->z};
}

u0 vec3_sub_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  _res->vec128 = _mm_sub_ps(_v0->vec128, _v1->vec128);
}

u0 vec3_scale_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  f32               _s
) {
  *_res = (vec3){.x = _v0->x * _s, .y = _v0->y * _s, .z = _v0->z * _s};
}

u0 vec3_scale_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  f32               _s
) {
  _res->vec128 = _mm_mul_ps(_v0->vec128, _mm_load1_ps(&_s));
}

u0 vec3_cross_inplace_impl_noSIMD(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  *_res = (vec3){.x = _v0->y * _v1->z - _v0->z * _v1->y,
                 .y = _v0->z * _v1->x - _v0->x * _v1->z,
                 .z = _v0->x * _v1->y - _v0->y * _v1->x};
}
u0 vec3_cross_inplace_impl_SSE(
  vec3 *const       _res,
  const vec3 *const _v0,
  const vec3 *const _v1
) {
  __m128 tmp0 =
    _mm_shuffle_ps(_v0->vec128, _v0->vec128, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 =
    _mm_shuffle_ps(_v1->vec128, _v1->vec128, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2  = _mm_mul_ps(tmp0, _v1->vec128);
  __m128 tmp3  = _mm_mul_ps(tmp0, tmp1);
  __m128 tmp4  = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  _res->vec128 = _mm_sub_ps(tmp3, tmp4);
}

u0 vec3_normalize_inplace_impl_noSIMD(vec3 *const _v0) {
  f32 len = vec3_length_impl_noSIMD(_v0);
  if (len > VEC3_NORM_TOLERANCE) {
    f32 inv = 1.0f / len;
    *_v0    = vec3_scale_impl_noSIMD(_v0, inv);
  }
  *_v0 = (vec3){0};
}

u0 vec3_normalize_inplace_impl_SSE(vec3 *const _v0) {
  __m128 dot     = _mm_mul_ps(_v0->vec128, _v0->vec128);
  __m128 shuf    = _mm_shuffle_ps(dot, dot, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 sums    = _mm_add_ps(dot, shuf);
  shuf           = _mm_movehl_ps(shuf, sums);
  sums           = _mm_add_ss(sums, shuf);
  __m128 inv_len = _mm_rsqrt_ss(sums);
  inv_len        = _mm_shuffle_ps(inv_len, inv_len, 0x00);
  _v0->vec128    = _mm_mul_ps(_v0->vec128, inv_len);
}

u0 init_mathlib(u0) {
  i32 cpu_caps[4] = {0};
  __cpuid(cpu_caps, 1);
  const bool has_sse = !!(cpu_caps[2] & (1 << 20));
  __cpuid(cpu_caps, 7);
  const bool has_avx2 = !!(cpu_caps[1] & (1 << 5));

  if (!has_sse) {
    GAME_WARNF("engine::mathlib: will not use SSE4.2 instructions");
  }
  if (!has_avx2) {
    GAME_WARNF("engine::mathlib: will not use AVX2 instructions");
  }

  vec3_add               = vec3_add_impl_noSIMD;
  vec3_sub               = vec3_sub_impl_noSIMD;
  vec3_scale             = vec3_scale_impl_noSIMD;
  vec3_dot               = vec3_dot_impl_noSIMD;
  vec3_cross             = vec3_cross_impl_noSIMD;
  vec3_normalize         = vec3_normalize_impl_noSIMD;
  vec3_normalize_safe    = vec3_normalize_safe_impl_noSIMD;
  vec3_length            = vec3_length_impl_noSIMD;
  vec3_dist              = vec3_dist_impl_noSIMD;
  vec3_approx_eq         = vec3_approx_eq_impl_noSIMD;
  vec3_equal             = vec3_equal_impl_noSIMD;
  vec3_add_inplace       = vec3_add_inplace_impl_noSIMD;
  vec3_sub_inplace       = vec3_sub_inplace_impl_noSIMD;
  vec3_scale_inplace     = vec3_scale_inplace_impl_noSIMD;
  vec3_cross_inplace     = vec3_cross_inplace_impl_noSIMD;
  vec3_normalize_inplace = vec3_normalize_inplace_impl_noSIMD;

  if (has_sse) {
    vec3_add               = vec3_add_impl_SSE;
    vec3_sub               = vec3_sub_impl_SSE;
    vec3_scale             = vec3_scale_impl_SSE;
    vec3_dot               = vec3_dot_impl_SSE;
    vec3_cross             = vec3_cross_impl_SSE;
    vec3_normalize         = vec3_normalize_impl_SSE;
    vec3_normalize_safe    = vec3_normalize_safe_impl_SSE;
    vec3_length            = vec3_length_impl_SSE;
    vec3_dist              = vec3_dist_impl_SSE;
    vec3_approx_eq         = vec3_approx_eq_impl_SSE;
    vec3_equal             = vec3_equal_impl_SSE;
    vec3_add_inplace       = vec3_add_inplace_impl_SSE;
    vec3_sub_inplace       = vec3_sub_inplace_impl_SSE;
    vec3_scale_inplace     = vec3_scale_inplace_impl_SSE;
    vec3_cross_inplace     = vec3_cross_inplace_impl_SSE;
    vec3_normalize_inplace = vec3_normalize_inplace_impl_SSE;
  }

  if (has_avx2) {
    // ...
  }

  printf("%p address of normalize\n", (void *)vec3_normalize);
}

/* used only for unit testing naming uniformity */
inline bool f32_equal_impl_noSIMD(f32 *a, f32 *b) {
  return fabsf(*b - *a) <= 1e-6f;
}
inline bool bool_equal_impl_noSIMD(bool *a, bool *b) { return *a == *b; }
/*
we use the noSIMD versions as a golden reference for testing.
*/
#define DO_TEST_SSE_VEC3_VEC3(_result_type, _op, ...)                          \
  do {                                                                         \
    _result_type res  = _op##_impl_noSIMD(__VA_ARGS__);                        \
    _result_type res1 = _op##_impl_SSE(__VA_ARGS__);                           \
    if (!_result_type##_approx_eq_impl_noSIMD(&res, &res1, 1e-6f)) {           \
      printf(                                                                  \
        "mathlib::unit warn: (%f, %f,%f) != (%f, %f,%f) operation: '%s' "      \
        "values: '%s' ",                                                       \
        res.x,                                                                 \
        res.y,                                                                 \
        res.z,                                                                 \
        res1.x,                                                                \
        res1.y,                                                                \
        res1.z,                                                                \
        #_op,                                                                  \
        #__VA_ARGS__                                                           \
      );                                                                       \
      printf("delta: (%f)\n", _result_type##_dist_impl_noSIMD(&res, &res1));   \
    }                                                                          \
  } while (0)

#define DO_TEST_SSE_F32_VEC3(_result_type, _op, ...)                           \
  do {                                                                         \
    _result_type res  = _op##_impl_noSIMD(__VA_ARGS__);                        \
    _result_type res1 = _op##_impl_SSE(__VA_ARGS__);                           \
    if (!_result_type##_equal_impl_noSIMD(&res, &res1)) {                      \
      printf(                                                                  \
        "mathlib::unit warn: (%f) != (%f) operation: '%s' values: '%s' "       \
        "delta: %f\n",                                                         \
        res,                                                                   \
        res1,                                                                  \
        #_op,                                                                  \
        #__VA_ARGS__,                                                          \
        fabsf(res1 - res)                                                      \
      );                                                                       \
    }                                                                          \
  } while (0)

#define DO_TEST_SSE_BOOL_VEC3(_result_type, _op, ...)                          \
  do {                                                                         \
    _result_type res  = _op##_impl_noSIMD(__VA_ARGS__);                        \
    _result_type res1 = _op##_impl_SSE(__VA_ARGS__);                           \
    if (!_result_type##_equal_impl_noSIMD(&res, &res1)) {                      \
      printf(                                                                  \
        "mathlib::unit warn: (%u) != (%u) operation: '%s' values: '%s'\n",     \
        res,                                                                   \
        res1,                                                                  \
        #_op,                                                                  \
        #__VA_ARGS__                                                           \
      );                                                                       \
    }                                                                          \
  } while (0)

static f32 generate_random_f32(u0) {
  f32 lol = (f32)rand();
  if (lol <= __FLT_EPSILON__) {
    lol += __FLT_EPSILON__;
  }
  return ((f32)rand() / lol);
}

static vec3 generate_random_vec3(u0) {
  return (vec3){.x = generate_random_f32(),
                .y = generate_random_f32(),
                .z = generate_random_f32()};
}

u0 unit_test_mathlib(u0) {

  // SSE: disable flush-to-zero/denorm
  _mm_setcsr((_mm_getcsr() & ~(_MM_FLUSH_ZERO_MASK | _MM_DENORMALS_ZERO_MASK)));

  i32 cpu_caps[4] = {0};
  __cpuid(cpu_caps, 1);
  const bool has_sse = !!(cpu_caps[2] & (1 << 20));
  __cpuid(cpu_caps, 7);
  const bool has_avx2 = !!(cpu_caps[1] & (1 << 5));

  if (has_sse) {
    goto test_sse;
  } else goto test_end;

test_sse:
  // clang-format off
  puts("testing vec3_add..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3(), b = generate_random_vec3();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_add, &a, &b);
    }
  }
  
  puts("testing vec3_sub..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3(), b = generate_random_vec3();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_sub, &a, &b);
    }
  }

  puts("testing vec3_scale..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3();
      f32 f = generate_random_f32();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_scale, &a, f);
    }
  }

  puts("testing vec3_dot..."); { 
    for (usize i = 0; i < 1000000; i++)  {
      vec3 a = generate_random_vec3(), b = generate_random_vec3();
      DO_TEST_SSE_F32_VEC3(f32, vec3_dot, &a, &b);
    }
  }

  puts("testing vec3_cross..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3(), b = generate_random_vec3();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_cross, &a, &b);
    }
  }

  puts("testing vec3_length..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3();
      DO_TEST_SSE_F32_VEC3(f32, vec3_length, &a);
    }
  }
  
  puts("testing vec3_dist..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3(), b = generate_random_vec3();
      DO_TEST_SSE_F32_VEC3(f32, vec3_dist, &a, &b);
    }
  }

  puts("testing vec3_normalize..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_normalize, &a);
    }
  }

  puts("testing vec3_normalize_safe..."); { 
    for (usize i = 0; i < 100000; i++)  {
      vec3 a = generate_random_vec3();
      DO_TEST_SSE_VEC3_VEC3(vec3, vec3_normalize_safe, &a);
    }
  }

  if (has_avx2) {
    goto test_avx;
  } else goto test_end;

test_avx:
test_end:
  (u0)0;
  // clang-format on
}
