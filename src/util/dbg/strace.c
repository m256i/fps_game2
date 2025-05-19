#include <stdatomic.h>
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <pthread.h>
#include <common.h>

#include <containers/str_hash_table.h>

static str_hash_table  function_lookup = {0};
static _Atomic bool    initialized     = false;
static pthread_mutex_t funcs_lock      = PTHREAD_MUTEX_INITIALIZER;

LONG WINAPI unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo);

u0 setup_stacktrace(u0) {
  SetUnhandledExceptionFilter(unhandled_exception_handerl);
}

u0 add_strace_name(uptr _addr, const char *_pretty_name) {
  if (!atomic_load(&initialized)) {
    str_hash_table_initialize(&function_lookup, sizeof(const char *), 10);
    atomic_store(&initialized, true);
  }

  pthread_mutex_lock(&funcs_lock);
  {
    char *str = calloc(1, 0xff);
    _ultoa_s((uptr)_addr, str, 0xff, 16);
    if (str_hash_table_contains(&function_lookup, str)) {
      free(str);
      pthread_mutex_unlock(&funcs_lock);
      return;
    }
    str_hash_table_insert(&function_lookup, str, (u0 *)_pretty_name);
  }
  pthread_mutex_unlock(&funcs_lock);
}

#define MAX_FRAMES 64

LONG WINAPI unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo) {
  if (!atomic_load(&initialized)) {
    str_hash_table_initialize(&function_lookup, sizeof(const char *), 10);
    atomic_store(&initialized, true);
  }

  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);

  CONTEXT      context = *ExceptionInfo->ContextRecord;
  STACKFRAME64 frame;
  ZeroMemory(&frame, sizeof(frame));
#ifdef _M_IX86
  DWORD imageType        = IMAGE_FILE_MACHINE_I386;
  frame.AddrPC.Offset    = context.Eip;
  frame.AddrFrame.Offset = context.Ebp;
  frame.AddrStack.Offset = context.Esp;
#elif _M_X64
  DWORD imageType        = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset    = context.Rip;
  frame.AddrFrame.Offset = context.Rsp;
  frame.AddrStack.Offset = context.Rsp;
#else
#error "Platform not supported!"
#endif
  frame.AddrPC.Mode    = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;

  fprintf(stderr, "\n=== Stack trace (most recent call first) ===\n");

  SYMBOL_INFO *symbol  = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256, 1);
  symbol->MaxNameLen   = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  pthread_mutex_lock(&funcs_lock);

  for (usize frameNum = 0; frameNum < MAX_FRAMES; frameNum++) {
    if (!StackWalk64(
          imageType,
          process,
          GetCurrentThread(),
          &frame,
          &context,
          NULL,
          SymFunctionTableAccess64,
          SymGetModuleBase64,
          NULL
        )) {
      break;
    }

    DWORD64 address = frame.AddrPC.Offset;
    if (address == 0) {
      break;
    }

    char addrstr[0xff] = {0};
    _ultoa_s(address, addrstr, 0xff, 16);

    DWORD64 displacement = 0;
    if (SymFromAddr(process, address, &displacement, symbol)) {
      fprintf(
        stderr,
        "%02d: %s + 0x%0llx (0x%0llx)\n",
        frameNum,
        symbol->Name,
        (unsigned long long)displacement,
        (unsigned long long)address
      );
    } else if (str_hash_table_contains(&function_lookup, addrstr)) {
      fprintf(
        stderr,
        "%02d: %s + 0x%0llx (0x%0llx)\n",
        frameNum,
        (char *)str_hash_table_at(&function_lookup, addrstr),
        (unsigned long long)displacement,
        (unsigned long long)address
      );
    } else {
      fprintf(
        stderr,
        "%02d: [0x%0llx]\n",
        frameNum,
        (unsigned long long)address
      );
    }
  }

  pthread_mutex_unlock(&funcs_lock);

  free(symbol);
  SymCleanup(process);
  return EXCEPTION_CONTINUE_SEARCH;
}
