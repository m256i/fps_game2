#ifndef WINDOWING_FRAME_PACER_H_
#define WINDOWING_FRAME_PACER_H_

#include <ddraw.h>
#include <common.h>
#include <stdatomic.h>
#include <dxgi.h>
#include <containers/p_linked_list.h>

typedef struct {
  /* keep these on seperate cache lines:
    worker thread will write to this
  */
  struct __attribute__((aligned(64))) {
    IDXGIOutput *output;
    /*
    the last time the monitor vblanked
    */
    _Atomic u64 last_vblank_time;
    _Atomic u64 last_monitor_cycle_time; /* how many cycles the last monitor update took */
    _Atomic u64 monitor_frame_count;     /* how many frames the monitor has presented */
  };
  u8 alignment_buffer[64];
  /* worker will only read from this */
  struct __attribute__((aligned(64))) {
    IDXGIAdapter1 *adapter;
    /*
    signal a stop to the pacer thread
    */
    _Atomic bool stop_flag;

    u64 last_render_start_time;
    u64 last_render_duration;
    u64 last_present_start_time;
    u64 last_present_duration;

    u64 target_present_time;

    u64 wait_until_present_frame;
    u64 last_present_render_time;

    bool presented_frame;
    bool updated_target;

    HANDLE pacer_thread_handle;
  };
} frame_pacer_context;

u0 initialize_frame_pacer(frame_pacer_context *const _ctx, HWND _hwnd);
u0 destroy_frame_pacer(frame_pacer_context *const _ctx);
/*
tests if the monitor can be updated
*/
bool monitor_pace(frame_pacer_context *const _ctx, u32 _hres, u32 _refresh_rate);
/*
  [_hres]:                  horizontal resolution of monitor
  [_refresh_rate]:          refresh rate of the monitor in hz
  [_last_present_latency]:  last time it took to call vkQueuePresent() in nanoseconcds
*/
u0 update_optimal_scanline(frame_pacer_context *const _ctx, u32 _hres, u32 _refresh_rate, u64 _last_present_latency);
u0 resync_cycle_start(frame_pacer_context *const _ctx);
u0 sync_cycle_start(frame_pacer_context *const _ctx);

u0 update_target_frame(frame_pacer_context *const _ctx);

inline u0 wait_for_vblank(frame_pacer_context *const _ctx) { _ctx->output->lpVtbl->WaitForVBlank(_ctx->output); }

#endif // WINDOWING_FRAME_PACER_H_