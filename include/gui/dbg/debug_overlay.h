#ifndef GUI_DBG_DEBUG_OVERLAY_H_
#define GUI_DBG_DEBUG_OVERLAY_H_

#include <common.h>
#include <gui/dbg/raster_font.h>
#include <renderer/gl_resource_manager.h>

/*
define a line to be drawn on the overlay string
every line must have a unique name and have a page
the format string is of the format "%10f" where 10 specifies the
amount of digits to be rendered and f the format the folowing formats exist:

b     -> bool
f     -> float
d     -> double
s     -> string
u     -> u32
i     -> i32
lu    -> u64
li    -> i64
hu    -> 32 bit hex value
hlu   -> 64 bit hex value
zu    -> usize
vec2  -> vec2
vec3  -> vec3
vec4  -> vec4

for the non scalar types e.g vec3 the precision is digits per element
*/
typedef struct {
  const char *fmt;
} debug_overlay_line;

struct parsed_overlay_line;
struct ssbo_data_cell;

typedef struct {
  struct parsed_overlay_line *lines;
  usize                       line_count;
  raster_font_atlas           font_atlas;
  /* assumed to be exactly screen_w * screen_h * 3 bytes in size */
  gl_resource_data            static_texture_resource;
  gl_resource_handle          static_texture_handle;
  gl_resource_data            fullscreen_quad_resource;
  gl_resource_handle          fullscreen_quad_handle;
  gl_resource_data            static_fs_shader_resource;
  gl_resource_handle          static_fs_shader_handle;
  gl_resource_data            font_atlas_resource;
  gl_resource_handle          font_atlas_handle;
  gl_resource_data            dynamic_shader_resource;
  gl_resource_handle          dynamic_shader_handle;
  GLuint                      ssbo_handle;
  /* ssbo data for the dynamically drawn text */
  usize                       ssbo_item_count;
  struct ssbo_data_cell      *ssbo_data;
} debug_overlay;

u0 DBG_debug_overlay_initialize(
  debug_overlay      *_ctx,
  debug_overlay_line *_lines,
  usize               _numlines,
  usize               _screen_w,
  usize               _screen_h
);

u0 DBG_destroy_debug_overlay(debug_overlay *_ctx);

/*
just call in the render loop
*/
u0 DBG_debug_overlay_render(debug_overlay *_ctx);
/*
keep in mind that vectors are made up of scalars that have individual
indices so "%vec2 %vec2" can be accessed through _fmt_index 0 -> 3
*/
u0 DBG_set_overlay_scalar(
  debug_overlay                *_ctx,
  usize                         _line_index,
  usize                         _fmt_index,
  u0                           *_value,
  usize __attribute__((unused)) _value_size
);
/*
this function is simply a helper to access multiple scalar indices at once
keep in mind to pass e.g 3 * sizeof(f32) for a vec3 in _value_size
*/
u0 DBG_set_overlay_vector(
  debug_overlay                *_ctx,
  usize                         _line_index,
  usize                         _fmt_index,
  usize                         _vector_size,
  u0                           *_value,
  usize __attribute__((unused)) _value_size
);

#endif // GUI_DBG_DEBUG_OVERLAY_H_
