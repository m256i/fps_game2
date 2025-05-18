#ifndef RENDERER_GL_RESOURCE_MANAGER_H_
#define RENDERER_GL_RESOURCE_MANAGER_H_

#include <containers/str_hash_table.h>
#include <common.h>

#include <glad/glad.h>
#include <string.h>

#include <util/mmcmp.h>
#include <util/array_copy.h>

#define MAX_RESOURCE_NAME_STRLEN 0xff
#define MAX_RESOURCE_PATH_STRLEN 0xff

typedef enum {
  RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_SHADER,
  RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_TEXTURE,
  RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE,
  RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER,
} gl_resource_ci_type;

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
  /* mutable */
  u8                    *vertex_data;
} vertex_buffer_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32     creation_info_type;
  GLenum  index_type;
  GLsizei index_count;
} index_buffer_creation_info;

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
  u32 creation_info_type;
} ssbo_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32 creation_info_type;
} ubo_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32     creation_info_type;
  /* NOT mutable */
  GLenum *attachements;
  usize   num_attachements;
  u32     width;
  u32     height;
  GLenum  internal_format;
  GLenum  format;
  GLenum  wrap_mode;
  /* mutable */
  u8     *image_data;
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
    index_buffer_creation_info  index_buffer;
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

/* deep copy function for gl_resource_data since clients might pass in temporary
 * structs and arrays */
static gl_resource_data
create_persistent_resource_data(const gl_resource_data *const _temp) {
  gl_resource_data out = {0};
  GAME_LOGF("doig deep copy on resource %s", _temp->resource_name);

  /* copy the name into a persistent buff */
  out.resource_name =
    strnclone_s(_temp->resource_name, MAX_RESOURCE_NAME_STRLEN);

  /* important, only shallow copy since our handle handles memory */
  out.impl_storage = _temp->impl_storage;

  GAME_LOGF("copied name: %s", out.resource_name);

  switch (_temp->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER:        {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.vertex_buffer.vertex_attributes = memclone(
      _temp->desc.vertex_buffer.vertex_attributes,
      sizeof *_temp->desc.vertex_buffer.vertex_attributes *
        _temp->desc.vertex_buffer.num_attributes
    );
    /* persistant data doesnt need the buffer */
    out.desc.vertex_buffer.vertex_data = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    const shader_creation_info *inptr  = &_temp->desc.shader;
    shader_creation_info       *outptr = &out.desc.shader;
    outptr->vertex_path =
      strnclone_s(inptr->vertex_path, MAX_RESOURCE_PATH_STRLEN);
    outptr->fragment_path =
      strnclone_s(inptr->fragment_path, MAX_RESOURCE_PATH_STRLEN);

    if (inptr->geo_path) {
      outptr->geo_path = strnclone_s(inptr->geo_path, MAX_RESOURCE_PATH_STRLEN);
    }

    if (inptr->tesselation_path) {
      outptr->tesselation_path =
        strnclone_s(inptr->tesselation_path, MAX_RESOURCE_PATH_STRLEN);
    }

    outptr->input_attributes = memclone(
      inptr->input_attributes,
      inptr->num_inputs * sizeof *inptr->input_attributes
    );
    outptr->uniform_attributes = memclone(
      inptr->uniform_attributes,
      inptr->num_uniforms * sizeof *inptr->uniform_attributes
    );
    outptr->ubo_binding_points = memclone(
      inptr->ubo_binding_points,
      inptr->num_ubos * sizeof *inptr->ubo_binding_points
    );
    outptr->ssbo_binding_points = memclone(
      inptr->ssbo_binding_points,
      inptr->num_ssbos * sizeof *inptr->ssbo_binding_points
    );
    outptr->output_attachements = memclone(
      inptr->output_attachements,
      inptr->num_outputs * sizeof *inptr->output_attachements
    );
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.texture.image_data   = NULL;
    out.desc.texture.attachements = memclone(
      _temp->desc.texture.attachements,
      _temp->desc.texture.num_attachements *
        sizeof *_temp->desc.texture.attachements
    );
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.image_texture.image_path = strnclone_s(
      _temp->desc.image_texture.image_path,
      MAX_RESOURCE_PATH_STRLEN
    );
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.pixel_buffer.data = NULL;
    break;
  }
  default: {
    GAME_CRITICALF(
      "unknown creation info passed to create_persistent_resource_data"
    );
    exit(1);
    break;
  }
  }
  return out;
}

