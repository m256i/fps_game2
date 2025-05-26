#ifndef WINDOWING_GAME_WINDOW_H_
#define WINDOWING_GAME_WINDOW_H_

#include <RGFW/RGFW.h>
#include <common.h>

#ifdef _WIN64
#include <windowing/vulkan_present.h>
#include <windowing/frame_pacer.h>
#endif

typedef bool (*WINDOW_RENDER_PROC)(void);

typedef struct {
#ifdef _WIN64
  vk_context          vk_ctx;
  frame_pacer_context fpc;
#endif
  RGFW_window        *internal_window;
  usize               screen_width, screen_height;
  u32                 refresh_rate;
  u32                 render_mode;
  WINDOW_RENDER_PROC  render_proc;
  const char         *window_name;
  bool                initialized;
  bool                in_focus;
} window;

// clang-format off

u0    create_gl_context                         (u0);
u0    create_global_window                      (const char *const _name, usize _w, usize _h, u32 _render_mode);
u0    destroy_global_window                     (u0);

u0    window_set_size                           (usize _w, usize _h);
u0    window_set_render_proc                    (WINDOW_RENDER_PROC _proc);
u0    window_set_render_mode                    (u32 _newmode);
u0    window_run_render_proc                    (u0);
bool  window_should_close                       (u0);
u64   window_get_last_input_to_photon_latency   (u0);

// clang-format on
#endif // WINDOWING_GAME_WINDOW_H_
