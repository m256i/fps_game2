#ifndef WINDOWING_FRAME_PACER_H_
#define WINDOWING_FRAME_PACER_H_

#include <ddraw.h>
#include <common.h>
#include <stdatomic.h>
#include <dxgi.h>
#include <tdh.h>
#include <evntrace.h>

#include <containers/spsc_u64.h>

typedef struct {
  f64  ewma_mean;   /* estimated mean render time (ns)        */
  f64  ewma_m2;     /* second moment for variance estimation  */
  bool initialized; /* first-sample flag                      */
} render_timing_data;

// clang-format off

u0  init_render_timing_data     (render_timing_data *data);
u0  update_render_timing_data   (render_timing_data *const _data, u64 _newtime_ns);
u64 get_average_render_latency  (render_timing_data *const _data);

// clang-format on

typedef struct {
  /* keep these on seperate cache lines:
    pacer thread will write to this */
  struct __attribute__((aligned(64))) {
    IDXGIOutput       *output;
    /* the last time the monitor vblanked */
    _Atomic u64        last_vblank_time;
    /* next times we will render/present */
    spsc_u64_16_ring_t present_queue;
    spsc_u64_16_ring_t render_queue;
  };
  struct __attribute__((aligned(64))) {
    spsc_u64_16_ring_t flip_queue;
  };
  /* pacer will only read from this */
  struct __attribute__((aligned(64))) {
    IDXGIAdapter1          *adapter;
    /* signal a stop to the pacer thread */
    _Atomic bool            stop_flag;
    u64                     last_render_duration;
    u64                     last_present_duration;
    render_timing_data      render_timing_data;
    render_timing_data      present_timing_data;
    u64                     last_render_start_time_stamp;
    u64                     last_input_to_photon_latency;
    HANDLE                  pacer_thread_handle;
    bool                    initialized;
    /* windows ETW stuff */
    TRACEHANDLE             s_hTrace;
    TRACEHANDLE             s_hSession;
    EVENT_TRACE_PROPERTIES *etw_props;
    HANDLE                  etw_listener_handle;
  };
} frame_pacer_context;

// clang-format off

u0    initialize_frame_pacer            (frame_pacer_context *const _ctx);
u0    destroy_frame_pacer               (frame_pacer_context *const _ctx);

u0    schedule_next_render_and_present  (frame_pacer_context *const _ctx, u32 _refresh_rate);
bool  should_render                     (frame_pacer_context *const _ctx);
bool  should_present                    (frame_pacer_context *const _ctx);

u0    start_tracking_render_time        (frame_pacer_context *const _ctx);
u0    stop_tracking_render_time         (frame_pacer_context *const _ctx);
u0    start_tracking_present_time       (frame_pacer_context *const _ctx);
u0    stop_tracking_present_time        (frame_pacer_context *const _ctx);

/* internals for event tracing */
VOID WINAPI EventRecordCallback(EVENT_RECORD *pEvent);
ULONG WINAPI BufferCallback(EVENT_TRACE_LOGFILEW *);

// clang-format on
#endif // WINDOWING_FRAME_PACER_H_
