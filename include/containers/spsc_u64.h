#ifndef CONTAINERS_SPSC_U64_H_
#define CONTAINERS_SPSC_U64_H_

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#include <common.h>

/* size of the container NEEDS to be power of 2!!!! */
#define CREATE_SPSC_U64_IMPL(N)                                                \
  typedef struct spsc_u64_##N##_index_t {                                      \
    _Alignas(64) atomic_size_t head;                                           \
    u8 pad1[64 - sizeof(atomic_size_t)];                                       \
    _Alignas(64) atomic_size_t tail;                                           \
    u8 pad2[64 - sizeof(atomic_size_t)];                                       \
  } spsc_u64_##N##_index_t;                                                    \
  typedef struct spsc_u64_##N##_ring_t {                                       \
    spsc_u64_##N##_index_t idx;                                                \
    _Alignas(64) u64 buffer[N];                                                \
  } spsc_u64_##N##_ring_t;                                                     \
  static inline void spsc_u64_##N##_init(spsc_u64_##N##_ring_t *q) {           \
    atomic_init(&q->idx.head, 0);                                              \
    atomic_init(&q->idx.tail, 0);                                              \
  }                                                                            \
  static inline bool spsc_u64_##N##_enqueue(                                   \
    spsc_u64_##N##_ring_t *q,                                                  \
    uint64_t               v                                                   \
  ) {                                                                          \
    usize head = atomic_load_explicit(&q->idx.head, memory_order_relaxed);     \
    usize tail = atomic_load_explicit(&q->idx.tail, memory_order_relaxed);     \
    if (((tail + 1) & (N - 1)) == head) return false;                          \
    _mm_stream_si64((long long int *)&q->buffer[tail], v);                     \
    atomic_store_explicit(                                                     \
      &q->idx.tail,                                                            \
      (tail + 1) & (N - 1),                                                    \
      memory_order_release                                                     \
    );                                                                         \
    return true;                                                               \
  }                                                                            \
  static inline bool spsc_u64_##N##_dequeue(                                   \
    spsc_u64_##N##_ring_t *q,                                                  \
    uint64_t              *out                                                 \
  ) {                                                                          \
    size_t tail = atomic_load_explicit(&q->idx.tail, memory_order_acquire);    \
    size_t head = atomic_load_explicit(&q->idx.head, memory_order_relaxed);    \
    if (head == tail) return false;                                            \
    *out = q->buffer[head];                                                    \
    atomic_store_explicit(                                                     \
      &q->idx.head,                                                            \
      (head + 1) & (N - 1),                                                    \
      memory_order_release                                                     \
    );                                                                         \
    return true;                                                               \
  }                                                                            \
  static inline bool spsc_u64_##N##_peek(                                      \
    spsc_u64_##N##_ring_t *q,                                                  \
    uint64_t              *out                                                 \
  ) {                                                                          \
    size_t tail = atomic_load_explicit(&q->idx.tail, memory_order_acquire);    \
    size_t head = atomic_load_explicit(&q->idx.head, memory_order_relaxed);    \
    if (head == tail) return false;                                            \
    *out = q->buffer[head];                                                    \
    return true;                                                               \
  }                                                                            \
  static inline void spsc_u64_##N##_clear(spsc_u64_##N##_ring_t *q) {          \
    atomic_store_explicit(&q->idx.head, 0, memory_order_release);              \
    atomic_store_explicit(&q->idx.tail, 0, memory_order_release);              \
  }                                                                            \
  static inline size_t spsc_u64_##N##_size(const spsc_u64_##N##_ring_t *q) {   \
    size_t head = atomic_load_explicit(&q->idx.head, memory_order_acquire);    \
    size_t tail = atomic_load_explicit(&q->idx.tail, memory_order_acquire);    \
    return (tail + N - head) & (N - 1);                                        \
  }

#define CREATE_SPSC_U64(N) CREATE_SPSC_U64_IMPL(N)

/* predefine the common ones */
CREATE_SPSC_U64(8)
CREATE_SPSC_U64(16)
CREATE_SPSC_U64(32)
CREATE_SPSC_U64(64)

#endif // CONTAINERS_SPSC_U64_H_
