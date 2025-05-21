#include <windowing/frame_pacer.h>
#include <windows.h>
#include <common.h>
#include <math.h>
#include <RGFW/RGFW.h>
#include <dxgi.h>
#include <math/interp.h>

#include <windows.h>
#include <conio.h>
#include <tdh.h>
#include <evntrace.h>
#include <stdio.h>
#include <strsafe.h>
#include <objbase.h>

#include <util/dbg/alloctrack.h>

u0 init_render_timing_data(render_timing_data *data) {
  data->ewma_mean   = 0.0;
  data->ewma_m2     = 0.0;
  data->initialized = false;
}

const static f64 EWMA_ALPHA = 0.05; /* smoothing factor for mean and variance */
const static f64 CONFIDENCE_Z = 2.27; /* z-score */

#define LOGFILE_PATH    L"./engine_etw.etl"
#define LOGSESSION_NAME L"game" GAME_CLIENT_VER_STR_L "_etw_consumer"

static u0 destroy_etw(frame_pacer_context *const _ctx) {
  static const wchar_t *const guidStr =
    L"{802EC45A-1E99-4B83-9920-87C98277BA9D}";

  GUID    ProviderGuid;
  HRESULT hr = CLSIDFromString(guidStr, &ProviderGuid);

  if (SUCCEEDED(hr)) {
    wprintf(L"GUID parsed successfully.\n");
  } else {
    wprintf(L"Failed to parse GUID. HRESULT: 0x%lx\n", hr);
    exit(1);
  }

  if (_ctx->s_hSession) {
    EnableTraceEx2(
      _ctx->s_hSession,
      &ProviderGuid,
      EVENT_CONTROL_CODE_DISABLE_PROVIDER,
      TRACE_LEVEL_VERBOSE,
      0,
      0,
      0,
      NULL
    );

    ControlTraceW(
      _ctx->s_hSession,
      LOGSESSION_NAME,
      _ctx->etw_props,
      EVENT_TRACE_CONTROL_STOP
    );
  }
  if (_ctx->s_hTrace && _ctx->s_hTrace != INVALID_PROCESSTRACE_HANDLE) {
    CloseTrace(_ctx->s_hTrace);
  }

  TRACKED_FREE(_ctx->etw_props);
  GAME_LOGF("ETW successfully destroyed");
}

static ULONG etw_listener(LPVOID _ptx) {
  frame_pacer_context *ctx = _ptx;
  ULONG                s   = ProcessTrace(&ctx->s_hTrace, 1, NULL, NULL);
  wprintf(L"ProcessTrace exited: %u\n", s);
  return s;
}

