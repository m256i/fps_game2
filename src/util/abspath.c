#include <util/array_copy.h>
#include <util/abspath.h>

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#define SEP_CHAR '\\'
#define IS_ABS_PATH(p)                                                         \
  (/* e.g. "C:\..." or "C:/..." */                                             \
   (((p)[0] >= 'A' && (p)[0] <= 'Z') || ((p)[0] >= 'a' && (p)[0] <= 'z')) &&   \
   (p)[1] == ':' && ((p)[2] == '\\' || (p)[2] == '/')                          \
  )
#else
#include <unistd.h>
#include <limits.h>
#define SEP_CHAR       '/'
#define IS_ABS_PATH(p) ((p)[0] == '/')
#endif

i32 get_executable_path(char *out, usize out_sz) {
#if defined(_WIN32)
  DWORD len = GetModuleFileNameA(NULL, out, (DWORD)out_sz);
  if (len == 0 || (usize)len >= out_sz) {
    return 0;
  }
  out[len] = '\0';
  return 1;

#else
  // Linux/Unix: readlink("/proc/self/exe")
  ssize_t len = readlink("/proc/self/exe", out, out_sz - 1);
  if (len < 1 || (usize)len >= out_sz - 1) {
    return 0;
  }
  out[len] = '\0';
  return 1;
#endif
}

char *get_executable_dir(void) {
#if defined(_WIN32)
  const usize path_max = MAX_PATH;
  char        buffer[MAX_PATH];
#else
  const usize path_max = PATH_MAX;
  char        buffer[PATH_MAX];
#endif

  if (!get_executable_path(buffer, sizeof(buffer))) return NULL;
  char *sep = strrchr(buffer, SEP_CHAR);
  if (!sep) {
    return strnclone_s(buffer, path_max);
  }
  *sep = '\0';
  return strnclone_s(buffer, path_max);
}

char *make_abs_path(const char *relpath) {
  if (!relpath || *relpath == '\0') return NULL;
  if (IS_ABS_PATH(relpath)) {
#if defined(_WIN32)
    return strnclone_s((char *)relpath, MAX_PATH);
#else
    return strnclone_s((char *)relpath, PATH_MAX);
#endif
  }

  char *exe_dir = get_executable_dir();
  if (!exe_dir) {
    GAME_WARNF("could not resolve executable path!");
    exit(1);
  }

  const usize dir_len = strlen(exe_dir);
  const usize rel_len = strlen(relpath);

  int need_sep = 1;
  if (dir_len > 0 && exe_dir[dir_len - 1] == SEP_CHAR) {
    need_sep = 0;
  }

  const usize total = dir_len + (need_sep ? 1 : 0) + rel_len + 1;
  char       *out   = (char *)TRACKED_MALLOC(total);
  if (!out) {
    GAME_WARNF("could not resolve path %s", out);
    TRACKED_FREE(exe_dir);
    return NULL;
  }

  memcpy(out, exe_dir, dir_len);
  if (need_sep) {
    out[dir_len] = SEP_CHAR;
    memcpy(out + dir_len + 1, relpath, rel_len);
    out[dir_len + 1 + rel_len] = '\0';
  } else {
    memcpy(out + dir_len, relpath, rel_len);
    out[dir_len + rel_len] = '\0';
  }

  TRACKED_FREE(exe_dir);
  GAME_LOGF("resolved path %s -> %s", relpath, out);
  return out;
}
