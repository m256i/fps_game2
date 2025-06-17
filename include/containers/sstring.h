#ifndef CONTAINERS_SSTRING_H_
#define CONTAINERS_SSTRING_H_

#include <util/array_copy.h>
#include <util/dbg/alloctrack.h>
#include <common.h>

#define SSTRING_SSO_SIZE 15

/*
-- short string that can hold up to UINT32_MAX - 1 many chars
-- it uses SSO of 15 bytes and still is only 16 bytes in total
-- default initialization is very cheap
*/
typedef struct {
  union {
    struct __attribute__((packed)) {
      char *data;
      u32   size;
      u16   cap_exponent;                /* capacity is 2^cap_exponent */
    };
    struct {
      char stack_buff[SSTRING_SSO_SIZE]; /*make sure its (2^N - 1) sized */
    };
  };
  union {
    struct {
      u8 uses_heap  : 1;
      u8 stack_size : 7; /* safe to only use 7 bits here */
    };
    u8 last_byte;        /* simply used for making the compiler shut up */
  };
} sstring;

/* return log2(next_power2(_size)) */
/*
TODO: kinda retarded
*/
static inline u32 next_sstring_cap(u32 _size) {
  GAME_ASSERT(_size);
  /*
  round _size up to the next power of 2 and return log2() of that number
  if [newsize == power of two] take the next one
  */
  _size |= _size >> 1;
  _size |= _size >> 2;
  _size |= _size >> 4;
  _size |= _size >> 8;
  _size |= _size >> 16;
  _size++;
  const usize result = ((sizeof(_size)) * CHAR_BIT - 1) - __builtin_clz(_size);
  return result;
}

/* better codegen than checking if a single BIT is set  */
static inline __attribute__((const)) bool sstring_uses_heap(sstring _str) {
  return _str.last_byte > 0x7f;
}

static inline sstring create_sstring(u0) { return (sstring){0}; }

static inline u0 sstring_resize(sstring *s, usize new_size) {
  GAME_ASSERT(s);
  GAME_ASSERT((!s->uses_heap) || s->data);
  if (new_size == 0) {
    if (sstring_uses_heap(*s)) {
      TRACKED_FREE(s->data);
    }
    memset(s, 0, sizeof *s);
    return;
  }
  /* stack case */
  if (new_size < SSTRING_SSO_SIZE) {
    if (sstring_uses_heap(*s)) {
      char *old      = s->data;
      usize copy_len = s->size < new_size ? s->size : new_size;
      memcpy(s->stack_buff, old, copy_len);
      TRACKED_FREE(old);
    }
    s->stack_buff[new_size] = '\0';
    s->stack_size           = (u8)new_size;
    GAME_LOGF("length after stack resize: %u", s->stack_size);
    return;
  }
  /* heap case */
  const usize needed_cap = 1ULL << next_sstring_cap(new_size);
  if (sstring_uses_heap(*s)) {
    if ((1ULL << s->cap_exponent) < new_size + 1) {
      char *newbuf    = TRACKED_REALLOC(s->data, needed_cap);
      s->data         = newbuf;
      s->cap_exponent = (u16)next_sstring_cap(new_size);
    }
  } else {
    /* transition from stack to heap */
    char *buf = TRACKED_MALLOC(needed_cap);
    memcpy(buf, s->stack_buff, SSTRING_SSO_SIZE);
    s->data         = buf;
    s->cap_exponent = (u16)next_sstring_cap(new_size);
    s->last_byte    = 0xff;
    GAME_LOGF("string after strclone from stack: %s", s->data);
  }
  GAME_LOGF("length after resize: %u", new_size);
  GAME_LOGF("cap after resize: %u", 1ull << s->cap_exponent);
  s->size           = (u32)new_size;
  s->data[new_size] = '\0';
}

/*
copy a external string into a already existing and properly sized
sstring
*/
static inline u0
sstring_copy_string(sstring *_dst, const char *_str, usize _size) {
  GAME_ASSERT(_str);
  if (sstring_uses_heap(*_dst)) {
    GAME_ASSERT(_dst->size == _size);
    memcpy(_dst->data, _str, _size);
  } else {
    GAME_ASSERT(_dst->stack_size == _size);
    memcpy(_dst->stack_buff, _str, _size);
  }
}

/*
construct our short string from a NULL TERMINATED c string
*/
static inline sstring sstring_from_cstring(const char *_str) {
  const usize str_length = strlen(_str);
  sstring     out        = {0};
  sstring_resize(&out, str_length);
  sstring_copy_string(&out, _str, str_length);
  return out;
}

static inline sstring sstring_from_string(char *_str, usize _strlen) {
  sstring out = {0};
  sstring_resize(&out, _strlen);
  sstring_copy_string(&out, _str, _strlen);
  return out;
}

