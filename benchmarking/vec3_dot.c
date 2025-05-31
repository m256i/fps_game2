#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

f32 vec3_dot(const vec3 *const _v0, const vec3 *const _v1) {
  const __m128 mul  = _mm_mul_ps(_v0->vec128, _v1->vec128);
  __m128       shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
  __m128       sums = _mm_add_ps(mul, shuf);
  shuf              = _mm_movehl_ps(shuf, sums);
  sums              = _mm_add_ss(sums, shuf);
  return _mm_cvtss_f32(sums);
}

f32 vec3_dot_basecase(const vec3 *const _v0, const vec3 *const _v1) {
  return _v0->x * _v1->x + _v0->y * _v1->y + _v0->z * _v1->z;
}

// High-resolution timer
static double elapsed_sec(struct timespec *start, struct timespec *end) {
  return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) * 1e-9;
}

int main() {
  const size_t N    = 1000000; // 10 million
  vec3        *data = _aligned_malloc(N * sizeof(vec3), 32);
  if (!data) {
    perror("aligned_alloc");
    return 1;
  }

  // Initialize random data
  srand((unsigned)time(NULL));
  for (size_t i = 0; i < N; i++) {
    data[i].x = (float)rand() / RAND_MAX + 1e-3f;
    data[i].y = (float)rand() / RAND_MAX + 1e-3f;
    data[i].z = (float)rand() / RAND_MAX + 1e-3f;
  }

  struct timespec t0, t1;
  double          t_dot, t_dotbase;
  volatile float  sink = 0.0f;

  // Benchmark vec3_normalize

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    f32 r  = vec3_dot((vec3 *)&data[i], (vec3 *)&data[N - i - 1]);
    sink  += r;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_dot = elapsed_sec(&t0, &t1);

  // Benchmark vec3_normalize_safe
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    f32 r  = vec3_dot_basecase((vec3 *)&data[i], (vec3 *)&data[N - i - 1]);
    sink  += r;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_dotbase = elapsed_sec(&t0, &t1);

  // Report
  printf("Results for %zu iterations (sink=%%f to avoid optimization)\n", N);
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_dot:",
    t_dot,
    N / (t_dot * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_dot_basecase:",
    t_dotbase,
    N / (t_dotbase * 1e6)
  );
  printf("sink = %f\n", sink);

  free(data);
  return 0;
}
