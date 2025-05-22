#include <renderer/gl_resource_manager.h>

gl_resource_manager_class gl_resource_manager = {0};

/* deep copy function for gl_resource_data since clients might pass in temporary
 * structs and arrays */
inline gl_resource_data
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
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.vertex_buffer.vertex_attributes =
        memclone(_temp->desc.vertex_buffer.vertex_attributes,
                 sizeof *_temp->desc.vertex_buffer.vertex_attributes *
                     _temp->desc.vertex_buffer.num_attributes);
    /* persistant data doesnt need the buffer */
    out.desc.vertex_buffer.vertex_data = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    const shader_creation_info *inptr = &_temp->desc.shader;
    shader_creation_info *outptr = &out.desc.shader;
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

    outptr->input_attributes =
        memclone(inptr->input_attributes,
                 inptr->num_inputs * sizeof *inptr->input_attributes);
    outptr->uniform_attributes =
        memclone(inptr->uniform_attributes,
                 inptr->num_uniforms * sizeof *inptr->uniform_attributes);
    outptr->ubo_binding_points =
        memclone(inptr->ubo_binding_points,
                 inptr->num_ubos * sizeof *inptr->ubo_binding_points);
    outptr->ssbo_binding_points =
        memclone(inptr->ssbo_binding_points,
                 inptr->num_ssbos * sizeof *inptr->ssbo_binding_points);
    outptr->output_attachements =
        memclone(inptr->output_attachements,
                 inptr->num_outputs * sizeof *inptr->output_attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.texture.image_data = NULL;
    out.desc.texture.attachements =
        memclone(_temp->desc.texture.attachements,
                 _temp->desc.texture.num_attachements *
                     sizeof *_temp->desc.texture.attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.image_texture.image_path = strnclone_s(
        _temp->desc.image_texture.image_path, MAX_RESOURCE_PATH_STRLEN);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.pixel_buffer.data = NULL;
    break;
  }
  default: {
    GAME_CRITICALF(
        "unknown creation info passed to create_persistent_resource_data");
    exit(1);
    break;
  }
  }
  return out;
}

inline u0 destroy_persistent_resource_data(gl_resource_data *_data) {
  if (_data->resource_name) {
    TRACKED_FREE(_data->resource_name);
    _data->resource_name = NULL;
  }

  switch (_data->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    TRACKED_FREE(_data->desc.vertex_buffer.vertex_attributes);
    _data->desc.vertex_buffer.vertex_attributes = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    shader_creation_info *ptr = &_data->desc.shader;
    TRACKED_FREE(ptr->vertex_path);
    TRACKED_FREE(ptr->fragment_path);
    TRACKED_FREE(ptr->geo_path);
    TRACKED_FREE(ptr->tesselation_path);
    TRACKED_FREE(ptr->input_attributes);
    TRACKED_FREE(ptr->uniform_attributes);
    TRACKED_FREE(ptr->ubo_binding_points);
    TRACKED_FREE(ptr->ssbo_binding_points);
    TRACKED_FREE(ptr->output_attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    TRACKED_FREE(_data->desc.texture.attachements);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    TRACKED_FREE(_data->desc.image_texture.image_path);
    break;
  }
  default: {
    GAME_CRITICALF(
        "unknown creation info passed to destroy_persistent_resource_data");
    exit(1);
    break;
  }
  }
}

