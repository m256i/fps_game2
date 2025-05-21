#ifndef CONTAINERS_STRMAP_H_
#define CONTAINERS_STRMAP_H_

#include <common.h>
// clang-format off

/* one slot in the table */
typedef struct {
  u32       hash;
  char      *key; /* strdup’d string */
  u32       id;    /* stable unique ID returned to user */
  bool      used;
  bool      deleted;
} hash_slot;

/* maps any nullterminated c string to a unique u32 % bucket_count */
typedef struct {
  usize       bucket_count;
  usize       used_count;
  u32         next_id;
  hash_slot   *data;
} str_indexer;

u0  str_indexer_initialize  (str_indexer *_map, usize _init_size);
u0  str_indexer_destroy     (str_indexer *_map);
u32 str_indexer_get_hash    (str_indexer *_map, const char *_key);
u32 str_indexer_contains    (const str_indexer *_map, const char *_key);
u0  str_indexer_erase       (str_indexer *_map, const char *_key);

// clang-format on
inline u0 str_indexer_dbg_print(str_indexer *_map) {
  puts("string indices: {");
  for (usize i = 0; i != _map->bucket_count; i++) {
    if (_map->data[i].used) {
      printf(
        "hash: %u index: %u key: '%s', \n",
        _map->data[i].hash,
        _map->data[i].id,
        _map->data[i].key
      );
    }
  }
  puts("}");
}

#endif // CONTAINERS_STRMAP_H_
