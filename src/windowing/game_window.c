#include <windowing/game_window.h>
#define RGFW_EXPOSE_NATIVE_WIN32
#define RGFW_VULKAN
#undef RGFW_OPENGL
#include <RGFW/RGFW.h>
#include <assert.h>

#include <util/dbg/alloctrack.h>

#ifdef _WIN64
#include <windows.h>
#endif

static u0 exclusive_fullscreen_hint(HWND hwnd, usize _width, usize _height) {
  SetWindowLongPtr(hwnd, GWL_STYLE,
                   WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);
  SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, _width, _height,
               SWP_FRAMECHANGED | SWP_SHOWWINDOW);
  HDC dc = GetDC(hwnd);
  SwapBuffers(dc);
  SwapBuffers(dc);
}

static window *global_window;

static u0 on_focus_callback(RGFW_window *win, RGFW_bool inFocus) {
  GAME_LOGF("callback called");
  if (inFocus && global_window && !global_window->in_focus) {
    GAME_LOGF("in focus after not");
    if (global_window->initialized) {
      SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

      DEVMODEA dm = {0};
      dm.dmSize = sizeof(dm);
      EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
      dm.dmPelsWidth = global_window->screen_width;
      dm.dmPelsHeight = global_window->screen_height;
      dm.dmFields =
          DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
      LONG result =
          ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_FULLSCREEN, NULL);
      if (result != DISP_CHANGE_SUCCESSFUL) {
        GAME_CRITICALF("could not change display resolution, error code %x",
                       result);
        exit(1);
      }

      GAME_LOGF("OpenGL version: %s", glGetString(GL_VERSION));

      RGFW_window_setFullscreen(win, RGFW_TRUE);

      exclusive_fullscreen_hint(win->src.window, global_window->screen_width,
                                global_window->screen_height);
      initialize_frame_pacer(&global_window->fpc);

      global_window->in_focus = true;

      GAME_LOGF("recreating window with bounds: %u %u\n",
                global_window->screen_width, global_window->screen_height);
      initialize_vulkan_context(
          &global_window->vk_ctx, win->src.window, global_window->screen_width,
          global_window->screen_height, global_window->render_mode);
    }
  } else if (!inFocus && global_window && global_window->in_focus) {
    GAME_LOGF("out of focus");
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    global_window->in_focus = false;

    if (global_window->initialized) {
      GAME_LOGF("destroying surface");
      destroy_frame_pacer(&global_window->fpc);
      destroy_vulkan_context(&global_window->vk_ctx);
    }
  }
}

u0 APIENTRY debug_callback(GLenum source, GLenum type, GLuint id,
                           GLenum severity, GLsizei length,
                           const GLchar *message, const void *userParam) {
  GAME_LOGF("GL_MESSAGE: %s type = 0x%x, severity = 0x%x, message = %s",
            (type == GL_DEBUG_TYPE_ERROR         ? "** GL_ERROR **"
             : GL_DEBUG_TYPE_PERFORMANCE         ? "GL_DEBUG_TYPE_PERFORMANCE"
             : GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  ? "GL_DEBUG_TYPE_UB"
             : GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "GL_DEBUG_TYPE_DEPRECATED"
                                                 : ""),
            type, severity, message);
}

u0 create_gl_context(u0) {
  RGFW_window *tmp_window = RGFW_createWindow(
      "opengl_context_window", RGFW_RECT(0, 0, 1, 1), RGFW_windowHide);
  RGFW_window_makeCurrent_OpenGL(tmp_window);
  RGFW_window_hide(tmp_window);

  if (gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress) == 0) {
    GAME_CRITICALF("failed to initialize glad! exiting.");
    exit(1);
  }

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, 0);
#endif

  GAME_LOGF("OpenGL version: %s", glGetString(GL_VERSION));
}

