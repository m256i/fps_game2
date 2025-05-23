#ifndef RENDERER_GL_API_H_
#define RENDERER_GL_API_H_

#include <assert.h>
#include <glad/glad.h>
#include <common.h>

inline usize gl_type_to_size(GLenum _gl_type) {
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

inline const char *gl_type_to_str(GLenum _gl_type) {
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
#endif // RENDERER_GL_API_H_
