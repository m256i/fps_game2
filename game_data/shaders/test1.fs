#version 460 core
#extension GL_ARB_bindless_texture : enable
// clang-format off

out vec4 FragColor;
in vec2 oTexCoords;
layout(bindless_sampler) uniform sampler2D tex;

void main() { 
  FragColor = texture(tex, oTexCoords); 
}