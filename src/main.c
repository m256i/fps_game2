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
#include <renderer/camera.h>

GLint  model_loc;
GLint  view_loc;
GLint  projection_loc;
GLuint program;

struct nk_context *ctx;

gl_resource_handle rh = {0}, tex_handle = {0}, shader_handle = {0};

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_colorf bg;

static game_camera cam;
u0 mouse_cb(RGFW_window *win, RGFW_point point, RGFW_point vector) {
  camera_process_mouse_movement(&cam, vector.x, -vector.y);
}

void print_mat4(f32 data[16]) {
  puts("---------------");
  printf("| %f %f %f %f |\n", data[0], data[1], data[2], data[3]);
  printf("| %f %f %f %f |\n", data[4], data[5], data[6], data[7]);
  printf("| %f %f %f %f |\n", data[8], data[9], data[10], data[11]);
  printf("| %f %f %f %f |\n", data[12], data[13], data[14], data[15]);
  puts("---------------");
}

void swap(f32 *a, f32 *b) {
  f32 tmp = *a;
  *a      = *b;
  *b      = tmp;
}

u64 framecount = 0;

bool render(u0) {
  ++framecount;
  u64 startTime = RGFW_getTimeNS();

  GL_CALL(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT););

  GL_CALL(glEnable(GL_DEPTH_TEST));
  GL_CALL(glEnable(GL_CULL_FACE));
  GL_CALL(glCullFace(GL_BACK));
  GL_CALL(glFrontFace(GL_CCW));

  GL_CALL(glUseProgram(program));

  mat4 model, view, proj;
  mat4_identity(&model);

  mat4_clear(&view);
  camera_get_view_matrix(&view, &cam);

  mat4_clear(&proj);
  mat4_perspective(&proj, DEG2RAD(90.f), 16.f / 9.f, 0.01f, 1000.f);

  GL_CALL(glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.data));
  GL_CALL(glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data));
  GL_CALL(glUniformMatrix4fv(projection_loc, 1, GL_FALSE, proj.data));

  GL_CALL(glBindVertexArray(rh->internal_storage.vbo.vao_handle));
  GL_CALL(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL));

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

  u64 endTime      = RGFW_getTimeNS();
  f64 frametime_ms = ((f64)(endTime - startTime)) / 1e6;

  if (framecount % 300 == 0) {
    GAME_LOGF(
      "render time: %lf ms (fps: %lf)",
      frametime_ms,
      1000.0 / frametime_ms
    );
  }
  return true;
}

void update_camera_move(game_camera *cam, RGFW_window *window, float velocity) {
  vec3 front_scaled = vec3_scale(&cam->vec_front, velocity);
  vec3 right_scaled = vec3_scale(&cam->vec_right, velocity);

  if (RGFW_isPressed(window, RGFW_w)) {
    cam->position = vec3_add(&cam->position, &front_scaled);
  }
  if (RGFW_isPressed(window, RGFW_s)) {
    cam->position = vec3_sub(&cam->position, &front_scaled);
  }
  if (RGFW_isPressed(window, RGFW_d)) {
    cam->position = vec3_add(&cam->position, &right_scaled);
  }
  if (RGFW_isPressed(window, RGFW_a)) {
    cam->position = vec3_sub(&cam->position, &right_scaled);
  }

  if (RGFW_isPressed(window, RGFW_space)) {
    cam->position.y += velocity;
  }
  if (RGFW_isPressed(window, RGFW_shiftL)) {
    cam->position.y -= velocity;
  }
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
  create_global_window("game client", 0, 0, RENDER_MODE_FRAME_PACE_EXP);

  float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,  0.5f,  0.5f,
                      0.5f,  -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f,
                      -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,  -0.5f};

  unsigned int indices[] = {0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5,
                            4, 0, 3, 3, 7, 4, 1, 5, 6, 6, 2, 1,
                            3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4};

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
        }
       },
     .num_attributes      = 1,
     .raw_size            = sizeof(vertices),
     .vertex_data         = (u8*)vertices,
     .index_data          = (u8*)indices,
     .index_count         = (sizeof(indices) / sizeof(u32)),
     .index_type          = GL_UNSIGNED_INT
    },
   .resource_name = "my_vertex_buffer0"
  };

  // gl_resource_data rd2 = (gl_resource_data){
  //   .desc.image_texture = {
  //     .creation_info_type = RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE,
  //     .image_path = "F:\\fps_game2\\0001.png",
  //     .compress = false,
  //     .scale = 0.1f,
  //     .wrap_mode = GL_REPEAT
  //   },
  //   .resource_name = "my_tex",
  // };

  // gl_resource_data rd3 = (gl_resource_data){
  //   .resource_name = "test_textue",
  //   .desc.texture = {
  //     .creation_info_type = RESOURCE_CREATION_INFO_TYPE_TEXTURE,
  //     .width = 3000,
  //     .height = 2000,
  //     .format = GL_RGBA,
  //     .internal_format = GL_RGBA8,
  //     .wrap_mode = GL_REPEAT,
  //     .image_data = NULL,
  //     .compress = false
  //   }
  // };

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
  // request_gl_resource(&rd3, &tex_handle);
  request_gl_resource(&rd4, &shader_handle);

  TRACKED_FREE(vp);
  TRACKED_FREE(fp);

  camera_initialize(
    &cam,
    make_vec3(-4, 0, 0),
    CAMERA_INITIAL_YAW,
    CAMERA_INITIAL_PITCH
  );

  RGFW_setMousePosCallback(mouse_cb);
  RGFW_window_showMouse(get_global_internal_window(), RGFW_FALSE);
  RGFW_window_mouseHold(get_global_internal_window(), RGFW_AREA(0, 0));

  /*
  TODO:
    -- implement resource handle shit
    -- make internal functions have internal linkage
    -- add namespace prefixes to functions
  */

  // GAME_LOGF(
  //   "texture dimensions after load: %lu %lu",
  //   rd2.desc.image_texture.width,
  //   rd2.desc.image_texture.height
  // );

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

  model_loc      = glGetUniformLocation(program, "model");
  view_loc       = glGetUniformLocation(program, "view");
  projection_loc = glGetUniformLocation(program, "projection");

  window_set_render_proc(render);

  // while (!window_should_close()) {
  //   update_camera_move(&cam, get_global_internal_window(), 0.0001);
  //   window_run_render_proc();
  // }
  double last_time = RGFW_getTimeNS();
  float  speed     = 10;

  while (!window_should_close()) {
    double current_time = RGFW_getTimeNS();
    double delta_time   = (current_time - last_time) / 1e9;
    last_time           = current_time;

    update_camera_move(
      &cam,
      get_global_internal_window(),
      speed * delta_time
    ); // adjust speed multiplier

    window_run_render_proc();
  }

  destroy_gl_resource(&rd, &rh);
  destroy_gl_resource(&rd4, &shader_handle);
  // destroy_gl_resource(&rd3, &tex_handle);

  destroy_global_window();
  fclose(lf);

  GAME_LOGF("game exited gracefully");
  return 0;
}
