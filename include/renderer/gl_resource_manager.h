#ifndef RENDERER_GL_RESOURCE_MANAGER_H_
#define RENDERER_GL_RESOURCE_MANAGER_H_

#include <containers/str_hash_table.h>
#include <common.h>

#include <glad/glad.h>
#include <string.h>
#include <util/mmcmp.h>

#define MAXIMUM_RESOURCE_NAME_STRLEN 0xff

typedef enum {
  RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER,
  RESOURCE_CREATION_INFO_TYPE_SSBO,
  RESOURCE_CREATION_INFO_TYPE_SHADER,
  RESOURCE_CREATION_INFO_TYPE_TEXTURE,
  RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE,
} gl_resource_ci_type;

typedef struct {
  /* this NEEDS to be the first element */
  u32     creation_info_type;
  GLenum *attachements;
  u32     width;
  u32     height;
  GLenum  format;
  usize   num_attachements;
} frame_buffer_creation_info;

typedef struct {
  GLenum  attribute_type;
  GLsizei attribute_count;
  GLsizei attribute_index;
} vertex_attribute_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32                    creation_info_type;
  vertex_attribute_info *vertex_attributes;
  u32                    size;
  usize                  num_attributes;
  GLenum                 buffer_usage;
} vertex_buffer_creation_info;

typedef struct {
  /* this NEEDS to be the first element */
  u32     creation_info_type;
  GLenum  index_type;
  GLsizei size;
} index_buffer_creation_info;
/*
this exists so we can access the u32 creation_info_type of every possible info
type without knowing which one it is beforehand
*/
typedef struct {
  /* this NEEDS to be the first element */
  u32 creation_info_type;
} dummy_creation_info;

/*
passing this as an identifier to request_resource()
*/
typedef struct {
  union creation_info {
    dummy_creation_info         dummy;
    frame_buffer_creation_info  frame_buffer;
    vertex_buffer_creation_info vertex_buffer;
    index_buffer_creation_info  index_buffer;
    /* TODO: implement all... */
  } desc;
  char *resource_name;
} gl_resource_data;
/*
add postpone_deletion callback
*/

/* deep copy function for gl_resource_data since clients might pass in temporary
 * structs and arrays */
static gl_resource_data
create_persistent_resource_data(const gl_resource_data *const _temp) {
  gl_resource_data out;
  GAME_LOGF("doig deep copy on resource %s", _temp->resource_name);

  /* copy the name into a persistent buff */
  usize temp_name_len =
    strnlen_s(_temp->resource_name, MAXIMUM_RESOURCE_NAME_STRLEN);
  GAME_LOGF("name strnlen: %zu", temp_name_len);

  out.resource_name = malloc(temp_name_len + 1);
  strncpy_s(
    out.resource_name,
    temp_name_len + 1,
    _temp->resource_name,
    MAXIMUM_RESOURCE_NAME_STRLEN
  );

  GAME_LOGF("copied name: %s", out.resource_name);

  switch (_temp->desc.dummy.creation_info_type) {
  /* first handle the cases where no custom deep copy is needed */
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER: {
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    usize attributes_byte_size =
      sizeof *_temp->desc.vertex_buffer.vertex_attributes *
      _temp->desc.vertex_buffer.num_attributes;
    GAME_LOGF("byte_size of vertex attriubte array: %zu", attributes_byte_size);
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.vertex_buffer.vertex_attributes = malloc(attributes_byte_size);
    memcpy(
      out.desc.vertex_buffer.vertex_attributes,
      _temp->desc.vertex_buffer.vertex_attributes,
      attributes_byte_size
    );
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    /* deep copy the attachements */
    usize attachements_byte_size =
      sizeof *_temp->desc.frame_buffer.attachements *
      _temp->desc.frame_buffer.num_attachements;
    memcpy(&out.desc, &_temp->desc, sizeof out.desc);
    out.desc.frame_buffer.attachements = malloc(attachements_byte_size);
    memcpy(
      out.desc.frame_buffer.attachements,
      _temp->desc.frame_buffer.attachements,
      attachements_byte_size
    );
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SSBO: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    /* implement ...*/
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
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER: {
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    free(_data->desc.vertex_buffer.vertex_attributes);
    _data->desc.vertex_buffer.vertex_attributes = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    free(_data->desc.frame_buffer.attachements);
    _data->desc.frame_buffer.attachements = NULL;
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SSBO: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {
    /* implement ...*/
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_IMAGE_TEXTURE: {
    /* implement ...*/
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
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {

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

static u0 request_resource(
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

  GLuint gl_handle = 0;

  /* resource does not yet exists */
  switch (resource_data->desc.dummy.creation_info_type) {
  case RESOURCE_CREATION_INFO_TYPE_FRAME_BUFFER: {
    GAME_LOGF("creating FBO");
    gl_handle = 1; /* implement */
    /* etc... */
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_VERTEX_BUFFER: {
    GAME_LOGF("creating VBO");
    gl_handle = 2; /* implement */
    /* etc... */
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_INDEX_BUFFER: {
    /* etc... */
    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SSBO: {

    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_SHADER: {

    break;
  }
  case RESOURCE_CREATION_INFO_TYPE_TEXTURE: {

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

static u0 destroy_resource(
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
