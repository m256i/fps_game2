#include <gui/dbg/debug_overlay.h>

#include <renderer/gl_api.h>
#include <util/abspath.h>
#include <ctype.h>
#include <string.h>
#include <util/dbg/alloctrack.h>
#include <containers/p_vector.h>
#include <containers/sstring.h>

#define MAKE_2CHAR_LITERAL(_c0, _c1) ((u16)_c1 << 8 | (u16)_c0)
#define MAKE_3CHAR_LITERAL(_c0, _c1, _c2)                                      \
  ((u32)_c2 << 16 | (u32)_c1 << 8 | (u32)_c0)
#define MAKE_4CHAR_LITERAL(_c0, _c1, _c2, _c3)                                 \
  ((u32)_c3 << 24 | (u32)_c2 << 16 | (u32)_c1 << 8 | (u32)_c0)

typedef enum {
  fmt_type_bool,
  fmt_type_f32,
  fmt_type_f64,
  fmt_type_string,
  fmt_type_u32,
  fmt_type_i32,
  fmt_type_u64,
  fmt_type_i64,
  fmt_type_h32,
  fmt_type_h64,
  fmt_type_usize,
  fmt_type_vec2,
  fmt_type_vec3,
  fmt_type_vec4,
  fmt_type_count, /* used for iterating */
} debug_line_fmt_type;

typedef struct {
  u32 fmt_type;
  u32 digit_count;
  u32 pixel_coords[2];
  u16 ssbo_offset;
} debug_line_fmt;

CREATE_VECTOR_TYPE(debug_line_fmt)

typedef struct parsed_overlay_line {
  sstring               format_string;
  vector_debug_line_fmt fmts;
} parsed_overlay_line;

typedef struct ssbo_data_cell {
  f32 screen_pos_x;
  f32 screen_pos_y;
  f32 umin;
  f32 vmin;
  f32 umax;
  f32 vmax;
} ssbo_data_cell;

