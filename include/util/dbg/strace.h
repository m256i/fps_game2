#ifndef UTIL_DBG_STRACE_H_
#define UTIL_DBG_STRACE_H_

#include <common.h>

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

LONG WINAPI unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo);
u0          setup_stacktrace(u0);

#endif // UTIL_DBG_STRACE_H_
