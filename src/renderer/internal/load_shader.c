#include <util/array_copy.h>
#include <util/loadfile.h>
#include <renderer/gl_api.h>
#include <renderer/gl_resource_manager.h>

static GLuint compile_shader_from_source(
  const char *src,
  GLenum      shader_type,
  const char *path
) {
  GLuint shader = glCreateShader(shader_type);
  GL_CALL(glShaderSource(shader, 1, &src, NULL));
  GL_CALL(glCompileShader(shader));

  GLint status = GL_FALSE;
  GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint log_len = 0;
    GL_CALL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len));
    if (log_len > 1) {
      char *log = TRACKED_MALLOC((usize)log_len);
      GL_CALL(glGetShaderInfoLog(shader, log_len, NULL, log));
      GAME_WARNF("Error compiling shader (%s):\n%s", path, log);
    } else {
      GAME_WARNF("Unknown error compiling shader (%s)", path);
    }
    GAME_ASSERT(false);
  }
  return shader;
}

static GLuint link_program(const GLuint *shaders, usize shader_count) {
  GLuint prog = glCreateProgram();
  for (usize i = 0; i != shader_count; ++i) {
    GL_CALL(glAttachShader(prog, shaders[i]));
  }
  GL_CALL(glLinkProgram(prog));

  GLint status = GL_FALSE;
  GL_CALL(glGetProgramiv(prog, GL_LINK_STATUS, &status));
  if (status != GL_TRUE) {
    GLint log_len = 0;
    GL_CALL(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len));
    if (log_len > 1) {
      char *log = TRACKED_MALLOC((usize)log_len);
      GL_CALL(glGetProgramInfoLog(prog, log_len, NULL, log));
      GAME_WARNF("Error linking program:\n%s", log);
    } else {
      GAME_WARNF("Unknown error linking program");
    }
    GAME_ASSERT(false);
  }
  for (usize i = 0; i != shader_count; ++i) {
    GL_CALL(glDetachShader(prog, shaders[i]));
    GL_CALL(glDeleteShader(shaders[i]));
  }
  return prog;
}

