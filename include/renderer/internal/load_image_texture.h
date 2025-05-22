#ifndef RENDERER_INTERNAL_LOAD_IMAGE_TEXTURE_H_
#define RENDERER_INTERNAL_LOAD_IMAGE_TEXTURE_H_

#include <glad/glad.h>
#include <common.h>

typedef struct {
  u32    width;
  u32    height;
  GLenum internal_format;
  GLenum format;
  GLuint handle;
} loaded_texture;

loaded_texture load_texture_from_file(
  char  *_path,
  bool   _compress,
  f32    _scale,
  GLenum _wrap_mode
);

u0 compress_rgba_dxt1(
  u8 *const __restrict _dst,
  const u8 *const __restrict _src,
  usize width,
  usize height
);

u0 compress_rgba_dxt5(
  u8 *const __restrict _dst,
  const u8 *const __restrict _src,
  usize width,
  usize height
);

#endif // RENDERER_INTERNAL_LOAD_IMAGE_TEXTURE_H_
