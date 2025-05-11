#include <stdio.h>
#include <RGFW/RGFW.h>
#include <log/log.h>
#include <common.h>
#include <windowing/game_window.h>

#include <glad/glad.h>
#include <windowing/vulkan_present.h>
#include <wgl/glad_wgl.h>

#include <math.h>
#include <ddraw.h>

#include <math/vector.h>

#include <windowing/frame_pacer.h>

const char *vertex_shader_src = "#version 460 core\n"
                                "layout(location = 0) in vec2 aPos;\n"
                                "uniform float OffsetX;"
                                "void main() {\n"
                                "    gl_Position = vec4(aPos.x + OffsetX, aPos.y, 0.0, 1.0);\n"
                                "}\n";

const char *fragment_shader_src = "#version 460 core\n"
                                  "out vec4 FragColor;\n"
                                  "void main() {\n"
                                  "    FragColor = vec4(0.2, 0.6, 1.0, 1.0);\n"
                                  "}\n";

int main() {
  FILE *lf = fopen("./game_logs.txt", "w");
  if (!lf) {
    puts("CRITICAL ERROR: unable to open log-file! exiting");
    exit(1);
  }
  log_add_fp(lf, LOG_TRACE);
  GAME_LOGF("initialising game client version %s", GAME_CLIENT_VER_STR);

  // unit_test_mathlib();

  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

  RGFW_window *tmp_window = RGFW_createWindow("opengl_context_window", RGFW_RECT(0, 0, 1, 1), RGFW_windowHide);
  RGFW_window_makeCurrent_OpenGL(tmp_window);
  RGFW_window_hide(tmp_window);

  if (gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress) == 0) {
    GAME_CRITICALF("failed to initialize glad! exiting.");
    exit(1);
  }

  RGFW_area monitor_size = RGFW_getScreenSize();
  GAME_LOGF("using screen space: (%d, %d)", monitor_size.w, monitor_size.h);

  window *pwin = create_window("game client", monitor_size.w, monitor_size.h, true);
  GAME_LOGF("OpenGL version: %s", glGetString(GL_VERSION));

  vk_context vk_ctx;
  initialize_vulkan_context(&vk_ctx, pwin->internal_window->src.window, pwin->screen_width, pwin->screen_height);

  frame_pacer_context *fpc = malloc(sizeof(frame_pacer_context));
  initialize_frame_pacer(fpc, pwin->internal_window->src.window);

  float vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};

  GLuint vao, vbo;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glCreateBuffers(1, &vbo);
  glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, 2 * sizeof(float));
  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao, 0, 0);

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader_src, NULL);
  glCompileShader(vs);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader_src, NULL);
  glCompileShader(fs);
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint offset_loc = glGetUniformLocation(program, "OffsetX");
  f64 old_time = 0;

  u64 rendered_frames = 0;

  sync_cycle_start(fpc);
  while (!window_should_close(pwin)) {
    update_target_frame(fpc);
    // resync_cycle_start(fpc);

    f64 ct = RGFW_getTimeNS() * 1e-9;
    f64 dt = ct - old_time;
    old_time = ct;

    if (rendered_frames % 5000 == 0) { printf("fps: %llu\n", (usize)(1.0 / dt)); }

    window_check_events(pwin);
    bind_vulkan_surface(&vk_ctx);

    float time = (float)RGFW_getTimeNS() * 0.000000005;
    float offset = sinf(time) * 0.5f;

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glUniform1f(offset_loc, offset);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    rendered_frames++;

    // const u64 cycle_delta_ns = (RGFW_getTimeNS() - start_time) % average_refresh_time;

    // GAME_LOGF("average refresh time: %llu", average_refresh_time);
    // GAME_LOGF("cycle delta: %llu", cycle_delta_ns);
    // GAME_LOGF("(f64)cycle_delta_ns / (f64)average_refresh_time: %llu", (f64)cycle_delta_ns /
    // (f64)average_refresh_time);

    if (monitor_pace(fpc, 1080, 144)) {
      // const u64 pre = RGFW_getTimeNS();
      //      update_optimal_scanline(fpc, 1080, 144, RGFW_getTimeNS() - pre);
      vulkan_present(&vk_ctx);
    }
  }

  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  destroy_frame_pacer(fpc);
  destroy_vulkan_context(&vk_ctx);

  puts("done");

  destroy_window(pwin);
  return 0;
}