u0 create_global_window(const char *const _name, usize _w, usize _h,
                        u32 _render_mode) {
  global_window = TRACKED_MALLOC(sizeof(window));
  memset(global_window, 0, sizeof *global_window);

  RGFW_window *win;
  RGFW_area prim_monitor_size = RGFW_getScreenSize();
  if (_w == 0) {
    _w = prim_monitor_size.w;
  }
  if (_h == 0) {
    _h = prim_monitor_size.h;
  }

  GAME_LOGF("using screen space: (%d, %d)", _w, _h);

  RGFW_rect winsize = RGFW_RECT(0, 0, _w, _h);

  global_window->window_name = _name;

  global_window->screen_width = _w;
  global_window->screen_height = _h;

  global_window->in_focus = true;
  global_window->render_mode = _render_mode;

  if (_render_mode == RENDER_MODE_FRAME_PACE_EXP ||
      _render_mode == RENDER_MODE_FRAME_PACE_EXP) {
    GAME_CRITICALF("using either RENDER_MODE_FRAME_PACE_EXP or "
                   "RENDER_MODE_FRAME_PACE_EXP remember to turn off G-Sync");
  }

  memset(&global_window->fpc, 0, sizeof global_window->fpc);
  memset(&global_window->vk_ctx, 0, sizeof global_window->vk_ctx);

  DEVMODEA dm = {0};
  dm.dmSize = sizeof(dm);
  EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
  dm.dmPelsWidth = _w;
  dm.dmPelsHeight = _h;
  dm.dmFields =
      DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
  LONG result = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_FULLSCREEN, NULL);
  if (result != DISP_CHANGE_SUCCESSFUL) {
    GAME_CRITICALF("could not change display resolution, error code %x",
                   result);
    exit(1);
  }

  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  win = RGFW_createWindow(_name, winsize,
                          RGFW_windowFullscreen | RGFW_windowNoResize |
                              RGFW_windowCenterCursor | RGFW_windowNoInitAPI);

  RGFW_window_setFullscreen(win, RGFW_TRUE);
  exclusive_fullscreen_hint(win->src.window, _w, _h);

  initialize_vulkan_context(&global_window->vk_ctx, win->src.window, _w, _h,
                            _render_mode);
  initialize_frame_pacer(&global_window->fpc);

  RGFW_setFocusCallback(on_focus_callback);

  global_window->refresh_rate = RGFW_getPrimaryMonitor().mode.refreshRate;
  global_window->initialized = true;
  global_window->internal_window = win;
}

u0 window_set_render_proc(WINDOW_RENDER_PROC _proc) {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  global_window->render_proc = _proc;
}

u0 window_set_render_mode(u32 _newmode) {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  u32 old_rendermode = global_window->render_mode;
  global_window->render_mode = _newmode;
  /* reinitialize window and vulkan context */
  if (old_rendermode != _newmode) {
    window_set_size(global_window->screen_width, global_window->screen_height);
  }
}

u0 window_set_size(usize _w, usize _h) {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  const char *old_name = global_window->window_name;
  u32 old_render_mode = global_window->render_mode;
  destroy_global_window();
  create_global_window(old_name, _w, _h, old_render_mode);
}

u0 destroy_global_window() {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  destroy_frame_pacer(&global_window->fpc);
  destroy_vulkan_context(&global_window->vk_ctx);
  RGFW_window_close(global_window->internal_window);
  TRACKED_FREE(global_window);
}

bool window_should_close(u0) {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  return RGFW_window_shouldClose(global_window->internal_window) == RGFW_TRUE;
}

static u0 window_check_events(u0) {
  GAME_ASSERT(global_window && global_window->internal_window &&
              global_window->initialized);
  const i32 event_sleep_time_ms = 0;
  RGFW_window_checkEvents(global_window->internal_window, event_sleep_time_ms);
}

u0 window_run_render_proc(u0) {
  window_check_events();
  if (!global_window->in_focus) {
    return;
  }
  switch (global_window->render_mode) {
  case RENDER_MODE_FRAME_PACE_EXP: {
    frame_pacer_context *const fpc = &global_window->fpc;
    schedule_next_render_and_present(fpc, global_window->refresh_rate);

    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    if (should_render(fpc)) {
      start_tracking_render_time(fpc);
      {
        bind_vulkan_surface(&global_window->vk_ctx);
        /* render */
        global_window->render_proc();
      }
      start_tracking_render_time(fpc);
    }
    if (should_present(fpc)) {
      start_tracking_present_time(fpc);
      {
        /* present */
        vulkan_present(&global_window->vk_ctx);
      }
      stop_tracking_present_time(fpc);
    }
    break;
  }
  case RENDER_MODE_VSYNC:
  case RENDER_MODE_IMMEDIATE: {
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    bind_vulkan_surface(&global_window->vk_ctx);
    global_window->render_proc();
    vulkan_present(&global_window->vk_ctx);
    break;
  }
  }
}

u64 window_get_last_input_to_photon_latency(u0) {
  return global_window->fpc.last_input_to_photon_latency;
}
