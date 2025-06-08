#version 460 core
// #extension GL_ARB_bindless_texture : enable
// clang-format off

layout(location = 0) in vec3 aPos;
// layout(location = 1) in vec2 aTexCoords;

// out vec2 oTexCoords;
// out vec2 u_mousePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float camera_distance;

void main() {
  // oTexCoords  = aTexCoords;
  // u_mousePos = vec2((OffsetX + 0.5), OffsetY);
  vec3 camera_pos = vec3(view[0][2], view[1][2], view[2][2]);
  camera_distance = length(view * model * vec4(aPos, 1.0) - vec4(camera_pos, 0));
  gl_Position = projection * view * model * vec4(aPos, 1.0);
}