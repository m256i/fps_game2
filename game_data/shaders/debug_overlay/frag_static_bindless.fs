#version 460 core
#extension GL_ARB_bindless_texture : enable

in vec2  oTextureCoords;
out vec4 oFragColor;

layout(bindless_sampler) uniform sampler2D uTexture;

void main() {
  vec2  tc     = vec2(oTextureCoords.x, 1.0 - oTextureCoords.y);
  float center = texture(uTexture, tc).r;

  bool  isEdge  = false;
  ivec2 offs[8] = ivec2[8](
    ivec2(1, 0),
    ivec2(-1, 0),
    ivec2(0, 1),
    ivec2(0, -1),
    ivec2(1, 1),
    ivec2(-1, 1),
    ivec2(1, -1),
    ivec2(-1, -1)
  );
  for (int i = 0; i < 8; ++i) {
    if (textureOffset(uTexture, tc, offs[i]).r == 0) {
      isEdge = true;
      break;
    }
  }

  if (isEdge && center > 0.5) {
    oFragColor = vec4(0.0, 0.0, 0.0, 1.0);
  } else if (center < 0.5) {
    oFragColor = vec4(1.0, 1.0, 1.0, 1.0);
  } else {
    discard;
  }
}