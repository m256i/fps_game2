#ifndef CONTAINERS_SSTRING_H_
#define CONTAINERS_SSTRING_H_

#include <util/array_copy.h>
#include <util/dbg/alloctrack.h>
#include <common.h>

#define SSTRING_SSO_SIZE 15

/*
-- short string that can hold up to USHORT_MAX - 1 many chars
-- it uses SSO of 15 bytes and still is only 16 bytes in total
-- default initialization is very cheap
*/
typedef struct {
  union {
    struct __attribute__((packed)) {
      char *data;
      u16   size;
      u8    cap_exponent;                /* capacity is 2^cap_exponent */
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
static inline usize next_sstring_cap(usize _size) {
  GAME_ASSERT(_size);
  return __builtin_clz(_size) + 1;
}

/* presumably better codegen than checking if a single BIT is set  */
static inline __attribute__((const)) bool sstring_uses_heap(sstring _str) {
  return _str.last_byte > 0x7f;
}

static inline sstring create_sstring(u0) { return (sstring){0}; }

/*
construct our short string from a NULL TERMINATED c string
*/
static inline sstring sstring_from_cstring(const char *_str) {
  const usize str_length = strlen(_str);
  sstring     out        = {0};
  if (str_length >= SSTRING_SSO_SIZE) {
    GAME_ASSERT(next_sstring_cap(str_length) <= (usize)UINT8_MAX);
    out.cap_exponent = (u8)next_sstring_cap(str_length);
    out.size         = str_length;
    out.data         = strnclone_s(_str, str_length);
    out.uses_heap    = 1;
  } else {
    out.stack_size = (u8)str_length;
    memcpy(out.stack_buff, _str, str_length);
  }
  return out;
}

static inline sstring sstring_from_string(char *_str, usize _strlen) {
  sstring out = {0};
  if (_strlen >= SSTRING_SSO_SIZE) {
    GAME_ASSERT(next_sstring_cap(_strlen) <= (usize)UINT8_MAX);
    out.cap_exponent = (u8)next_sstring_cap(_strlen);
    out.size         = _strlen;
    out.data         = strnclone_s(_str, _strlen);
    out.uses_heap    = 1;
  } else {
    out.stack_size = (u8)_strlen;
    memcpy(out.stack_buff, _str, _strlen);
  }
  return out;
}

static inline char *sstring_at(sstring *_str) {}

static inline u0 destroy_sstring(sstring *_str) {
  if (sstring_uses_heap(*_str)) {
    TRACKED_FREE(_str->data);
    _str->data = 0;
  }
  memset(_str, 0, sizeof *_str);
}

#endif // CONTAINERS_SSTRING_H_
