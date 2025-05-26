#include "containers/str_hash_table.h"
#include <renderer/gl_api.h>
#include <renderer/gl_resource_manager.h>
#include <renderer/internal/load_image_texture.h>
#include <renderer/internal/load_shader.h>

gl_resource_manager_class gl_resource_manager = {0};

/* deep copy function for gl_resource_data since clients might pass in temporary
 * structs and arrays */
gl_resource_data
create_persistent_resource_data(const gl_resource_data *const _temp) {
  gl_resource_data out = {0};
  GAME_LOGF("doig deep copy on resource %s %d", _temp->resource_name, _temp->desc.dummy.creation_info_type);

  /* copy the name into a persistent buff */
  out.resource_name =
    strnclone_s(_temp->resource_name, MAX_RESOURCE_NAME_STRLEN);

  /* important, only shallow copy since our handle handles memory */
  out.impl_storage = _temp->impl_storage;
  GAME_LOGF("copied name: %s", out.resource_name);

  switch (_temp->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.ssbo.data = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.ubo.data = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.vertex_buffer.vertex_attributes = memclone(
      _temp->desc.vertex_buffer.vertex_attributes,
      sizeof *_temp->desc.vertex_buffer.vertex_attributes *
        _temp->desc.vertex_buffer.num_attributes
    );
    out.desc.vertex_buffer.vertex_data = NULL;
    out.desc.vertex_buffer.index_data  = NULL;
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
    out.desc.texture.image_data = NULL;
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

  GAME_LOGF("out resource: %s %d", out.resource_name, out.desc.dummy.creation_info_type);
  return out;
}

u0 destroy_persistent_resource_data(gl_resource_data *_data) {
  if (_data->resource_name) {
    GAME_LOGF("freeing resource: %s type %d", _data->resource_name, _data->desc.dummy.creation_info_type);
    TRACKED_FREE(_data->resource_name);
    _data->resource_name = NULL;
  }

  switch (_data->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER:
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER:        {
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
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    TRACKED_FREE(_data->desc.image_texture.image_path);
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
bool resource_data_eq(
  const gl_resource_data *const _r0,
  const gl_resource_data *const _r1
) {
  bool same_info_type =
    _r0->desc.dummy.creation_info_type == _r1->desc.dummy.creation_info_type;
  bool same_name = strcmp(_r0->resource_name, _r1->resource_name) == 0;
  bool same_desc = true;

  switch (_r0->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER: {
    const ssbo_creation_info *a = &_r0->desc.ssbo;
    const ssbo_creation_info *b = &_r1->desc.ssbo;

    same_desc &= a->binding_point == b->binding_point;
    same_desc &= a->byte_size == b->byte_size;
    same_desc &= a->usage == b->usage;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    const ubo_creation_info *a = &_r0->desc.ubo;
    const ubo_creation_info *b = &_r1->desc.ubo;

    same_desc &= a->binding_point == b->binding_point;
    same_desc &= a->byte_size == b->byte_size;
    same_desc &= a->usage == b->usage;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    const render_buffer_creation_info *a = &_r0->desc.render_buffer;
    const render_buffer_creation_info *b = &_r1->desc.render_buffer;
    same_desc = a->internal_format == b->internal_format &&
                a->height == b->height && a->width == b->width &&
                a->multisample == b->multisample &&
                a->sample_count == b->sample_count;
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
    same_desc &= a->index_count == b->index_count;
    same_desc &= a->index_type == b->index_type;
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
      GAME_ASSERT(a->input_attributes[i].name);
      GAME_ASSERT(b->input_attributes[i].name);
      same_desc &=
        !strcmp(a->input_attributes[i].name, b->input_attributes[i].name);
    }
    for (usize i = 0; i != a->num_uniforms; ++i) {
      same_desc &=
        a->uniform_attributes[i].type == b->uniform_attributes[i].type;
      same_desc &=
        a->uniform_attributes[i].size == b->uniform_attributes[i].size;
      same_desc &=
        a->uniform_attributes[i].optional == b->uniform_attributes[i].optional;
      GAME_ASSERT(a->uniform_attributes[i].name);
      GAME_ASSERT(a->uniform_attributes[i].name);
      same_desc &=
        a->uniform_attributes[i].optional == b->uniform_attributes[i].optional;
      GAME_ASSERT(a->uniform_attributes[i].name);
      GAME_ASSERT(a->uniform_attributes[i].name);
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

    same_desc &= a->width == b->width;
    same_desc &= a->height == b->height;
    same_desc &= a->internal_format == b->internal_format;
    same_desc &= a->format == b->format;
    same_desc &= a->wrap_mode == b->wrap_mode;
    same_desc &= a->compress == b->compress;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    const image_texture_creation_info *a = &_r0->desc.image_texture;
    const image_texture_creation_info *b = &_r1->desc.image_texture;

    same_desc &= fabsf(a->scale - b->scale) < __FLT_EPSILON__;
    same_desc &= a->wrap_mode == b->wrap_mode;
    same_desc &= a->compress == b->compress;

    GAME_ASSERT(a->image_path);
    GAME_ASSERT(b->image_path);

    same_desc &= !strcmp(a->image_path, b->image_path);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    const pixel_buffer_creation_info *a  = &_r0->desc.pixel_buffer;
    const pixel_buffer_creation_info *b  = &_r1->desc.pixel_buffer;
    same_desc                           &= a->byte_size == b->byte_size;
    same_desc                           &= a->usage == b->usage;
    break;
  }
  default: {
    same_desc = false;
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

GLuint create_gl_fbo(u0) {
  GLuint handle = 0;
  GL_CALL(glGenFramebuffers(1, &handle));
  return handle;
}

u0 destroy_gl_fbo(gl_resource_handle _handle) {
  GL_CALL(glDeleteFramebuffers(1, &_handle->internal_handle));
}

GLuint create_gl_vbo(gl_resource_data *const _resource_data) {
  GLuint vao, vbo, ebo;
  GL_CALL(glCreateVertexArrays(1, &vao));
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glCreateBuffers(1, &vbo));

  const vertex_buffer_creation_info *const vc =
    &_resource_data->desc.vertex_buffer;

  GL_CALL(
    glNamedBufferData(vbo, vc->raw_size, vc->vertex_data, vc->buffer_usage)
  );

  GL_CALL(glCreateBuffers(1, &ebo));
  GL_CALL(glNamedBufferData(
    ebo,
    gl_type_to_size(vc->index_type) * vc->index_count,
    vc->index_data,
    vc->buffer_usage
  ));

  struct vbo_ebo_pair {
    GLuint vbo;
    GLuint ebo;
  };

  /* the user doesnt need the vbo, ebo handles after creation*/
  _resource_data->impl_storage = TRACKED_MALLOC(sizeof(struct vbo_ebo_pair));
  ((struct vbo_ebo_pair *)_resource_data->impl_storage)->vbo = vbo;
  ((struct vbo_ebo_pair *)_resource_data->impl_storage)->ebo = ebo;

  GLint  offset = 0;
  GLuint stride = 0;

  for (usize i = 0; i < vc->num_attributes; i++) {
    vertex_attribute_info *a_info = &vc->vertex_attributes[i];
    stride += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  GL_CALL(glVertexArrayVertexBuffer(vao, 0, vbo, 0, stride));

  for (usize i = 0; i < vc->num_attributes; i++) {
    vertex_attribute_info *a_info = &vc->vertex_attributes[i];
    GL_CALL(glEnableVertexArrayAttrib(vao, a_info->attribute_index));
    GL_CALL(glVertexArrayAttribFormat(
      vao,
      a_info->attribute_index,
      a_info->attribute_count,
      a_info->attribute_type,
      GL_FALSE,
      offset
    ));

    GL_CALL(glVertexArrayAttribBinding(vao, a_info->attribute_index, 0));

    GAME_LOGF(" create vbo: offset was: %lu", offset);
    offset += gl_type_to_size(a_info->attribute_type) * a_info->attribute_count;
  }

  GL_CALL(glVertexArrayElementBuffer(vao, ebo));

  /* cleanup */
  GL_CALL(glBindVertexArray(0));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));

  GAME_LOGF(" create vbo: stride was: %lu", stride);
  return vao;
}

u0 destroy_gl_vbo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GLuint vao = _handle->internal_handle;

  struct vbo_ebo_pair {
    GLuint vbo;
    GLuint ebo;
  };

  GLuint vbo = ((struct vbo_ebo_pair *)_resource_data->impl_storage)->vbo;
  GLuint ebo = ((struct vbo_ebo_pair *)_resource_data->impl_storage)->ebo;

  GL_CALL(glDeleteBuffers(1, &ebo));
  GL_CALL(glDeleteVertexArrays(1, &vao));
  GL_CALL(glDeleteBuffers(1, &vbo));

  TRACKED_FREE(_resource_data->impl_storage);
}

GLuint create_gl_rbo(gl_resource_data *const _resource_data) {
  GLuint handle = 0;
  GL_CALL(glGenRenderbuffers(1, &handle));
  GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, handle));
  if (_resource_data->desc.render_buffer.multisample) {
    GL_CALL(glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      _resource_data->desc.render_buffer.sample_count,
      _resource_data->desc.render_buffer.internal_format,
      _resource_data->desc.render_buffer.width,
      _resource_data->desc.render_buffer.height
    ));
  } else {
    GL_CALL(glRenderbufferStorage(
      GL_RENDERBUFFER,
      _resource_data->desc.render_buffer.internal_format,
      _resource_data->desc.render_buffer.width,
      _resource_data->desc.render_buffer.height
    ));
  }
  return handle;
}

u0 destroy_gl_rbo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GL_CALL(glDeleteRenderbuffers(1, &_handle->internal_handle));
}

