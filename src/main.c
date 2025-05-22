#include <RGFW/RGFW.h>
#include <common.h>
#include <log/log.h>
#include <stdio.h>
#include <util/dbg/alloctrack.h>

#include <windowing/game_window.h>

#include <glad/glad.h>
#include <wgl/glad_wgl.h>
#include <windowing/vulkan_present.h>

#include <ddraw.h>
#include <math.h>

#include <math/vec3.h>

#include <windowing/frame_pacer.h>

#include <containers/str_hash_table.h>
// #include <renderer/gl_resource_manager.h>

#include <util/dbg/strace.h>

#include <renderer/gl_resource_manager.h>

const char *vertex_shader_src =
    "#version 460 core\n"
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

gl_resource_handle rh = {0};

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
  glBindVertexArray(rh->internal_handle);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  // Sleep(5);

  f64 ct = RGFW_getTimeNS();
  // f64 dt = ct - old_time;
  old_time = ct;

  if (rendered_frames % 300 == 0) {
    // printf("render fps: %llu\n", (usize)(1e9 / dt));
    printf("last input->photon latency: %lfms\n",
           ((f64)window_get_last_input_to_photon_latency()) / 1e6);
  }

  ++rendered_frames;
  return true;
}

int main(u0) {
  setup_stacktrace();

  FILE *lf = fopen("./game_logs.txt", "w");
  if (!lf) {
    puts("CRITICAL ERROR: unable to open log-file! exiting");
    exit(1);
  }
  log_add_fp(lf, LOG_TRACE);
  GAME_LOGF("initialising game client version %s", GAME_CLIENT_VER_STR);

  // unit_test_mathlib();

  init_mathlib();

  vec3 a = make_vec3(1, 2, 3);
  vec3 b = make_vec3(3, 2, 1);

  volatile vec3 n0 = vec3_normalize(&a);
  volatile vec3 n1 = vec3_normalize(&b);

  printf("n0: %f %f %f\n", n0.x, n0.y, n0.z);
  printf("n1: %f %f %f\n", n1.x, n1.y, n1.z);

  volatile vec3 n2 = vec3_normalize_safe(&a);
  volatile vec3 n3 = vec3_normalize_safe(&b);

  printf("n2: %f %f %f\n", n2.x, n2.y, n2.z);
  printf("n3: %f %f %f\n", n3.x, n3.y, n3.z);

  create_gl_context();
  create_global_window("game client", 0, 0, RENDER_MODE_FRAME_PACE_EXP);

  // // clang-format on
  // gl_resource_handle rh  = {0};
  // gl_resource_handle rh1 = {0};
  // request_gl_resource(&rd, &rh);
  // request_gl_resource(&rd, &rh1);
  // printf("rh internal %u\n", rh->internal_handle);
  // printf("rh index %u\n", rh->hashed_resource_index);
  // request_gl_resource(&rd, &rh);
  // destroy_gl_resource(&rd, &rh);
  // destroy_gl_resource(&rd, &rh);
  // destroy_gl_resource(&rd, &rh1);
  // request_gl_resource(&rd, &rh);

  const f32 vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};

  // clang-format off
  gl_resource_data rd = (gl_resource_data){
   .desc.vertex_buffer = {
     .creation_info_type = RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
     .buffer_usage       = GL_STATIC_DRAW,
     .vertex_attributes  = (vertex_attribute_info[]){
       (vertex_attribute_info){
         .attribute_type = GL_FLOAT,
         .attribute_count = 2,
         .attribute_index = 0}
       },
     .num_attributes     = 1,
     .raw_size           = sizeof(vertices),
     .vertex_data        = (u8*)vertices
    },
   .resource_name = "my_vertex_buffer0"
  };
  // clang-format on

  request_gl_resource(&rd, &rh);

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

  destroy_global_window();
  fclose(lf);

  GAME_LOGF("game exited gracefully");
  return 0;
}
