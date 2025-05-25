#include <containers/str_hash_table.h>

#include <assert.h>
#include <string.h>
#include <util/zrealloc.h>

u0 str_hash_table_initialize(
  str_hash_table *const _table,
  usize                 _obj_size,
  usize                 _obj_count
) {
  GAME_ASSERT(!_table->initialized);
  GAME_ASSERT(_table && _obj_size && _obj_count);

  str_indexer_initialize(&_table->str_hasher, _obj_count);

  _table->obj_size      = _obj_size;
  _table->obj_array_len = _table->str_hasher.bucket_count;
  _table->buckets = calloc(_table->obj_array_len, sizeof(str_hash_table_slot));
  if (!_table->buckets) {
    GAME_CRITICALF("OOM");
    exit(1);
  }
  _table->initialized = true;
}

u0 str_hash_table_dbg_print(str_hash_table *const _table) {
  puts("hash table: {");
  for (usize i = 0; i != _table->str_hasher.bucket_count; i++) {
    if (_table->str_hasher.data[i].used && _table->str_hasher.data[i].key) {
      printf(
        "index: %u hash: %u key: %s value_first_byte: %u,\n",
        _table->str_hasher.data[i].id,
        _table->str_hasher.data[i].hash,
        _table->str_hasher.data[i].key,
        *(u8 *)_table->buckets[i].data
      );
    }
  }
  puts("}");
}

u32 str_hash_table_insert(
  str_hash_table *const _table,
  const char           *_key,
  u0                   *_obj
) {
  GAME_ASSERT(_table->initialized);
  u32 idx = str_indexer_contains(&_table->str_hasher, _key);
  /* new insert? */
  if (idx == UINT32_MAX) {
    idx = str_indexer_get_hash(&_table->str_hasher, _key);
    /* str hasher resized after string lookup */
    if (_table->str_hasher.bucket_count != _table->obj_array_len) {
      usize old_size        = _table->obj_array_len;
      _table->obj_array_len = _table->str_hasher.bucket_count;
      _table->buckets       = zrealloc(
        _table->buckets,
        old_size * sizeof(str_hash_table_slot),
        _table->obj_array_len * sizeof(str_hash_table_slot)
      );
      if (!_table->buckets) {
        GAME_CRITICALF("OOM");
        exit(1);
      }
    }
    _table->buckets[idx].used = true;
    /* if the bucket was previously empty the .data pointer is NULL */
    _table->buckets[idx].data = malloc(_table->obj_size);
    if (!_table->buckets[idx].data) {
      GAME_CRITICALF("OOM");
      exit(1);
    }
  }
  memcpy(_table->buckets[idx].data, _obj, _table->obj_size);
  return idx;
}

u0 str_hash_table_erase(str_hash_table *const _table, const char *_key) {
  GAME_ASSERT(_table->initialized);
  const u32 idx = str_indexer_contains(&_table->str_hasher, _key);

  if (idx == UINT32_MAX) {
    return;
  }

  _table->buckets[idx].used = false;
  if (_table->buckets[idx].data) {
    free(_table->buckets[idx].data);
  }
  _table->buckets[idx].data = NULL;
  str_indexer_erase(&_table->str_hasher, _key);

  if (_table->str_hasher.bucket_count != _table->obj_array_len) {
    usize old_size        = _table->obj_array_len;
    _table->obj_array_len = _table->str_hasher.bucket_count;
    _table->buckets       = zrealloc(
      _table->buckets,
      old_size * sizeof(str_hash_table_slot),
      _table->obj_array_len * sizeof(str_hash_table_slot)
    );
    if (!_table->buckets) {
      GAME_CRITICALF("OOM");
      exit(1);
    }
  }
}

u0 *str_hash_table_at(str_hash_table *const _table, const char *_key) {
  GAME_ASSERT(_table->initialized);
  const u32 idx = str_indexer_contains(&_table->str_hasher, _key);
  if (idx == UINT32_MAX) {
    return NULL;
  }
  /* slot not empty */
  return _table->buckets[idx].data;
}

bool str_hash_table_contains(str_hash_table *const _table, const char *_key) {
  GAME_ASSERT(_table->initialized);
  const u32 idx = str_indexer_contains(&_table->str_hasher, _key);
  return idx != UINT32_MAX;
}

u32 str_hash_table_get_index(str_hash_table *const _table, const char *_key) {
  GAME_ASSERT(_table->initialized);
  return str_indexer_contains(&_table->str_hasher, _key);
}

u0 *str_hash_table_at_index(str_hash_table *const _table, u32 _index) {
  GAME_ASSERT(_table->initialized);
  GAME_ASSERT(_index < _table->obj_array_len);

  /* slot empty */
  if (!_table->buckets[_index].used) {
    return NULL;
  }
  /* slot not empty */
  return _table->buckets[_index].data;
}

u0 str_hash_table_destroy(str_hash_table *const _table) {
  GAME_ASSERT(_table->initialized);

  for (usize i = 0; i != _table->obj_array_len; i++) {
    if (_table->buckets[i].data) {
      free(_table->buckets[i].data);
    }
  }
  free(_table->buckets);
  _table->buckets = NULL;

  str_indexer_destroy(&_table->str_hasher);
  _table->obj_array_len = 0;
  _table->obj_size      = 0;
  _table->initialized   = false;
}
