#ifndef UTIL_DBG_STRACE_H_
#define UTIL_DBG_STRACE_H_

#include <common.h>

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#define STRACE()                                                               \
  do {                                                                         \
    add_strace_name((uptr)__builtin_return_address(0), __PRETTY_FUNCTION__);   \
  } while (0)

u0          add_strace_name(uptr _addr, const char *_pretty_name);
LONG WINAPI unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo);
u0          setup_stacktrace(u0);

#endif // UTIL_DBG_STRACE_H_
