#ifndef GUI_DBG_RASTER_FONT_H_
#define GUI_DBG_RASTER_FONT_H_

#include <math/vec4.h>
#include <common.h>
#include <containers/str_hash_table.h>

#define FNT_GLYPH_WIDTH  8 /* width of one glyph            */
#define FNT_GLYPH_HEIGHT 8 /* height of one glyph           */
#define FNT_GLYPH_VSPACE 3 /* vertical space between lines  */

/*
returns a pointer to a 8x8x3 bmp buffer that contains the character
for that codepoint (e.g 'a')
*/
u8 *FNT_codepoint_to_bmp(char _codep);
u8 *FNT_bake_string_to_bmp(const char *_text, usize *_out_w, usize *_out_h);

typedef struct {
  u8            *data;
  usize          width;
  usize          height;
  /*
  maps a codepoint to an x offset in the atlas texture (0,1)
  since they are all on one line and we know their height
  */
  str_hash_table codepoint_to_uv_map;
} raster_font_atlas;

raster_font_atlas FNT_bake_atlas(u0);
u0                FNT_destroy_atlas(raster_font_atlas *_atlas);
/*
returns the UV coordinates for the given codepoint in the atlas
*/
vec4 FNT_uv_for_codep(raster_font_atlas *const _atlas, char _codep);

#endif // GUI_DBG_RASTER_FONT_H_