GLuint create_gl_pbo(gl_resource_data *const _resource_data) {
  GLuint handle = 0;
  GL_CALL(glGenBuffers(1, &handle));
  if (_resource_data->desc.pixel_buffer.pack) {
    GL_CALL(glBindBuffer(
      _resource_data->desc.pixel_buffer.pack ? GL_PIXEL_PACK_BUFFER
                                             : GL_PIXEL_UNPACK_BUFFER,
      handle
    ));
    GL_CALL(glBufferData(
      _resource_data->desc.pixel_buffer.pack ? GL_PIXEL_PACK_BUFFER
                                             : GL_PIXEL_UNPACK_BUFFER,
      _resource_data->desc.pixel_buffer.byte_size,
      _resource_data->desc.pixel_buffer.data,
      _resource_data->desc.pixel_buffer.usage
    ));
  }
  return handle;
}

u0 destroy_gl_pbo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GL_CALL(glDeleteBuffers(1, &_handle->internal_handle));
}

GLuint create_gl_image_texture(gl_resource_data *const _resource_data) {
  image_texture_creation_info *const it = &_resource_data->desc.image_texture;
  const loaded_texture               lt = load_texture_from_file(
    it->image_path,
    it->compress,
    it->scale,
    it->wrap_mode
  );

  it->width           = lt.width;
  it->height          = lt.height;
  it->internal_format = lt.internal_format;
  it->format          = lt.format;

  GAME_LOGF(
    "new tex: %lu %lu, %lu %lu",
    it->width,
    it->height,
    it->internal_format,
    it->format
  );
  return lt.handle;
}

