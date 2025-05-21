#include <stdlib.h>
#include <util/dbg/alloctrack.h>
#include <common.h>
#include <containers/str_hash_table.h>
#include <stdatomic.h>
#include <pthread.h>

#include <containers/hashmap.h>

typedef struct {
  uptr        address;
  const char *alloc_loc;
} alloc_block;

struct hashmap *alloc_map   = NULL;
_Atomic bool    initialized = false;
pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static u64 user_hash(const u0 *item, u64 seed0, u64 seed1) {
  const alloc_block *user = item;
  return hashmap_sip(&user->address, sizeof(uptr), seed0, seed1);
}

int user_compare(const u0 *a, const u0 *b, u0 *udata) {
  const alloc_block *ua = a;
  const alloc_block *ub = b;
  return ua->address != ub->address;
}

u0 init(u0) {
  if (!atomic_load(&initialized)) {
    alloc_map = hashmap_new(
      sizeof(alloc_block),
      10,
      0,
      0,
      user_hash,
      user_compare,
      NULL,
      NULL
    );
    atexit(dump_alloc_statistics);
    atomic_store(&initialized, true);
  }
}

u0 *tracked_malloc(usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  u0 *mem = malloc(_size);
  hashmap_set(
    alloc_map,
    &(alloc_block){.address = (uptr)mem, .alloc_loc = _function}
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_aligned_malloc(
  usize       _size,
  usize       _alignment,
  const char *_function
) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  u0 *mem = _aligned_malloc(_size, _alignment);
  hashmap_set(
    alloc_map,
    &(alloc_block){.address = (uptr)mem, .alloc_loc = _function}
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_aligned_realloc(
  u0         *_ptr,
  usize       _size,
  usize       _alignment,
  const char *_function
) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  hashmap_delete(
    alloc_map,
    &(alloc_block){.address = (uptr)_ptr, .alloc_loc = _function}
  );
  u0 *mem = _aligned_realloc(_ptr, _size, _alignment);
  hashmap_set(
    alloc_map,
    &(alloc_block){.address = (uptr)mem, .alloc_loc = _function}
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_realloc(u0 *_ptr, usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  hashmap_delete(
    alloc_map,
    &(alloc_block){.address = (uptr)_ptr, .alloc_loc = _function}
  );
  u0 *mem = realloc(_ptr, _size);
  hashmap_set(
    alloc_map,
    &(alloc_block){.address = (uptr)mem, .alloc_loc = _function}
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_calloc(usize _count, usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  u0 *mem = calloc(_count, _size);
  hashmap_set(
    alloc_map,
    &(alloc_block){.address = (uptr)mem, .alloc_loc = _function}
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 tracked_free(u0 *_ptr, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  if (!hashmap_delete(
        alloc_map,
        &(alloc_block){.address = (uptr)_ptr, .alloc_loc = _function}
      )) {
    GAME_CRITICALF("free on unknown item %p in function: %s", _ptr, _function);
  }
  free(_ptr);
  pthread_mutex_unlock(&alloc_mutex);
}

u0 tracked_aligned_free(u0 *_ptr, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  if (!hashmap_delete(
        alloc_map,
        &(alloc_block){.address = (uptr)_ptr, .alloc_loc = _function}
      )) {
    GAME_CRITICALF("free on unknown item %p in function: %s", _ptr, _function);
  }
  _aligned_free(_ptr);
  pthread_mutex_unlock(&alloc_mutex);
}

u0 dump_alloc_statistics(u0) {
  if (!atomic_load(&initialized)) {
    return;
  }
  pthread_mutex_lock(&alloc_mutex);
  puts("=== allocation tracker report ===");

  size_t iter = 0;
  void  *item;
  while (hashmap_iter(alloc_map, &iter, &item)) {
    const alloc_block *block = item;
    printf(
      "  > %p block allocated in %s still in use \n",
      (u0 *)block->address,
      block->alloc_loc
    );
  }
  pthread_mutex_unlock(&alloc_mutex);
}
