#ifndef GUI_DBG_DEBUG_OVERLAY_H_
#define GUI_DBG_DEBUG_OVERLAY_H_

#include <gui/dbg/raster_font.h>
#include <common.h>

typedef struct {
} debug_overlay;

/*
define a line to be drawn on the overlay string
every line must have a unique name and have a page
the format string is of the format "%10f" where 10 specifies the
amount of digits to be rendered and f the format the folowing formats exist:

f     -> float
d     -> double
str   -> string
u     -> u32
lu    -> u64
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

static inline u0
DBG_debug_overlay_initialie(debug_overlay_line *_lines, usize _numlines) {}

#endif // GUI_DBG_DEBUG_OVERLAY_H_
