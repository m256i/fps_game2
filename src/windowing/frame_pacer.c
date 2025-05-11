#include <windowing/frame_pacer.h>>
#include <windows.h>
#include <common.h>
#include <math.h>
#include <RGFW/RGFW.h>
#include <dxgi.h>

DWORD WINAPI pacer_thread_proc(_In_ LPVOID _params) {
  GAME_LOGF("... and running!");
  frame_pacer_context *ctx = (frame_pacer_context *)_params;
  u64 last_vblank_time = 0;
  u64 monitor_fc = 0;
  while (atomic_load_explicit(&ctx->stop_flag, memory_order_relaxed) != true) {
    wait_for_vblank(ctx);
    const u64 time = RGFW_getTimeNS();
    monitor_fc++;
    atomic_store_explicit(&ctx->last_vblank_time, time, memory_order_release);
    atomic_store_explicit(&ctx->monitor_frame_count, monitor_fc, memory_order_release);
    last_vblank_time = time;
  }
  return 0;
}

u0 initialize_frame_pacer(frame_pacer_context *const _ctx, HWND _hwnd) {
  _ctx->adapter = NULL;
  _ctx->output = NULL;
  _ctx->target_scanline = 0;
  _ctx->cycle_start_time = 0;
  _ctx->present_monitor_frame_count_target = 0;
  _ctx->frame_ready_at_monitor_count = 0;
  _ctx->last_present_time = 0;
  _ctx->last_present_framecount = 0;

  atomic_store(&_ctx->last_vblank_time, 0ull);
  atomic_store(&_ctx->last_monitor_cycle, 0ull);
  atomic_store(&_ctx->monitor_frame_count, 0ull);
  atomic_store(&_ctx->stop_flag, false);

  IDXGIFactory1 *factory = NULL;
  if (FAILED(CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory))) {
    GAME_LOGF("failed to create DXGI factory!");
    exit(1);
  }

  if (FAILED(factory->lpVtbl->EnumAdapters1(factory, 0, &_ctx->adapter))) {
    factory->lpVtbl->Release(factory);
    GAME_LOGF("failed to enum adapters");
    exit(1);
  }

  if (FAILED(_ctx->adapter->lpVtbl->EnumOutputs(_ctx->adapter, 0, &_ctx->output))) {
    _ctx->adapter->lpVtbl->Release(_ctx->adapter);
    _ctx->adapter = NULL;
    factory->lpVtbl->Release(factory);
    GAME_LOGF("failed to enum outputs");
    exit(1);
  }

  factory->lpVtbl->Release(factory);
  DWORD _tid;
  _ctx->pacer_thread_handle = CreateThread(NULL, 0, pacer_thread_proc, _ctx, 0, &_tid);
  (u0) _tid;
  GAME_LOGF("pacing thread up...");
}

u0 destroy_frame_pacer(frame_pacer_context *const _ctx) {
  atomic_store_explicit(&_ctx->stop_flag, true, memory_order_release);
  /*
  wait before destroying resources to prevent crash
  */
  while (WaitForSingleObject(_ctx->pacer_thread_handle, 0) != WAIT_OBJECT_0) {
    continue;
  }

  if (_ctx->output) {
    _ctx->output->lpVtbl->Release(_ctx->output);
    _ctx->output = NULL;
  }
  if (_ctx->adapter) {
    _ctx->adapter->lpVtbl->Release(_ctx->adapter);
    _ctx->adapter = NULL;
  }
  CoUninitialize();
  GAME_LOGF("destroyed frame pacer");
}

/*
  [_hres]:                  horizontal resolution of monitor
  [_refresh_rate]:          refresh rate of the monitor in hz
  [_last_present_latency]:  last time it took to call vkQueuePresent() in nanoseconcds
*/
u0 update_optimal_scanline(frame_pacer_context *const _ctx, u32 _hres, u32 _refresh_rate, u64 _last_present_latency) {
  //(optional debug)
  // printf("target_scanline = %u (margin_ns = %.0f ns, line_time = %.3f ns)\n", target, margin_ns, line_time_ns);
}

bool monitor_pace(frame_pacer_context *const _ctx, u32 _hres, u32 _refresh_rate) {
  // const u64 monitor_last_vblank_time = atomic_load_explicit(&_ctx->last_vblank_time, memory_order_acquire);
  const u64 fc = atomic_load_explicit(&_ctx->monitor_frame_count, memory_order_acquire);
  // printf("monitor pace, framecount:  %llu\n", fc);
  // printf("_ctx->present_monitor_frame_count_target:  %llu\n", _ctx->present_monitor_frame_count_target);
  // printf("monitor last vblank time: %llu\n", monitor_last_vblank_time);
  // printf("_ctx->last_present_time: %llu\n", _ctx->last_present_time);
  // printf("_ctx->present_monitor_frame_count_target - fc delta: %llu\n",
  // (_ctx->present_monitor_frame_count_target - fc));
  if (fc >= _ctx->present_monitor_frame_count_target) {
    _ctx->last_present_framecount = fc;
    return true;
  }
  return false;
}

u0 resync_cycle_start(frame_pacer_context *const _ctx) {
  //_ctx->cycle_start_time = atomic_load_explicit(&_ctx->last_vblank_time, memory_order_acquire);
}

u0 sync_cycle_start(frame_pacer_context *const _ctx) {
  wait_for_vblank(_ctx);
  // _ctx->cycle_start_time = RGFW_getTimeNS();
}

u0 update_target_frame(frame_pacer_context *const _ctx) {
  // puts("update target frame");
  /* if we havent rendered yet at all we just set current monitor frame */
  if (_ctx->present_monitor_frame_count_target == 0 || _ctx->last_present_framecount == 0) {
    const u64 fc = atomic_load_explicit(&_ctx->monitor_frame_count, memory_order_acquire);
    // printf("initial target update: fc %llu\n", fc);
    _ctx->present_monitor_frame_count_target = fc;
  } else if (_ctx->last_present_framecount >= _ctx->present_monitor_frame_count_target) {
    //  printf("last present == target frame\n");
    /* else we increment */
    _ctx->present_monitor_frame_count_target = _ctx->last_present_framecount + 1;
    // printf("present_monitor_frame_count_target %llu\n", _ctx->present_monitor_frame_count_target);
  }
}
