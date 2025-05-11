#ifndef P_VECTOR_H
#define P_VECTOR_H

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Optional callbacks for non-POD types
typedef void (*vec_destructor_fn)(void *);
typedef void *(*vec_copy_fn)(const void *);

#define CREATE_VECTOR_TYPE(TYPE)                                                                                       \
                                                                                                                       \
  typedef struct {                                                                                                     \
    TYPE *data;             /* element buffer */                                                                       \
    size_t size;            /* number of elements */                                                                   \
    size_t capacity;        /* allocated slots */                                                                      \
    vec_destructor_fn dtor; /* element destructor */                                                                   \
    vec_copy_fn copier;     /* element copy    */                                                                      \
  } vector_##TYPE;                                                                                                     \
                                                                                                                       \
  /**                                                                                                                  \
   * Create a new, empty vector.                                                                                       \
   */                                                                                                                  \
  static inline vector_##TYPE vector_##TYPE##_create(vec_copy_fn cp, vec_destructor_fn dt) {                           \
    return (vector_##TYPE){.data = NULL, .size = 0, .capacity = 0, .dtor = dt, .copier = cp};                          \
  }                                                                                                                    \
  static inline vector_##TYPE vector_##TYPE##_create_() {                                                              \
    return (vector_##TYPE){.data = NULL, .size = 0, .capacity = 0, .dtor = NULL, .copier = NULL};                      \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Free all resources held by the vector.                                                                            \
   */                                                                                                                  \
  static inline void vector_##TYPE##_free(vector_##TYPE *v) {                                                          \
    if (v->dtor) {                                                                                                     \
      for (size_t i = 0; i < v->size; ++i)                                                                             \
        v->dtor(&v->data[i]);                                                                                          \
    }                                                                                                                  \
    free(v->data);                                                                                                     \
    v->data = NULL;                                                                                                    \
    v->size = v->capacity = 0;                                                                                         \
  }                                                                                                                    \
                                                                                                                       \
  /* Iterate begin/end */                                                                                              \
  static inline TYPE *vector_##TYPE##_begin(const vector_##TYPE *v) { return v->data; }                                \
  static inline TYPE *vector_##TYPE##_end(const vector_##TYPE *v) { return v->data + v->size; }                        \
                                                                                                                       \
  /* Capacity queries */                                                                                               \
  static inline size_t vector_##TYPE##_size(const vector_##TYPE *v) { return v->size; }                                \
  static inline size_t vector_##TYPE##_capacity(const vector_##TYPE *v) { return v->capacity; }                        \
  static inline int vector_##TYPE##_empty(const vector_##TYPE *v) { return v->size == 0; }                             \
                                                                                                                       \
  /**                                                                                                                  \
   * Try to reserve at least new_cap slots.                                                                            \
   * Returns 0 or VEC_ERR_OOM.                                                                                         \
   */                                                                                                                  \
  static inline int vector_##TYPE##_try_reserve(vector_##TYPE *v, size_t new_cap) {                                    \
    if (new_cap <= v->capacity) return 0;                                                                              \
    TYPE *nptr = realloc(v->data, new_cap * sizeof(TYPE));                                                             \
    if (!nptr) return -1;                                                                                              \
    v->data = nptr;                                                                                                    \
    v->capacity = new_cap;                                                                                             \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Reserve capacity, abort on failure.                                                                               \
   */                                                                                                                  \
  static inline void vector_##TYPE##_reserve(vector_##TYPE *v, size_t new_cap) {                                       \
    if (vector_##TYPE##_try_reserve(v, new_cap) != 0) assert(!"OOM");                                                  \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Shrink capacity to size if capacity >= 4*size, to avoid ping-pong.                                                \
   */                                                                                                                  \
  static inline void vector_##TYPE##_shrink_to_fit(vector_##TYPE *v) {                                                 \
    if (v->capacity >= 4 * v->size) {                                                                                  \
      TYPE *nptr = realloc(v->data, v->size * sizeof(TYPE));                                                           \
      if (nptr) {                                                                                                      \
        v->data = nptr;                                                                                                \
        v->capacity = v->size;                                                                                         \
      }                                                                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Push back element with copy into vector.                                                                          \
   */                                                                                                                  \
  static inline int vector_##TYPE##_push_back(vector_##TYPE *v, TYPE const *x) {                                       \
    if (v->size == v->capacity) {                                                                                      \
      size_t nc = v->capacity ? v->capacity * 2 : 4;                                                                   \
      int e = vector_##TYPE##_try_reserve(v, nc);                                                                      \
      if (e) return e;                                                                                                 \
    }                                                                                                                  \
    if (v->copier) {                                                                                                   \
      TYPE *copy = (TYPE *)v->copier(x);                                                                               \
      v->data[v->size++] = *copy;                                                                                      \
      free(copy);                                                                                                      \
    } else {                                                                                                           \
      v->data[v->size++] = *x;                                                                                         \
    }                                                                                                                  \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Pop last element, returns error if empty.                                                                         \
   */                                                                                                                  \
  static inline int vector_##TYPE##_pop_back(vector_##TYPE *v, TYPE *out) {                                            \
    if (!v->size) return -1;                                                                                           \
    --v->size;                                                                                                         \
    if (out) *out = v->data[v->size];                                                                                  \
    if (v->dtor) v->dtor(&v->data[v->size]);                                                                           \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Unordered erase (swap with last), O(1).                                                                           \
   */                                                                                                                  \
  static inline int vector_##TYPE##_erase_swap(vector_##TYPE *v, size_t pos) {                                         \
    if (pos >= v->size) return -1;                                                                                     \
    if (pos != v->size - 1) v->data[pos] = v->data[v->size - 1];                                                       \
    --v->size;                                                                                                         \
    if (v->dtor) v->dtor(&v->data[v->size]);                                                                           \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Stable erase at pos, O(n).                                                                                        \
   */                                                                                                                  \
  static inline int vector_##TYPE##_erase(vector_##TYPE *v, size_t pos) {                                              \
    if (pos >= v->size) return -1;                                                                                     \
    TYPE tmp = v->data[pos];                                                                                           \
    memmove(&v->data[pos], &v->data[pos + 1], (v->size - pos - 1) * sizeof(TYPE));                                     \
    --v->size;                                                                                                         \
    if (v->dtor) v->dtor(&tmp);                                                                                        \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Insert element at pos, O(n).                                                                                      \
   */                                                                                                                  \
  static inline int vector_##TYPE##_insert(vector_##TYPE *v, size_t pos, TYPE const *x) {                              \
    if (pos > v->size) return -1;                                                                                      \
    if (v->size == v->capacity) {                                                                                      \
      size_t nc = v->capacity ? v->capacity * 2 : 4;                                                                   \
      int e = vector_##TYPE##_try_reserve(v, nc);                                                                      \
      if (e) return e;                                                                                                 \
    }                                                                                                                  \
    memmove(&v->data[pos + 1], &v->data[pos], (v->size - pos) * sizeof(TYPE));                                         \
    if (v->copier) {                                                                                                   \
      TYPE *copy = (TYPE *)v->copier(x);                                                                               \
      v->data[pos] = *copy;                                                                                            \
      free(copy);                                                                                                      \
    } else {                                                                                                           \
      v->data[pos] = *x;                                                                                               \
    }                                                                                                                  \
    ++v->size;                                                                                                         \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Insert range of elements from C-array, O(n).                                                                      \
   */                                                                                                                  \
  static inline int vector_##TYPE##_insert_range(vector_##TYPE *v, size_t pos, const TYPE *src, size_t n) {            \
    if (pos > v->size) return -1;                                                                                      \
    if (v->size + n > v->capacity) {                                                                                   \
      size_t nc = v->capacity ? v->capacity * 2 : 4;                                                                   \
      while (nc < v->size + n)                                                                                         \
        nc *= 2;                                                                                                       \
      int e = vector_##TYPE##_try_reserve(v, nc);                                                                      \
      if (e) return e;                                                                                                 \
    }                                                                                                                  \
    memmove(&v->data[pos + n], &v->data[pos], (v->size - pos) * sizeof(TYPE));                                         \
    memcpy(&v->data[pos], src, n * sizeof(TYPE));                                                                      \
    v->size += n;                                                                                                      \
    return 0;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /**                                                                                                                  \
   * Erase a range [first, last), O(n).                                                                                \
   */                                                                                                                  \
  static inline int vector_##TYPE##_erase_range(vector_##TYPE *v, size_t first, size_t last) {                         \
    if (first > last || last > v->size) return -1;                                                                     \
    size_t n = last - first;                                                                                           \
    for (size_t i = first; i < last && v->dtor; ++i)                                                                   \
      v->dtor(&v->data[i]);                                                                                            \
    memmove(&v->data[first], &v->data[last], (v->size - last) * sizeof(TYPE));                                         \
    v->size -= n;                                                                                                      \
    return 0;                                                                                                          \
  }

#endif // P_VECTOR_H