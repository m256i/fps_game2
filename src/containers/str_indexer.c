#include <assert.h>
#include <common.h>

#include <containers/str_indexer.h>
#include <util/fnv1a32.h>

inline char *portable_strdup(const char *s) {
  size_t len  = strlen(s) + 1;
  char  *copy = malloc(len);
  if (!copy) return NULL;
  return memcpy(copy, s, len);
}

/* grow table to new_capacity, re-inserting all live entries */
inline u0 str_indexer_resize(str_indexer *t, usize new_capacity) {
  hash_slot *new_data = calloc(new_capacity, sizeof *new_data);
  if (!new_data) {
    GAME_CRITICALF("OOM");
    exit(1);
  }
  /* move each live slot over—with its key, hash, and id */
  for (usize i = 0; i < t->bucket_count; i++) {
    hash_slot *old = &t->data[i];
    if (!old->used) continue;

    usize idx = (usize)(old->hash % new_capacity);
    while (new_data[idx].used) {
      idx = (idx + 1) % new_capacity;
    }
    new_data[idx]         = *old; /* copies hash, key pointer, id, flags */
    new_data[idx].deleted = false;
  }
  free(t->data);
  t->data         = new_data;
  t->bucket_count = new_capacity;
}

/* find slot for hash+key (either existing or first free/tombstone) */
inline usize str_indexer_probe_slot(str_indexer *t, u32 hash, const char *key) {
  GAME_ASSERT(t->bucket_count > 0);
  usize idx = (usize)(hash % t->bucket_count);
  for (usize probes = 0; probes < t->bucket_count; probes++) {
    hash_slot *slot = &t->data[idx];

    if (!slot->used && !slot->deleted) {
      return idx; /* new slot */
    }
    if (slot->used && slot->hash == hash && strcmp(slot->key, key) == 0) {
      return idx; /* existing key */
    }
    idx = (idx + 1) % t->bucket_count;
  }
  GAME_CRITICALF("str_indexer full!");
  exit(1);
}

u0 str_indexer_initialize(str_indexer *_map, usize _init_size) {
  _map->bucket_count = _init_size;
  _map->used_count   = 0;
  _map->next_id      = 0;
  _map->data         = NULL;
  if (_init_size == 0) return;
  _map->data = calloc(_init_size, sizeof *_map->data);
  if (!_map->data) {
    GAME_CRITICALF("OOM");
    exit(1);
  }
}

u0 str_indexer_destroy(str_indexer *_map) {
  if (!_map->data) return;
  for (usize i = 0; i < _map->bucket_count; i++) {
    if (_map->data[i].used && _map->data[i].key) {
      free(_map->data[i].key);
    }
  }
  free(_map->data);
  _map->data         = NULL;
  _map->bucket_count = _map->used_count = 0;
  _map->next_id                         = 0;
}

u32 str_indexer_get_hash(str_indexer *_map, const char *_key) {
  GAME_ASSERT(_map->bucket_count > 0);

  /* resize if load > 0.7 */
  if ((f64)(_map->used_count + 1) > _map->bucket_count * 0.7) {
    str_indexer_resize(_map, _map->bucket_count * 2);
  }

  u32        hash = fnv32_hash(_key);
  usize      idx  = str_indexer_probe_slot(_map, hash, _key);
  hash_slot *slot = &_map->data[idx];

  if (!slot->used) {
    /* fresh insert */
    if (slot->deleted) slot->deleted = false;
    slot->hash = hash;
    slot->key  = portable_strdup(_key);
    if (!slot->key) {
      GAME_CRITICALF("OOM");
      exit(1);
    }
    slot->id   = _map->next_id++;
    slot->used = true;
    _map->used_count++;
  }
  return slot->id;
}

u32 str_indexer_contains(const str_indexer *_map, const char *key) {
  if (_map->used_count == 0) {
    return UINT32_MAX;
  }

  u32              hash = fnv32_hash(key);
  usize            idx  = str_indexer_probe_slot(_map, hash, key);
  const hash_slot *slot = &_map->data[idx];

  if (slot->used && slot->hash == hash && strcmp(slot->key, key) == 0) {
    return (u32)slot->id;
  } else {
    return UINT32_MAX;
  }
}

u0 str_indexer_erase(str_indexer *_map, const char *_key) {
  GAME_ASSERT(_map->bucket_count > 0);
  u32   hash = fnv32_hash(_key);
  usize idx  = (usize)(hash % _map->bucket_count);
  for (usize probes = 0; probes < _map->bucket_count; probes++) {
    hash_slot *slot = &_map->data[idx];
    if (!slot->used && !slot->deleted) return;
    if (slot->used && slot->hash == hash && strcmp(slot->key, _key) == 0) {
      free(slot->key);
      slot->key     = NULL;
      slot->used    = false;
      slot->deleted = true;
      _map->used_count--;
      return;
    }
    idx = (idx + 1) % _map->bucket_count;
  }
}