static u0 add_parsed_line_format(
  parsed_overlay_line *_line,
  u32                  _format_type,
  usize                _digit_count,
  usize                _line_index,
  u16                 *current_ssbo_offset
) {
  const u32 x_index = sstring_length(&_line->format_string) * FNT_GLYPH_WIDTH +
                      (FNT_GLYPH_WIDTH / 2);
  const u32 y_index =
    _line_index * FNT_GLYPH_HEIGHT + ((_line_index + 1) * FNT_GLYPH_VSPACE) + 1;

  /* TODO: cleanup */

  if (_format_type == fmt_type_vec2) {
    /* first number comes after the '(' so + 8 */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count  = _digit_count,
        .fmt_type     = fmt_type_f32,
        .pixel_coords = {x_index + FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset  = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (2 + _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;

    sstring_push(&_line->format_string, '(');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ')');
  } else if (_format_type == fmt_type_vec3) {
    /* first number comes after the '(' so + 8 */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count  = _digit_count,
        .fmt_type     = fmt_type_f32,
        .pixel_coords = {x_index + FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset  = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (2 + _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' then _digit_count
      whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (3 + 2 * _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;

    sstring_push(&_line->format_string, '(');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ')');
  } else if (_format_type == fmt_type_vec4) {
    /* first number comes after the '(' so + 8 */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count  = _digit_count,
        .fmt_type     = fmt_type_f32,
        .pixel_coords = {x_index + FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset  = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (2 + _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' then _digit_count
      whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (3 + 2 * _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;
    /* skip '(' then _digit_count whitespaces then ',' then _digit_count
      whitespaces then ',' then _digit_count whitespaces then ',' */
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count = _digit_count,
        .fmt_type    = fmt_type_f32,
        .pixel_coords =
          {x_index + (4 + 3 * _digit_count) * FNT_GLYPH_WIDTH, y_index},
        .ssbo_offset = *current_ssbo_offset,
      }
    );
    *current_ssbo_offset += _digit_count;

    sstring_push(&_line->format_string, '(');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ',');
    sstring_push_n(&_line->format_string, ' ', _digit_count);
    sstring_push(&_line->format_string, ')');
  } else {
    vector_debug_line_fmt_push_back(
      &_line->fmts,
      &(debug_line_fmt){
        .digit_count  = _digit_count,
        .fmt_type     = _format_type,
        .pixel_coords = {x_index, y_index},
        .ssbo_offset  = *current_ssbo_offset,
      }
    );

    *current_ssbo_offset += _digit_count;
    sstring_push_n(&_line->format_string, ' ', _digit_count);
  }
}

static u0 initialize_overlay_value(
  debug_overlay *_ctx,
  usize          _line_index,
  usize          _fmt_index
) {
  GAME_ASSERT(_ctx->ssbo_data);
  GAME_ASSERT(_ctx->line_count > _line_index);
  GAME_ASSERT(_ctx->lines[_line_index].fmts.size > _fmt_index);

  debug_line_fmt *line_fmt = &_ctx->lines[_line_index].fmts.data[_fmt_index];
  const usize     ssbo_base_offset = line_fmt->ssbo_offset;

  char *raw_string = alloca(line_fmt->digit_count);
  /* fill with spaces because thats what the font renderer expects */

  if (line_fmt->fmt_type == fmt_type_bool ||
      line_fmt->fmt_type == fmt_type_string) {
    memset(raw_string, ' ', line_fmt->digit_count);
  } else {
    memset(raw_string, '0', line_fmt->digit_count);
  }

  for (usize i = 0; i != line_fmt->digit_count; i++) {
    const f32  screen_pos_x    = line_fmt->pixel_coords[0] / 1920.f;
    const f32  screen_pos_y    = line_fmt->pixel_coords[1] / 1080.f;
    const f32  screen_stride_x = (f32)FNT_GLYPH_WIDTH / 1920.f;
    const vec4 atlas_uv = FNT_uv_for_codep(&_ctx->font_atlas, raw_string[i]);

    _ctx->ssbo_data[ssbo_base_offset + i].screen_pos_x =
      screen_pos_x + i * screen_stride_x;
    /* since these are always on one line the y pos is always fixed */
    _ctx->ssbo_data[ssbo_base_offset + i].screen_pos_y = screen_pos_y;
    printf(
      "ssbo data: screenpos: {%f, %f}\n",
      _ctx->ssbo_data[ssbo_base_offset + i].screen_pos_x,
      _ctx->ssbo_data[ssbo_base_offset + i].screen_pos_y
    );

    _ctx->ssbo_data[ssbo_base_offset + i].umin = atlas_uv.x;
    _ctx->ssbo_data[ssbo_base_offset + i].vmin = atlas_uv.y;
    _ctx->ssbo_data[ssbo_base_offset + i].umax = atlas_uv.z;
    _ctx->ssbo_data[ssbo_base_offset + i].vmax = atlas_uv.w;
  }
}

static u32 digit_to_u32(char _d) {
  assert(_d >= '0' && _d <= '9');
  return (u32)(_d - '0');
}

u0 DBG_set_overlay_scalar(
  debug_overlay                *_ctx,
  usize                         _line_index,
  usize                         _fmt_index,
  u0                           *_value,
  usize __attribute__((unused)) _value_size
) {
  GAME_ASSERT(_ctx->ssbo_data);
  GAME_ASSERT(_ctx->line_count > _line_index);
  GAME_ASSERT(_ctx->lines[_line_index].fmts.size > _fmt_index);

  debug_line_fmt *line_fmt = &_ctx->lines[_line_index].fmts.data[_fmt_index];
  const usize     ssbo_base_offset = line_fmt->ssbo_offset;

#ifdef GAME_DEBUG
  switch (line_fmt->fmt_type) {
  case fmt_type_bool: GAME_ASSERT(_value_size == sizeof(bool)); break;
  case fmt_type_f32:  GAME_ASSERT(_value_size == sizeof(f32)); break;
  case fmt_type_f64:  GAME_ASSERT(_value_size == sizeof(f64)); break;
  case fmt_type_string:
    GAME_ASSERT(
      _value_size == line_fmt->digit_count ||
      _value_size == line_fmt->digit_count + 1
    );
    break;
  case fmt_type_u32:   GAME_ASSERT(_value_size == sizeof(u32)); break;
  case fmt_type_i32:   GAME_ASSERT(_value_size == sizeof(i32)); break;
  case fmt_type_u64:   GAME_ASSERT(_value_size == sizeof(u64)); break;
  case fmt_type_i64:   GAME_ASSERT(_value_size == sizeof(i64)); break;
  case fmt_type_h32:   GAME_ASSERT(_value_size == sizeof(u32)); break;
  case fmt_type_h64:   GAME_ASSERT(_value_size == sizeof(u64)); break;
  case fmt_type_usize: GAME_ASSERT(_value_size == sizeof(usize)); break;
  case fmt_type_vec2:  GAME_ASSERT(_value_size == sizeof(f32) * 2); break;
  case fmt_type_vec3:  GAME_ASSERT(_value_size == sizeof(f32) * 3); break;
  case fmt_type_vec4:  GAME_ASSERT(_value_size == sizeof(f32) * 4); break;
  default:             GAME_ASSERT(false);
  }
#endif
  const usize true_digit_size = line_fmt->digit_count;
  char       *raw_string      = alloca(true_digit_size + 1);
  /* fill with spaces because thats what the font renderer expects */
  memset(raw_string, ' ', true_digit_size);
  raw_string[true_digit_size] = '\0';

  /* TODO: cleanup */

  switch (line_fmt->fmt_type) {
  case fmt_type_bool: {
    *((bool *)_value) ? snprintf(raw_string, true_digit_size + 1, "true")
                      : snprintf(raw_string, true_digit_size + 1, "false");
    break;
  }
  case fmt_type_string: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%*s",
      line_fmt->digit_count,
      (char *)_value
    );
    break;
  }
  case fmt_type_f32: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*.*g",
      line_fmt->digit_count,
      line_fmt->digit_count - 1,
      *(f32 *)_value
    );
    break;
  }
  case fmt_type_f64: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*.*g",
      line_fmt->digit_count,
      line_fmt->digit_count - 1,
      *(f64 *)_value
    );
    break;
  }
  case fmt_type_u32: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*u",
      line_fmt->digit_count,
      *(u32 *)_value
    );
    break;
  }
  case fmt_type_i32: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*d",
      line_fmt->digit_count,
      *(i32 *)_value
    );
    break;
  }
  case fmt_type_u64: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*llu",
      line_fmt->digit_count,
      *(u64 *)_value
    );
    break;
  }
  case fmt_type_i64: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*lld",
      line_fmt->digit_count,
      *(i64 *)_value
    );
    break;
  }
  case fmt_type_h32: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "0x%0*x",
      line_fmt->digit_count - 2,
      *(u32 *)_value
    );
    break;
  }
  case fmt_type_h64: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "0x%0*llx",
      line_fmt->digit_count - 2,
      *(u64 *)_value
    );
    break;
  }
  case fmt_type_usize: {
    snprintf(
      raw_string,
      true_digit_size + 1,
      "%0*zu",
      line_fmt->digit_count,
      *(usize *)_value
    );
    break;
  }
  default: GAME_ASSERT(false);
  }

  for (usize i = 0; i != true_digit_size; i++) {
    const vec4 atlas_uv = FNT_uv_for_codep(&_ctx->font_atlas, raw_string[i]);
    _ctx->ssbo_data[ssbo_base_offset + i].umin = atlas_uv.x;
    _ctx->ssbo_data[ssbo_base_offset + i].vmin = atlas_uv.y;
    _ctx->ssbo_data[ssbo_base_offset + i].umax = atlas_uv.z;
    _ctx->ssbo_data[ssbo_base_offset + i].vmax = atlas_uv.w;
  }
}

