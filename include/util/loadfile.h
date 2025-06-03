#ifndef UTIL_LOADFILE_H_
#define UTIL_LOADFILE_H_

#include <common.h>
#include <util/dbg/alloctrack.h>

static inline char *read_file_to_string_basic(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    GAME_WARNF("Failed to open file: %s", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);

  char *buf = TRACKED_MALLOC((usize)len + 1);

  if (!buf) {
    GAME_WARNF("Out of memory reading file: %s", path);
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, (usize)len, f) != (usize)len) {
    GAME_WARNF("Failed to read file: %s", path);
    TRACKED_FREE(buf);
    fclose(f);
    return NULL;
  }
  buf[len] = '\0';
  fclose(f);
  return buf;
}

#endif // UTIL_LOADFILE_H_