GLuint create_gl_texture(gl_resource_data *const _resource_data) {
  texture_creation_info *const ti     = &_resource_data->desc.texture;
  GLuint                       handle = 0;

  GL_CALL(glGenTextures(1, &handle));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, handle));

  const usize xsize = ti->width;
  const usize ysize = ti->height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  /* initial data provided */
  if (ti->image_data && ti->compress) {
    /* only actually compress on RGB and RGBA */
    switch (ti->format) {
    case GL_RGB: {
      const usize total_size = ((xsize + 3) / 4) * ((ysize + 3) / 4) * 8ull;
      u8         *compressed_data = TRACKED_MALLOC(total_size);
      compress_rgba_dxt1(compressed_data, ti->image_data, xsize, ysize);
      GL_CALL(glCompressedTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
        xsize,
        ysize,
        0,
        total_size,
        compressed_data
      ));
      TRACKED_FREE(compressed_data);
      goto done;
    }
    case GL_RGBA: {
      const usize total_size = ((xsize + 3) / 4) * ((ysize + 3) / 4) * 16ull;
      u8         *compressed_data = TRACKED_MALLOC(total_size);
      compress_rgba_dxt5(compressed_data, ti->image_data, xsize, ysize);
      GL_CALL(glCompressedTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
        xsize,
        ysize,
        0,
        total_size,
        compressed_data
      ));
      TRACKED_FREE(compressed_data);
      goto done;
    }
    default: {
      goto no_compress;
    }
    }
  }