u0 DBG_set_overlay_vector(
  debug_overlay                *_ctx,
  usize                         _line_index,
  usize                         _fmt_index,
  usize                         _vector_size,
  u0                           *_value,
  usize __attribute__((unused)) _value_size
) {
  const usize scalar_size = _value_size / _vector_size;
  for (usize i = 0; i != _vector_size; i++) {
    DBG_set_overlay_scalar(
      _ctx,
      _line_index,
      _fmt_index + i,
      ((u8 *)_value) + (i * scalar_size),
      scalar_size
    );
  }
}

u0 DBG_debug_overlay_initialize(
  debug_overlay      *_ctx,
  debug_overlay_line *_lines,
  usize               _numlines,
  usize               _screen_w,
  usize               _screen_h
) {
  /* bake the font atlas */
  _ctx->font_atlas = FNT_bake_atlas();
  /* pase the format strings and set up texture and SSBO indices accordingly */
  parsed_overlay_line *out_lines =
    TRACKED_CALLOC(_numlines, sizeof(parsed_overlay_line));
  _ctx->line_count = _numlines;
  _ctx->lines      = out_lines;

  sstring full_screen_string  = {0};
  u16     current_ssbo_offset = 0;

  // i am not sorry :)
  for (usize i = 0; i != _numlines; i++) {
    debug_overlay_line *line = &_lines[i];
    out_lines[i].fmts        = vector_debug_line_fmt_create_();
    /* TODO: cleanup */
    for (const char *c = &line->fmt[0]; *c;) {
      if (*c == '%') {
        c++;
        assert(*c && isdigit(*c));
        usize digit_count = digit_to_u32(*c++);
        while (*c && isdigit(*c)) {
          digit_count *= 10;
          digit_count += digit_to_u32(*c++);
        }
        switch (*c) {
        case 'b':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_bool,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        case 'f':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_f32,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        case 'd':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_f64,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        case 's':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_string,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        case 'u':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_u32,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        case 'i':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_i32,
            digit_count,
            i,
            &current_ssbo_offset
          );
          break;
        default: {
          assert(*(c + 1));
          switch (*(u16 *)c) {
          case MAKE_2CHAR_LITERAL('l', 'u'):
            c += 2;
            add_parsed_line_format(
              &out_lines[i],
              fmt_type_u64,
              digit_count,
              i,
              &current_ssbo_offset
            );
            break;
          case MAKE_2CHAR_LITERAL('l', 'i'):
            c += 2;
            add_parsed_line_format(
              &out_lines[i],
              fmt_type_i64,
              digit_count,
              i,
              &current_ssbo_offset
            );
            break;
          case MAKE_2CHAR_LITERAL('z', 'u'):
            c += 2;
            add_parsed_line_format(
              &out_lines[i],
              fmt_type_usize,
              digit_count,
              i,
              &current_ssbo_offset
            );
            break;
          default: {
            assert(*(c + 3));
            u32 n_4bytes = 0;
            memcpy(&n_4bytes, c, 4);
            switch (n_4bytes) {
            case MAKE_4CHAR_LITERAL('v', 'e', 'c', '2'):
              c += 4;
              add_parsed_line_format(
                &out_lines[i],
                fmt_type_vec2,
                digit_count,
                i,
                &current_ssbo_offset
              );
              break;
            case MAKE_4CHAR_LITERAL('v', 'e', 'c', '3'):
              c += 4;
              add_parsed_line_format(
                &out_lines[i],
                fmt_type_vec3,
                digit_count,
                i,
                &current_ssbo_offset
              );
              break;
            case MAKE_4CHAR_LITERAL('v', 'e', 'c', '4'):
              c += 4;
              add_parsed_line_format(
                &out_lines[i],
                fmt_type_vec4,
                digit_count,
                i,
                &current_ssbo_offset
              );
              break;
            default: {
              GAME_LOGF("unkown format in string: %s", c);
              exit(1);
            }
            }
            break;
          }
          }
          break;
        }
        }
      } else {
        sstring_push(&out_lines[i].format_string, *c);
        ++c;
      }
    }
    sstring_push(&out_lines[i].format_string, '\n');
    sstring_append(&full_screen_string, &out_lines[i].format_string);
    destroy_sstring(&out_lines[i].format_string);
  }

  _ctx->font_atlas_resource =
    (gl_resource_data){.resource_name = "debug_overlay_dynamic_texture",
                       .desc.texture  = {
                          .creation_info_type =
                           RESOURCE_CREATION_INFO_TYPE_TEXTURE,
                          .width           = _ctx->font_atlas.width,
                          .height          = _ctx->font_atlas.height,
                          .format          = GL_RGB,
                          .internal_format = GL_RGB8,
                          .wrap_mode       = GL_CLAMP_TO_EDGE,
                          .image_data      = _ctx->font_atlas.data,
                          .compress        = true,
                       }};

  request_gl_resource(&_ctx->font_atlas_resource, &_ctx->font_atlas_handle);

  char *vp0 = make_abs_path(
    "../game_data/shaders/debug_overlay/vert_dynamic_bindless.vs"
  );
  char *fp0 =
    make_abs_path("../game_data/shaders/debug_overlay/frag_static_bindless.fs");

  _ctx->dynamic_shader_resource =
    (gl_resource_data){.resource_name = "debug_overlay_dynamic_shader",
                       .desc.shader   = {
                           .creation_info_type =
                           RESOURCE_CREATION_INFO_TYPE_SHADER,
                           .vertex_path   = vp0,
                           .fragment_path = fp0,
                       }};

  request_gl_resource(
    &_ctx->dynamic_shader_resource,
    &_ctx->dynamic_shader_handle
  );

  const GLuint dynamic_shader_handle =
    _ctx->dynamic_shader_handle->internal_storage.shader;

  const GLuint64 dynamic_bindless_handle =
    _ctx->font_atlas_handle->internal_storage.texture.bindless_handle;

  GL_CALL(glProgramUniformHandleui64ARB(
    dynamic_shader_handle,
    glGetUniformLocation(dynamic_shader_handle, "uTexture"),
    dynamic_bindless_handle
  ));

  TRACKED_FREE(vp0);
  TRACKED_FREE(fp0);

  _ctx->ssbo_item_count = (usize)current_ssbo_offset;

  GL_CALL(glCreateBuffers(1, &_ctx->ssbo_handle));
  GL_CALL(glNamedBufferStorage(
    _ctx->ssbo_handle,
    sizeof(ssbo_data_cell) * _ctx->ssbo_item_count,
    NULL,
    GL_MAP_WRITE_BIT          /* we’ll map & write into it                  */
      | GL_MAP_PERSISTENT_BIT /* keep one mapping around                    */
      | GL_MAP_COHERENT_BIT   /* writes become visible without manual flush */
  ));
  _ctx->ssbo_data = glMapNamedBuffer(_ctx->ssbo_handle, GL_WRITE_ONLY);

  for (size_t i = 0; i < _ctx->line_count; ++i) {
    for (size_t j = 0; j < _ctx->lines[i].fmts.size; ++j) {
      initialize_overlay_value(_ctx, i, j);
    }
  }

  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ctx->ssbo_handle));
  usize texture_w = 0, texture_h = 0;

  u8 *static_texture = FNT_bake_string_to_bmp(
    sstring_data(&full_screen_string),
    &texture_w,
    &texture_h
  );

  _ctx->static_texture_resource =
    (gl_resource_data){.resource_name = "debug_overlay_static_texture",
                       .desc.texture  = {
                          .creation_info_type =
                           RESOURCE_CREATION_INFO_TYPE_TEXTURE,
                          .width           = _screen_w,
                          .height          = _screen_h,
                          .format          = GL_RGB,
                          .internal_format = GL_RGB8,
                          .wrap_mode       = GL_CLAMP_TO_EDGE,
                          .image_data      = NULL,
                          .compress        = true,
                       }};

  request_gl_resource(
    &_ctx->static_texture_resource,
    &_ctx->static_texture_handle
  );

  const GLuint texture_handle =
    _ctx->static_texture_handle->internal_storage.texture.handle;

  GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_handle));
  GL_CALL(glClearTexImage(
    texture_handle,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    (GLubyte[]){0xFF, 0xFF, 0xFF}
  ));

  GL_CALL(glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0,
    0,
    texture_w,
    texture_h,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    static_texture
  ));

  GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
  TRACKED_FREE(static_texture);

  const static f32 quad_verts[] = {
    -1.0f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    -1.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    0.0f,
    1.0f
  };
  const static u32 quad_indices[] = {0, 1, 2, 2, 3, 0};

  _ctx->fullscreen_quad_resource = (gl_resource_data){
    .desc.vertex_buffer =
      {.creation_info_type = RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
       .buffer_usage       = GL_STATIC_DRAW,
       .vertex_attributes =
         (vertex_attribute_info[]){
           (vertex_attribute_info){.attribute_type  = GL_FLOAT,
                                   .attribute_count = 2,
                                   .attribute_index = 0},
           (vertex_attribute_info){.attribute_type  = GL_FLOAT,
                                   .attribute_count = 2,
                                   .attribute_index = 1}
         },
       .num_attributes = 2,
       .raw_size       = sizeof(quad_verts),
       .vertex_data    = (u8 *)quad_verts,
       .index_data     = (u8 *)quad_indices,
       .index_count    = (sizeof(quad_indices) / sizeof(u32)),
       .index_type     = GL_UNSIGNED_INT},
    .resource_name = "debug_overlay_fsquad"
  };

  request_gl_resource(
    &_ctx->fullscreen_quad_resource,
    &_ctx->fullscreen_quad_handle
  );

  char *vp =
    make_abs_path("../game_data/shaders/debug_overlay/vert_static_bindless.vs");
  char *fp =
    make_abs_path("../game_data/shaders/debug_overlay/frag_static_bindless.fs");

  _ctx->static_fs_shader_resource =
    (gl_resource_data){.resource_name = "debug_overlay_static_shader",
                       .desc.shader   = {
                           .creation_info_type =
                           RESOURCE_CREATION_INFO_TYPE_SHADER,
                           .vertex_path   = vp,
                           .fragment_path = fp,
                       }};

  request_gl_resource(
    &_ctx->static_fs_shader_resource,
    &_ctx->static_fs_shader_handle
  );

  const GLuint shader_handle =
    _ctx->static_fs_shader_handle->internal_storage.shader;

  const GLuint64 bindless_handle =
    _ctx->static_texture_handle->internal_storage.texture.bindless_handle;

  GL_CALL(glProgramUniformHandleui64ARB(
    shader_handle,
    glGetUniformLocation(shader_handle, "uTexture"),
    bindless_handle
  ));

  TRACKED_FREE(vp);
  TRACKED_FREE(fp);
}