static u0 destroy_persistent_resource_data(gl_resource_data *_data) {
  if (_data->resource_name) {
    free(_data->resource_name);
    _data->resource_name = NULL;
  }

  switch (_data->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER:        {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    free(_data->desc.vertex_buffer.vertex_attributes);
    _data->desc.vertex_buffer.vertex_attributes = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    shader_creation_info *ptr = &_data->desc.shader;
    free(ptr->vertex_path);
    free(ptr->fragment_path);
    free(ptr->geo_path);
    free(ptr->tesselation_path);
    free(ptr->input_attributes);
    free(ptr->uniform_attributes);
    free(ptr->ubo_binding_points);
    free(ptr->ssbo_binding_points);
    free(ptr->output_attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    free(_data->desc.texture.attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    free(_data->desc.image_texture.image_path);
    break;
  }
  default: {
    GAME_CRITICALF(
      "unknown creation info passed to destroy_persistent_resource_data"
    );
    exit(1);
    break;
  }
  }
}

/* cancer */
static bool resource_data_eq(
  const gl_resource_data *const _r0,
  const gl_resource_data *const _r1
) {
  bool same_info_type =
    _r0->desc.dummy.creation_info_type == _r1->desc.dummy.creation_info_type;
  bool same_name = strcmp(_r0->resource_name, _r1->resource_name) == 0;
  bool same_desc = false;

  switch (_r0->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    const frame_buffer_creation_info *a = &_r0->desc.frame_buffer;
    const frame_buffer_creation_info *b = &_r1->desc.frame_buffer;
    if (a->width == b->width && a->height == b->height &&
        a->format == b->format && a->num_attachements == b->num_attachements) {
      same_desc = true;
      for (usize i = 0; i < a->num_attachements; ++i) {
        if (a->attachements[i] != b->attachements[i]) {
          same_desc = false;
          break;
        }
      }
    }
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    const vertex_buffer_creation_info *a = &_r0->desc.vertex_buffer;
    const vertex_buffer_creation_info *b = &_r1->desc.vertex_buffer;
    if (a->size == b->size && a->buffer_usage == b->buffer_usage &&
        a->num_attributes == b->num_attributes) {
      same_desc = true;
      for (usize i = 0; i < a->num_attributes; ++i) {
        const vertex_attribute_info *va = &a->vertex_attributes[i];
        const vertex_attribute_info *vb = &b->vertex_attributes[i];
        if (va->attribute_type != vb->attribute_type ||
            va->attribute_count != vb->attribute_count ||
            va->attribute_index != vb->attribute_index) {
          same_desc = false;
          break;
        }
      }
    }
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER: {
    const index_buffer_creation_info *a = &_r0->desc.index_buffer;
    const index_buffer_creation_info *b = &_r1->desc.index_buffer;
    same_desc = (a->index_type == b->index_type && a->size == b->size);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SSBO: {

    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {

    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_COMPUTE_TEXTURE: {

    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {

    break;
  }
  default: {
    GAME_CRITICALF("unknown creation info passed to create_resource");
    exit(1);
    break;
  }
  }

  GAME_LOGF(
    "resource_data_eq() same info: %s same name: %s same desc: %s",
    same_info_type ? "true" : "false",
    same_name ? "true" : "false",
    same_desc ? "true" : "false"
  );

  return same_info_type && same_name && same_desc;
}

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
static gl_resource_manager_class gl_resource_manager;

static GLuint create_gl_fbo(gl_resource_data *const resource_data) {
  GLuint handle = 0;
  glGenFramebuffers(1, &handle);
  return handle;
}

static u0 destroy_gl_fbo(
  gl_resource_data *const resource_data,
  gl_resource_handle      _handle
) {
  glDeleteFramebuffers(1, &_handle->internal_handle);
}

static usize gl_type_to_size(GLenum _gl_type) {
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
  default:                assert(false);
  }
}

static GLuint create_gl_vbo(gl_resource_data *const _resource_data) {
  GLuint vao, vbo;
  glCreateVertexArrays(1, &vao);

  _resource_data->impl_storage            = malloc(sizeof(vao));
  *(GLuint *)_resource_data->impl_storage = vao;

  glBindVertexArray(vao);
  glCreateBuffers(1, &vbo);
  glNamedBufferData(
    vbo,
    _resource_data.desc.vertex_buffer.raw_size,
    _resource_data.desc.vertex_buffer.vertex_data,
    _resource_data.desc.vertex_buffer.buffer_usage
  );

  GLint  offset = 0;
  GLuint stride = 0;

  for (usize i = 0; i < _resource_data.desc.vertex_buffer.num_attributes; i++) {
    vertex_attribute_info *a_info =
      _resource_data.desc.vertex_buffer.vertex_attributes[i];

    stride += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  glVertexArrayVertexBuffer(vao, 0, vbo, 0, stride);

  for (usize i = 0; i < _resource_data.desc.vertex_buffer.num_attributes; i++) {
    vertex_attribute_info *a_info =
      _resource_data.desc.vertex_buffer.vertex_attributes[i];

    glEnableVertexArrayAttrib(vao, a_info->attribute_index);
    glVertexArrayAttribFormat(
      vao,
      a_info->attribute_index,
      a_info->attribute_count,
      a_info->attribute_type,
      GL_FALSE,
      offset
    );
    glVertexArrayAttribBinding(vao, a_info->attribute_index, 0);

    GAME_LOGF(" create vbo: offset was: %lu", offset);
    offset += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  /* cleanup */
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GAME_LOGF(" create vbo: stride was: %lu", stride);
  return vbo;
}

static u0 destroy_gl_vbo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GLuint vbo = _handle->internal_handle;
  GLuint vao = *(GLuint *)_resource_data->impl_storage;

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);

  free(_resource_data->impl_storage);
}

static GLuint create_gl_resource(const gl_resource_data *const resource_data) {
  switch (resource_data->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    GAME_LOGF("creating FBO");
    return create_gl_fbo(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    GAME_LOGF("creating VBO");
    return create_gl_vbo(resource_data);
    /* etc... */
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    break;
  }
  default: {
    GAME_CRITICALF("unknown creation info passed to create_resource");
    exit(1);
    break;
  }
  }
}

static u0 request_gl_resource(
  const gl_resource_data *const resource_data,
  gl_resource_handle           *_handle
) {
  static str_hash_table *const table = &gl_resource_manager.table;
  static str_hash_table *const handle_pointer_table =
    &gl_resource_manager.handle_pointers;

  /* initialize global resource manager once */
  if (!gl_resource_manager.initialized) {
    GAME_LOGF("creating gl_resource_manager");
    str_hash_table_initialize(table, sizeof(resource_table_slot), 10);
    str_hash_table_initialize(
      handle_pointer_table,
      sizeof(gl_resource_handle),
      10
    );
    gl_resource_manager.initialized = true;
  }

  GAME_LOGF("request_resource() on %s", resource_data->resource_name);

  /* have we already created that resource (client handle not null) for that
   * client */
  if (*_handle != NULL) {
#ifdef GAME_DEBUG
    GAME_LOGF("handle already in use by client");
    assert(str_hash_table_contains(table, resource_data->resource_name));
    assert(
      str_hash_table_get_index(table, resource_data->resource_name) ==
      (*_handle)->hashed_resource_index
    );
    resource_table_slot *resource_slot =
      str_hash_table_at_index(table, (*_handle)->hashed_resource_index);
    /* resource desc matches? */
    if (resource_data_eq(&resource_slot->resource_data, resource_data)) {
      /* since _handle was != NULL we dont increment refcount */
      return;
    } else {
      GAME_CRITICALF(
        "tried to obtain resource '%s' with conflicting creation info",
        resource_data->resource_name
      );
      exit(1);
    }
#else
    /* in release mode we dont do any checks */
    return;
#endif
  } else if (str_hash_table_contains(table, resource_data->resource_name)) {
    GAME_LOGF(
      "new client called request_resource on existing resource: %s",
      resource_data->resource_name
    );
    /* handle NULL but resource exists already */
    assert(str_hash_table_contains(
      handle_pointer_table,
      resource_data->resource_name
    ));

    *_handle =
      *(gl_resource_handle *)
        str_hash_table_at(handle_pointer_table, resource_data->resource_name);

    assert(*_handle);
    GAME_LOGF(
      "handle: index %u internal handle: %u",
      (*_handle)->hashed_resource_index,
      (*_handle)->internal_handle
    );

    /* handle already exists so just set it */
    resource_table_slot *const resource_slot =
      str_hash_table_at_index(table, (*_handle)->hashed_resource_index);
    /* new client, so increment refcount */
    ++resource_slot->ref_count;
    return;
  }

  GAME_LOGF("creating new gl object");
  const GLuint gl_handle = create_gl_resource(resource_data);

  /* make sure handle pointer table was properly cleared before */
  assert(
    !str_hash_table_contains(handle_pointer_table, resource_data->resource_name)
  );

  gl_resource_handle new_handle = malloc(sizeof **_handle);

  str_hash_table_insert(
    handle_pointer_table,
    resource_data->resource_name,
    &new_handle
  );

  *_handle                          = new_handle;
  (*_handle)->hashed_resource_index = str_hash_table_insert(
    table,
    resource_data->resource_name,
    &(resource_table_slot){
      .resource_data   = create_persistent_resource_data(resource_data),
      .internal_handle = gl_handle, /* change when we actually do the stuff */
      .ref_count       = 1,
    }
  );
  (*_handle)->internal_handle = gl_handle;
}

static u0 destroy_gl_resource(
  const gl_resource_data *const resource_data,
  gl_resource_handle           *_handle
) {
  static str_hash_table *const table = &gl_resource_manager.table;
  static str_hash_table *const handle_pointer_table =
    &gl_resource_manager.handle_pointers;

  assert(gl_resource_manager.initialized);
  GAME_LOGF("destroy_resource() on %s", resource_data->resource_name);

  if (!*_handle) {
    GAME_LOGF("handle already NULL");
    return;
  }

  assert(str_hash_table_contains(table, resource_data->resource_name));
  assert(
    str_hash_table_get_index(table, resource_data->resource_name) ==
    (*_handle)->hashed_resource_index
  );

  resource_table_slot *resource_slot =
    str_hash_table_at_index(table, (*_handle)->hashed_resource_index);
  assert(resource_slot->ref_count > 0);
  --resource_slot->ref_count;

  if (resource_slot->ref_count == 0) {
    /* TODO: implement deletion postpone callback */
    if (true) {
      /*
      TODO: implement OpenGL buffer deletion logic
      */
      /* free the main table slot */
      destroy_persistent_resource_data(&resource_slot->resource_data);
      str_hash_table_erase(table, resource_data->resource_name);

      assert(str_hash_table_contains(
        handle_pointer_table,
        resource_data->resource_name
      ));

      free(*_handle);
      GAME_LOGF("freed handle data");
      str_hash_table_erase(handle_pointer_table, resource_data->resource_name);
      GAME_LOGF("resource fully destroyed");
    } else {
      /*  TODO: summon a low prio thread that periodically (every 20 seconds or
      so checks all of the callbacks) and pushes the ones that returned true
      into a queue for the main thread to destroy them
      */
      GAME_LOGF(
        "destroy_resource() on %s moved its data to postpone list!",
        resource_data->resource_name
      );
    }
  }
  *_handle = NULL;
}

#endif // RENDERER_GL_RESOURCE_MANAGER_H_