/* cancer */
inline bool resource_data_eq(const gl_resource_data *const _r0,
                             const gl_resource_data *const _r1) {
  bool same_info_type =
      _r0->desc.dummy.creation_info_type == _r1->desc.dummy.creation_info_type;
  bool same_name = strcmp(_r0->resource_name, _r1->resource_name) == 0;
  bool same_desc = false;

  switch (_r0->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    const render_buffer_creation_info *a = &_r0->desc.render_buffer;
    const render_buffer_creation_info *b = &_r1->desc.render_buffer;
    same_desc = a->internal_format == b->internal_format &&
                a->height == b->height && a->width == b->width;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    const vertex_buffer_creation_info *a = &_r0->desc.vertex_buffer;
    const vertex_buffer_creation_info *b = &_r1->desc.vertex_buffer;
    if (a->raw_size == b->raw_size && a->buffer_usage == b->buffer_usage &&
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
    same_desc =
        (a->index_type == b->index_type && a->index_count == b->index_count);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    const shader_creation_info *a = &_r0->desc.shader;
    const shader_creation_info *b = &_r1->desc.shader;

    same_desc &= !strcmp(a->vertex_path, b->vertex_path);
    same_desc &= !strcmp(a->fragment_path, b->fragment_path);
    if (a->geo_path && b->geo_path) {
      same_desc &= !strcmp(a->geo_path, b->geo_path);
    }
    if (a->tesselation_path && b->tesselation_path) {
      same_desc &= !strcmp(a->tesselation_path, b->tesselation_path);
    }
    same_desc &= a->num_inputs == b->num_inputs;
    same_desc &= a->num_uniforms == b->num_uniforms;
    same_desc &= a->num_ubos == b->num_ubos;
    same_desc &= a->num_ssbos == b->num_ssbos;
    same_desc &= a->num_outputs == b->num_outputs;

    for (usize i = 0; i != a->num_inputs; ++i) {
      same_desc &= a->input_attributes[i].size == b->input_attributes[i].size;
      same_desc &= a->input_attributes[i].type == b->input_attributes[i].type;
      assert(a->input_attributes[i].name);
      assert(b->input_attributes[i].name);
      same_desc &=
          !strcmp(a->input_attributes[i].name, b->input_attributes[i].name);
    }

    for (usize i = 0; i != a->num_uniforms; ++i) {
      same_desc &=
          a->uniform_attributes[i].type == b->uniform_attributes[i].type;
      same_desc &=
          a->uniform_attributes[i].size == b->uniform_attributes[i].size;
      same_desc &= a->uniform_attributes[i].optional ==
                   b->uniform_attributes[i].optional;
      assert(a->uniform_attributes[i].name);
      assert(a->uniform_attributes[i].name);
      same_desc &=
          !strcmp(a->uniform_attributes[i].name, b->uniform_attributes[i].name);
    }
    for (usize i = 0; i != a->num_ubos; ++i) {
      same_desc &= a->ubo_binding_points[i] == b->ubo_binding_points[i];
    }
    for (usize i = 0; i != a->num_ssbos; ++i) {
      same_desc &= a->ssbo_binding_points[i] == b->ssbo_binding_points[i];
    }
    for (usize i = 0; i != a->num_outputs; ++i) {
      same_desc &= a->output_attachements[i] == b->output_attachements[i];
    }
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    const texture_creation_info *a = &_r0->desc.texture;
    const texture_creation_info *b = &_r1->desc.texture;

    same_desc &= a->num_attachements == b->num_attachements;
    same_desc &= a->width == b->width;
    same_desc &= a->height == b->height;
    same_desc &= a->internal_format == b->internal_format;
    same_desc &= a->format == b->format;
    same_desc &= a->wrap_mode == b->wrap_mode;

    for (usize i = 0; i < a->num_attachements; i++) {
      same_desc &= a->attachements[i] == b->attachements[i];
    }
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    const image_texture_creation_info *a = &_r0->desc.image_texture;
    const image_texture_creation_info *b = &_r1->desc.image_texture;

    same_desc &= fabsf(a->scale - b->scale) < __FLT_EPSILON__;
    same_desc &= a->wrap_mode == b->wrap_mode;
    same_desc &= a->compress == b->compress;

    assert(a->image_path);
    assert(b->image_path);

    same_desc &= !strcmp(a->image_path, b->image_path);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    const pixel_buffer_creation_info *a = &_r0->desc.pixel_buffer;
    const pixel_buffer_creation_info *b = &_r1->desc.pixel_buffer;

    same_desc &= a->byte_size == b->byte_size;
    same_desc &= a->usage == b->usage;
    break;
  }
  default: {
    GAME_CRITICALF("unknown creation info passed to create_resource");
    exit(1);
    break;
  }
  }

  GAME_LOGF("resource_data_eq() same info: %s same name: %s same desc: %s",
            same_info_type ? "true" : "false", same_name ? "true" : "false",
            same_desc ? "true" : "false");
  return same_info_type && same_name && same_desc;
}

inline GLuint create_gl_fbo(gl_resource_data *const resource_data) {
  GLuint handle = 0;
  glGenFramebuffers(1, &handle);
  return handle;
}

inline u0 destroy_gl_fbo(gl_resource_data *const resource_data,
                         gl_resource_handle _handle) {
  glDeleteFramebuffers(1, &_handle->internal_handle);
}