no_compress:
  GL_CALL(glTexImage2D(
    GL_TEXTURE_2D,
    0,
    ti->internal_format,
    xsize,
    ysize,
    0,
    ti->format,
    GL_UNSIGNED_BYTE,
    ti->image_data /* either valid or null */
  ));
done:

  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ti->wrap_mode));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ti->wrap_mode));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
  return handle;
}

u0 destroy_gl_texture(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GL_CALL(glDeleteTextures(1, &_handle->internal_handle));
}

GLuint create_gl_ssbo(gl_resource_data *const _resource_data) {
  GLuint ssbo;
  GL_CALL(glGenBuffers(1, &ssbo));
  GL_CALL(glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo));
  GL_CALL(glBufferData(
    GL_SHADER_STORAGE_BUFFER,
    _resource_data->desc.ssbo.byte_size,
    _resource_data->desc.ssbo.data,
    _resource_data->desc.ssbo.usage
  ));
  return ssbo;
}

u0 destroy_gl_ssbo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GL_CALL(glDeleteBuffers(1, &_handle->internal_handle));
}

GLuint create_gl_ubo(gl_resource_data *const _resource_data) {
  GLuint ubo;
  GL_CALL(glGenBuffers(1, &ubo));
  GL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, ubo));
  GL_CALL(glBufferData(
    GL_UNIFORM_BUFFER,
    _resource_data->desc.ssbo.byte_size,
    _resource_data->desc.ssbo.data,
    _resource_data->desc.ssbo.usage
  ));
  return ubo;
}

u0 destroy_gl_ubo(
  gl_resource_data *const _resource_data,
  gl_resource_handle      _handle
) {
  GL_CALL(glDeleteBuffers(1, &_handle->internal_handle));
}