GLuint load_shader_from_path(shader_creation_info *const _creation_info) {
  if (!_creation_info || !_creation_info->vertex_path ||
      !_creation_info->fragment_path) {
    GAME_WARNF("vertex_path and fragment_path must not be NULL");
    GAME_ASSERT(false);
  }

  char *vert_src = read_file_to_string_basic(_creation_info->vertex_path);
  GAME_ASSERT(vert_src);

  char *frag_src = read_file_to_string_basic(_creation_info->fragment_path);
  GAME_ASSERT(frag_src);

  GLuint vert_shader = compile_shader_from_source(
    vert_src,
    GL_VERTEX_SHADER,
    _creation_info->vertex_path
  );
  TRACKED_FREE(vert_src);
  GAME_ASSERT(vert_shader);

  GLuint frag_shader = compile_shader_from_source(
    frag_src,
    GL_FRAGMENT_SHADER,
    _creation_info->fragment_path
  );
  TRACKED_FREE(frag_src);
  GAME_ASSERT(frag_shader);

  GLuint shaders[2] = {vert_shader, frag_shader};
  GLuint program    = link_program(shaders, 2);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  GAME_ASSERT(program);

  GLint num_active_attribs = 0;
  GL_CALL(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_active_attribs));
  _creation_info->num_inputs = (usize)num_active_attribs;

  if (num_active_attribs > 0) {
    _creation_info->input_attributes = TRACKED_MALLOC(
      sizeof(shader_input_attribute) * (size_t)num_active_attribs
    );

    for (GLint i = 0; i < num_active_attribs; ++i) {
      char    name_buf[256];
      GLsizei name_len = 0;
      GLint   size     = 0;
      GLenum  type     = 0;

      GL_CALL(glGetActiveAttrib(
        program,
        (GLuint)i,
        sizeof(name_buf),
        &name_len,
        &size,
        &type,
        name_buf
      ));

      name_buf[name_len] = '\0';
      GLint location     = glGetAttribLocation(program, name_buf);

      shader_input_attribute *attr = &_creation_info->input_attributes[i];
      attr->name                   = strnclone_s(name_buf, 256);
      attr->type                   = type;
      attr->location               = location;
      attr->size                   = size;
    }
  } else {
    _creation_info->input_attributes = NULL;
  }

  GLint total_uniforms = 0;
  GL_CALL(glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &total_uniforms));

  shader_uniform_attribute *plain_uniforms = NULL;
  usize                     plain_count    = 0;

  if (total_uniforms > 0) {
    plain_uniforms =
      TRACKED_MALLOC(sizeof(shader_uniform_attribute) * (size_t)total_uniforms);

    for (GLint index = 0; index < total_uniforms; ++index) {
      char    name_buf[256];
      GLsizei name_len = 0;
      GLint   size     = 0;
      GLenum  type     = 0;

      GL_CALL(glGetActiveUniform(
        program,
        (GLuint)index,
        sizeof(name_buf),
        &name_len,
        &size,
        &type,
        name_buf
      ));
      name_buf[name_len] = '\0';

      GLint block_index = -1;
      GL_CALL(glGetActiveUniformsiv(
        program,
        1,
        (GLuint *)&index,
        GL_UNIFORM_BLOCK_INDEX,
        &block_index
      ));
      if (block_index != -1) {
        continue;
      }

      /* check if part of an SSBO (shader storage block) by querying the same
       * property: Note: GL_UNIFORM_BLOCK_INDEX also returns -1 for SSBO
       * uniforms, so we need to skip SSBOs differently. Modern GL has
       * GL_SHADER_STORAGE_BLOCK interface to list SSBOs, but individual SSBO
       * members do not show up as "active uniforms" to GL_ACTIVE_UNIFORMS. So
       * the check above is sufficient to skip UBOs; SSBOs won't appear here. */
      GLint location = glGetUniformLocation(program, name_buf);

      shader_uniform_attribute *u = &plain_uniforms[plain_count++];
      u->name                     = strnclone_s(name_buf, 256);
      u->type                     = type;
      u->location                 = location;
      u->size                     = size;
    }
  }

  if (plain_count > 0) {
    _creation_info->uniform_attributes = TRACKED_REALLOC(
      plain_uniforms,
      sizeof(shader_uniform_attribute) * (usize)plain_count
    );
    _creation_info->num_uniforms = plain_count;
  } else {
    TRACKED_FREE(plain_uniforms);
    _creation_info->uniform_attributes = NULL;
    _creation_info->num_uniforms       = 0;
  }

  GLint num_ubos = 0;
  GL_CALL(glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &num_ubos));
  _creation_info->num_ubos = (usize)num_ubos;

  if (num_ubos > 0) {
    _creation_info->ubo_binding_points =
      TRACKED_MALLOC(sizeof(GLuint) * (usize)num_ubos);
    for (GLint i = 0; i < num_ubos; ++i) {
      GLint binding = 0;
      GL_CALL(glGetActiveUniformBlockiv(
        program,
        (GLuint)i,
        GL_UNIFORM_BLOCK_BINDING,
        &binding
      ));
      _creation_info->ubo_binding_points[i] = (GLuint)binding;
    }
  } else {
    _creation_info->ubo_binding_points = NULL;
  }

  GLint num_ssbos = 0;
  GL_CALL(glGetProgramInterfaceiv(
    program,
    GL_SHADER_STORAGE_BLOCK,
    GL_ACTIVE_RESOURCES,
    &num_ssbos
  ));
  _creation_info->num_ssbos = (usize)num_ssbos;

  if (num_ssbos > 0) {
    _creation_info->ssbo_binding_points =
      TRACKED_MALLOC(sizeof(GLuint) * (size_t)num_ssbos);
    for (GLint i = 0; i < num_ssbos; ++i) {
      GLenum props[]    = {GL_BUFFER_BINDING};
      GLint  results[1] = {0};
      GL_CALL(glGetProgramResourceiv(
        program,
        GL_SHADER_STORAGE_BLOCK,
        (GLuint)i,
        1,
        props,
        1,
        NULL,
        results
      ));
      _creation_info->ssbo_binding_points[i] = (GLuint)results[0];
    }
  } else {
    _creation_info->ssbo_binding_points = NULL;
  }

  GLint num_outputs = 0;
  GL_CALL(glGetProgramInterfaceiv(
    program,
    GL_PROGRAM_OUTPUT,
    GL_ACTIVE_RESOURCES,
    &num_outputs
  ));
  _creation_info->num_outputs = (usize)num_outputs;

  if (num_outputs > 0) {
    _creation_info->output_attachements =
      TRACKED_MALLOC(sizeof(GLenum) * (usize)num_outputs);
    for (GLint i = 0; i < num_outputs; ++i) {
      GLenum props[]    = {GL_LOCATION};
      GLint  results[1] = {0};
      GL_CALL(glGetProgramResourceiv(
        program,
        GL_PROGRAM_OUTPUT,
        (GLuint)i,
        1,
        props,
        1,
        NULL,
        results
      ));
      _creation_info->output_attachements[i] = (GLenum)results[0];
    }
  } else {
    _creation_info->output_attachements = NULL;
  }

  glUseProgram(0);
  return program;
}
