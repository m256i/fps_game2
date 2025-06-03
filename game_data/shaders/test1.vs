#version 460 core
#extension GL_ARB_bindless_texture : enable
// clang-format off

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 oTexCoords;

uniform float OffsetX;
uniform float OffsetY;

void main() {
  oTexCoords  = aTexCoords;
  gl_Position = vec4(aPos.x * 2 + OffsetX, aPos.y * 2 + OffsetY, 0.0, 1.0);
}