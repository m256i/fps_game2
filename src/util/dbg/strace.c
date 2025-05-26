#include <stdatomic.h>

#ifdef _WIN64
#define _CRT_SECURE_NO_WARNINGS
#include  <windows.h>
#include <dbghelp.h>
#else 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <pthread.h>
#include <common.h>
#include <util/dbg/alloctrack.h>

#ifdef _WIN64
typedef struct {
  uint64_t key;
  char    *value;
  bool     used;
} hm_entry;

typedef struct {
  hm_entry *entries;
  size_t    capacity;
  size_t    mask;
  size_t    size;
} hashmap;

/* small hmap implementation that isnt tracked to prevent infinite recursion */
__attribute__((no_strace)) u0    hm_init(hashmap *hm, size_t capacity);
__attribute__((no_strace)) u0    hm_resize(hashmap *hm);
__attribute__((no_strace)) char *hm_at(hashmap *hm, uint64_t key);
__attribute__((no_strace)) u0 hm_insert(hashmap *hm, uint64_t key, char *value);

static hashmap         function_lookup = {0};
static _Atomic bool    initialized     = false;
static pthread_mutex_t funcs_lock      = PTHREAD_MUTEX_INITIALIZER;

__attribute__((no_strace)) LONG WINAPI
unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo);

__attribute__((no_strace)) u0 setup_stacktrace(u0) {
  SetUnhandledExceptionFilter(unhandled_exception_handerl);
}
#else

u0 signal_handler(i32 sig) {
    u0 *buffer[64];
    i32 nptrs = backtrace(buffer, 64);
    fprintf(stderr, "=== Stack trace (most recent call) === error: %d\n", sig);
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
    exit(1);
}

u0 setup_stacktrace(u0) {
  signal(SIGSEGV, signal_handler);
  signal(SIGABRT, signal_handler);
}
#endif

#ifdef _WIN64
/*
we dont need a debug macro check as gcc doesnt emit calls
to this handler when building in release mode

this function is called at the entry of every function
through my custom plugin "./ftrace_plugin"

the "no_strace" plugin makes gcc ignore that function in the plugin
*/
void __instrument_strace(void *_addr, char *_funcname) {
  if (!atomic_load(&initialized)) {
    hm_init(&function_lookup, 16);
    atomic_store(&initialized, true);
  }
  pthread_mutex_lock(&funcs_lock);
  {
    hm_insert(&function_lookup, (uptr)_addr, _funcname);
  }
  pthread_mutex_unlock(&funcs_lock);
}

#define MAX_FRAMES 64

__attribute__((no_strace)) LONG WINAPI
unhandled_exception_handerl(EXCEPTION_POINTERS *ExceptionInfo) {
  if (!atomic_load(&initialized)) {
    hm_init(&function_lookup, 16);
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

  fprintf(stderr, "\n=== Stack trace (most recent call) ===\n");

  SYMBOL_INFO *symbol  = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256, 1);
  symbol->MaxNameLen   = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  pthread_mutex_lock(&funcs_lock);

  for (usize frame_num = 0; frame_num < MAX_FRAMES; frame_num++) {
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

    char *const func_lookup = hm_at(&function_lookup, address);

    DWORD64 displacement = 0;
    if (SymFromAddr(process, address, &displacement, symbol)) {
      fprintf(
        stderr,
        "%02d: %s + 0x%0llx (0x%0llx)\n",
        (i32)frame_num,
        symbol->Name,
        (u64)displacement,
        (u64)address
      );
    } else if (func_lookup) {
      fprintf(
        stderr,
        "%02d: %s + 0x%0llx (0x%0llx)\n",
        (i32)frame_num,
        func_lookup,
        (u64)displacement,
        (u64)address
      );
    } else {
      fprintf(stderr, "%02d: [0x%0llx]\n", (i32)frame_num, (u64)address);
    }
  }

  pthread_mutex_unlock(&funcs_lock);
  free(symbol);
  SymCleanup(process);

  dump_alloc_statistics();

  return EXCEPTION_CONTINUE_SEARCH;
}

#define LOAD_FACTOR_NUM 3
#define LOAD_FACTOR_DEN 4

__attribute__((no_strace)) u0 hm_init(hashmap *hm, size_t capacity) {
  if (!hm || (capacity & (capacity - 1)) != 0) {
    puts("hm_init error");
    exit(1);
  }
  hm->capacity = capacity;
  hm->mask     = capacity - 1;
  hm->size     = 0;
  hm->entries  = calloc(capacity, sizeof(hm_entry));
}

__attribute__((no_strace)) u0 hm_resize(hashmap *hm) {
  usize     new_capacity = hm->capacity << 1;
  usize     new_mask     = new_capacity - 1;
  hm_entry *new_entries  = calloc(new_capacity, sizeof(hm_entry));
  if (!new_entries) {
    puts("strace hm_resize OOM");
    exit(1);
  }
  for (usize i = 0; i < hm->capacity; i++) {
    hm_entry *e = &hm->entries[i];
    if (!e->used) continue;

    usize idx = (usize)e->key & new_mask;
    while (new_entries[idx].used) {
      idx = (idx + 1) & new_mask;
    }
    new_entries[idx].used  = true;
    new_entries[idx].key   = e->key;
    new_entries[idx].value = e->value;
  }

  free(hm->entries);
  hm->entries  = new_entries;
  hm->capacity = new_capacity;
  hm->mask     = new_mask;
}

__attribute__((no_strace)) u0 hm_insert(hashmap *hm, usize key, char *value) {
  if (!hm || !hm->entries) {
    puts("strace hmap not initialized");
    exit(1);
  }

  if ((hm->size + 1) * LOAD_FACTOR_DEN > hm->capacity * LOAD_FACTOR_NUM) {
    hm_resize(hm);
  }

  usize idx = (usize)key & hm->mask;
  for (;;) {
    hm_entry *e = &hm->entries[idx];
    if (!e->used) {
      e->used  = true;
      e->key   = key;
      e->value = value;
      hm->size++;
      return;
    }
    if (e->key == key) {
      e->value = value;
      return;
    }
    idx = (idx + 1) & hm->mask;
  }
}

__attribute__((no_strace)) char *hm_at(hashmap *hm, uptr key) {
  if (!hm || !hm->entries) return NULL;
  usize idx = (usize)key & hm->mask;
  for (;;) {
    hm_entry *e = &hm->entries[idx];
    if (!e->used) return NULL;
    if (e->key == key) return e->value;
    idx = (idx + 1) & hm->mask;
  }
}

#endif
