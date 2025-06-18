#version 460 core
#extension GL_ARB_bindless_texture : enable
// clang-format off

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTextureCoords;

out vec2 oTextureCoords;

void main() {
  oTextureCoords = aTextureCoords;
  gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}