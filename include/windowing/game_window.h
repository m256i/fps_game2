#ifndef WINDOWING_GAME_WINDOW_H_
#define WINDOWING_GAME_WINDOW_H_

#include <RGFW/RGFW.h>
#include <common.h>

typedef struct {
  RGFW_window *internal_window;
  usize screen_width, screen_height;
  bool is_fullscreen;
} window;

window *create_window(const char *const _name, usize _w, usize _h, bool fullscreen);
u0 destroy_window(window *const _window);
bool window_should_close(const window *const _window);
u0 window_check_events(const window *const _window);
u0 window_swap_buffers(const window *const _window);
#endif // WINDOWING_GAME_WINDOW_H_