GLuint impl_create_gl_resource(gl_resource_data *const resource_data) {
  switch (resource_data->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    GAME_LOGF("creating FBO");
    return create_gl_fbo();
  }
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    GAME_LOGF("creating RBO");
    return create_gl_rbo(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    GAME_LOGF("creating VBO");
    return create_gl_vbo(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER: {
    GAME_LOGF("creating SSBO");
    return create_gl_ssbo(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    GAME_LOGF("creating UBO");
    return create_gl_ubo(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    GAME_LOGF("creating texture");
    return create_gl_texture(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    GAME_LOGF("creating image texture");
    return create_gl_image_texture(resource_data);
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    GAME_LOGF("creating PBO");
    return create_gl_pbo(resource_data);
    break;
  }
  default: {
    GAME_CRITICALF("unknown creation info passed to create_resource");
    exit(1);
    break;
  }
  }
  return (GLuint)-1;
}

u0 impl_destroy_gl_resource(
  gl_resource_data *const resource_data,
  gl_resource_handle      _handle
) {
  switch (resource_data->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    GAME_LOGF("destroying FBO");
    destroy_gl_fbo(_handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_RENDER_BUFFER: {
    GAME_LOGF("destroying RBO");
    destroy_gl_rbo(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    GAME_LOGF("destroying VBO");
    destroy_gl_vbo(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER_STORAGE_BUFFER: {
    GAME_LOGF("destroying SSBO");
    destroy_gl_ssbo(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_UNIFORM_BUFFER: {
    GAME_LOGF("destroying UBO");
    destroy_gl_ubo(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    GAME_LOGF("destroying texture");
    destroy_gl_texture(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    GAME_LOGF("destroying image texture");
    destroy_gl_texture(resource_data, _handle);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_PIXEL_BUFFER: {
    GAME_LOGF("destroying PBO");
    destroy_gl_pbo(resource_data, _handle);
    break;
  }
  default: {
    GAME_CRITICALF("unknown creation info passed to create_resource");
    exit(1);
    break;
  }
  }
}

u0 request_gl_resource(
  gl_resource_data *const resource_data,
  gl_resource_handle     *_handle
) {
  static str_hash_table *const table = &gl_resource_manager.table;
  static str_hash_table *const handle_pointer_table =
    &gl_resource_manager.handle_pointers;

  GAME_ASSERT(resource_data->resource_name);
  GAME_ASSERT(
    resource_data->desc.dummy.creation_info_type !=
    RESOURCE_CREATION_INFO_TYPE_INVALID
  );

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
    GAME_ASSERT(str_hash_table_contains(table, resource_data->resource_name));
    GAME_ASSERT(
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
    GAME_ASSERT(str_hash_table_contains(
      handle_pointer_table,
      resource_data->resource_name
    ));

    *_handle =
      *(gl_resource_handle *)
        str_hash_table_at(handle_pointer_table, resource_data->resource_name);

    GAME_ASSERT(*_handle);
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
  const GLuint gl_handle = impl_create_gl_resource(resource_data);

  /* make sure handle pointer table was properly cleared before */
  GAME_ASSERT(
    !str_hash_table_contains(handle_pointer_table, resource_data->resource_name)
  );

  gl_resource_handle new_handle = TRACKED_MALLOC(sizeof **_handle);

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
  GAME_LOGF("inserted resource table slot: %p", str_hash_table_at(table, resource_data->resource_name));
  (*_handle)->internal_handle = gl_handle;
}

u0 destroy_gl_resource(
  gl_resource_data *const resource_data,
  gl_resource_handle     *_handle
) {
  static str_hash_table *const table = &gl_resource_manager.table;
  static str_hash_table *const handle_pointer_table =
    &gl_resource_manager.handle_pointers;

  GAME_ASSERT(gl_resource_manager.initialized);
  GAME_ASSERT(resource_data->resource_name);
  GAME_ASSERT(
    resource_data->desc.dummy.creation_info_type !=
    RESOURCE_CREATION_INFO_TYPE_INVALID
  );

  GAME_LOGF("destroy_resource() on %s", resource_data->resource_name);

  if (!*_handle) {
    GAME_LOGF("handle already NULL");
    return;
  }

  GAME_ASSERT(str_hash_table_contains(table, resource_data->resource_name));
  GAME_ASSERT(
    str_hash_table_get_index(table, resource_data->resource_name) ==
    (*_handle)->hashed_resource_index
  );

  resource_table_slot *resource_slot =
    str_hash_table_at_index(table, (*_handle)->hashed_resource_index);

  GAME_LOGF("resource slot address: %p", resource_slot);
  GAME_ASSERT(resource_slot->ref_count > 0);
  --resource_slot->ref_count;

  if (resource_slot->ref_count == 0) {
    /* TODO: implement deletion postpone callback */
    if (true) {
      /* free the main table slot */
      GAME_ASSERT(str_hash_table_contains(
        handle_pointer_table,
        resource_data->resource_name
      ));

      /* the OpenGL side of the resource is getting destroyed in here */
      impl_destroy_gl_resource(resource_data, *_handle);
      destroy_persistent_resource_data(&resource_slot->resource_data);

      str_hash_table_erase(table, resource_data->resource_name);
      str_hash_table_erase(handle_pointer_table, resource_data->resource_name);

      TRACKED_FREE(*_handle);
      GAME_LOGF("resource fully destroyed");
    } else {
      /*  TODO: summon a low prio thread that periodically (every 20 seconds
      or so checks all of the callbacks) and pushes the ones that returned
      true into a queue for the main thread to destroy them
      */
      GAME_LOGF(
        "destroy_resource() on %s moved its data to postpone list!",
        resource_data->resource_name
      );
    }
  }
  *_handle = NULL;
}
