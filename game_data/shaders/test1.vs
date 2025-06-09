#version 460 core
// #extension GL_ARB_bindless_texture : enable
// clang-format off

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTexCoords;

// out vec2 oTexCoords;
// out vec2 u_mousePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float camera_distance;
out vec3 oNormals;

void main() {
  oNormals = aNormals;
  vec3 camera_pos = -vec3(view[3][0], view[3][1], view[3][2]) * mat3(view);
  camera_distance = length(view * model * vec4(aPos, 1.0) - vec4(camera_pos, 0));
  camera_distance = (camera_distance * camera_distance) / 2;
  gl_Position = projection * view * (model) * vec4(aPos, 1.0);
}