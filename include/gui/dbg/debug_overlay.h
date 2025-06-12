#ifndef GUI_DBG_DEBUG_OVERLAY_H_
#define GUI_DBG_DEBUG_OVERLAY_H_

#include <cctype>
#include <ctype.h>
#include <gui/dbg/raster_font.h>
#include <util/dbg/alloctrack.h>
#include <common.h>

/*
define a line to be drawn on the overlay string
every line must have a unique name and have a page
the format string is of the format "%10f" where 10 specifies the
amount of digits to be rendered and f the format the folowing formats exist:

b     -> bool
f     -> float
d     -> double
s   -> string
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
typedef enum {
  fmt_type_boolean,
  fmt_type_float,
  fmt_type_double,
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
} debug_line_fmt;

typedef struct {
  usize       page;
  const char *name;
  const char *fmt;
} debug_overlay_line;

typedef struct {
  u8*   data_buffer;
  char* value;
  const char* name;
  usize fmt_count;
  u32*  fmt_types;
} parsed_overlay_line;

typedef struct {
  parsed_overlay_line* lines;
  usize line_count; 
  raster_font_atlas font_atlas;
} debug_overlay;

typedef struct {
  u8 b : 3;
  u8 _ : 1;
} _3bytes_helper;

static inline u32 digit_to_u32(char _d) {
  assert(_d >= '0' && _d <= '9');
  return (u32)(_d - '0');
}

static inline u0
DBG_debug_overlay_initialize(debug_overlay* _ctx, debug_overlay_line *_lines, usize _numlines) {
  /* bake the font atlas */
  _ctx->font_atlas = FNT_bake_atlas();
  /* pase the format strings and set up texture and SSBO indices accordingly */
  parsed_overlay_line* out_lines = TRACKED_CALLOC(_numlines, sizeof(parsed_overlay_line));
  for (usize i = 0; i != _numlines; i++) {
    debug_overlay_line* line = &_lines[i];
    for (char* c = &line->fmt[0]; *c; c++) {
      if (*c == '%') {
        ++c;
        assert(*c && isdigit(*c));
        usize digit_count = digit_to_u32(*c);
        while (*c && isdigit(*c)) {
          digit_count *= 10;
          digit_count += digit_to_u32(*c);
          ++c;
        }
        // i am not sorry
        switch (*c) {
        case 'b': break;
        case 'f': break;
        case 'd': break;
        case 's': break;
        case 'u': break;
        case 'i': break;
        default: {
          assert(*(c+1));
          switch (*(u16*)c) {
          case 'lu': break;
          case 'li': break;
          case 'zu': break;
          default: {
            assert(*(c+2));
            switch (((_3bytes_helper*)(c))->b) {
            case 'hlu': break;
            case 'vec2': break;
            case 'vec3': break;
            case 'vec4': break;
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
      } 
    }
  }
}

#endif // GUI_DBG_DEBUG_OVERLAY_H_
