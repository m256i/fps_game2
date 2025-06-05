#include <glad/glad.h>
#include <RGFW/RGFW.h>
#include <common.h>
#include <log/log.h>
#include <stdio.h>
#include <util/dbg/alloctrack.h>
#include <windowing/game_window.h>
#include <windowing/vulkan_present.h>
#include <math.h>
#include <math/vec3.h>
#include <windowing/frame_pacer.h>
#include <util/abspath.h>
#include <containers/str_hash_table.h>
#include <util/dbg/strace.h>
#include <renderer/gl_resource_manager.h>
#include <gui/debug_menu.h>
#include <gui/nuklear.h>
#include <renderer/gl_api.h>

GLint  offsetX_loc;
GLint  offsetY_loc;
u64    rendered_frames = 0;
GLuint program;

struct nk_context *ctx;

gl_resource_handle rh = {0}, tex_handle = {0}, shader_handle = {0};

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_colorf bg;

bool render(u0) {

  GL_CALL(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
  GL_CALL(
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)
  );

  RGFW_point mousepos    = RGFW_getGlobalMousePoint();
  float      mousepos_x  = (f32)mousepos.x / RGFW_getScreenSize().w;
  float      mousepos_y  = (f32)mousepos.y / RGFW_getScreenSize().h;
  mousepos.y            /= RGFW_getScreenSize().h;

  GL_CALL(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
  GL_CALL(glUseProgram(program));
  GL_CALL(glUniform1f(offsetX_loc, mousepos_x * 2.f - 1));
  GL_CALL(glUniform1f(offsetY_loc, 1.f - mousepos_y * 2.f));
  GL_CALL(glBindVertexArray(rh->internal_storage.vbo.vao_handle));
  GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

  nk_glfw3_new_frame();
  if (nk_begin(
        ctx,
        "Demo",
        nk_rect(50, 50, 230, 250),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
          NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE
      )) {
    enum { EASY, HARD };
    static int op       = EASY;
    static int property = 20;

    nk_layout_row_static(ctx, 30, 80, 1);
    if (nk_button_label(ctx, "button")) GAME_LOGF("button pressed");
    nk_layout_row_dynamic(ctx, 30, 2);

    if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
    if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

    nk_layout_row_dynamic(ctx, 25, 1);
    nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    nk_layout_row_dynamic(ctx, 20, 1);
    nk_label(ctx, "background:", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 25, 1);

    if (nk_combo_begin_color(
          ctx,
          nk_rgb_cf(bg),
          nk_vec2(nk_widget_width(ctx), 400)
        )) {
      nk_layout_row_dynamic(ctx, 120, 1);
      bg = nk_color_picker(ctx, bg, NK_RGBA);
      nk_layout_row_dynamic(ctx, 25, 1);
      bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
      bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
      bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
      bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
      nk_combo_end(ctx);
    }
  }
  nk_end(ctx);

  nk_glfw3_render();

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
  const static usize WIDTH  = 3000;
  const static usize HEIGHT = 2000;

  u32 *img = calloc(sizeof(u32) * WIDTH * HEIGHT, 1);
  if (!img) return NULL;

  const f64 xmin = -2.0, xmax = 1.0;
  const f64 ymin = -1.0, ymax = 1.0;

  for (usize j = 0; j < HEIGHT; ++j) {
    f64 ci = ymin + (ymax - ymin) * j / (HEIGHT - 1);
    for (usize i = 0; i < WIDTH; ++i) {
      f64 cr = xmin + (xmax - xmin) * i / (WIDTH - 1);
      f64 zr = 0.0, zi = 0.0;
      i32 it = 0;
      while (zr * zr + zi * zi <= 4.0 && it < 100) {
        f64 tmp = zr * zr - zi * zi + cr;
        zi      = 2.0 * zr * zi + ci;
        zr      = tmp;
        ++it;
      }
      if (1000 - it < 10) {
        img[j * WIDTH + i] = 0xff010101;
      } else {
        img[j * WIDTH + i] =
          ((it * 2) | ((u32)sqrt(it * it * it)) << 16) | 0xff000000;
      }
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

  create_gl_context();
  create_global_window("game client", 0, 0, RENDER_MODE_VSYNC);

  float vertices[] = {
    0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
  };

  unsigned int indices[]     = {0, 1, 3, 1, 2, 3};
  u32         *testImageData = make_mandelbrot_image();

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
      .width = 3000,
      .height = 2000,
      .format = GL_RGBA,
      .internal_format = GL_RGBA8,
      .wrap_mode = GL_REPEAT,
      .image_data = (u8*)testImageData,
      .compress = false
    }
  };

  char* vp = make_abs_path("../game_data/shaders/test1.vs");
  char* fp = make_abs_path("../game_data/shaders/test1.fs");

  gl_resource_data rd4 = (gl_resource_data){
    .resource_name = "myShader1",
    .desc.shader = {
      .creation_info_type = RESOURCE_CREATION_INFO_TYPE_SHADER,
      .vertex_path    = vp,
      .fragment_path  = fp,
    }
  };

  // clang-format on

  request_gl_resource(&rd, &rh);

  // request_gl_resource(&rd2, &tex_handle);
  request_gl_resource(&rd3, &tex_handle);
  request_gl_resource(&rd4, &shader_handle);

  TRACKED_FREE(vp);
  TRACKED_FREE(fp);

  /*
  TODO:
    -- implement resource handle shit
    -- make internal functions have internal linkage
    -- add namespace prefixes to functions
  */

  free(testImageData);

  GAME_LOGF(
    "texture dimensions after load: %lu %lu",
    rd2.desc.image_texture.width,
    rd2.desc.image_texture.height
  );

  ctx = nk_glfw3_init(
    get_global_internal_window(),
    NK_GLFW3_INSTALL_CALLBACKS,
    MAX_VERTEX_BUFFER,
    MAX_ELEMENT_BUFFER
  );

  {
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin((void **)&atlas);
    nk_glfw3_font_stash_end();
  }

  program = (GLuint)shader_handle->internal_storage.shader;
  printf("program %u\n", program);

  // GL_CALL(glProgramUniformHandleui64ARB(
  //   program,
  //   glGetUniformLocation(program, "tex"),
  //   *(GLuint64 *)rd3.impl_storage
  // ));

  offsetX_loc = glGetUniformLocation(program, "OffsetX");
  offsetY_loc = glGetUniformLocation(program, "OffsetY");

  window_set_render_proc(render);

  while (!window_should_close()) {
    window_run_render_proc();
  }

  destroy_gl_resource(&rd, &rh);
  destroy_gl_resource(&rd3, &tex_handle);
  destroy_gl_resource(&rd4, &shader_handle);

  destroy_global_window();
  fclose(lf);

  GAME_LOGF("game exited gracefully");
  return 0;
}
