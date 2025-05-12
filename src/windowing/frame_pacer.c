#include <windowing/frame_pacer.h>
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

  atomic_store_explicit(&ctx->last_monitor_cycle_time, (u64)(6.92 * 1e6), memory_order_release);
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

  _ctx->last_render_start_time = 0;
  _ctx->last_render_duration = 0;
  _ctx->last_present_start_time = 0;
  _ctx->last_present_duration = 0;

  _ctx->target_present_time = 0;
  _ctx->wait_until_present_frame = 0;
  _ctx->last_present_render_time = 0;

  _ctx->presented_frame = false;
  _ctx->updated_target = false;

  atomic_store(&_ctx->last_vblank_time, 0ull);
  atomic_store(&_ctx->last_monitor_cycle_time, 0ull);
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

u0 resync_cycle_start(frame_pacer_context *const _ctx) {
  //_ctx->cycle_start_time = atomic_load_explicit(&_ctx->last_vblank_time, memory_order_acquire);
}

u0 sync_cycle_start(frame_pacer_context *const _ctx) {
  wait_for_vblank(_ctx);
  // _ctx->cycle_start_time = RGFW_getTimeNS();
}

bool monitor_pace(frame_pacer_context *const _ctx, u32 _refresh_rate) {
  const u64 screen_cycle_time_ns = (u64)((1000.0 / (f64)_refresh_rate) * 1e6);
  const u64 time = RGFW_getTimeNS();

  if (time >= _ctx->target_present_time && !_ctx->presented_frame) {
    _ctx->presented_frame = true;
    return true;
  }
  return false;
}

u0 update_target_frame(frame_pacer_context *const _ctx, u32 _refresh_rate) {
  const u64 last_vblank_time = atomic_load_explicit(&_ctx->last_vblank_time, memory_order_acquire);
  const u64 last_cycle_time = (u64)((1000.0 / (f64)_refresh_rate) * 1e6);

  static u64 previous_blank_time = 0;

  if (last_vblank_time > previous_blank_time) {
    // puts("---------------------------------");
    // puts("new frame from monitor!");
    previous_blank_time = last_vblank_time;
    _ctx->updated_target = false;
    _ctx->presented_frame = false;
  } else {
    _ctx->updated_target = true;
  }
  if (_ctx->updated_target) { return; }

  // printf("last_vlank_time: %lf\n", ((f64)last_vblank_time) / 1e6);
  // printf("last_cycle_time: %lf\n", ((f64)last_cycle_time) / 1e6);
  // printf("    delta: %lf\n", (f64)((i64)last_vblank_time - (i64)_ctx->target_present_time) / 1e6);

  const u64 render_latency = _ctx->last_render_duration;
  /*
  in order to know how much we actually failed at predicting we need to subtract
  our preemptive extra time last_present_render_time from the delta
  the higher this number is the earlier we tried to predict (incorrectly)
  */
  i64 corrected_delta = (i64)last_vblank_time - ((i64)_ctx->target_present_time + (i64)_ctx->last_present_render_time);

  // printf("corrected delta: %lf\n", ((f64)corrected_delta) / 1e6);

  const u64 time = RGFW_getTimeNS();

  /* is our fps too low to actually frame pace? */
  if (time + render_latency >= last_vblank_time + last_cycle_time) {
    /* just present immediatly */
    const u64 next_optimal_present_time = RGFW_getTimeNS();
    _ctx->target_present_time = next_optimal_present_time;
  } else {
    const u64 next_optimal_present_time = last_vblank_time + last_cycle_time - render_latency - 550000;
    // printf("next_optimal_present_time: %lf\n", ((f64)next_optimal_present_time) / 1e6);
    _ctx->target_present_time = next_optimal_present_time;
    _ctx->last_present_render_time = render_latency;
  }
  _ctx->updated_target = true;
}