static u0 setup_etw(frame_pacer_context *const _ctx) {
  static const GUID SessionGuid = {
    0xae44cb98,
    0xbd11,
    0x4069,
    {0x80, 0x93, 0x77, 0x0e, 0xc9, 0x25, 0x8a, 0x12}
  };

  _ctx->s_hTrace   = 0;
  _ctx->s_hSession = 0;

  ULONG              status;
  const static ULONG BufferSize = sizeof(EVENT_TRACE_PROPERTIES) +
                                  sizeof(LOGFILE_PATH) +
                                  sizeof(LOGSESSION_NAME);

  _ctx->etw_props = (EVENT_TRACE_PROPERTIES *)TRACKED_MALLOC(BufferSize);
  ZeroMemory(_ctx->etw_props, BufferSize);

  _ctx->etw_props->Wnode.BufferSize    = BufferSize;
  _ctx->etw_props->Wnode.Guid          = SessionGuid;
  _ctx->etw_props->Wnode.ClientContext = 1;
  _ctx->etw_props->Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
  _ctx->etw_props->LogFileMode =
    EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_FILE_MODE_CIRCULAR;

  _ctx->etw_props->MaximumFileSize  = 1;
  _ctx->etw_props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
  _ctx->etw_props->LogFileNameOffset =
    sizeof(EVENT_TRACE_PROPERTIES) + sizeof(LOGSESSION_NAME);

  StringCbCopyW(
    (LPWSTR)((char *)_ctx->etw_props + _ctx->etw_props->LogFileNameOffset),
    sizeof(LOGFILE_PATH),
    LOGFILE_PATH
  );

  StringCbCopyW(
    (LPWSTR)((char *)_ctx->etw_props + _ctx->etw_props->LoggerNameOffset),
    sizeof(LOGSESSION_NAME),
    LOGSESSION_NAME
  );

  status = ControlTraceW(
    0,
    LOGSESSION_NAME,
    _ctx->etw_props,
    EVENT_TRACE_CONTROL_STOP
  );

  if (status == ERROR_SUCCESS) {
    wprintf(L"Successfully stopped the ETW session: %s\n", LOGSESSION_NAME);
  } else if (status == ERROR_WMI_INSTANCE_NOT_FOUND) {
    wprintf(L"ETW session '%s' not found.\n", LOGSESSION_NAME);
  } else {
    wprintf(
      L"Failed to stop ETW session '%s'. Error: 0x%x\n",
      LOGSESSION_NAME,
      status
    );
  }

  status = StartTraceW(&_ctx->s_hSession, LOGSESSION_NAME, _ctx->etw_props);
  if (status != ERROR_SUCCESS) {
    wprintf(L"StartTraceW failed: 0x%x\n", status);
    destroy_etw(_ctx);
    exit(1);
  }

  // {802EC45A-1E99-4B83-9920-87C98277BA9D} for Dxgi-Krnl
  static const wchar_t *const guidStr =
    L"{802EC45A-1E99-4B83-9920-87C98277BA9D}";

  GUID    ProviderGuid;
  HRESULT hr = CLSIDFromString(guidStr, &ProviderGuid);

  if (SUCCEEDED(hr)) {
    wprintf(L"GUID parsed successfully.\n");
  } else {
    wprintf(L"Failed to parse GUID. HRESULT: 0x%lx\n", hr);
    exit(1);
  }

  status = EnableTraceEx2(
    _ctx->s_hSession,
    &ProviderGuid,
    EVENT_CONTROL_CODE_ENABLE_PROVIDER,
    TRACE_LEVEL_VERBOSE,
    -1ull,
    0,
    0,
    NULL
  );

  if (status != ERROR_SUCCESS) {
    wprintf(L"EnableTraceEx2 failed: 0x%x\n", status);
    destroy_etw(_ctx);
    exit(1);
  }

  EVENT_TRACE_LOGFILEW trace = {0};
  trace.LoggerName           = (LPWSTR)LOGSESSION_NAME;
  trace.LogFileName          = NULL;
  trace.ProcessTraceMode     = PROCESS_TRACE_MODE_REAL_TIME |
                           PROCESS_TRACE_MODE_EVENT_RECORD |
                           PROCESS_TRACE_MODE_RAW_TIMESTAMP;

  trace.EventRecordCallback = EventRecordCallback;
  trace.BufferCallback      = BufferCallback;
  trace.Context             = &_ctx->flip_queue;

  _ctx->s_hTrace = OpenTraceW(&trace);
  if (_ctx->s_hTrace == INVALID_PROCESSTRACE_HANDLE) {
    wprintf(L"OpenTraceW failed\n");
    destroy_etw(_ctx);
    exit(1);
  }

  _ctx->etw_listener_handle =
    CreateThread(NULL, 0, etw_listener, _ctx, 0, NULL);
  GAME_LOGF("ETW successfully created");
}

u0 update_render_timing_data(render_timing_data *const _data, u64 _newtime_ns) {
  const f64 x = (f64)_newtime_ns;

  if (!_data->initialized) {
    _data->ewma_mean   = x;
    _data->ewma_m2     = 0.0;
    _data->initialized = true;
    return;
  }

  const f64 delta  = x - _data->ewma_mean;
  _data->ewma_mean = EWMA_ALPHA * x + (1.0 - EWMA_ALPHA) * _data->ewma_mean;
  _data->ewma_m2 =
    (1.0 - EWMA_ALPHA) * (_data->ewma_m2 + EWMA_ALPHA * delta * delta);
}

u64 get_average_render_latency(render_timing_data *const _data) {
  if (!_data->initialized) {
    return 0;
  }
  const f64 sigma     = sqrt(_data->ewma_m2);
  const f64 margin    = _data->ewma_mean + (CONFIDENCE_Z * sigma);
  const u64 margin_ns = (u64)ceil(margin);
  return margin_ns;
}

u0 wait_for_vblank(frame_pacer_context *const _ctx) {
  _ctx->output->lpVtbl->WaitForVBlank(_ctx->output);
}

DWORD WINAPI pacer_thread_proc(_In_ LPVOID _params) {
  GAME_LOGF("... and running!");
  frame_pacer_context *ctx = (frame_pacer_context *)_params;
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
  while (atomic_load_explicit(&ctx->stop_flag, memory_order_relaxed) != true) {
    wait_for_vblank(ctx);
    SetThreadPriorityBoost(GetCurrentThread(), FALSE);
    /* TODO: random stupid offset */
    atomic_store_explicit(
      &ctx->last_vblank_time,
      RGFW_getTimeNS(),
      memory_order_release
    );
  }
  return 0;
}

