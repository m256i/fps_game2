#ifndef RENDERER_GL_RESOURCE_MANAGER_H_
#define RENDERER_GL_RESOURCE_MANAGER_H_

#include <math.h>

#include <containers/str_hash_table.h>
#include <common.h>

#include <glad/glad.h>
#include <string.h>

#include <util/mmcmp.h>
#include <util/array_copy.h>
#include <util/dbg/alloctrack.h>

#define MAX_RESOURCE_NAME_STRLEN   0xff
#define MAX_RESOURCE_PATH_STRLEN   0xff
#define MAX_VERTEX_COUNT           10000000
#define MAX_VERTEX_ATTRIB_COUNT    100
#define MAX_IMAGE_SIZEX            10000
#define MAX_IMAGE_SIZEY            10000
#define MAX_PIXEL_BUFFER_BYTE_SIZE MAX_IMAGE_SIZEX *MAX_IMAGE_SIZEY

typedef enum {
  RESOURCE_CREATION_INFO_TYPE_INVALID,
  RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_SHADER,
  RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_TEXTURE,
  RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE,
  RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER,
} gl_resource_ci_type;

/*
TODOLIST when changing one of these structs:
  if data needs deep copy:
    - update case in create_persistent_resource_data()
    - update case in destroy_persistent_resource_data()
  update case in resource_data_eq()
*/

typedef struct {
  /* this NEEDS to be the first element */
  u32 creation_info_type;
} frame_buffer_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  GLenum internal_format;
  usize  width;
  usize  height;
  bool   multisample;
  usize  sample_count;
} render_buffer_creation_info;

typedef struct {
  GLenum  attribute_type;
  GLsizei attribute_count;
  GLsizei attribute_index;
} vertex_attribute_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32                    creation_info_type;
  /* NOT mutable */
  vertex_attribute_info *vertex_attributes;
  usize                  num_attributes;
  u32                    raw_size;
  GLenum                 buffer_usage;
  GLenum                 index_type;
  usize                  index_count;
  /* mutable */
  u8                    *vertex_data;
  u8                    *index_data;
} vertex_buffer_creation_info;

typedef struct {
  GLenum      type;
  GLuint      size;
  /* NOT mutable */
  const char *name;
} shader_input_attribute;

typedef struct {
  GLenum      type;
  GLuint      size;
  /* NOT mutable */
  const char *name;
  bool        optional;
} shader_uniform_attribute;

typedef struct {
  /* this NEEDS to be the first element */
  u32                       creation_info_type;
  /* all NOT mutable */
  char                     *vertex_path;
  char                     *fragment_path;
  char                     *geo_path;
  char                     *tesselation_path;
  shader_input_attribute   *input_attributes;
  usize                     num_inputs;
  shader_uniform_attribute *uniform_attributes;
  usize                     num_uniforms;
  GLuint                   *ubo_binding_points;
  usize                     num_ubos;
  GLuint                   *ssbo_binding_points;
  usize                     num_ssbos;
  GLenum                   *output_attachements;
  usize                     num_outputs;
} shader_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  usize  byte_size;
  usize  binding_point;
  GLenum usage;
  u8    *data;
} ssbo_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  usize  byte_size;
  usize  binding_point;
  GLenum usage;
  u8    *data;
} ubo_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  u32    width;
  u32    height;
  GLenum internal_format;
  GLenum format;
  GLenum wrap_mode;
  bool   compress;
  /* mutable */
  u8    *image_data;
} texture_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  f32    scale;
  GLenum wrap_mode;
  bool   compress;
  /* NOT mutable */
  char  *image_path;
  /* readonly (filled by resource manager on creation)*/
  u32    width;
  u32    height;
  GLenum internal_format;
  GLenum format;
} image_texture_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32    creation_info_type;
  usize  byte_size;
  u8    *data;
  GLenum usage;
  /* PIXEL_PACK if true; PIXEL_UNPACK if false */
  bool   pack;
} pixel_buffer_creation_info;

/*
this exists so we can access the u32 creation_info_type of every possible info
type without knowing which one it is beforehand
*/
typedef struct {
  /* this NEEDS to be the first element */
  u32 creation_info_type;
} dummy_creation_info;

/*
signature of the postpone callback
*/
typedef GASYNC_CALLBACK bool (*POSTPONE_PROC)(u0 *);

/*
passing this as an identifier to request_resource()
*/
typedef struct {
  union creation_info {
    dummy_creation_info         dummy;
    frame_buffer_creation_info  frame_buffer;
    render_buffer_creation_info render_buffer;
    vertex_buffer_creation_info vertex_buffer;
    shader_creation_info        shader;
    ssbo_creation_info          ssbo;
    ubo_creation_info           ubo;
    texture_creation_info       texture;
    image_texture_creation_info image_texture;
    pixel_buffer_creation_info  pixel_buffer;
    /* TODO: implement all... */
  } desc;
  char         *resource_name;
  /*
  if NULL: do not postpone deletion after refcount 0 else:
  return TRUE from the callback to signal deletion
  */
  POSTPONE_PROC postpone_callback;
  u0           *impl_storage;
} gl_resource_data;

/*
TODO: implement postpone_deletion callback
*/

typedef struct {
  GLuint internal_handle;
  u32    hashed_resource_index;
} *gl_resource_handle, gl_resource_handle_data;

typedef struct {
  gl_resource_data resource_data;
  GLuint           internal_handle;
  i32              ref_count;
} resource_table_slot;

typedef struct {
  str_hash_table table;
  str_hash_table handle_pointers;
  bool           initialized;
} gl_resource_manager_class;

/* global instance because gl is also shared state */
extern gl_resource_manager_class gl_resource_manager;

u0 request_gl_resource(
  gl_resource_data *const resource_data,
  gl_resource_handle     *_handle
);

u0 destroy_gl_resource(
  gl_resource_data *const resource_data,
  gl_resource_handle     *_handle
);

#endif // RENDERER_GL_RESOURCE_MANAGER_H_
