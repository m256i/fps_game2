#ifndef GUI_DBG_DEBUG_OVERLAY_H_
#define GUI_DBG_DEBUG_OVERLAY_H_

#include <ctype.h>
#include <gui/dbg/raster_font.h>
#include <string.h>
#include <util/dbg/alloctrack.h>
#include <containers/p_vector.h>
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
typedef struct {
  usize       page;
  const char *name;
  const char *fmt;
} debug_overlay_line;

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
} debug_line_fmt_type;

typedef struct {
  u32 fmt_type;
  u32 digit_count;
  u32 pixel_coords[2];
} debug_line_fmt;

CREATE_VECTOR_TYPE(debug_line_fmt)

typedef struct {
  char                 *format_string;
  const char           *name;
  vector_debug_line_fmt fmts;
  usize                 data_buffer_size;
  u8                   *data_buffer;
} parsed_overlay_line;

typedef struct {
  parsed_overlay_line *lines;
  usize                line_count;
  raster_font_atlas    font_atlas;
} debug_overlay;

static inline u32 digit_to_u32(char _d) {
  assert(_d >= '0' && _d <= '9');
  return (u32)(_d - '0');
}

#define MAKE_2CHAR_LITERAL(_c0, _c1) ((u16)_c1 << 8 | (u16)_c0)
#define MAKE_3CHAR_LITERAL(_c0, _c1, _c2)                                      \
  ((u32)_c2 << 16 | (u32)_c1 << 8 | (u32)_c0)
#define MAKE_4CHAR_LITERAL(_c0, _c1, _c2, _c3)                                 \
  ((u32)_c3 << 24 | (u32)_c2 << 16 | (u32)_c1 << 8 | (u32)_c0)

/*
pass size as count of characters not including the null terminator
*/
static inline char *string_push(char *_str, usize _size, char _c) {
  _str            = TRACKED_REALLOC((u0 *)_str, _size + 2);
  _str[_size + 1] = '\0';
  _str[_size]     = _c;
  return _str;
}

/*
pass size as count of characters not including the null terminator
*/
static inline char *string_push_n(char *_str, usize _size, usize _n, char _c) {
  _str = TRACKED_REALLOC((u0 *)_str, _size + _n + 2);
  for (usize i = _size; i < _size + _n; ++i) {
    _str[i] = _c;
  }
  _str[_size + _n] = '\0';
  return _str;
}

static inline u0 add_parsed_line_format(
  parsed_overlay_line *_line,
  u32                  _format_type,
  usize                _format_type_size,
  usize                _digit_count,
  usize               *_line_len,
  usize                _line_index
) {
  vector_debug_line_fmt_push_back(
    &_line->fmts,
    &(debug_line_fmt){
      .digit_count = _digit_count,
      .fmt_type    = _format_type,
    }
  );

  if (_format_type == fmt_type_vec2) {
    _line->format_string = string_push(_line->format_string, *_line_len, '(');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    _line->format_string = string_push(_line->format_string, *_line_len, ')');
    (*_line_len)++;

  } else if (_format_type == fmt_type_vec3) {

    _line->format_string = string_push(_line->format_string, *_line_len, '(');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    _line->format_string = string_push(_line->format_string, *_line_len, ')');
    (*_line_len)++;

  } else if (_format_type == fmt_type_vec4) {
    _line->format_string = string_push(_line->format_string, *_line_len, '(');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    string_push(_line->format_string, *_line_len, ',');
    (*_line_len)++;
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);

    _line->format_string = string_push(_line->format_string, *_line_len, ')');
    (*_line_len)++;

  } else {
    _line->format_string =
      string_push_n(_line->format_string, *_line_len, _digit_count, ' ');
    (*_line_len)            += _digit_count;
    _line->data_buffer_size += _format_type_size;
    _line->data_buffer =
      TRACKED_REALLOC(_line->data_buffer, _line->data_buffer_size);
  }

  /*
  TODO: special format for vectors '(   ,   ,   )'
  // (000000,0123.45,001234)
  add UV index based on _line_index and _line_len
  */
}

static inline u0 DBG_debug_overlay_initialize(
  debug_overlay      *_ctx,
  debug_overlay_line *_lines,
  usize               _numlines
) {
  /* bake the font atlas */
  _ctx->font_atlas = FNT_bake_atlas();
  /* pase the format strings and set up texture and SSBO indices accordingly */
  parsed_overlay_line *out_lines =
    TRACKED_CALLOC(_numlines, sizeof(parsed_overlay_line));
  _ctx->line_count = _numlines;
  _ctx->lines      = out_lines;
  // i am not sorry :)
  for (usize i = 0; i != _numlines; i++) {
    usize               line_strlen = 0;
    debug_overlay_line *line        = &_lines[i];
    out_lines[i].fmts               = vector_debug_line_fmt_create_();
    out_lines[i].name               = line->name;
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
          puts("parsing boolean format");
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_bool,
            sizeof(bool),
            digit_count,
            &line_strlen,
            i
          );
          break;
        case 'f':
          c++;
          puts("parsing float format");
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_f32,
            sizeof(f32),
            digit_count,
            &line_strlen,
            i
          );
          break;
        case 'd':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_f64,
            sizeof(f64),
            digit_count,
            &line_strlen,
            i
          );
          break;
        case 's':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_string,
            sizeof(char),
            digit_count,
            &line_strlen,
            i
          );
          break;
        case 'u':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_u32,
            sizeof(u32),
            digit_count,
            &line_strlen,
            i
          );
          break;
        case 'i':
          c++;
          add_parsed_line_format(
            &out_lines[i],
            fmt_type_i32,
            sizeof(i32),
            digit_count,
            &line_strlen,
            i
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
              sizeof(u64),
              digit_count,
              &line_strlen,
              i
            );
            break;
          case MAKE_2CHAR_LITERAL('l', 'i'):
            c += 2;
            add_parsed_line_format(
              &out_lines[i],
              fmt_type_i64,
              sizeof(i64),
              digit_count,
              &line_strlen,
              i
            );
            break;
          case MAKE_2CHAR_LITERAL('z', 'u'):
            c += 2;
            add_parsed_line_format(
              &out_lines[i],
              fmt_type_usize,
              sizeof(usize),
              digit_count,
              &line_strlen,
              i
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
                sizeof(f32) * 2,
                digit_count,
                &line_strlen,
                i
              );
              break;
            case MAKE_4CHAR_LITERAL('v', 'e', 'c', '3'):
              c += 4;
              add_parsed_line_format(
                &out_lines[i],
                fmt_type_vec3,
                sizeof(f32) * 3,
                digit_count,
                &line_strlen,
                i
              );
              break;
            case MAKE_4CHAR_LITERAL('v', 'e', 'c', '4'):
              c += 4;
              add_parsed_line_format(
                &out_lines[i],
                fmt_type_vec4,
                sizeof(f32) * 4,
                digit_count,
                &line_strlen,
                i
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
        out_lines[i].format_string =
          string_push(out_lines[i].format_string, line_strlen, *c);
        ++line_strlen;
        ++c;
      }
    }
  }
  puts("lmao");
}

#endif // GUI_DBG_DEBUG_OVERLAY_H_
