#ifndef UTIL_SAFE_STRING_H_
#define UTIL_SAFE_STRING_H_

#include <util/dbg/alloctrack.h>
#include <common.h>
#include <string.h>

static inline usize strlen_safe(const char* _str, usize _maxlen) {
    usize l = 0;
    while (l < _maxlen && _str[l] != '\0') {
        ++l;
    }
    return l;
}

static inline usize strcmp_safe(const char* _str0, const char* _str1, usize _maxlen) {
    usize i = 0;
    for (; i < _maxlen; ++i) {
        u8 ca = (u8)_str0[i];
        u8 cb = (u8)_str1[i];
        if (ca != cb) {
            return (int32_t)ca - (int32_t)cb;
        }
        if (ca == '\0') {
            return 0;
        }
    }
    return 0;
}

static inline char *strcpy_safe(const char *_src, usize _bufsize) {
    if (!_bufsize) {
        return NULL;
    }
    char *dst = TRACKED_MALLOC(_bufsize);
    if (!dst) {
        return NULL;
    }
    usize copy_len = strlen_safe(_src, _bufsize - 1);
    if (copy_len > 0) {
        memcpy(dst, _src, copy_len);
    }
    dst[copy_len] = '\0';
    return dst;
}

#endif // UTIL_SAFE_STRING_H_