inline GLuint create_gl_vbo(gl_resource_data *const _resource_data) {
  GLuint vao, vbo;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glCreateBuffers(1, &vbo);
  glNamedBufferData(vbo, _resource_data->desc.vertex_buffer.raw_size,
                    _resource_data->desc.vertex_buffer.vertex_data,
                    _resource_data->desc.vertex_buffer.buffer_usage);

  /* the user doesnt need the vbo handle after creation*/
  _resource_data->impl_storage = TRACKED_MALLOC(sizeof(vbo));
  *(GLuint *)_resource_data->impl_storage = vbo;

  GLint offset = 0;
  GLuint stride = 0;

  for (usize i = 0; i < _resource_data->desc.vertex_buffer.num_attributes;
       i++) {
    vertex_attribute_info *a_info =
        &_resource_data->desc.vertex_buffer.vertex_attributes[i];

    stride += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  glVertexArrayVertexBuffer(vao, 0, vbo, 0, stride);

  for (usize i = 0; i < _resource_data->desc.vertex_buffer.num_attributes;
       i++) {
    vertex_attribute_info *a_info =
        &_resource_data->desc.vertex_buffer.vertex_attributes[i];

    glEnableVertexArrayAttrib(vao, a_info->attribute_index);
    glVertexArrayAttribFormat(vao, a_info->attribute_index,
                              a_info->attribute_count, a_info->attribute_type,
                              GL_FALSE, offset);
    glVertexArrayAttribBinding(vao, a_info->attribute_index, 0);

    GAME_LOGF(" create vbo: offset was: %lu", offset);
    offset += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  /* cleanup */
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GAME_LOGF(" create vbo: stride was: %lu", stride);
  return vao;
}

inline u0 destroy_gl_vbo(gl_resource_data *const _resource_data,
                         gl_resource_handle _handle) {
  GLuint vao = _handle->internal_handle;
  GLuint vbo = *(GLuint *)_resource_data->impl_storage;

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);

  TRACKED_FREE(_resource_data->impl_storage);
}

inline GLuint create_gl_resource(gl_resource_data *const resource_data) {
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

u0 request_gl_resource(gl_resource_data *const resource_data,
                       gl_resource_handle *_handle) {
  static str_hash_table *const table = &gl_resource_manager.table;
  static str_hash_table *const handle_pointer_table =
      &gl_resource_manager.handle_pointers;

  /* initialize global resource manager once */
  if (!gl_resource_manager.initialized) {
    GAME_LOGF("creating gl_resource_manager");
    str_hash_table_initialize(table, sizeof(resource_table_slot), 10);
    str_hash_table_initialize(handle_pointer_table, sizeof(gl_resource_handle),
                              10);
    gl_resource_manager.initialized = true;
  }

  GAME_LOGF("request_resource() on %s", resource_data->resource_name);

  /* have we already created that resource (client handle not null) for that
   * client */
  if (*_handle != NULL) {
#ifdef GAME_DEBUG
    GAME_LOGF("handle already in use by client");
    assert(str_hash_table_contains(table, resource_data->resource_name));
    assert(str_hash_table_get_index(table, resource_data->resource_name) ==
           (*_handle)->hashed_resource_index);
    resource_table_slot *resource_slot =
        str_hash_table_at_index(table, (*_handle)->hashed_resource_index);
    /* resource desc matches? */
    if (resource_data_eq(&resource_slot->resource_data, resource_data)) {
      /* since _handle was != NULL we dont increment refcount */
      return;
    } else {
      GAME_CRITICALF(
          "tried to obtain resource '%s' with conflicting creation info",
          resource_data->resource_name);
      exit(1);
    }
#else
    /* in release mode we dont do any checks */
    return;
#endif
  } else if (str_hash_table_contains(table, resource_data->resource_name)) {
    GAME_LOGF("new client called request_resource on existing resource: %s",
              resource_data->resource_name);
    /* handle NULL but resource exists already */
    assert(str_hash_table_contains(handle_pointer_table,
                                   resource_data->resource_name));

    *_handle = *(gl_resource_handle *)str_hash_table_at(
        handle_pointer_table, resource_data->resource_name);

    assert(*_handle);
    GAME_LOGF("handle: index %u internal handle: %u",
              (*_handle)->hashed_resource_index, (*_handle)->internal_handle);

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
  assert(!str_hash_table_contains(handle_pointer_table,
                                  resource_data->resource_name));

  gl_resource_handle new_handle = TRACKED_MALLOC(sizeof **_handle);

  str_hash_table_insert(handle_pointer_table, resource_data->resource_name,
                        &new_handle);

  *_handle = new_handle;
  (*_handle)->hashed_resource_index = str_hash_table_insert(
      table, resource_data->resource_name,
      &(resource_table_slot){
          .resource_data = create_persistent_resource_data(resource_data),
          .internal_handle =
              gl_handle, /* change when we actually do the stuff */
          .ref_count = 1,
      });
  (*_handle)->internal_handle = gl_handle;
}

u0 destroy_gl_resource(gl_resource_data *const resource_data,
                       gl_resource_handle *_handle) {
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
  assert(str_hash_table_get_index(table, resource_data->resource_name) ==
         (*_handle)->hashed_resource_index);

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

      assert(str_hash_table_contains(handle_pointer_table,
                                     resource_data->resource_name));

      TRACKED_FREE(*_handle);
      GAME_LOGF("freed handle data");
      str_hash_table_erase(handle_pointer_table, resource_data->resource_name);
      GAME_LOGF("resource fully destroyed");
    } else {
      /*  TODO: summon a low prio thread that periodically (every 20 seconds or
      so checks all of the callbacks) and pushes the ones that returned true
      into a queue for the main thread to destroy them
      */
      GAME_LOGF("destroy_resource() on %s moved its data to postpone list!",
                resource_data->resource_name);
    }
  }
  *_handle = NULL;
}