static inline usize sstring_length(const sstring *const _str) {
  GAME_ASSERT(_str);
  if (sstring_uses_heap(*_str)) {
    return _str->size;
  } else {
    return _str->stack_size;
  }
}

static inline u0 sstring_reserve(sstring *_str, u32 _newcap) {
  const u32 old_size = sstring_length(_str);
  u32       old_cap  = 0;
  if (sstring_uses_heap(*_str)) {
    old_cap = 1ull << _str->cap_exponent;
  } else {
    old_cap = SSTRING_SSO_SIZE;
  }

  if (old_cap >= _newcap) {
    return;
  }

  sstring_resize(_str, _newcap);
  if (sstring_uses_heap(*_str)) {
    _str->size = old_size;
  } else {
    _str->stack_size = old_size;
  }
}

static inline char *sstring_end(sstring *const _str) {
  const u32 _len = sstring_length(_str);
  if (_len == 0) {
    return NULL;
  }
  if (sstring_uses_heap(*_str)) {
    return &_str->data[_len];
  } else {
    return &_str->stack_buff[_len];
  }
}

static inline char *sstring_data(sstring *const _str) {
  if (sstring_uses_heap(*_str)) {
    return _str->data;
  }
  return _str->stack_buff;
}

static inline char *sstring_at(sstring *_str, usize _index) {
  GAME_ASSERT(_str);
  GAME_ASSERT(_str->data);
  GAME_ASSERT(_index < sstring_length(_str));
  return &(sstring_data(_str)[_index]);
}

static inline u0 sstring_append(sstring *_dst, sstring *_str0) {
  GAME_ASSERT(_str0);
  GAME_ASSERT(_dst);
  const u32 len0 = sstring_length(_dst);
  const u32 len1 = sstring_length(_str0);
  GAME_LOGF(" len0 + len1: %u", len0 + len1);
  sstring_resize(_dst, len0 + len1);
  memcpy(&sstring_data(_dst)[len0], sstring_data(_str0), len1);
}

static inline char *
sstring_find(sstring *const _heystack, sstring *const _needle) {
  return strstr(sstring_data(_heystack), sstring_data(_needle));
}

static inline sstring
sstring_substr(sstring *const _src, usize _start, usize _end) {
  GAME_ASSERT(_end > _start);
  GAME_ASSERT(sstring_length(_src) > _end);
  sstring out = {0};
  sstring_resize(&out, _end - _start);
  sstring_copy_string(&out, sstring_at(_src, _start), _end - _start);
  return out;
}

static inline u0 sstring_insert(sstring *_dst, sstring *_src, usize _pos) {
  GAME_ASSERT(_src);
  GAME_ASSERT(_dst);
  const u32 len0      = sstring_length(_src);
  const u32 len1      = sstring_length(_dst);
  const u32 after_pos = _pos + sstring_length(_src);

  if (_pos == sstring_length(_dst)) {
    sstring_append(_dst, _src);
    return;
  }
  sstring_resize(_dst, len0 + len1);
  const u32 length_after_pos = sstring_length(_dst) - _pos;
  sstring   end_part =
    sstring_from_string(sstring_at(_dst, _pos), length_after_pos);

  memcpy(sstring_at(_dst, _pos), sstring_data(_src), len1);
  memcpy(
    sstring_at(_dst, after_pos),
    sstring_data(&end_part),
    length_after_pos
  );
}

static inline bool sstring_equals(sstring *const _str0, sstring *const _str1) {
  const u32 len0 = sstring_length(_str0);
  const u32 len1 = sstring_length(_str1);
  if (len0 != len1) {
    return false;
  }
  return !strcmp_safe(sstring_data(_str0), sstring_data(_str1), len0);
}

// static inline char *
// sstring_replace(sstring *const _heystack, sstring *_substring) {
//   char *offset = sstring_find(_heystack, _substring);
//   if (!offset) {
//     return NULL;
//   }
// }

static inline u0 sstring_push(sstring *const _str, char _c) {
  const u32 old_length = sstring_length(_str);
  sstring_resize(_str, old_length + 1);
  *(sstring_at(_str, old_length)) = _c;
}

static inline u0 sstring_push_n(sstring *const _str, char _c, usize _n) {
  const u32 old_length = sstring_length(_str);
  sstring_resize(_str, old_length + (_n));
  for (usize i = 0; i != _n; i++) {
    *(sstring_at(_str, old_length + i)) = _c;
  }
}

static inline u0 destroy_sstring(sstring *_str) {
  if (sstring_uses_heap(*_str)) {
    GAME_ASSERT(_str->data);
    TRACKED_FREE(_str->data);
    _str->data = NULL;
  }
  memset(_str, 0, sizeof *_str);
}

#endif // CONTAINERS_SSTRING_H_