u0 DBG_debug_overlay_render(debug_overlay *_ctx) {
  GL_CALL(glClear(GL_DEPTH_BUFFER_BIT));
  GL_CALL(glUseProgram(_ctx->static_fs_shader_handle->internal_storage.shader));
  GL_CALL(glBindVertexArray(
    _ctx->fullscreen_quad_handle->internal_storage.vbo.vao_handle
  ));
  GL_CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

  GL_CALL(glUseProgram(_ctx->dynamic_shader_handle->internal_storage.shader));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ctx->ssbo_handle));
  GL_CALL(glDrawElementsInstanced(
    GL_TRIANGLES,
    6,
    GL_UNSIGNED_INT,
    NULL,
    _ctx->ssbo_item_count
  ));
}

u0 DBG_destroy_debug_overlay(debug_overlay *_ctx) {
  FNT_destroy_atlas(&_ctx->font_atlas);
  destroy_gl_resource(
    &_ctx->static_texture_resource,
    &_ctx->static_texture_handle
  );
  destroy_gl_resource(
    &_ctx->fullscreen_quad_resource,
    &_ctx->fullscreen_quad_handle
  );
  destroy_gl_resource(
    &_ctx->static_fs_shader_resource,
    &_ctx->static_fs_shader_handle
  );
  destroy_gl_resource(&_ctx->font_atlas_resource, &_ctx->font_atlas_handle);
  destroy_gl_resource(
    &_ctx->dynamic_shader_resource,
    &_ctx->dynamic_shader_handle
  );
  for (usize i = 0; i != _ctx->line_count; i++) {
    vector_debug_line_fmt_free(&_ctx->lines[i].fmts);
  }
  TRACKED_FREE(_ctx->lines);

  GL_CALL(glUnmapNamedBuffer(_ctx->ssbo_handle));
  _ctx->ssbo_data = NULL;

  GL_CALL(glDeleteBuffers(1, &_ctx->ssbo_handle));
  _ctx->ssbo_handle = 0;
}
