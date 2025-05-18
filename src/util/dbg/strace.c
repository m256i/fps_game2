#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <util/dbg/strace.h>

#include <common.h>

u0 setup_stacktrace(u0) {
  SetUnhandledExceptionFilter(unhandled_exception_handerl);
}

#define MAX_FRAMES 64

LONG WINAPI unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo) {
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

  // Walk the stack
  for (int frameNum = 0; frameNum < MAX_FRAMES; frameNum++) {
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
    } else {
      fprintf(
        stderr,
        "%02d: [0x%0llx]\n",
        frameNum,
        (unsigned long long)address
      );
    }
  }

  free(symbol);
  SymCleanup(process);
  return EXCEPTION_CONTINUE_SEARCH;
}
