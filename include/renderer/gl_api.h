#ifndef RENDERER_GL_API_H_
#define RENDERER_GL_API_H_

#include <assert.h>
#include <glad/glad.h>
#include <common.h>

static inline bool is_valid_vertex_attrib_type(GLenum type) {
  switch (type) {
  case GL_BYTE:
  case GL_UNSIGNED_BYTE:
  case GL_SHORT:
  case GL_UNSIGNED_SHORT:
  case GL_INT:
  case GL_UNSIGNED_INT:
  case GL_HALF_FLOAT:
  case GL_FLOAT:
  case GL_DOUBLE:
  case GL_FIXED:
  case GL_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_10F_11F_11F_REV: return true;
  default:                              return false;
  }
}

static inline bool is_valid_image_fmt(GLenum format) {
  switch (format) {
  // Base formats
  case GL_RED:
  case GL_RG:
  case GL_RGB:
  case GL_RGBA:
  case GL_BGR:
  case GL_BGRA:

  // Integer formats (used with integer internal formats)
  case GL_RED_INTEGER:
  case GL_RG_INTEGER:
  case GL_RGB_INTEGER:
  case GL_RGBA_INTEGER:
  case GL_BGR_INTEGER:
  case GL_BGRA_INTEGER:

  // Luminance / Alpha (deprecated in core profile but valid in compat/OpenGL
  // ES)
  case GL_LUMINANCE:
  case GL_LUMINANCE_ALPHA:
  case GL_ALPHA:
  case GL_INTENSITY:

  // Depth and stencil
  case GL_DEPTH_COMPONENT:
  case GL_DEPTH_STENCIL:   return true;

  default: return false;
  }
}

static inline bool is_valid_internal_image_fmt(GLenum format) {
  switch (format) {
  // Unsigned normalized color formats
  case GL_R8:
  case GL_RG8:
  case GL_RGB8:
  case GL_RGBA8:
  case GL_SRGB8:
  case GL_SRGB8_ALPHA8:

  // Signed normalized color formats
  case GL_R8_SNORM:
  case GL_RG8_SNORM:
  case GL_RGB8_SNORM:
  case GL_RGBA8_SNORM:

  // Integer formats
  case GL_R8UI:
  case GL_RG8UI:
  case GL_RGB8UI:
  case GL_RGBA8UI:
  case GL_R8I:
  case GL_RG8I:
  case GL_RGB8I:
  case GL_RGBA8I:

  // Floating point formats
  case GL_R16F:
  case GL_RG16F:
  case GL_RGB16F:
  case GL_RGBA16F:
  case GL_R32F:
  case GL_RG32F:
  case GL_RGB32F:
  case GL_RGBA32F:

  // Depth/stencil formats
  case GL_DEPTH_COMPONENT16:
  case GL_DEPTH_COMPONENT24:
  case GL_DEPTH_COMPONENT32:
  case GL_DEPTH_COMPONENT32F:
  case GL_DEPTH24_STENCIL8:
  case GL_DEPTH32F_STENCIL8:

  // Packed formats
  case GL_RGB10_A2:
  case GL_RGB10_A2UI:
  case GL_R11F_G11F_B10F:

  // compressed formats (if extensions are available)
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:

  // Stencil formats (for renderbuffers)
  case GL_STENCIL_INDEX8: return true;

  default: return false;
  }
}

static inline usize gl_type_to_size(GLenum _gl_type) {
  switch (_gl_type) {
  case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
  case GL_BYTE:           return sizeof(GLbyte);
  case GL_UNSIGNED_SHORT: return sizeof(GLushort);
  case GL_SHORT:          return sizeof(GLshort);
  case GL_UNSIGNED_INT:   return sizeof(GLuint);
  case GL_INT:            return sizeof(GLint);
  case GL_FIXED:          return sizeof(GLfixed);
  case GL_FLOAT:          return sizeof(GLfloat);
  case GL_HALF_FLOAT:     return sizeof(GLhalf);
  case GL_DOUBLE:         return sizeof(GLdouble);
  default:                GAME_ASSERT(false);
  }
}

static inline const char *gl_type_to_str(GLenum _gl_type) {
  switch (_gl_type) {
  case GL_UNSIGNED_BYTE:  return "GL_UNSIGNED_BYTE";
  case GL_BYTE:           return "GL_BYTE";
  case GL_UNSIGNED_SHORT: return "GL_UNSIGNED_SHORT";
  case GL_SHORT:          return "GL_SHORT";
  case GL_UNSIGNED_INT:   return "GL_UNSIGNED_INT";
  case GL_INT:            return "GL_INT";
  case GL_FIXED:          return "GL_FIXED";
  case GL_FLOAT:          return "GL_FLOAT";
  case GL_HALF_FLOAT:     return "GL_HALF_FLOAT";
  case GL_DOUBLE:         return "GL_DOUBLE";
  default:                GAME_ASSERT(false);
  }
}

#ifdef GAME_DEBUG
#define GL_CALL(fn)                                                            \
  do {                                                                         \
    while (glGetError() != GL_NO_ERROR) {                                      \
    }                                                                          \
    fn;                                                                        \
    GLenum _gl_err;                                                            \
    while ((_gl_err = glGetError()) != GL_NO_ERROR) {                          \
      GAME_CRITICALF(                                                          \
        "OpenGL error 0x%X after call `%s` at %s:%d\n",                        \
        _gl_err,                                                               \
        #fn,                                                                   \
        __FILE__,                                                              \
        __LINE__                                                               \
      );                                                                       \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#else
#define GL_CALL(fn) fn
#endif // GAME_DEBUG

static inline bool supports_bindless_textures(u0) {
  if(!glad_glGetTextureHandleARB || !glad_glMakeTextureHandleResidentARB) {
    GAME_LOGF("host system doesn't support bindless textures");
    return false;
  }

  GLuint tex;
  u32    dummy[4 * 4] = {0};
  GL_CALL(glGenTextures(1, &tex));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex));
  GL_CALL(glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    4,
    4,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    dummy
  ));

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  while (glGetError() != GL_NO_ERROR) {
    continue;
  }

  GLuint64 handle = glGetTextureHandleARB(tex);
  GL_CALL(glMakeTextureHandleResidentARB(handle));
  GLenum err = glGetError();

  glMakeTextureHandleNonResidentARB(handle);
  glDeleteTextures(1, &tex);

  if (err == GL_NO_ERROR) {
    GAME_LOGF("host system supports bindless textures");
  } else {
    GAME_LOGF("host system doesn't support bindless textures");
  }
  return (err == GL_NO_ERROR);
}

#endif // RENDERER_GL_API_H_
