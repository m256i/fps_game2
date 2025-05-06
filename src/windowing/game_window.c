#include <windowing/game_window.h>
#define RGFW_EXPOSE_NATIVE_WIN32
#define RGFW_VULKAN
#undef RGFW_OPENGL
#include <RGFW/RGFW.h>
#include <assert.h>

#ifdef _WIN64
#include <windows.h>
#endif

static u0 exclusive_fullscreen_hint(HWND hwnd, usize _width, usize _height) {
  SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);
  SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, _width, _height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
  HDC dc = GetDC(hwnd);
  SwapBuffers(dc);
  SwapBuffers(dc);
}

static u0 on_focus_callback(RGFW_window *win, RGFW_bool inFocus) {
  if (inFocus) {
    GAME_LOGF("in focus!");
    exclusive_fullscreen_hint(win->src.window, RGFW_getScreenSize().w, RGFW_getScreenSize().h);
    RGFW_window_setFullscreen(win, RGFW_TRUE);
  }
}

window *create_window(const char *const _name, usize _w, usize _h, bool _fullscreen) {
  window *out_window = malloc(sizeof(window));
  memset(out_window, 0, sizeof *out_window);

  RGFW_window *win;
  RGFW_rect winsize = RGFW_RECT(0, 0, _w, _h);

  out_window->is_fullscreen = _fullscreen;
  out_window->screen_width = _w;
  out_window->screen_height = _h;

  if (_fullscreen) {
#ifdef _WIN64
    DEVMODEA dm = {0};
    dm.dmSize = sizeof(dm);
    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    dm.dmPelsWidth = _w;
    dm.dmPelsHeight = _h;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
    LONG result = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_FULLSCREEN, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL) {
      GAME_CRITICALF("could not change display resolution, error code %x", result);
      exit(1);
    }
#endif
    win = RGFW_createWindow(
        _name, winsize, RGFW_windowFullscreen | RGFW_windowNoResize | RGFW_windowCenterCursor | RGFW_windowNoInitAPI);
    RGFW_setFocusCallback(on_focus_callback);
    exclusive_fullscreen_hint(win->src.window, _w, _h);
  } else {
    win = RGFW_createWindow(_name, winsize, RGFW_windowCenter | RGFW_windowCenterCursor | RGFW_windowNoInitAPI);
  }
  out_window->internal_window = win;
  return out_window;
}

u0 destroy_window(window *const _window) {
  assert(_window && _window->internal_window);
  RGFW_window_close(_window->internal_window);
  free(_window);
}

bool window_should_close(const window *const _window) {
  assert(_window && _window->internal_window);
  return RGFW_window_shouldClose(_window->internal_window) == RGFW_TRUE;
}

u0 window_check_events(const window *const _window) {
  assert(_window && _window->internal_window);
  const i32 event_sleep_time_ms = 0;
  RGFW_window_checkEvents(_window->internal_window, event_sleep_time_ms);
}

u0 window_swap_buffers(const window *const _window) {
  assert(_window && _window->internal_window);
  RGFW_window_swapBuffers(_window->internal_window);
}