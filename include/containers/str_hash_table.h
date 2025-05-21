#ifndef CONTAINERS_STR_HASH_TABLE_H_
#define CONTAINERS_STR_HASH_TABLE_H_

#include <common.h>
#include <containers/str_indexer.h>

typedef struct {
  u0  *data;
  bool used;
} str_hash_table_slot;

typedef struct {
  str_indexer          str_hasher;
  usize                obj_size;
  usize                obj_array_len;
  str_hash_table_slot *buckets;
  bool                 initialized;
} str_hash_table;

u0 str_hash_table_initialize(
  str_hash_table *const _table,
  usize                 _obj_size,
  usize                 _obj_count
);
u32 str_hash_table_insert(
  str_hash_table *const _table,
  const char           *_key,
  u0                   *_obj
);
u0   str_hash_table_erase(str_hash_table *const _table, const char *_key);
u0  *str_hash_table_at(str_hash_table *const _table, const char *_key);
bool str_hash_table_contains(str_hash_table *const _table, const char *_key);
u32  str_hash_table_get_index(str_hash_table *const _table, const char *_key);
u0  *str_hash_table_at_index(str_hash_table *const _table, u32 _index);
u0   str_hash_table_destroy(str_hash_table *const _table);
u0   str_hash_table_dbg_print(str_hash_table *const _table);

#endif // CONTAINERS_STR_HASH_TABLE_H_
