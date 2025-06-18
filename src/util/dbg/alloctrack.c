#include <stdlib.h>
#include <string.h>
#include <util/dbg/alloctrack.h>
#include <common.h>
#include <containers/str_hash_table.h>
#include <stdatomic.h>
#include <pthread.h>

#include <containers/hashmap.h>

#define VERBOSE_TRACKING

typedef struct {
  uptr        address;
  usize       alloc_size;
  usize       alignment;
  const char *alloc_loc;
} alloc_block;

struct hashmap *alloc_map   = NULL;
bool            initialized = false;
pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static u64 user_hash(const u0 *item, u64 seed0, u64 seed1) {
  const alloc_block *user = item;
  return hashmap_sip(&user->address, sizeof(uptr), seed0, seed1);
}

static int user_compare(const u0 *a, const u0 *b, u0 *udata) {
  const alloc_block *ua = a;
  const alloc_block *ub = b;
  return ua->address != ub->address;
}

static u0 init(u0) {
  if (!initialized) {
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
    initialized = true;
  }
}

u0 *tracked_malloc(usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  u0 *mem = malloc(_size);
  GAME_ASSERT(mem);
  // GAME_LOGF("TRACKED_MALLOC() called!");
  hashmap_set(
    alloc_map,
    &(alloc_block){
      .address    = (uptr)mem,
      .alloc_size = _size,
      .alignment  = _Alignof(u0 *),
      .alloc_loc  = _function,
    }
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

#ifdef _WIN64
  u0 *mem = _aligned_malloc(_size, _alignment);
#else
  u0 *mem = aligned_alloc(_alignment, _size);
#endif
  GAME_ASSERT(mem);
  hashmap_set(
    alloc_map,
    &(alloc_block){
      .address    = (uptr)mem,
      .alloc_size = _size,
      .alignment  = _alignment,
      .alloc_loc  = _function,
    }
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_realloc(u0 *_ptr, usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  if (_ptr) {
    size_t       iter = 0;
    alloc_block *item = NULL;
    while (hashmap_iter(alloc_map, &iter, (u0 **)&item)) {
      const alloc_block *block = item;
      if (block->address == (uptr)_ptr) {
        break;
      } else {
        item = NULL;
      }
    }
    if (item) {
#ifdef VERBOSE_TRACKING
      GAME_LOGF("TRACKED_REALLOC() on previously allocated item:");
      GAME_LOGF("   size      : %zu", item->alloc_size);
      GAME_LOGF("   align     : %zu", item->alignment);
      GAME_LOGF("   ptr       : %p", (u0 *)item->address);
      GAME_LOGF("   new size  : %zu", _size);
#endif
      if (!hashmap_delete(alloc_map, item)) {
        GAME_CRITICALF(
          "TRACKED_REALLOC() on unknown address: %p size: %zu",
          _ptr,
          _size
        );
      }
    }
  }
  u0 *mem = realloc(_ptr, _size);
  GAME_ASSERT(mem);
  hashmap_set(
    alloc_map,
    &(alloc_block){
      .address    = (uptr)mem,
      .alloc_size = _size,
      .alignment  = _Alignof(u0 *),
      .alloc_loc  = _function,
    }
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 *tracked_calloc(usize _count, usize _size, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();
  u0 *mem = calloc(_count, _size);
  GAME_ASSERT(mem);
  hashmap_set(
    alloc_map,
    &(alloc_block){
      .address    = (uptr)mem,
      .alloc_size = _size,
      .alignment  = _Alignof(u0 *),
      .alloc_loc  = _function,
    }
  );
  pthread_mutex_unlock(&alloc_mutex);
  return mem;
}

u0 tracked_free(u0 *_ptr, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();

  if (!_ptr) {
    GAME_CRITICALF("free on NULL in %s", _function);
  }

  size_t iter = 0;
  void  *item;
  while (hashmap_iter(alloc_map, &iter, &item)) {
    const alloc_block *block = item;
    if (block->address == (uptr)_ptr) {
      if (!hashmap_delete(alloc_map, item)) {
        GAME_CRITICALF("free on unknown item %p in: %s", _ptr, _function);
      }
      break;
    }
  }

  free(_ptr);
  pthread_mutex_unlock(&alloc_mutex);
}

u0 tracked_aligned_free(u0 *_ptr, const char *_function) {
  pthread_mutex_lock(&alloc_mutex);
  init();

  if (!_ptr) {
    GAME_CRITICALF("free on NULL in %s", _function);
  }

  size_t iter = 0;
  void  *item;
  while (hashmap_iter(alloc_map, &iter, &item)) {
    const alloc_block *block = item;
    if (block->address == (uptr)_ptr) {
      if (!hashmap_delete(alloc_map, item)) {
        GAME_CRITICALF("free on unknown item %p in: %s", _ptr, _function);
      }
      break;
    }
  }

#ifdef _WIN64
  _aligned_free(_ptr);
#else
  free(_ptr);
#endif
  pthread_mutex_unlock(&alloc_mutex);
}

u0 dump_alloc_statistics(u0) {
  if (!initialized) {
    return;
  }
  pthread_mutex_lock(&alloc_mutex);
  puts("=== allocation tracker report ===");

  usize total_leaked = 0;

  size_t iter = 0;
  void  *item;
  while (hashmap_iter(alloc_map, &iter, &item)) {
    const alloc_block *block  = item;
    total_leaked             += block->alloc_size;
    printf(
      "  > %p block allocated in %s (size: %zu alignment: %zu) still in use \n",
      (u0 *)block->address,
      block->alloc_loc,
      block->alloc_size,
      block->alignment
    );
  }
  printf("  (total bytes leaked: %zu)\n", total_leaked);
  puts("=== allocation report finished ===");
  pthread_mutex_unlock(&alloc_mutex);
}
