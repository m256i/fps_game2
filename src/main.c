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
                                "uniform float OffsetY;"
                                "void main() {\n"
                                "    gl_Position = vec4(aPos.x + OffsetX, aPos.y + OffsetY, 0.0, 1.0);\n"
                                "}\n";

const char *fragment_shader_src = "#version 460 core\n"
                                  "out vec4 FragColor;\n"
                                  "void main() {\n"
                                  "    FragColor = vec4(0.2, 0.6, 1.0, 1.0);\n"
                                  "}\n";

GLint offsetX_loc;
GLint offsetY_loc;
u64 rendered_frames = 0;
GLuint vao, vbo;
GLuint program;

bool render(u0) {
  f64 old_time = RGFW_getTimeNS();
  float time = (float)RGFW_getTimeNS() / 1e9 * 10;
  float offset = sinf(time * 1) * 0.5f;
  /* p100 fakelag */
  // Sleep(6);
  RGFW_point mousepos = RGFW_getGlobalMousePoint();
  float mousepos_x = (f32)mousepos.x / RGFW_getScreenSize().w;
  float mousepos_y = (f32)mousepos.y / RGFW_getScreenSize().h;
  mousepos.y /= RGFW_getScreenSize().h;

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(program);
  glUniform1f(offsetX_loc, mousepos_x * 2.f - 1);
  glUniform1f(offsetY_loc, 1.f - mousepos_y * 2.f);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  f64 ct = RGFW_getTimeNS();
  f64 dt = ct - old_time;
  old_time = ct;

  if (rendered_frames % 300 == 0) {
    // printf("render fps: %llu\n", (usize)(1e9 / dt));
    printf("last input->photon latency: %lfms\n", ((f64)window_get_last_input_to_photon_latency()) / 1e6);
  }

  rendered_frames++;
  return true;
}

int main() {
  FILE *lf = fopen("./game_logs.txt", "w");
  if (!lf) {
    puts("CRITICAL ERROR: unable to open log-file! exiting");
    exit(1);
  }
  log_add_fp(lf, LOG_TRACE);
  GAME_LOGF("initialising game client version %s", GAME_CLIENT_VER_STR);

  // unit_test_mathlib();

  create_gl_context();
  create_global_window("game client", 0, 0, RENDER_MODE_FRAME_PACE_EXP);

  float vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};

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
  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  offsetX_loc = glGetUniformLocation(program, "OffsetX");
  offsetY_loc = glGetUniformLocation(program, "OffsetY");

  window_set_render_proc(render);

  while (!window_should_close()) {
    window_run_render_proc();
  }

  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  puts("done");
  destroy_global_window();
  return 0;
}