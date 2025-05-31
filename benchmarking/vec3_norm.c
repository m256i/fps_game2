#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define VEC2_NORM_TOLERANCE 1e-8f * 1e-8f

typedef float f32;

typedef struct {
  _Alignas(32) union {
    struct {
      f32 x, y, z;
    };
    struct {
      f32 r, g, b;
    };
    __m128 vec128;
  };
} vec3;

__attribute__((noinline)) vec3 vec3_normalize(const vec3 *const _v) {
  const f32 sum     = _v->x * _v->x + _v->y * _v->y + _v->z * _v->z;
  f32       inv_len = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set1_ps(sum)));
  inv_len           = inv_len * (1.5f - ((0.5f * sum) * inv_len * inv_len));
  return (vec3){.vec128 = _mm_mul_ps(_v->vec128, _mm_set1_ps(inv_len))};
}

__attribute__((noinline)) vec3 vec3_normalize2(const vec3 *const _v) {
  const __m128 sums = _mm_dp_ps(_v->vec128, _v->vec128, 0x7F);
  const __m128 inv  = _mm_rsqrt_ss(sums);

  // 3) One Newton-Raphson iteration
  const __m128 half      = _mm_set_ss(0.5f);
  const __m128 threeHalf = _mm_set_ss(1.5f);

  const __m128 inv2 = _mm_mul_ss(inv, inv);         // inv²
  const __m128 tmp  = _mm_mul_ss(sums, inv2);       // sum * inv²
  const __m128 nr =
    _mm_sub_ss(threeHalf, _mm_mul_ss(half, tmp));   // (1.5 - 0.5*sum*inv²)
  __m128 inv1   = _mm_mul_ss(inv, nr);              // refined scalar
  __m128 invLen = _mm_shuffle_ps(inv1, inv1, 0x00); // broadcast

  return (vec3){.vec128 = _mm_mul_ps(_v->vec128, invLen)};
}

static inline vec3 vec3_scale(const vec3 *const _v0, f32 _s) {
  return (vec3){.vec128 = _mm_mul_ps(_v0->vec128, _mm_load_ps1(&_s))};
}

static inline f32 vec3_dot(const vec3 *const _v0, const vec3 *const _v1) {
  const __m128 mul  = _mm_mul_ps(_v0->vec128, _v1->vec128);
  __m128       shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
  __m128       sums = _mm_add_ps(mul, shuf);
  shuf              = _mm_movehl_ps(shuf, sums);
  sums              = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

static inline f32 vec3_length(const vec3 *const _v0) {
  return sqrtf(vec3_dot(_v0, _v0));
}

__attribute__((noinline)) vec3 vec3_normalize_unsafe(const vec3 *const _v) {
  f32 len = vec3_length(_v);
  return vec3_scale(_v, 1.f / len);
}

__attribute__((noinline)) vec3 vec3_normalize_safe(const vec3 *const _v) {
  f32 len = vec3_length(_v);
  if (len > 1e-8f * 1e-8f) {
    f32 inv = 1.0f / len;
    return vec3_scale(_v, inv);
  }
  return (vec3){0};
}

__attribute__((noinline)) vec3 vec3_normalize_basecase(const vec3 *const _v) {
  const f32 len = 1.f / sqrtf(_v->x * _v->x + _v->y * _v->y + _v->z * _v->z);
  return (vec3){.x = _v->x * len, .y = _v->y * len, .z = _v->z * len};
}

// High-resolution timer
static double elapsed_sec(struct timespec *start, struct timespec *end) {
  return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) * 1e-9;
}

int main() {
  const size_t N    = 100ull * 1000ull * 1000ull; // 10 million
  vec3        *data = _aligned_malloc(N * sizeof(vec3), 32);
  if (!data) {
    perror("aligned_alloc");
    return 1;
  }

  SetProcessPriorityBoost(GetCurrentProcess(), FALSE);
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  // Initialize random data
  srand((unsigned)time(NULL));
  for (size_t i = 0; i < N; i++) {
    data[i].x = (float)rand() / RAND_MAX + 1e-3f;
    data[i].y = (float)rand() / RAND_MAX + 1e-3f;
    data[i].z = (float)rand() / RAND_MAX + 1e-3f;
  }

  struct timespec t0, t1;
  double          t_normalize, t_safe, t_unsafe, t_base, t_normalize2;
  volatile float  sink = 0.0f;

  // Benchmark vec3_normalize
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_normalize(&data[i]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_normalize = elapsed_sec(&t0, &t1);

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_normalize2(&data[i]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_normalize2 = elapsed_sec(&t0, &t1);

  // Benchmark vec3_normalize_safe
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_normalize_safe(&data[i]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_safe = elapsed_sec(&t0, &t1);

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_normalize_unsafe(&data[i]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_unsafe = elapsed_sec(&t0, &t1);

  // Benchmark vec3_normalize_basecase
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_normalize_basecase(&data[i]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_base = elapsed_sec(&t0, &t1);

  // Report
  printf("Results for %zu iterations (sink=%%f to avoid optimization)\n", N);
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_normalize:",
    t_normalize,
    N / (t_normalize * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_normalize2:",
    t_normalize2,
    N / (t_normalize2 * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_normalize_safe:",
    t_safe,
    N / (t_safe * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_normalize_basecase:",
    t_base,
    N / (t_base * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_normalize_unsafe:",
    t_unsafe,
    N / (t_unsafe * 1e6)
  );

  printf("sink = %f\n", sink);

  free(data);
  return 0;
}
