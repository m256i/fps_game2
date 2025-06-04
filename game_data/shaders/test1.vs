#version 460 core
// #extension GL_ARB_bindless_texture : enable
// clang-format off

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 oTexCoords;
out vec2 u_mousePos;

uniform float OffsetX;
uniform float OffsetY;

void main() {
  oTexCoords  = aTexCoords;
  u_mousePos = vec2((OffsetX + 0.5), OffsetY);
  gl_Position = vec4(aPos.x * 2, aPos.y * 2, 0.0, 1.0);
}