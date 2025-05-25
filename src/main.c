#include "renderer/gl_api.h"
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
  "layout(location = 0) in vec3 aPos;\n"
  "layout(location = 1) in vec2 aTexCoords;\n"
  "out vec2 oTexCoords;\n"
  "uniform float OffsetX;"
  "uniform float OffsetY;"
  "void main() {\n"
  "    oTexCoords = aTexCoords;\n"
  "    gl_Position = vec4(aPos.x + OffsetX, aPos.y + OffsetY, 0.0, 1.0);\n"
  "}\n";

const char *fragment_shader_src = "#version 460 core\n"
                                  "out vec4 FragColor;\n"
                                  "in vec2 oTexCoords;"
                                  "uniform sampler2D tex;"
                                  "void main() {\n"
                                  "    FragColor = texture(tex, oTexCoords);\n"
                                  "}\n";

GLint  offsetX_loc;
GLint  offsetY_loc;
u64    rendered_frames = 0;
GLuint program;

gl_resource_handle rh = {0}, tex_handle = {0};

bool render(u0) {
  f64        old_time    = RGFW_getTimeNS();
  float      time        = (float)RGFW_getTimeNS() / 1e9 * 10;
  float      offset      = sinf(time * 1) * 0.5f;
  /* p100 fakelag */
  // Sleep(6);
  RGFW_point mousepos    = RGFW_getGlobalMousePoint();
  float      mousepos_x  = (f32)mousepos.x / RGFW_getScreenSize().w;
  float      mousepos_y  = (f32)mousepos.y / RGFW_getScreenSize().h;
  mousepos.y            /= RGFW_getScreenSize().h;

  GL_CALL(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
  GL_CALL(glUseProgram(program));
  GL_CALL(glUniform1f(offsetX_loc, mousepos_x * 2.f - 1));
  GL_CALL(glUniform1f(offsetY_loc, 1.f - mousepos_y * 2.f));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_handle->internal_handle));
  GL_CALL(glBindVertexArray(rh->internal_handle));
  GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

  f64 ct   = RGFW_getTimeNS();
  // f64 dt = ct - old_time;
  old_time = ct;

  if (rendered_frames % 300 == 0) {
    // printf("render fps: %llu\n", (usize)(1e9 / dt));
    printf(
      "last input->photon latency: %lfms\n",
      ((f64)window_get_last_input_to_photon_latency()) / 1e6
    );
  }

  ++rendered_frames;
  return true;
}

/* REALLY IMPORTANT FUNCTION DO NOT REMOVE! */
u32 *make_mandelbrot_image(u0) {
  const static usize WIDTH  = 2000;
  const static usize HEIGHT = 1000;

  u32 *img = malloc(sizeof(u32) * WIDTH * HEIGHT);
  if (!img) return NULL;

  const f64 xmin = -2.0, xmax = 1.0;
  const f64 ymin = -1.0, ymax = 1.0;

  for (usize j = 0; j < HEIGHT; ++j) {
    f64 ci = ymin + (ymax - ymin) * j / (HEIGHT - 1);
    for (usize i = 0; i < WIDTH; ++i) {
      f64 cr = xmin + (xmax - xmin) * i / (WIDTH - 1);
      f64 zr = 0.0, zi = 0.0;
      i32 it = 0;
      while (zr * zr + zi * zi <= 4.0 && it < 1000) {
        f64 tmp = zr * zr - zi * zi + cr;
        zi      = 2.0 * zr * zi + ci;
        zr      = tmp;
        ++it;
      }
      u8 c               = (u8)(255 * it / (1000 - 1));
      img[j * WIDTH + i] = (c << 24) | (c << 16) | (c << 8) | 0xFF;
    }
  }
  return img;
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
  /* wont get printed in release mode */
  GAME_LOGF("engine (ver: " GAME_CLIENT_VER_STR ")  running in debug mode");

  // unit_test_mathlib();

  init_mathlib();

  create_gl_context();
  create_global_window("game client", 0, 0, RENDER_MODE_FRAME_PACE_EXP);

  float vertices[] = {
    0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
  };

  unsigned int indices[] = {0, 1, 3, 1, 2, 3};

  u32 *testImageData = make_mandelbrot_image();

  // clang-format off
  gl_resource_data rd = (gl_resource_data){
   .desc.vertex_buffer = {
     .creation_info_type = RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
     .buffer_usage       = GL_STATIC_DRAW,
     .vertex_attributes  = (vertex_attribute_info[]){
        (vertex_attribute_info){
          .attribute_type   = GL_FLOAT,
          .attribute_count  = 3,
          .attribute_index  = 0
        },
        (vertex_attribute_info){
          .attribute_type   = GL_FLOAT,
          .attribute_count  = 2,
          .attribute_index  = 1
        }
       },
     .num_attributes      = 2,
     .raw_size            = sizeof(vertices),
     .vertex_data         = (u8*)vertices,
     .index_data          = (u8*)indices,
     .index_count         = 6,
     .index_type          = GL_UNSIGNED_INT
    },
   .resource_name = "my_vertex_buffer0"
  };

  gl_resource_data rd2 = (gl_resource_data){
    .desc.image_texture = {
      .creation_info_type = RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE,
      .image_path = "F:\\fps_game2\\0001.png",
      .compress = false,
      .scale = 0.1f,
      .wrap_mode = GL_REPEAT
    },
    .resource_name = "my_tex",
  };

  gl_resource_data rd3 = (gl_resource_data){
    .resource_name = "test_textue",
    .desc.texture = {
      .creation_info_type = RESOURCE_CREATION_INFO_TYPE_TEXTURE,
      .width = 2000,
      .height = 1000,
      .format = GL_RGBA,
      .internal_format = GL_RGBA8,
      .wrap_mode = GL_REPEAT,
      .image_data = (u8*)testImageData,
      .compress = true
    }
  };
  // clang-format on

  request_gl_resource(&rd, &rh);
  // request_gl_resource(&rd2, &tex_handle);
  request_gl_resource(&rd3, &tex_handle);

  GAME_LOGF(
    "texture dimensions after load: %lu %lu",
    rd2.desc.image_texture.width,
    rd2.desc.image_texture.height
  );

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

  destroy_gl_resource(&rd, &rh);
  destroy_gl_resource(&rd3, &tex_handle);

  glDeleteProgram(program);

  destroy_global_window();
  fclose(lf);

  GAME_LOGF("game exited gracefully");
  return 0;
}
