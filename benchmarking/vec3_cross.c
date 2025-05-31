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

static inline vec3 vec3_cross(const vec3 *const _v0, const vec3 *const _v1) {
  __m128 tmp0 =
    _mm_shuffle_ps(_v0->vec128, _v0->vec128, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 =
    _mm_shuffle_ps(_v1->vec128, _v1->vec128, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2 = _mm_mul_ps(tmp0, _v1->vec128);
  __m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
  __m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  return (vec3){.vec128 = _mm_sub_ps(tmp3, tmp4)};
}

static inline vec3
vec3_cross_basecase(const vec3 *const _v0, const vec3 *const _v1) {
  return (vec3){.x = _v0->y * _v1->z - _v0->z * _v1->y,
                .y = _v0->z * _v1->x - _v0->x * _v1->z,
                .z = _v0->x * _v1->y - _v0->y * _v1->x};
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

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_cross((vec3 *)&data[i], (vec3 *)&data[N - i - 1]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_dot = elapsed_sec(&t0, &t1);

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (size_t i = 0; i < N; i++) {
    vec3 r  = vec3_cross_basecase((vec3 *)&data[i], (vec3 *)&data[N - i - 1]);
    sink   += r.x + r.y + r.z;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t_dotbase = elapsed_sec(&t0, &t1);

  // Report
  printf("Results for %zu iterations (sink=%%f to avoid optimization)\n", N);
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_cross:",
    t_dot,
    N / (t_dot * 1e6)
  );
  printf(
    "%-25s %10.6f sec, %10.6f Mops/sec\n",
    "vec3_cross_basecase:",
    t_dotbase,
    N / (t_dotbase * 1e6)
  );
  printf("sink = %f\n", sink);

  free(data);
  return 0;
}