u0 initialize_frame_pacer(frame_pacer_context *const _ctx) {
  if (_ctx->initialized) {
    return;
  }
  _ctx->adapter     = NULL;
  _ctx->output      = NULL;
  _ctx->initialized = true;

  _ctx->last_render_duration         = 0;
  _ctx->last_present_duration        = 0;
  _ctx->last_render_start_time_stamp = 0;

  init_render_timing_data(&_ctx->render_timing_data);
  init_render_timing_data(&_ctx->present_timing_data);

  spsc_u64_16_init(&_ctx->present_queue);
  spsc_u64_16_init(&_ctx->render_queue);

  atomic_store(&_ctx->last_vblank_time, 0ull);
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

  if (FAILED(_ctx->adapter->lpVtbl->EnumOutputs(_ctx->adapter, 0, &_ctx->output)
      )) {
    _ctx->adapter->lpVtbl->Release(_ctx->adapter);
    _ctx->adapter = NULL;
    factory->lpVtbl->Release(factory);
    GAME_LOGF("failed to enum outputs");
    exit(1);
  }

  factory->lpVtbl->Release(factory);
  DWORD _tid;
  _ctx->pacer_thread_handle =
    CreateThread(NULL, 0, pacer_thread_proc, _ctx, 0, &_tid);
  (u0) _tid;
  GAME_LOGF("pacing thread up...");

  // setup_etw(_ctx);
}

u0 destroy_frame_pacer(frame_pacer_context *const _ctx) {
  if (!_ctx->initialized) {
    return;
  }
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
  // destroy_etw(_ctx);

  // while (WaitForSingleObject(_ctx->etw_listener_handle, 0) != WAIT_OBJECT_0)
  // {
  //   continue;
  // }

  _ctx->s_hTrace   = 0;
  _ctx->s_hSession = 0;

  GAME_LOGF("destroyed frame pacer");
  _ctx->initialized = false;
}

u0 start_tracking_render_time(frame_pacer_context *const _ctx) {
  /* make sure that these are called properly */
  _ctx->last_render_duration         = RGFW_getTimeNS();
  _ctx->last_render_start_time_stamp = _ctx->last_render_duration;
}
u0 stop_tracking_render_time(frame_pacer_context *const _ctx) {
  _ctx->last_render_duration = RGFW_getTimeNS() - _ctx->last_render_duration;
  update_render_timing_data(
    &_ctx->render_timing_data,
    _ctx->last_render_duration
  );
}

u0 start_tracking_present_time(frame_pacer_context *const _ctx) {
  /* make sure that these are called properly */
  _ctx->last_present_duration = RGFW_getTimeNS();
}

u0 stop_tracking_present_time(frame_pacer_context *const _ctx) {
  _ctx->last_present_duration = RGFW_getTimeNS() - _ctx->last_present_duration;
  update_render_timing_data(
    &_ctx->present_timing_data,
    _ctx->last_present_duration
  );
}

/*
schedules the next render and returns in how many vblanks we can present
*/
u0 schedule_next_render_and_present(
  frame_pacer_context *const _ctx,
  u32                        _refresh_rate
) {
  if (!_ctx->initialized) {
    return;
  }

  const u64 screen_cycle_time_ns = (u64)((1000.0 / (f64)_refresh_rate) * 1e6);
  const u64 time                 = RGFW_getTimeNS();

  static u64 previous_vblank = 0;
  u64        last_vblank =
    atomic_load_explicit(&_ctx->last_vblank_time, memory_order_acquire);

  /*
  TODO: make based on measurements
  */
  const u64 vsync_update_error_margin_ns = screen_cycle_time_ns / 4;

  /* do we not have a new vblank? */
  if (last_vblank == previous_vblank && previous_vblank != 0) {
    /* did windows forget to wake up our WaitForVSyncThread? */
    if (time >=
        last_vblank + screen_cycle_time_ns + vsync_update_error_margin_ns) {
      GAME_WARNF("gsync delayed vblank skipping");
    } else {
      return;
    }
  } else {
    // GAME_LOGF("---------- NEW VBLANK ----------");
    // GAME_LOGF("new: %llu old: %llu", last_vblank, previous_vblank);
    previous_vblank = last_vblank;

    const usize fq_sz = spsc_u64_16_size(&_ctx->flip_queue);

    for (usize i = 0; i != fq_sz; i++) {
      u64 flip_rts = 0;
      spsc_u64_16_dequeue(&_ctx->flip_queue, &flip_rts);

      u64 flip_ts =
        ((u64)((f64)flip_rts / (f64)RGFW_getTimerFreq() * 1e9) -
         ((f64)RGFW_getTimerOffsetRaw() / (f64)RGFW_getTimerFreq() * 1e9));
      // GAME_LOGF("flip timestamp received: %llu", flip_ts);
      // GAME_LOGF("time: %llu", RGFW_getTimeNS());
      // GAME_LOGF("timer offset: %llu", RGFW_getTimerOffsetRaw());
      // GAME_LOGF("delta: %lf", ((f64)RGFW_getTimeNS() - (f64)flip_ts) / 1e6);
    }

    /*
    TODO: on low fps these result in always missing frames obviously
    */
    spsc_u64_16_clear(&_ctx->render_queue);
    spsc_u64_16_clear(&_ctx->present_queue);
  }

  _ctx->last_input_to_photon_latency =
    last_vblank - _ctx->last_render_start_time_stamp;

  // GAME_LOGF("time: %llu", time);

  const u64 render_time_estimate =
    get_average_render_latency(&_ctx->render_timing_data);
  const u64 present_time_estimate =
    get_average_render_latency(&_ctx->present_timing_data);
  // TODO: make based on statistics
  const u64 driver_latency_margin = 450000; // 450us
  const u64 timing_error_margin   = 500;    // 0.5us

  // GAME_LOGF("render_time_estimate: %llu", render_time_estimate);
  // GAME_LOGF("present_time_estimate: %llu", present_time_estimate);

  /*
  TODO: on low fps these together with the queue flushing in line 185/186 result
  in always missing frames
  */
  usize target_vblank_offset = 1;
  while (true) {
    const u64 next_vblank =
      last_vblank + (screen_cycle_time_ns * target_vblank_offset);
    /* TODO: if the framerate is hovering at a state where it continuously
    switches between present every nth vsync and present every nth/2 vsync try
    to stay at the lower rate for some time to prevent stuttering
    */
    const i64 next_optimal_present_time =
      (i64)next_vblank - present_time_estimate - driver_latency_margin;
    //   GAME_LOGF("next_optimal_present_time: %lli",
    //   next_optimal_present_time);

    /* margin too tight try to render + present until the next vsync */
    if (next_optimal_present_time <= (i64)time + (i64)timing_error_margin) {
      GAME_WARNF("present margin too tight, trying next vsync");
      if (target_vblank_offset <= 4) {
        target_vblank_offset++;
        continue;
      }
      // GAME_WARNF("present margin too tight, but skipping too many vsyncs,
      // just doing something...");
    }

    const i64 next_optimal_render_time = (i64)next_optimal_present_time -
                                         render_time_estimate -
                                         timing_error_margin;
    // GAME_LOGF("next_optimal_render_time: %lli", next_optimal_render_time);

    if (next_optimal_render_time <= (i64)time + (i64)timing_error_margin) {
      // GAME_WARNF("render margin too tight, trying next vsync");
      if (target_vblank_offset <= 4) {
        target_vblank_offset++;
        continue;
      }
      // GAME_WARNF("render margin too tight, but skipping too many vsyncs, just
      // doing something...");
    }

    spsc_u64_16_enqueue(&_ctx->render_queue, (u64)next_optimal_render_time);
    spsc_u64_16_enqueue(&_ctx->present_queue, (u64)next_optimal_present_time);
    break;
  }
}

bool should_present(frame_pacer_context *const _ctx) {
  if (!_ctx->initialized) {
    return false;
  }
  const u64 time   = RGFW_getTimeNS();
  bool      found  = false;
  u64       target = 0;
  while (spsc_u64_16_peek(&_ctx->present_queue, &target)) {
    if (time >= target) {
      // GAME_LOGF("presented at time: %llu (delta: %llu)", time, time -
      // target);
      spsc_u64_16_dequeue(&_ctx->present_queue, &target);
      found = true;
    } else {
      break;
    }
  }
  return found;
}

bool should_render(frame_pacer_context *const _ctx) {
  if (!_ctx->initialized) {
    return false;
  }
  const u64 time   = RGFW_getTimeNS();
  bool      found  = false;
  u64       target = 0;
  while (spsc_u64_16_peek(&_ctx->render_queue, &target)) {
    if (time >= target) {
      // GAME_LOGF("rendered at time: %llu (delta: %llu)", time, time - target);
      spsc_u64_16_dequeue(&_ctx->render_queue, &target);
      found = true;
    } else {
      break;
    }
  }
  return found;
}
